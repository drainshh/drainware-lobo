#include "lagcomp.h"
#include "../aimbot/aimbot.h"

std::deque< Backtrack::Record > Backtrack::records[ 65 ];
std::deque< Backtrack::IncomingSequence > Backtrack::sequences;
Backtrack::Cvars Backtrack::cvars;

float Backtrack::getExtraTicks( )
{
	auto network = g_interfaces.m_engine_client->get_net_channel_info( );
	if ( !network )
		return 0.f;

	return std::clamp( network->getLatency( 1 ) - network->getLatency( 0 ), 0.f, 0.2f );
}

 float Backtrack::getLerp( )
{
	auto ratio = std::clamp( Backtrack::cvars.interpRatio->get_float( ), Backtrack::cvars.minInterpRatio->get_float( ),
	                         Backtrack::cvars.maxInterpRatio->get_float( ) );

	return std::max( Backtrack::cvars.interp->get_float( ),
	                 ( ratio / ( ( Backtrack::cvars.maxUpdateRate ) ? Backtrack::cvars.maxUpdateRate->get_float( )
	                                                                                            : Backtrack::cvars.updateRate->get_float( ) ) ) );
}

 float Backtrack::valid( float simtime )
{
	auto network = g_interfaces.m_engine_client->get_net_channel_info( );
	if ( !network )
		return false;

	float deadTime = static_cast< int >( g_ctx.serverTime( ) - Backtrack::cvars.maxUnlag->get_float( ) );
	if ( simtime < deadTime )
		return false;

	auto delta = std::clamp( network->getLatency( 0 ) + network->getLatency( 1 ) + getLerp( ), 0.f, Backtrack::cvars.maxUnlag->get_float( ) ) -
	             ( g_ctx.serverTime( ) - simtime );
	if ( !GET_VARIABLE( g_variables.m_backtrack_extend, bool ) )

		return std::fabsf( delta ) <= 0.2f;
	else
	{
		
			return std::fabsf( delta ) <= 0.2f + GET_VARIABLE( g_variables.fakeLatencyAmount, int ) / 1000.f;
	}
}

void Backtrack::update( int stage ) noexcept
{
	if ( !GET_VARIABLE( g_variables.m_backtrack_enable, bool ) || !g_ctx.m_local || !g_ctx.m_local->is_alive( ) ) {
		for ( auto& record : records )
			record.clear( );

		return;
	}

	if ( stage == render_start ) {
		g_entity_cache.enumerate( e_enumeration_type::type_players, [ & ]( c_base_entity* entity ) {
			if ( !entity || entity == g_ctx.m_local || entity->is_dormant( ) || !entity->is_alive( ) ||
			     entity->get_team( ) == g_ctx.m_local->get_team( ) ) {
				records[ entity->get_index( ) ].clear( );
				return;
			}

			if ( !records[ entity->get_index( ) ].empty( ) &&
			     ( records[ entity->get_index( ) ].front( ).simulationTime == entity->get_simulation_time( ) ) )
				return;

			Record record{ };
			if ( const model_t* mod = entity->get_model( ); mod )
				record.hdr = g_interfaces.m_model_info->get_studio_model( mod );
			record.head           = entity->get_hitbox_position( hitbox_head );
			record.neck           = entity->get_hitbox_position( hitbox_neck );
			record.body           = entity->get_hitbox_position( hitbox_stomach );
			record.legs           = entity->get_hitbox_position( hitbox_left_foot );

			record.origin         = entity->get_abs_origin( );
			record.simulationTime = entity->get_simulation_time( );
			record.mins           = entity->get_collideable( )->get_obb_mins( );
			record.max            = entity->get_collideable( )->get_obb_maxs( );
			entity->setup_bones( record.matrix, 256, 0x7FF00, g_interfaces.m_global_vars_base->m_current_time );
			records[ entity->get_index( ) ].push_front( record );

			while ( records[ entity->get_index( ) ].size( ) > 3 &&
			        records[ entity->get_index( ) ].size( ) >
			            static_cast< size_t >( timeToTicks( static_cast< float >( GET_VARIABLE( g_variables.m_backtrack_time_limit, int ) ) / 1000.f +
			                                                getExtraTicks( ) ) ) )
				records[ entity->get_index( ) ].pop_back( );

			if ( auto invalid = std::find_if( std::cbegin( records[ entity->get_index( ) ] ), std::cend( records[ entity->get_index( ) ] ),
			                                  []( const Record& rec ) { return !valid( rec.simulationTime ); } );
			     invalid != std::cend( records[ entity->get_index( ) ] ) )
				records[ entity->get_index( ) ].erase( invalid, std::cend( records[ entity->get_index( ) ] ) );
		} );
	}
}

void Backtrack::run( c_user_cmd* cmd ) noexcept
{
	if ( !GET_VARIABLE( g_variables.m_backtrack_enable, bool ) )
		return;

	if ( !( cmd->m_buttons &in_attack ) )
		return;

	if ( !g_ctx.m_local )
		return;
	if ( g_aimbot.get_aimbot_settings( ).m_aim_to_backtrack )
		return;
	

	auto localEyePosition = g_ctx.m_local->get_eye_position( false );

	auto bestFov{ 255.f };
	c_base_entity* bestTarget{ };
	int bestTargetIndex{ };
	c_vector bestTargetHead{ };
	int bestRecord{ };

	const auto aimPunch = g_ctx.m_local->get_punch( );

	g_entity_cache.enumerate( e_enumeration_type::type_players, [ & ]( c_base_entity* entity ) {
		if ( !entity || entity == g_ctx.m_local || entity->is_dormant( ) || !entity->is_alive( ) ||
		     entity->get_team( ) == g_ctx.m_local->get_team( ) ) {
			
			return;
		}

		auto head = entity->get_bone_position( 8 );

			auto angle = g_math.calculate_angle( localEyePosition, head );

		if ( float calculated_fov = g_math.calculate_fov( g_ctx.m_cmd->m_view_point, angle ); calculated_fov < bestFov ) {
			bestFov         = calculated_fov;
			bestTarget      = entity;
			bestTargetIndex = entity->get_index( );
			bestTargetHead  = head;
		}
	} );

	if ( bestTarget ) {
		if ( records[ bestTargetIndex ].size( ) <= 3)
			return;

		bestFov = 255.f;

		for ( size_t i = 0; i < records[ bestTargetIndex ].size( ); i++ ) {
			auto& record = records[ bestTargetIndex ][ i ];
			if ( !valid( record.simulationTime ) )
				continue;

				auto angle = g_math.calculate_angle( localEyePosition, record.head );

			if ( float calculated_fov = g_math.calculate_fov( g_ctx.m_cmd->m_view_point, angle ); calculated_fov < bestFov ) {
				bestFov    = calculated_fov;
				bestRecord = i;
			}
		}
	}

	if ( bestRecord ) {
		auto record    = records[ bestTargetIndex ][ bestRecord ];
		cmd->m_tick_count = timeToTicks( record.simulationTime + getLerp( ) );
	}
}

void Backtrack::AddLatencyToNetwork( c_net_channel* network, float latency ) noexcept
{
	for ( auto& sequence : sequences ) {
		if ( g_ctx.serverTime( ) - sequence.servertime >= latency ) {
			network->m_in_reliable_state = sequence.inreliablestate;
			network->m_in_sequence_nr    = sequence.sequencenr;
			break;
		}
	}
}

void Backtrack::UpdateIncomingSequences( ) noexcept
{
	static int lastIncomingSequenceNumber = 0;

	if ( !GET_VARIABLE(g_variables.m_backtrack_extend,bool) )
		return;

	if ( !g_ctx.m_local )
		return;

	auto network = g_interfaces.m_engine_client->get_net_channel_info( );
	if ( !network )
		return;

	if ( network->InSequenceNr != lastIncomingSequenceNumber ) {
		lastIncomingSequenceNumber = network->InSequenceNr;

		IncomingSequence sequence{ };
		sequence.inreliablestate = network->InReliableState;
		sequence.sequencenr      = network->InSequenceNr;
		sequence.servertime      = g_ctx.serverTime( );
		sequences.push_front( sequence );
	}

	while ( sequences.size( ) > 2048 )
		sequences.pop_back( );
}

int Backtrack::timeToTicks( float time ) noexcept
{
	return static_cast< int >( 0.5f + time / g_interfaces.m_global_vars_base->m_interval_per_tick );
}
