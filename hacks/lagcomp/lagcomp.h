#pragma once
#include <array>
#include <deque>
#include <optional>
#include "../../game/sdk/includes/includes.h"
#include "../../globals/includes/includes.h"
#include "../entity_cache/entity_cache.h"
class c_base_entity;
class c_net_channel;
class c_user_cmd;
class c_studio_hdr;

namespace Backtrack
{
	void update( int stage ) noexcept;
	void run( c_user_cmd* cmd ) noexcept;
	void AddLatencyToNetwork( c_net_channel* network, float latency ) noexcept;
	void UpdateIncomingSequences( ) noexcept;
	float getExtraTicks( );
	float getLerp( );
	float valid( float simtime );
	struct Record {
		c_vector head;
		c_vector neck;
		c_vector body;
		c_vector legs;
		c_vector origin;
		c_vector max;
		c_vector mins;
		float simulationTime;
		matrix3x4_t matrix[ 256 ];
		studiohdr_t* hdr;
	};

	extern std::deque< Record > records[ 65 ];

	struct Cvars {
		c_cconvar* updateRate;
		c_cconvar* maxUpdateRate;
		c_cconvar* interp;
		c_cconvar* interpRatio;
		c_cconvar* minInterpRatio;
		c_cconvar* maxInterpRatio;
		c_cconvar* maxUnlag;
	};

	extern Cvars cvars;

	struct IncomingSequence {
		int inreliablestate;
		int sequencenr;
		float servertime;
	};

	extern std::deque< IncomingSequence > sequences;

	int timeToTicks( float time ) noexcept;

	static void init( ) noexcept
	{
		records->clear( );

		cvars.updateRate     = g_interfaces.m_convar->find_var( "cl_updaterate" );
		cvars.maxUpdateRate  = g_interfaces.m_convar->find_var( "sv_maxupdaterate" );
		cvars.interp         = g_interfaces.m_convar->find_var( "cl_interp" );
		cvars.interpRatio    = g_interfaces.m_convar->find_var( "cl_interp_ratio" );
		cvars.minInterpRatio = g_interfaces.m_convar->find_var( "sv_client_min_interp_ratio" );
		cvars.maxInterpRatio = g_interfaces.m_convar->find_var( "sv_client_max_interp_ratio" );
		cvars.maxUnlag       = g_interfaces.m_convar->find_var( "sv_maxunlag" );
	}
} // namespace Backtrack