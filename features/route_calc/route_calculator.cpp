#include "route_calculator.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace route_calc
{
namespace
{
std::vector< RoutePoint > g_route_points;
int g_selected_route_point = -1;
int g_target_route_point = -1;
RoutePoint g_aimed_pixelsurf_candidate;
bool g_has_aimed_pixelsurf_candidate = false;
std::vector< ComboResult > g_combo_results;
std::string g_last_calculation_message = "not calculated";

std::string timestamp_string( )
{
	const auto now = std::chrono::system_clock::now( );
	const auto time = std::chrono::system_clock::to_time_t( now );
	std::tm local_time{ };
	localtime_s( &local_time, &time );

	std::ostringstream out;
	out << std::put_time( &local_time, "%H:%M:%S" );
	return out.str( );
}

std::string vec3_string( const Vec3& value )
{
	std::ostringstream out;
	out << '(' << std::fixed << std::setprecision( 2 ) << value.x << ',' << value.y << ',' << value.z << ')';
	return out.str( );
}

bool presses_duck( Action action )
{
	return action == Action::Duck || action == Action::JumpDuck;
}

bool presses_jump( Action action )
{
	return action == Action::Jump || action == Action::JumpDuck;
}

void log_settings_once( const Settings& settings )
{
	static bool initialized_logged = false;
	if ( !initialized_logged ) {
		AppendDebugLog( "routecalc", "initialized" );
		initialized_logged = true;
	}

	static float last_tick_interval = -1.0f;
	if ( std::fabs( last_tick_interval - settings.tick_interval ) < 0.000001f )
		return;

	last_tick_interval = settings.tick_interval;

	std::ostringstream out;
	out << std::fixed << std::setprecision( 6 ) << "constants tick=" << settings.tick_interval << " gravity=" << settings.gravity
	    << " ent_gravity=" << settings.ent_gravity << " half_gravity=" << HalfGravity( settings )
	    << " jump_impulse=" << settings.jump_impulse << " duck_shift=" << settings.duck_origin_shift
	    << " unduck_shift=" << settings.in_air_unduck_origin_shift << " duck_step=" << DuckStep( settings )
	    << " duck_ticks=" << DuckTicks( settings );
	AppendDebugLog( "routecalc", out.str( ) );
}

void add_event( RouteCandidate& candidate, RouteEvent::Type type, const PlayerState& state, const char* note )
{
	RouteEvent event{ };
	event.type = type;
	event.tick = state.tick;
	event.state = state;
	event.note = note ? note : "";
	candidate.events.push_back( event );
}

void renumber_route_points( )
{
	for ( int i = 0; i < static_cast< int >( g_route_points.size( ) ); ++i )
		g_route_points[ i ].id = i + 1;

	if ( g_selected_route_point >= static_cast< int >( g_route_points.size( ) ) )
		g_selected_route_point = static_cast< int >( g_route_points.size( ) ) - 1;
	if ( g_target_route_point >= static_cast< int >( g_route_points.size( ) ) )
		g_target_route_point = static_cast< int >( g_route_points.size( ) ) - 1;
}

const RoutePoint* route_point_at( const int index )
{
	if ( index < 0 || index >= static_cast< int >( g_route_points.size( ) ) )
		return nullptr;
	return &g_route_points[ index ];
}

RoutePoint* mutable_route_point_at( const int index )
{
	if ( index < 0 || index >= static_cast< int >( g_route_points.size( ) ) )
		return nullptr;
	return &g_route_points[ index ];
}

const RoutePoint* first_point_of_type( const PointType type )
{
	for ( const auto& point : g_route_points ) {
		if ( point.type == type )
			return &point;
	}
	return nullptr;
}

float length_2d( const Vec3& value )
{
	return std::sqrt( value.x * value.x + value.y * value.y );
}

Vec3 delta_vec( const RoutePoint& start, const RoutePoint& end )
{
	return { end.position.x - start.position.x, end.position.y - start.position.y, end.position.z - start.position.z };
}

bool pixelsurf_normal_valid( const RoutePoint& point )
{
	return point.has_normal && std::fabs( point.normal.z ) < 0.7f;
}

void log_calculation_input( const RoutePoint& floor, const RoutePoint& target )
{
	const Vec3 delta = delta_vec( floor, target );
	std::ostringstream out;
	out << "calculate input floor_id=" << floor.id << " floor_pos=" << vec3_string( floor.position ) << " target_id=" << target.id
	    << " target_pos=" << vec3_string( target.position );
	if ( target.has_normal )
		out << " target_normal=" << vec3_string( target.normal );
	else
		out << " target_normal=(unavailable)";
	out << " delta=" << vec3_string( delta ) << " distance2d=" << length_2d( delta ) << " height_delta=" << delta.z
	    << " selected_candidate=" << target.source_pixel_assist_index << " source=" << target.source;
	AppendDebugLog( "routecalc", out.str( ) );
}

void set_invalid_result( const std::string& reason )
{
	g_last_calculation_message = "no validated route: " + reason;
	AppendDebugLog( "routecalc", "validate result=reject reason=" + reason );
}

void add_combo( const char* text, const float score, const RoutePoint& start, const RoutePoint& end, const char* notes, const ComboResult::Status status,
                const char* status_reason, const int max_displayed, const bool stop_at_max_displayed )
{
	if ( stop_at_max_displayed && max_displayed > 0 && static_cast< int >( g_combo_results.size( ) ) >= max_displayed )
		return;

	ComboResult result{ };
	result.index = static_cast< int >( g_combo_results.size( ) ) + 1;
	result.text = text ? text : "";
	result.score = score;
	result.start_point_id = start.id;
	result.end_point_id = end.id;
	result.notes = notes ? notes : "";
	result.status = status;
	result.status_reason = status_reason ? status_reason : "";
	g_combo_results.push_back( result );

	std::ostringstream out;
	out << "combo #" << result.index << " floor=" << start.id << " target=" << end.id << " text=\"" << result.text << "\" score=" << result.score
	    << " status=" << ToString( result.status ) << " reason=\"" << result.status_reason << "\"";
	AppendDebugLog( "routecalc", out.str( ) );
}
}

const char* ToString( Action action )
{
	switch ( action ) {
	case Action::None:
		return "none";
	case Action::Jump:
		return "jump";
	case Action::Duck:
		return "duck";
	case Action::JumpDuck:
		return "jump+duck";
	case Action::ReleaseDuck:
		return "release duck";
	default:
		return "unknown";
	}
}

const char* ToString( RouteEvent::Type type )
{
	switch ( type ) {
	case RouteEvent::Type::None:
		return "none";
	case RouteEvent::Type::StandingJump:
		return "standing_jump";
	case RouteEvent::Type::LongJump:
		return "longjump";
	case RouteEvent::Type::CrouchJump:
		return "crouchjump";
	case RouteEvent::Type::MiniJumpRisk:
		return "minijump_risk";
	case RouteEvent::Type::Apex:
		return "apex";
	case RouteEvent::Type::Landed:
		return "landed";
	default:
		return "unknown";
	}
}

const char* ToString( PointType type )
{
	switch ( type ) {
	case PointType::Floor:
		return "floor";
	case PointType::PixelSurf:
		return "pixelsurf";
	default:
		return "unknown";
	}
}

const char* ToString( ComboResult::Status status )
{
	switch ( status ) {
	case ComboResult::Status::Invalid:
		return "invalid";
	case ComboResult::Status::Unvalidated:
		return "unvalidated";
	case ComboResult::Status::DebugHeuristic:
		return "debug heuristic";
	case ComboResult::Status::GeometryValidated:
		return "geometry validated";
	case ComboResult::Status::ManualObserved:
		return "manual observed";
	default:
		return "unknown";
	}
}

float HalfGravity( const Settings& settings )
{
	return settings.ent_gravity * settings.gravity * 0.5f * settings.tick_interval;
}

float DuckStep( const Settings& settings )
{
	return settings.duck_speed * settings.duck_step_scale * settings.tick_interval;
}

int DuckTicks( const Settings& settings )
{
	const float step = DuckStep( settings );
	if ( step <= 0.0f )
		return 0;
	return static_cast< int >( std::ceil( 1.0f / step ) );
}

void AppendDebugLog( const char* feature, const std::string& message )
{
	std::filesystem::create_directories( "C:\\drainware" );
	std::ofstream file( "C:\\drainware\\debuglog.txt", std::ios::app );
	if ( !file.good( ) )
		return;

	file << '[' << timestamp_string( ) << "][" << ( feature ? feature : "routecalc" ) << "] " << message << '\n';
}

PlayerState SimulateTick( const Settings& settings, PlayerState state, const TickInput& input )
{
	// Offline vertical-only CS:GO movement sketch based on the Clarity Compendium:
	// Duck() runs before CheckJumpButton(), StartGravity applies half gravity before jump logic,
	// and in-air FinishDuck()/FinishUnDuck() shift origin by +/-9 from the 72->54 hull delta.
	const bool was_on_ground = state.on_ground;
	const bool was_ducking = state.ducking;
	const bool was_ducked = state.ducked;
	const bool wants_jump = presses_jump( input.action );
	const bool wants_duck = presses_duck( input.action ) ? true : ( input.action == Action::ReleaseDuck ? false : state.duck_held );

	if ( wants_duck && !state.duck_held && !state.ducked )
		state.ducking = true;

	state.duck_held = wants_duck;

	const float duck_step = DuckStep( settings );
	if ( was_on_ground ) {
		if ( state.duck_held ) {
			state.duck_amount = std::min( 1.0f, state.duck_amount + duck_step );
			if ( state.duck_amount >= 1.0f ) {
				state.ducked = true;
				state.ducking = false;
			}
		} else {
			state.duck_amount = std::max( 0.0f, state.duck_amount - duck_step );
			if ( state.duck_amount <= 0.0f ) {
				state.ducking = false;
				state.ducked = false;
			}
		}
	}

	state.velocity.z -= HalfGravity( settings );

	if ( wants_jump && was_on_ground ) {
		state.on_ground = false;

		if ( state.ducking || state.ducked || wants_duck )
			state.velocity.z = settings.ground_factor * settings.jump_impulse;
		else
			state.velocity.z += settings.ground_factor * settings.jump_impulse;
	}

	if ( !was_on_ground && !state.on_ground ) {
		if ( state.duck_held && state.unduck_origin_shifted ) {
			state.origin.z += settings.duck_origin_shift;
			state.unduck_origin_shifted = false;
			state.duck_origin_shifted = false;
			state.ducked = true;
			state.ducking = false;
			state.duck_amount = 1.0f;
		} else if ( state.duck_held && !state.duck_origin_shifted ) {
			state.origin.z += settings.duck_origin_shift;
			state.duck_origin_shifted = true;
			state.ducked = true;
			state.ducking = false;
			state.duck_amount = 1.0f;
		} else if ( !state.duck_held && state.duck_origin_shifted ) {
			state.origin.z += settings.in_air_unduck_origin_shift;
			state.duck_origin_shifted = false;
			state.ducked = false;
			state.ducking = false;
			state.duck_amount = 0.0f;
		} else if ( !state.duck_held && was_ducking && !was_ducked ) {
			// FinishUnDuck() can apply the in-air -9 before FinishDuck() ever gave +9.
			state.origin.z += settings.in_air_unduck_origin_shift;
			state.unduck_origin_shifted = true;
			state.duck_origin_shifted = false;
			state.ducking = false;
			state.duck_amount = 0.0f;
		}
	}

	state.origin.x += state.velocity.x * settings.tick_interval;
	state.origin.y += state.velocity.y * settings.tick_interval;
	state.origin.z += state.velocity.z * settings.tick_interval;

	if ( !state.on_ground )
		state.velocity.z -= HalfGravity( settings );

	if ( state.origin.z <= settings.ground_z && state.velocity.z <= 0.0f ) {
		state.origin.z = settings.ground_z;
		state.velocity.z = 0.0f;
		state.on_ground = true;
		state.unduck_origin_shifted = false;
	}

	++state.tick;
	return state;
}

RouteCandidate SimulateRoute( const Settings& settings, const std::string& name, const std::vector< TickInput >& inputs )
{
	log_settings_once( settings );

	RouteCandidate candidate{ };
	candidate.name = name;
	candidate.inputs = inputs;

	PlayerState state{ };
	candidate.states.push_back( state );
	candidate.apex_z = state.origin.z;

	bool saw_apex = false;
	float previous_z_velocity = state.velocity.z;

	for ( int i = 0; i < settings.max_ticks; ++i ) {
		const TickInput input = i < static_cast< int >( inputs.size( ) ) ? inputs[ i ] : TickInput{ };
		const PlayerState before = state;
		state = SimulateTick( settings, state, input );

		if ( presses_jump( input.action ) && before.on_ground ) {
			if ( input.action == Action::JumpDuck )
				add_event( candidate, RouteEvent::Type::LongJump, state, "+jump and +duck on the same ground tick" );
			else if ( before.ducking || before.ducked || input.action == Action::Duck )
				add_event( candidate, RouteEvent::Type::CrouchJump, state, "jump while duck transition is active" );
			else
				add_event( candidate, RouteEvent::Type::StandingJump, state, "standing jump uses impulse plus current z velocity" );
		}

		if ( !state.duck_held && !before.on_ground &&
		     ( ( !before.unduck_origin_shifted && state.unduck_origin_shifted ) ||
		       ( before.duck_origin_shifted && !state.duck_origin_shifted ) || before.ducking || before.ducked ) )
			add_event( candidate, RouteEvent::Type::MiniJumpRisk, state, "duck released in air before preserving the +9 origin shift" );

		if ( !saw_apex && previous_z_velocity > 0.0f && state.velocity.z <= 0.0f ) {
			add_event( candidate, RouteEvent::Type::Apex, state, "vertical velocity crossed zero" );
			saw_apex = true;
		}

		candidate.apex_z = std::max( candidate.apex_z, state.origin.z );
		candidate.states.push_back( state );

		if ( !before.on_ground && state.on_ground ) {
			add_event( candidate, RouteEvent::Type::Landed, state, "returned to ground plane" );
			break;
		}

		previous_z_velocity = state.velocity.z;
	}

	candidate.score = candidate.apex_z;
	return candidate;
}

std::vector< RouteCandidate > FindSimpleJumpRoutes( const Settings& settings )
{
	std::vector< RouteCandidate > routes;
	routes.reserve( 4U );

	routes.push_back( SimulateRoute( settings, "Standing jump", { { Action::Jump } } ) );
	routes.push_back( SimulateRoute( settings, "Crouchjump", { { Action::Duck }, { Action::Duck }, { Action::Duck }, { Action::Jump }, { Action::Duck } } ) );
	routes.push_back( SimulateRoute( settings, "Longjump", { { Action::JumpDuck }, { Action::Duck } } ) );
	routes.push_back( SimulateRoute( settings, "Minijump risk", { { Action::JumpDuck }, { Action::ReleaseDuck } } ) );

	std::sort( routes.begin( ), routes.end( ), []( const RouteCandidate& lhs, const RouteCandidate& rhs ) {
		return lhs.apex_z > rhs.apex_z;
	} );

	return routes;
}

void ClearRoutePoints( )
{
	if ( !g_route_points.empty( ) )
		AppendDebugLog( "routecalc", "points_cleared count=" + std::to_string( g_route_points.size( ) ) );
	g_route_points.clear( );
	g_combo_results.clear( );
	g_last_calculation_message = "points cleared";
	g_selected_route_point = -1;
	g_target_route_point = -1;
}

int AddRoutePoint( const RoutePoint& point )
{
	RoutePoint route_point = point;
	route_point.id = static_cast< int >( g_route_points.size( ) ) + 1;
	if ( route_point.source.empty( ) )
		route_point.source = route_point.type == PointType::Floor ? "manual_floor" : "manual_pixelsurf";
	g_route_points.push_back( route_point );
	g_selected_route_point = static_cast< int >( g_route_points.size( ) ) - 1;
	if ( route_point.type == PointType::PixelSurf )
		g_target_route_point = g_selected_route_point;

	std::ostringstream out;
	out << "add_point id=" << route_point.id << " type=" << ToString( route_point.type ) << " source=" << route_point.source
	    << " pos=" << vec3_string( route_point.position );
	if ( route_point.has_normal )
		out << " normal=" << vec3_string( route_point.normal );
	else
		out << " normal=(unavailable)";
	out << " assist_index=" << route_point.source_pixel_assist_index;
	AppendDebugLog( "routecalc", out.str( ) );
	if ( route_point.source == "pixelsurf_assist_import" )
		AppendDebugLog( "routecalc", "pixelsurf_candidate_imported route_id=" + std::to_string( route_point.id ) +
		                                  " assist_index=" + std::to_string( route_point.source_pixel_assist_index ) );
	return g_selected_route_point;
}

const std::vector< RoutePoint >& GetRoutePoints( )
{
	return g_route_points;
}

bool DeleteRoutePoint( const int index )
{
	if ( index < 0 || index >= static_cast< int >( g_route_points.size( ) ) )
		return false;

	const int deleted_id = g_route_points[ index ].id;
	AppendDebugLog( "routecalc", "delete_point id=" + std::to_string( deleted_id ) + " index=" + std::to_string( index ) );
	g_route_points.erase( g_route_points.begin( ) + index );
	renumber_route_points( );
	return true;
}

bool DeleteSelectedOrLastRoutePoint( )
{
	if ( g_route_points.empty( ) )
		return false;

	const int index = route_point_at( g_selected_route_point ) ? g_selected_route_point : static_cast< int >( g_route_points.size( ) ) - 1;
	return DeleteRoutePoint( index );
}

void SetRoutePointType( const int index, const PointType type )
{
	if ( auto* point = mutable_route_point_at( index ) ) {
		const PointType previous_type = point->type;
		point->type = type;
		if ( point->source.empty( ) || point->source == "manual_floor" || point->source == "manual_pixelsurf" )
			point->source = type == PointType::Floor ? "manual_floor" : "manual_pixelsurf";
		if ( previous_type != type )
			AppendDebugLog( "routecalc", "point_type_changed id=" + std::to_string( point->id ) + " type=" + ToString( type ) );
	}
}

void SetSelectedRoutePointIndex( const int index )
{
	const int previous = g_selected_route_point;
	g_selected_route_point = route_point_at( index ) ? index : -1;
	if ( previous != g_selected_route_point ) {
		const auto* point = route_point_at( g_selected_route_point );
		AppendDebugLog( "routecalc", "selected_point_changed index=" + std::to_string( g_selected_route_point ) +
		                                  " id=" + std::to_string( point ? point->id : 0 ) );
	}
}

int GetSelectedRoutePointIndex( )
{
	return g_selected_route_point;
}

const RoutePoint* GetSelectedRoutePoint( )
{
	return route_point_at( g_selected_route_point );
}

RoutePoint* GetMutableSelectedRoutePoint( )
{
	return mutable_route_point_at( g_selected_route_point );
}

void SetTargetRoutePointIndex( const int index )
{
	const int previous = g_target_route_point;
	g_target_route_point = route_point_at( index ) ? index : -1;
	if ( previous != g_target_route_point ) {
		const auto* point = route_point_at( g_target_route_point );
		AppendDebugLog( "routecalc", "target_point_changed index=" + std::to_string( g_target_route_point ) +
		                                  " id=" + std::to_string( point ? point->id : 0 ) );
	}
}

int GetTargetRoutePointIndex( )
{
	return g_target_route_point;
}

const RoutePoint* GetTargetRoutePoint( )
{
	return route_point_at( g_target_route_point );
}

void SetAimedPixelSurfAssistCandidate( const RoutePoint& point )
{
	g_aimed_pixelsurf_candidate = point;
	g_aimed_pixelsurf_candidate.type = PointType::PixelSurf;
	g_aimed_pixelsurf_candidate.id = 0;
	if ( g_aimed_pixelsurf_candidate.source.empty( ) )
		g_aimed_pixelsurf_candidate.source = "pixelsurf_assist_candidate";
	g_has_aimed_pixelsurf_candidate = true;
}

void ClearAimedPixelSurfAssistCandidate( )
{
	g_has_aimed_pixelsurf_candidate = false;
}

bool HasAimedPixelSurfAssistCandidate( )
{
	return g_has_aimed_pixelsurf_candidate;
}

const RoutePoint* GetAimedPixelSurfAssistCandidate( )
{
	return g_has_aimed_pixelsurf_candidate ? &g_aimed_pixelsurf_candidate : nullptr;
}

std::vector< ComboResult > CalculateSimpleCombos( const int max_displayed, const bool stop_at_max_displayed, const bool strict_validation,
                                                  const float max_calculation_distance, const std::string& manual_observed_combo,
                                                  const std::string& manual_observed_map, const std::string& manual_observed_area )
{
	g_combo_results.clear( );
	g_last_calculation_message = "calculating";

	const RoutePoint* floor = first_point_of_type( PointType::Floor );
	const RoutePoint* pixelsurf = first_point_of_type( PointType::PixelSurf );
	int floor_count = 0;
	int pixelsurf_count = 0;
	for ( const auto& point : g_route_points ) {
		if ( point.type == PointType::Floor )
			++floor_count;
		else if ( point.type == PointType::PixelSurf )
			++pixelsurf_count;
	}

	AppendDebugLog( "routecalc", "calculate start_points=" + std::to_string( g_route_points.size( ) ) + " floor=" +
	                                  std::to_string( floor_count ) + " pixelsurf=" + std::to_string( pixelsurf_count ) +
	                                  " strict=" + std::to_string( strict_validation ? 1 : 0 ) );

	if ( !floor || !pixelsurf ) {
		set_invalid_result( "need at least one floor and one pixelsurf point" );
		return g_combo_results;
	}

	log_calculation_input( *floor, *pixelsurf );

	const Vec3 delta = delta_vec( *floor, *pixelsurf );
	const float distance2d = length_2d( delta );
	const float height_delta = delta.z;
	const bool imported_pixelsurf = pixelsurf->source == "pixelsurf_assist_import" && pixelsurf->source_pixel_assist_index >= 0;
	const bool normal_valid = pixelsurf_normal_valid( *pixelsurf );

	if ( strict_validation ) {
		if ( !imported_pixelsurf ) {
			set_invalid_result( "no_imported_pixelsurf_point" );
			return g_combo_results;
		}
		if ( !pixelsurf->has_normal ) {
			set_invalid_result( "target_normal_missing" );
			return g_combo_results;
		}
		if ( !normal_valid ) {
			std::ostringstream out;
			out << "validate target_id=" << pixelsurf->id << " normal=" << vec3_string( pixelsurf->normal ) << " normal_z=" << pixelsurf->normal.z
			    << " result=reject reason=floor_normal_not_pixelsurf";
			AppendDebugLog( "routecalc", out.str( ) );
			g_last_calculation_message = "no validated route: target normal invalid / floor normal";
			return g_combo_results;
		}
		if ( distance2d > std::max( 64.0f, max_calculation_distance ) ) {
			set_invalid_result( "target_distance_over_limit" );
			return g_combo_results;
		}
	}

	const bool plausible_single_jump_height = height_delta >= -24.0f && height_delta <= 86.0f;
	const ComboResult::Status fallback_status = strict_validation ? ComboResult::Status::DebugHeuristic : ComboResult::Status::Unvalidated;
	const char* fallback_reason = strict_validation ? "basic geometry passed; still debug-only, no collision route search"
	                                                : "unvalidated; vertical-only sim cannot solve map route";

	if ( !manual_observed_combo.empty( ) ) {
		std::ostringstream observed;
		observed << "map=" << ( manual_observed_map.empty( ) ? "unknown" : manual_observed_map ) << " area="
		         << ( manual_observed_area.empty( ) ? "unknown" : manual_observed_area ) << " sequence=\"" << manual_observed_combo << "\" start="
		         << vec3_string( floor->position ) << " target=" << vec3_string( pixelsurf->position ) << " result=manual_reference";
		AppendDebugLog( "routecalc_observed", observed.str( ) );
		add_combo( manual_observed_combo.c_str( ), 98.0f, *floor, *pixelsurf, "manual observation/reference only", ComboResult::Status::ManualObserved,
		           "manual_success/debug_calibration; not solver validated", max_displayed, stop_at_max_displayed );
	}

	if ( strict_validation && !plausible_single_jump_height ) {
		set_invalid_result( "vertical_only_cannot_solve; needs_multi_step_route" );
		return g_combo_results;
	}

	if ( strict_validation )
		AppendDebugLog( "routecalc", "validate result=pass confidence=debug_only" );

	add_combo( "mj + j -> pixelsurf", 72.0f, *floor, *pixelsurf, "debug heuristic; compare against manual observation", fallback_status,
	           fallback_reason, max_displayed, stop_at_max_displayed );

	if ( !strict_validation ) {
		add_combo( "longjump -> jump -> crouch jump -> pixelsurf", 60.0f, *floor, *pixelsurf, "legacy heuristic; unvalidated", ComboResult::Status::Unvalidated,
		           fallback_reason, max_displayed, stop_at_max_displayed );
		add_combo( "jump (stand) -> headbang (ducked) -> jump (ducked) -> minijump -> pixelsurf (stand)", 56.0f, *floor, *pixelsurf,
		           "legacy heuristic; real collision solver TODO", ComboResult::Status::Unvalidated, fallback_reason, max_displayed, stop_at_max_displayed );
		add_combo( "crouchjump -> pixelsurf", 52.0f, *floor, *pixelsurf, "legacy heuristic; unvalidated", ComboResult::Status::Unvalidated, fallback_reason,
		           max_displayed, stop_at_max_displayed );
	}

	if ( !g_combo_results.empty( ) )
		g_last_calculation_message = "solver: vertical-only/debug heuristic; full route search not implemented";

	if ( !stop_at_max_displayed && max_displayed > 0 && static_cast< int >( g_combo_results.size( ) ) > max_displayed )
		g_combo_results.resize( max_displayed );

	return g_combo_results;
}

const std::vector< ComboResult >& GetComboResults( )
{
	return g_combo_results;
}

const ComboResult* GetPrimaryCombo( )
{
	return g_combo_results.empty( ) ? nullptr : &g_combo_results.front( );
}

const std::string& GetLastCalculationMessage( )
{
	return g_last_calculation_message;
}
}
