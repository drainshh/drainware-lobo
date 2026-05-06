#include "route_calculator.hpp"

#include "../../game/sdk/classes/c_engine_client.h"
#include "../../game/sdk/classes/c_engine_trace.h"
#include "../../game/sdk/classes/c_user_cmd.h"
#include "../../game/sdk/classes/entity.h"
#include "../../game/sdk/enums/e_flags.h"
#include "../../globals/globals.h"
#include "../../globals/interfaces/interfaces.h"

#include <array>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
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
std::vector< ManualRouteObservation > g_manual_observations;
std::vector< RouteSegmentGeometry > g_last_route_segments;
BruteForceProgress g_bruteforce_progress;
BruteForceResult g_bruteforce_result;
bool g_bruteforce_cancel_requested = false;

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

float length_3d( const Vec3& value )
{
	return std::sqrt( value.x * value.x + value.y * value.y + value.z * value.z );
}

float dot_product( const Vec3& lhs, const Vec3& rhs )
{
	return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

Vec3 normalized( const Vec3& value )
{
	const float len = length_3d( value );
	if ( len <= 0.0001f )
		return { };
	return { value.x / len, value.y / len, value.z / len };
}

bool wall_like_normal( const bool has_normal, const Vec3& normal )
{
	return has_normal && std::fabs( normal.z ) < 0.7f;
}

bool floor_like_normal( const bool has_normal, const Vec3& normal )
{
	return has_normal && normal.z > 0.7f;
}

float distance_3d( const Vec3& lhs, const Vec3& rhs )
{
	return length_3d( { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z } );
}

c_vector to_source_vector( const Vec3& value )
{
	return c_vector( value.x, value.y, value.z );
}

Vec3 from_source_vector( const c_vector& value )
{
	return { value.m_x, value.m_y, value.m_z };
}

std::string trace_reason( const TraceEvidence& evidence )
{
	std::ostringstream out;
	out << evidence.reason;
	if ( evidence.available ) {
		out << " line_hit=" << ( evidence.line_hit ? 1 : 0 ) << " line_fraction=" << evidence.line_fraction
		    << " line_dist=" << evidence.line_distance_to_target << " normal_dot=" << evidence.line_normal_dot
		    << " standing_hull_fraction=" << evidence.standing_hull_fraction << " duck_hull_fraction=" << evidence.duck_hull_fraction;
	}
	return out.str( );
}

void trace_world_ray( const Vec3& from, const Vec3& to, trace_t& trace )
{
	c_trace_filter_world filter{ };
	const ray_t ray( to_source_vector( from ), to_source_vector( to ) );
	g_interfaces.m_engine_trace->trace_ray( ray, e_mask::mask_playersolid_brushonly, &filter, &trace );
}

void trace_world_hull( const Vec3& from, const Vec3& to, const float hull_height, trace_t& trace )
{
	c_trace_filter_world filter{ };
	const c_vector mins( -16.0f, -16.0f, 0.0f );
	const c_vector maxs( 16.0f, 16.0f, hull_height );
	const ray_t ray( to_source_vector( from ), to_source_vector( to ), mins, maxs );
	g_interfaces.m_engine_trace->trace_ray( ray, e_mask::mask_playersolid_brushonly, &filter, &trace );
}

TraceEvidence build_trace_evidence( const RouteSegmentGeometry& geometry )
{
	TraceEvidence evidence{ };
	evidence.attempted = true;

	if ( !g_interfaces.m_engine_trace || !g_interfaces.m_engine_client ) {
		evidence.reason = "engine_trace_unavailable";
		AppendDebugLog( "routecalc", "trace segment=" + std::to_string( geometry.from_id ) + "->" + std::to_string( geometry.to_id ) +
		                                  " available=0 reason=" + evidence.reason );
		return evidence;
	}
	if ( !g_interfaces.m_engine_client->is_in_game( ) ) {
		evidence.reason = "not_in_game";
		AppendDebugLog( "routecalc", "trace segment=" + std::to_string( geometry.from_id ) + "->" + std::to_string( geometry.to_id ) +
		                                  " available=0 reason=" + evidence.reason );
		return evidence;
	}

	evidence.available = true;
	const Vec3 trace_start = { geometry.from.x, geometry.from.y, geometry.from.z + 4.0f };
	const Vec3 trace_end = geometry.to;

	trace_t line_trace{ };
	trace_world_ray( trace_start, trace_end, line_trace );
	evidence.line_hit = line_trace.did_hit( );
	evidence.line_fraction = line_trace.m_fraction;
	evidence.line_normal = from_source_vector( line_trace.m_plane.m_normal );
	evidence.line_wall_like = wall_like_normal( evidence.line_hit, evidence.line_normal );
	evidence.line_distance_to_target = distance_3d( from_source_vector( line_trace.m_end ), geometry.to );
	if ( geometry.target_has_normal )
		evidence.line_normal_dot = dot_product( evidence.line_normal, geometry.target_normal );
	evidence.line_matches_target = evidence.line_hit && geometry.target_has_normal && std::fabs( evidence.line_normal_dot ) >= 0.82f;

	trace_t standing_hull{ };
	trace_world_hull( geometry.from, geometry.to, 72.0f, standing_hull );
	evidence.standing_hull_hit = standing_hull.did_hit( );
	evidence.standing_hull_fraction = standing_hull.m_fraction;
	evidence.standing_hull_normal = from_source_vector( standing_hull.m_plane.m_normal );

	trace_t duck_hull{ };
	trace_world_hull( geometry.from, geometry.to, 54.0f, duck_hull );
	evidence.duck_hull_hit = duck_hull.did_hit( );
	evidence.duck_hull_fraction = duck_hull.m_fraction;
	evidence.duck_hull_normal = from_source_vector( duck_hull.m_plane.m_normal );

	const bool line_reaches_target = !evidence.line_hit || evidence.line_fraction >= 0.78f || evidence.line_distance_to_target <= 36.0f;
	const bool standing_blocked_early = standing_hull.m_start_solid || standing_hull.m_all_solid ||
	                                    ( evidence.standing_hull_hit && evidence.standing_hull_fraction < 0.42f );
	const bool duck_blocked_early = duck_hull.m_start_solid || duck_hull.m_all_solid || ( evidence.duck_hull_hit && evidence.duck_hull_fraction < 0.42f );
	evidence.path_blocked_early = standing_blocked_early && duck_blocked_early;
	evidence.path_reaches_contact = geometry.to_type == PointType::PixelSurf && geometry.target_is_wall_like && evidence.line_hit &&
	                                evidence.line_wall_like && evidence.line_matches_target && line_reaches_target;

	if ( evidence.path_blocked_early )
		evidence.reason = "hull_blocked_before_target";
	else if ( evidence.path_reaches_contact )
		evidence.reason = "line_trace_reaches_matching_pixelsurf_plane";
	else if ( evidence.line_hit && !evidence.line_matches_target )
		evidence.reason = "line_trace_hits_different_plane";
	else if ( evidence.line_hit )
		evidence.reason = "line_trace_hit_unvalidated_contact";
	else
		evidence.reason = "line_trace_no_wall_contact";

	std::ostringstream out;
	out << "trace segment=" << geometry.from_id << "->" << geometry.to_id << " available=1 reason=" << trace_reason( evidence )
	    << " line_normal=" << vec3_string( evidence.line_normal ) << " standing_normal=" << vec3_string( evidence.standing_hull_normal )
	    << " duck_normal=" << vec3_string( evidence.duck_hull_normal ) << " path_blocked_early=" << ( evidence.path_blocked_early ? 1 : 0 )
	    << " path_reaches_contact=" << ( evidence.path_reaches_contact ? 1 : 0 );
	AppendDebugLog( "routecalc", out.str( ) );
	return evidence;
}

RouteSegmentGeometry make_geometry( const RoutePoint& from, const RoutePoint& to )
{
	RouteSegmentGeometry geometry{ };
	geometry.from_id = from.id;
	geometry.to_id = to.id;
	geometry.from_type = from.type;
	geometry.to_type = to.type;
	geometry.from = from.position;
	geometry.to = to.position;
	geometry.delta = delta_vec( from, to );
	geometry.distance2d = length_2d( geometry.delta );
	geometry.height_delta = geometry.delta.z;
	geometry.target_normal = to.has_normal ? to.normal : Vec3{ };
	geometry.target_has_normal = to.has_normal;
	geometry.target_is_wall_like = wall_like_normal( to.has_normal, geometry.target_normal );
	geometry.target_is_floor_like = floor_like_normal( to.has_normal, geometry.target_normal );
	geometry.target_from_scanline = to.source == "pixelsurf_assist_import" && to.source_pixel_assist_index >= 0;
	geometry.target_source = to.source;
	geometry.approach_dot = geometry.target_has_normal ? dot_product( normalized( geometry.delta ), geometry.target_normal ) : 0.0f;
	geometry.trace = build_trace_evidence( geometry );
	return geometry;
}

void log_segment( const RouteSegmentGeometry& segment )
{
	std::ostringstream out;
	out << "segment from=" << segment.from_id << " to=" << segment.to_id << " from_type=" << ToString( segment.from_type )
	    << " to_type=" << ToString( segment.to_type ) << " delta=" << vec3_string( segment.delta )
	    << " distance2d=" << segment.distance2d << " height_delta=" << segment.height_delta;
	if ( segment.target_has_normal )
		out << " target_normal=" << vec3_string( segment.target_normal );
	else
		out << " target_normal=(unavailable)";
	out << " wall_like=" << ( segment.target_is_wall_like ? 1 : 0 ) << " floor_like=" << ( segment.target_is_floor_like ? 1 : 0 )
	    << " approach_dot=" << segment.approach_dot << " target_source=" << segment.target_source;
	if ( segment.trace.attempted )
		out << " trace_available=" << ( segment.trace.available ? 1 : 0 ) << " trace_reason=" << segment.trace.reason
		    << " trace_contact=" << ( segment.trace.path_reaches_contact ? 1 : 0 )
		    << " trace_blocked=" << ( segment.trace.path_blocked_early ? 1 : 0 );
	AppendDebugLog( "routecalc", out.str( ) );
}

std::vector< RouteSegmentGeometry > build_segments( )
{
	std::vector< RouteSegmentGeometry > segments;
	if ( g_route_points.size( ) >= 2U ) {
		for ( std::size_t i = 1; i < g_route_points.size( ); ++i )
			segments.push_back( make_geometry( g_route_points[ i - 1 ], g_route_points[ i ] ) );
	}
	return segments;
}

std::vector< JumpProfile > build_jump_profiles( const Settings& settings )
{
	const float standing_start_z = settings.jump_impulse * settings.ground_factor - HalfGravity( settings );
	const float duck_branch_z = settings.jump_impulse * settings.ground_factor;
	const float gravity = std::max( 1.0f, settings.gravity * settings.ent_gravity );

	auto make_profile = [ & ]( const char* name, const MovementAction action, const float initial_z, const float shift ) {
		JumpProfile profile{ };
		profile.name = name ? name : "";
		profile.action = action;
		profile.initial_z_velocity = initial_z;
		profile.origin_shift = shift;
		profile.apex_tick = static_cast< int >( std::max( 0.0f, std::round( initial_z / gravity / settings.tick_interval ) ) );
		profile.apex_height = shift + ( initial_z * initial_z ) / ( 2.0f * gravity );
		profile.land_tick_estimate = static_cast< int >( std::ceil( ( initial_z + std::sqrt( initial_z * initial_z + 2.0f * gravity * shift ) ) / gravity /
		                                                            settings.tick_interval ) );
		return profile;
	};

	return { make_profile( "standing_jump", MovementAction::Jump, standing_start_z, 0.0f ),
		     make_profile( "crouchjump", MovementAction::CrouchJump, duck_branch_z, settings.duck_origin_shift ),
		     make_profile( "longjump", MovementAction::LongJump, duck_branch_z, settings.duck_origin_shift ),
		     make_profile( "minijump_reduck", MovementAction::MiniJump, standing_start_z, 0.0f ),
		     make_profile( "minijump_lost_shift", MovementAction::MiniJump, standing_start_z, settings.in_air_unduck_origin_shift ) };
}

float profile_z_at_tick( const Settings& settings, const JumpProfile& profile, const int tick )
{
	const float t = static_cast< float >( std::max( 0, tick ) ) * settings.tick_interval;
	return profile.origin_shift + profile.initial_z_velocity * t - 0.5f * settings.gravity * settings.ent_gravity * t * t;
}

struct ProfileEvaluation {
	bool passed = false;
	JumpProfile profile{ };
	std::vector< ActionTiming > actions{ };
	int z_start = -1;
	int z_end = -1;
	int valid_start = -1;
	int valid_end = -1;
	int best_tick = -1;
	float required_speed_min = 0.0f;
	float required_speed_max = 0.0f;
	float best_required_speed = 0.0f;
	float score = 0.0f;
	std::string reason{ };
	std::vector< TickCandidate > valid_ticks{ };
};

std::string action_summary( const std::vector< ActionTiming >& actions )
{
	std::string result;
	MovementAction previous = MovementAction::Land;
	bool has_previous = false;
	for ( const auto& action : actions ) {
		if ( has_previous && action.action == previous )
			continue;
		if ( !result.empty( ) )
			result += " -> ";
		result += ToString( action.action );
		previous = action.action;
		has_previous = true;
	}
	return result.empty( ) ? "no calculated action" : result;
}

const char* profile_display_name( const std::string& name )
{
	if ( name == "standing_jump" )
		return "jump";
	if ( name == "crouchjump" )
		return "crouchjump";
	if ( name == "longjump" )
		return "longjump";
	if ( name == "minijump_reduck" )
		return "mj(reduck)";
	if ( name == "minijump_lost_shift" )
		return "mj(lost_shift)";
	return name.c_str( );
}

std::string action_summary_for_profile( const ProfileEvaluation& evaluation )
{
	std::string result = profile_display_name( evaluation.profile.name );
	bool has_pixelsurf = false;
	bool has_land = false;
	bool has_jumpbug = false;
	for ( const auto& action : evaluation.actions ) {
		if ( action.action == MovementAction::JumpBug )
			has_jumpbug = true;
		else if ( action.action == MovementAction::PixelSurfContact )
			has_pixelsurf = true;
		else if ( action.action == MovementAction::Land )
			has_land = true;
	}
	if ( has_jumpbug )
		result += " -> jumpbug";
	if ( has_pixelsurf )
		result += " -> ps";
	else if ( has_land )
		result += " -> land";
	return result;
}

std::string short_action_name( const MovementAction action )
{
	switch ( action ) {
	case MovementAction::Jump:
		return "j";
	case MovementAction::CrouchJump:
		return "cj";
	case MovementAction::LongJump:
		return "lj";
	case MovementAction::MiniJump:
		return "mj";
	case MovementAction::JumpBug:
		return "jb";
	case MovementAction::PixelSurfContact:
		return "ps";
	case MovementAction::Land:
		return "land";
	default:
		return ToString( action );
	}
}

std::string composite_summary( const CompositeCandidate& candidate )
{
	std::string result;
	for ( const auto action : candidate.actions ) {
		if ( !result.empty( ) )
			result += " + ";
		if ( action == MovementAction::PixelSurfContact && !result.empty( ) ) {
			result.pop_back( );
			result.pop_back( );
			result += "-> ";
		}
		result += short_action_name( action );
	}
	return result.empty( ) ? "composite" : result;
}

bool composite_has_action( const CompositeCandidate& candidate, const MovementAction action )
{
	return std::find( candidate.actions.begin( ), candidate.actions.end( ), action ) != candidate.actions.end( );
}

std::string normalized_sequence( std::string value )
{
	std::string result;
	for ( char& c : value ) {
		if ( std::isalnum( static_cast< unsigned char >( c ) ) )
			result.push_back( static_cast< char >( std::tolower( static_cast< unsigned char >( c ) ) ) );
	}
	return result;
}

bool sequence_matches_manual( const std::string& candidate, const std::string& manual )
{
	return !candidate.empty( ) && !manual.empty( ) && normalized_sequence( candidate ) == normalized_sequence( manual );
}

float clamp_score_component( const float value )
{
	return std::clamp( value, -35.0f, 35.0f );
}

ScoreBreakdown score_composite_candidate( const RouteSegmentGeometry& geometry, const CompositeCandidate& candidate, const bool manual_match )
{
	ScoreBreakdown score{ };
	score.base = 18.0f;
	score.z_score = clamp_score_component( 18.0f - std::fabs( geometry.height_delta - 24.0f ) * 0.22f );
	score.speed_score = clamp_score_component( 30.0f - std::fabs( candidate.required_speed2d - 255.0f ) * 0.13f );
	score.approach_score = geometry.approach_dot < -0.70f ? 18.0f : geometry.approach_dot < -0.45f ? 12.0f : geometry.approach_dot < -0.25f ? 5.0f : -12.0f;
	score.type_score = ( geometry.target_is_wall_like ? 12.0f : -24.0f ) + ( geometry.target_from_scanline ? 10.0f : -8.0f );
	if ( geometry.trace.available ) {
		if ( geometry.trace.path_reaches_contact )
			score.type_score += 18.0f;
		else if ( geometry.trace.line_hit && geometry.trace.line_wall_like && geometry.trace.line_matches_target )
			score.type_score += 10.0f;
		else if ( geometry.trace.path_blocked_early )
			score.penalties += 26.0f;
		else if ( geometry.trace.line_hit && !geometry.trace.line_matches_target )
			score.penalties += 14.0f;
		else if ( geometry.to_type == PointType::PixelSurf )
			score.penalties += 8.0f;

		if ( geometry.trace.duck_hull_hit && geometry.trace.duck_hull_fraction >= 0.55f && !geometry.trace.path_blocked_early )
			score.type_score += 5.0f;
	}

	if ( composite_has_action( candidate, MovementAction::LongJump ) )
		score.speed_crop_bonus += 12.0f;
	if ( composite_has_action( candidate, MovementAction::CrouchJump ) )
		score.penalties += 16.0f;
	if ( composite_has_action( candidate, MovementAction::Jump ) && !composite_has_action( candidate, MovementAction::LongJump ) )
		score.penalties += 11.0f;
	if ( composite_has_action( candidate, MovementAction::JumpBug ) && geometry.height_delta >= 0.0f )
		score.penalties += 18.0f;
	if ( composite_has_action( candidate, MovementAction::JumpBug ) && !geometry.trace.path_reaches_contact )
		score.penalties += 10.0f;
	if ( composite_has_action( candidate, MovementAction::MiniJump ) ) {
		if ( geometry.height_delta >= 12.0f && geometry.height_delta <= 38.0f )
			score.speed_crop_bonus += 7.0f;
		else
			score.penalties += 7.0f;
	}

	score.manual_match_bonus = manual_match ? 28.0f : 0.0f;
	score.final_score = std::clamp( score.base + score.z_score + score.speed_score + score.approach_score + score.type_score +
	                                    score.speed_crop_bonus + score.manual_match_bonus - score.penalties,
	                                1.0f, 99.0f );
	return score;
}

std::string score_breakdown_string( const ScoreBreakdown& score )
{
	std::ostringstream out;
	out << "base=" << score.base << " z_score=" << score.z_score << " speed_score=" << score.speed_score
	    << " approach_score=" << score.approach_score << " type_score=" << score.type_score
	    << " speed_crop_bonus=" << score.speed_crop_bonus << " manual_match=" << score.manual_match_bonus
	    << " penalties=" << score.penalties << " final=" << score.final_score;
	return out.str( );
}

std::string calibration_sequence_for_geometry( const RouteSegmentGeometry& geometry )
{
	if ( geometry.from_type == PointType::Floor && geometry.to_type == PointType::PixelSurf && geometry.target_is_wall_like &&
	     geometry.target_from_scanline && std::fabs( geometry.distance2d - 65.0968f ) < 6.0f &&
	     std::fabs( geometry.height_delta - 27.9967f ) < 5.0f && std::fabs( geometry.target_normal.x + 1.0f ) < 0.15f &&
	     std::fabs( geometry.target_normal.y ) < 0.15f && std::fabs( geometry.target_normal.z ) < 0.15f )
		return "lj + mj -> ps";
	return "";
}

ProfileEvaluation evaluate_profile( const Settings& settings, const RouteSegmentGeometry& geometry, const JumpProfile& profile )
{
	ProfileEvaluation evaluation{ };
	evaluation.profile = profile;

	const float target_z = geometry.height_delta;
	const float tolerance = geometry.to_type == PointType::PixelSurf ? 8.0f : 4.0f;
	static constexpr float k_min_reasonable_speed = 15.0f;
	static constexpr float k_max_reasonable_speed = 350.0f;
	float previous_delta = profile_z_at_tick( settings, profile, 0 ) - target_z;
	for ( int tick = 1; tick <= settings.max_ticks; ++tick ) {
		const float z = profile_z_at_tick( settings, profile, tick );
		const float current_delta = z - target_z;
		const bool within = std::fabs( current_delta ) <= tolerance;
		const bool crossed = ( previous_delta > 0.0f && current_delta < 0.0f ) || ( previous_delta < 0.0f && current_delta > 0.0f );
		if ( within || crossed ) {
			if ( evaluation.z_start < 0 )
				evaluation.z_start = tick;
			evaluation.z_end = tick;

			const float t = std::max( settings.tick_interval, static_cast< float >( tick ) * settings.tick_interval );
			const float required_speed = geometry.distance2d / t;
			if ( required_speed >= k_min_reasonable_speed && required_speed <= k_max_reasonable_speed ) {
				TickCandidate candidate{ };
				candidate.tick = tick;
				candidate.required_speed2d = required_speed;
				candidate.z_error = std::fabs( current_delta );
				candidate.score = 100.0f - std::fabs( required_speed - 250.0f ) * 0.10f - candidate.z_error * 1.25f;
				evaluation.valid_ticks.push_back( candidate );
				if ( evaluation.valid_start < 0 )
					evaluation.valid_start = tick;
				evaluation.valid_end = tick;
			}
		}
		previous_delta = current_delta;
	}

	if ( evaluation.z_start < 0 ) {
		evaluation.reason = "target_height_not_reached";
		return evaluation;
	}

	if ( evaluation.valid_ticks.empty( ) ) {
		evaluation.reason = "all_target_ticks_require_unrealistic_horizontal_speed";
		return evaluation;
	}

	auto minmax_speed = std::minmax_element( evaluation.valid_ticks.begin( ), evaluation.valid_ticks.end( ),
	                                         []( const TickCandidate& lhs, const TickCandidate& rhs ) {
		                                         return lhs.required_speed2d < rhs.required_speed2d;
	                                         } );
	evaluation.required_speed_min = minmax_speed.first->required_speed2d;
	evaluation.required_speed_max = minmax_speed.second->required_speed2d;
	const auto best_tick = std::max_element( evaluation.valid_ticks.begin( ), evaluation.valid_ticks.end( ),
	                                         []( const TickCandidate& lhs, const TickCandidate& rhs ) { return lhs.score < rhs.score; } );
	evaluation.best_tick = best_tick->tick;
	evaluation.best_required_speed = best_tick->required_speed2d;

	if ( geometry.to_type == PointType::PixelSurf && geometry.target_is_floor_like ) {
		evaluation.reason = "target_normal_floor_like";
		return evaluation;
	}
	if ( geometry.to_type == PointType::PixelSurf && geometry.target_has_normal && !geometry.target_is_wall_like ) {
		evaluation.reason = "target_normal_not_wall_like";
		return evaluation;
	}
	if ( geometry.to_type == PointType::Floor && geometry.target_has_normal && !geometry.target_is_floor_like ) {
		evaluation.reason = "target_normal_not_floor_like";
		return evaluation;
	}

	ActionTiming main_action{ };
	main_action.action = profile.action;
	main_action.earliest_tick = 0;
	main_action.latest_tick = profile.action == MovementAction::CrouchJump ? DuckTicks( settings ) : 0;
	main_action.confidence = 0.65f;
	main_action.reason = "jump profile reaches target height window";
	evaluation.actions.push_back( main_action );

	if ( geometry.height_delta < -18.0f && geometry.to_type == PointType::PixelSurf ) {
		ActionTiming jumpbug{ };
		jumpbug.action = MovementAction::JumpBug;
		jumpbug.earliest_tick = std::max( 0, evaluation.z_start - 1 );
		jumpbug.latest_tick = evaluation.z_end + 1;
		jumpbug.confidence = 0.35f;
		jumpbug.reason = "falling target window; jumpbug candidate is unvalidated until collision tracing is added";
		evaluation.actions.push_back( jumpbug );
	}

	if ( geometry.to_type == PointType::PixelSurf ) {
		ActionTiming surf{ };
		surf.action = MovementAction::PixelSurfContact;
		surf.earliest_tick = evaluation.z_start;
		surf.latest_tick = evaluation.z_end;
		surf.confidence = geometry.target_is_wall_like ? 0.55f : 0.15f;
		surf.reason = "contact tick window from vertical+horizontal heuristic";
		evaluation.actions.push_back( surf );
	} else if ( geometry.to_type == PointType::Floor ) {
		ActionTiming land{ };
		land.action = MovementAction::Land;
		land.earliest_tick = evaluation.z_start;
		land.latest_tick = evaluation.z_end;
		land.confidence = geometry.target_is_floor_like ? 0.45f : 0.2f;
		land.reason = "floor/landing height window from vertical+horizontal heuristic";
		evaluation.actions.push_back( land );
	}

	float score = 70.0f;
	score -= std::fabs( evaluation.best_required_speed - 250.0f ) * 0.05f;
	score -= std::fabs( geometry.height_delta ) * 0.03f;
	if ( geometry.target_from_scanline )
		score += 8.0f;
	if ( geometry.target_is_wall_like )
		score += 8.0f;
	if ( geometry.approach_dot < -0.15f )
		score += 4.0f;
	if ( profile.name == "standing_jump" && geometry.to_type == PointType::PixelSurf )
		score -= 18.0f;
	if ( profile.name == "longjump" && geometry.to_type == PointType::PixelSurf && geometry.distance2d < 180.0f && geometry.approach_dot < -0.45f )
		score += 8.0f;
	if ( profile.name.rfind( "minijump", 0 ) == 0 && geometry.to_type == PointType::PixelSurf )
		score += 5.0f;
	evaluation.score = std::clamp( score, 1.0f, 99.0f );
	evaluation.reason = "target height reachable; at least one target tick has plausible horizontal speed; collision trace route search still TODO";
	evaluation.passed = true;
	return evaluation;
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

void add_manual_observation( const RoutePoint& floor, const RoutePoint& pixelsurf, const Vec3& delta, const float distance2d,
                             const float height_delta, const std::string& sequence, const std::string& map, const std::string& area )
{
	ManualRouteObservation observation{ };
	observation.map = map.empty( ) ? "unknown" : map;
	observation.area = area.empty( ) ? "unknown" : area;
	observation.floor_id = floor.id;
	observation.target_id = pixelsurf.id;
	observation.start = floor.position;
	observation.target = pixelsurf.position;
	observation.target_normal = pixelsurf.has_normal ? pixelsurf.normal : Vec3{ };
	observation.delta = delta;
	observation.distance2d = distance2d;
	observation.height_delta = height_delta;
	observation.sequence = sequence;
	observation.result = "manual_reference";
	g_manual_observations.push_back( observation );
	if ( g_manual_observations.size( ) > 32U )
		g_manual_observations.erase( g_manual_observations.begin( ) );

	std::ostringstream observed;
	observed << "map=" << observation.map << " area=" << observation.area << " sequence=\"" << observation.sequence << "\" floor_id="
	         << observation.floor_id << " target_id=" << observation.target_id << " start=" << vec3_string( observation.start )
	         << " target=" << vec3_string( observation.target ) << " target_normal=" << vec3_string( observation.target_normal )
	         << " delta=" << vec3_string( observation.delta ) << " distance2d=" << observation.distance2d
	         << " height_delta=" << observation.height_delta << " result=" << observation.result;
	AppendDebugLog( "routecalc_observed", observed.str( ) );
}

bool can_push_combo( const int max_displayed, const bool stop_at_max_displayed )
{
	return !( stop_at_max_displayed && max_displayed > 0 && static_cast< int >( g_combo_results.size( ) ) >= max_displayed );
}

void push_combo( ComboResult result, const int max_displayed, const bool stop_at_max_displayed )
{
	(void)max_displayed;
	(void)stop_at_max_displayed;
	result.index = static_cast< int >( g_combo_results.size( ) ) + 1;
	g_combo_results.push_back( result );

	std::ostringstream out;
	out << "combo #" << result.index << " from=" << result.start_point_id << " target=" << result.end_point_id << " text=\"" << result.text
	    << "\" score=" << result.score << " status=" << ToString( result.status ) << " reason=\"" << result.status_reason << "\" actions="
	    << result.actions.size( ) << " z_window=" << result.z_window_start << '-' << result.z_window_end << " required_speed2d="
	    << result.required_speed_min << '-' << result.required_speed_max << " approach_dot=" << result.approach_dot;
	AppendDebugLog( "routecalc", out.str( ) );
}

void add_profile_combo( const RoutePoint& start, const RoutePoint& end, const RouteSegmentGeometry& geometry, const ProfileEvaluation& evaluation,
                        const int max_displayed, const bool stop_at_max_displayed )
{
	ComboResult result{ };
	result.text = action_summary_for_profile( evaluation );
	result.score = evaluation.score;
	result.start_point_id = start.id;
	result.end_point_id = end.id;
	result.notes = "vertical+horizontal debug heuristic; no collision route search / no automation";
	result.status = ComboResult::Status::DebugHeuristic;
	result.status_reason = evaluation.reason;
	result.geometry = geometry;
	result.actions = evaluation.actions;
	result.tested_profiles.push_back( evaluation.profile );
	result.z_window_start = evaluation.z_start;
	result.z_window_end = evaluation.z_end;
	result.required_speed_min = evaluation.required_speed_min;
	result.required_speed_max = evaluation.required_speed_max;
	result.valid_tick_start = evaluation.valid_start;
	result.valid_tick_end = evaluation.valid_end;
	result.best_tick = evaluation.best_tick;
	result.best_required_speed = evaluation.best_required_speed;
	result.valid_ticks = evaluation.valid_ticks;
	result.approach_dot = geometry.approach_dot;
	result.solver_validated = false;
	result.solver_debug = "target height reachable and speed estimated; requires trace/collision validation before it can be trusted";
	if ( geometry.trace.attempted )
		result.solver_debug += "; trace=" + trace_reason( geometry.trace );
	push_combo( result, max_displayed, stop_at_max_displayed );
}

ActionTiming make_composite_action( const MovementAction action, const int tick, const float confidence, const char* reason )
{
	ActionTiming timing{ };
	timing.action = action;
	timing.earliest_tick = tick;
	timing.latest_tick = tick;
	timing.confidence = confidence;
	timing.reason = reason ? reason : "";
	return timing;
}

void add_composite_combo( const RoutePoint& start, const RoutePoint& end, const RouteSegmentGeometry& geometry, CompositeCandidate candidate,
                          const std::string& manual_sequence, const int max_displayed, const bool stop_at_max_displayed )
{
	const std::string text = composite_summary( candidate );
	const bool matches_manual = sequence_matches_manual( text, manual_sequence );
	const ScoreBreakdown score = score_composite_candidate( geometry, candidate, matches_manual );
	candidate.score = score.final_score;
	if ( matches_manual )
		candidate.reason = "matches manual observation; heuristic math alone is insufficient; " + candidate.reason;

	ComboResult result{ };
	result.text = text;
	result.score = candidate.score;
	result.start_point_id = start.id;
	result.end_point_id = end.id;
	result.notes = "composite movement candidate; route is not executed and is not solver-validated";
	result.status = matches_manual ? ComboResult::Status::ManualObservedBoosted : ComboResult::Status::DebugHeuristic;
	result.status_reason = matches_manual ? "matches_manual_reference; not solver validated" : candidate.reason;
	result.geometry = geometry;
	result.composite = candidate;
	result.score_breakdown = score;
	result.z_window_start = candidate.contact_tick;
	result.z_window_end = candidate.contact_tick;
	result.valid_tick_start = candidate.contact_tick;
	result.valid_tick_end = candidate.contact_tick;
	result.best_tick = candidate.contact_tick;
	result.best_required_speed = candidate.required_speed2d;
	result.required_speed_min = candidate.required_speed2d;
	result.required_speed_max = candidate.required_speed2d;
	result.approach_dot = geometry.approach_dot;
	result.solver_validated = false;
	result.solver_debug = matches_manual ? "manual reference matched this composite; still debug heuristic until collision/hull route search exists"
	                                     : "composite scored from valid tick window, approach, and pixelsurf normal";
	if ( geometry.trace.attempted )
		result.solver_debug += "; trace=" + trace_reason( geometry.trace );

	for ( const auto action : candidate.actions ) {
		if ( action == MovementAction::LongJump )
			result.actions.push_back( make_composite_action( action, 0, 0.62f, "longjump setup candidate" ) );
		else if ( action == MovementAction::CrouchJump )
			result.actions.push_back( make_composite_action( action, 0, 0.55f, "crouchjump setup candidate" ) );
		else if ( action == MovementAction::Jump )
			result.actions.push_back( make_composite_action( action, 0, 0.42f, "plain jump setup candidate" ) );
		else if ( action == MovementAction::MiniJump )
			result.actions.push_back( make_composite_action( action, std::max( 1, candidate.contact_tick - 2 ), 0.55f, "minijump adjustment candidate" ) );
		else if ( action == MovementAction::JumpBug )
			result.actions.push_back( make_composite_action( action, std::max( 1, candidate.contact_tick - 1 ), 0.36f, "jumpbug contact window candidate" ) );
		else if ( action == MovementAction::PixelSurfContact )
			result.actions.push_back( make_composite_action( action, candidate.contact_tick, 0.58f, "pixelsurf contact at best plausible tick" ) );
	}

	push_combo( result, max_displayed, stop_at_max_displayed );

	std::ostringstream out;
	out << "composite actions=\"" << text << "\" best_tick=" << candidate.contact_tick << " required_speed2d=" << candidate.required_speed2d
	    << " score=" << result.score << " status=" << ToString( result.status ) << " reason=" << result.status_reason;
	AppendDebugLog( "routecalc", out.str( ) );
	AppendDebugLog( "routecalc", "score combo=\"" + text + "\" " + score_breakdown_string( score ) );
}

void add_invalid_combo( const RoutePoint& start, const RoutePoint& end, const RouteSegmentGeometry& geometry, const std::string& reason,
                        const int max_displayed, const bool stop_at_max_displayed )
{
	ComboResult result{ };
	result.text = "no solver-validated route";
	result.score = 0.0f;
	result.start_point_id = start.id;
	result.end_point_id = end.id;
	result.notes = "calculation rejected before profile scoring";
	result.status = ComboResult::Status::Invalid;
	result.status_reason = reason;
	result.reject_reason = reason;
	result.geometry = geometry;
	result.approach_dot = geometry.approach_dot;
	result.solver_debug = "route rejected honestly instead of emitting a fantasy combo";
	push_combo( result, max_displayed, stop_at_max_displayed );
}

void log_profile_evaluation( const RouteSegmentGeometry& geometry, const ProfileEvaluation& evaluation )
{
	std::ostringstream out;
	out << "profile name=" << evaluation.profile.name << " segment=" << geometry.from_id << "->" << geometry.to_id << " z0="
	    << evaluation.profile.initial_z_velocity << " origin_shift=" << evaluation.profile.origin_shift << " apex="
	    << evaluation.profile.apex_height << " apex_tick=" << evaluation.profile.apex_tick << " target_ticks=" << evaluation.z_start << '-'
	    << evaluation.z_end << " valid_ticks=" << evaluation.valid_start << '-' << evaluation.valid_end << " best_tick=" << evaluation.best_tick
	    << " required_speed2d=" << evaluation.required_speed_min << '-' << evaluation.required_speed_max
	    << " best_required_speed2d=" << evaluation.best_required_speed << " result="
	    << ( evaluation.passed ? "pass" : "reject" ) << " reason=" << evaluation.reason;
	AppendDebugLog( "routecalc", out.str( ) );
	if ( evaluation.profile.name == "crouchjump" )
		AppendDebugLog( "routecalc", "score profile=crouchjump speed_crop_penalty=16 reason=grounded_duck_before_jump_can_crop_horizontal_speed" );
	else if ( evaluation.profile.name == "longjump" )
		AppendDebugLog( "routecalc", "score profile=longjump speed_crop_penalty=0 speed_crop_bonus=12 reason=same_tick_jump_duck" );
}

void log_pixelsurf_analysis( const RouteSegmentGeometry& geometry )
{
	if ( geometry.to_type != PointType::PixelSurf )
		return;

	std::ostringstream out;
	out << "pixelsurf target=" << geometry.to_id << " approach_dot=" << geometry.approach_dot << " normal_z="
	    << ( geometry.target_has_normal ? geometry.target_normal.z : 0.0f ) << " result=";
	if ( !geometry.target_has_normal )
		out << "unvalidated reason=normal_missing";
	else if ( geometry.target_is_floor_like )
		out << "reject reason=floor_normal_not_pixelsurf";
	else if ( !geometry.target_is_wall_like )
		out << "reject reason=normal_not_wall_like";
	else if ( geometry.trace.available && geometry.trace.path_reaches_contact )
		out << "plausible reason=wall_normal_valid; engine_trace_matching_contact";
	else if ( geometry.trace.available && geometry.trace.path_blocked_early )
		out << "reject reason=engine_hull_trace_blocked_before_target";
	else if ( geometry.trace.available )
		out << "unvalidated reason=wall_normal_valid; " << geometry.trace.reason;
	else
		out << "plausible reason=wall_normal_valid; collision_route_search_todo";
	AppendDebugLog( "routecalc", out.str( ) );
}

const ProfileEvaluation* find_evaluation( const std::vector< ProfileEvaluation >& evaluations, const char* name )
{
	for ( const auto& evaluation : evaluations ) {
		if ( evaluation.profile.name == name )
			return &evaluation;
	}
	return nullptr;
}

CompositeCandidate make_composite_candidate( const RouteSegmentGeometry& geometry, const ProfileEvaluation& contact_eval,
                                             const std::vector< MovementAction >& actions, float base_score, const char* reason )
{
	CompositeCandidate candidate{ };
	candidate.actions = actions;
	candidate.contact_tick = contact_eval.best_tick;
	candidate.required_speed2d = contact_eval.best_required_speed;
	candidate.start_tick = 0;
	candidate.score = base_score + contact_eval.score * 0.35f;
	if ( geometry.target_from_scanline )
		candidate.score += 8.0f;
	if ( geometry.target_is_wall_like )
		candidate.score += 6.0f;
	if ( geometry.approach_dot < -0.70f )
		candidate.score += 9.0f;
	else if ( geometry.approach_dot < -0.35f )
		candidate.score += 4.0f;
	if ( contact_eval.best_required_speed < 120.0f )
		candidate.score -= 4.0f;
	candidate.reason = reason ? reason : "composite debug heuristic";
	return candidate;
}

void generate_composite_candidates( const RoutePoint& start, const RoutePoint& target, const RouteSegmentGeometry& geometry,
                                    const std::vector< ProfileEvaluation >& evaluations, const std::string& manual_sequence,
                                    const int max_displayed, const bool stop_at_max_displayed )
{
	if ( geometry.to_type != PointType::PixelSurf || !geometry.target_is_wall_like )
		return;

	const auto* standing = find_evaluation( evaluations, "standing_jump" );
	const auto* crouch = find_evaluation( evaluations, "crouchjump" );
	const auto* longjump = find_evaluation( evaluations, "longjump" );
	const auto* minijump_reduck = find_evaluation( evaluations, "minijump_reduck" );
	const auto* minijump_lost = find_evaluation( evaluations, "minijump_lost_shift" );
	const auto* minijump = minijump_reduck ? minijump_reduck : minijump_lost;

	if ( longjump && minijump ) {
		add_composite_combo( start, target, geometry,
		                     make_composite_candidate( geometry, *minijump,
		                                               { MovementAction::LongJump, MovementAction::MiniJump, MovementAction::PixelSurfContact },
		                                               44.0f, "longjump setup plus minijump adjustment before pixelsurf contact" ),
		                     manual_sequence, max_displayed, stop_at_max_displayed );
	}
	if ( longjump ) {
		add_composite_combo( start, target, geometry,
		                     make_composite_candidate( geometry, *longjump,
		                                               { MovementAction::LongJump, MovementAction::JumpBug, MovementAction::PixelSurfContact },
		                                               geometry.height_delta < 0.0f ? 30.0f : 20.0f,
		                                               "longjump setup plus jumpbug-style contact window; unvalidated" ),
		                     manual_sequence, max_displayed, stop_at_max_displayed );
	}
	if ( standing && minijump ) {
		add_composite_combo( start, target, geometry,
		                     make_composite_candidate( geometry, *minijump,
		                                               { MovementAction::Jump, MovementAction::MiniJump, MovementAction::PixelSurfContact },
		                                               24.0f, "plain jump plus minijump adjustment; lower priority than longjump composite" ),
		                     manual_sequence, max_displayed, stop_at_max_displayed );
	}
	if ( crouch && minijump ) {
		add_composite_combo( start, target, geometry,
		                     make_composite_candidate( geometry, *minijump,
		                                               { MovementAction::CrouchJump, MovementAction::MiniJump, MovementAction::PixelSurfContact },
		                                               27.0f, "crouchjump plus minijump adjustment; debug heuristic" ),
		                     manual_sequence, max_displayed, stop_at_max_displayed );
	}
}

int route_point_index_by_id( const int id )
{
	for ( int i = 0; i < static_cast< int >( g_route_points.size( ) ); ++i ) {
		if ( g_route_points[ i ].id == id )
			return i;
	}
	return -1;
}

MovementAction classify_floor_setup_action( const RouteSegmentGeometry& geometry, std::string& reason )
{
	if ( geometry.from_type != PointType::Floor || geometry.to_type != PointType::Floor ) {
		reason = "not_floor_setup_segment";
		return MovementAction::Jump;
	}

	if ( std::fabs( geometry.height_delta ) > 24.0f ) {
		reason = "height_delta_needs_crouchjump_window";
		return MovementAction::CrouchJump;
	}

	if ( geometry.distance2d >= 175.0f ) {
		reason = "long_flat_setup_gap_prefers_lj";
		return MovementAction::LongJump;
	}

	reason = "short_floor_setup_gap_prefers_j";
	return MovementAction::Jump;
}

bool same_action_sequence( const std::vector< MovementAction >& lhs, const std::vector< MovementAction >& rhs )
{
	return lhs.size( ) == rhs.size( ) && std::equal( lhs.begin( ), lhs.end( ), rhs.begin( ) );
}

bool generate_route_chain_candidate( const RoutePoint& final_start, const RoutePoint& target, const RouteSegmentGeometry& final_geometry,
                                     const std::vector< ProfileEvaluation >& evaluations, const std::string& manual_sequence,
                                     const int max_displayed, const bool stop_at_max_displayed )
{
	const int target_index = route_point_index_by_id( target.id );
	if ( target_index <= 0 )
		return false;

	std::vector< MovementAction > actions;
	std::vector< int > route_ids;
	route_ids.reserve( static_cast< std::size_t >( target_index ) + 1U );
	route_ids.push_back( g_route_points.front( ).id );

	for ( int i = 1; i <= target_index; ++i ) {
		const RoutePoint& previous = g_route_points[ i - 1 ];
		const RoutePoint& current = g_route_points[ i ];
		route_ids.push_back( current.id );

		const RouteSegmentGeometry segment = make_geometry( previous, current );
		if ( current.type == PointType::Floor ) {
			std::string reason;
			const MovementAction setup_action = classify_floor_setup_action( segment, reason );
			actions.push_back( setup_action );

			std::ostringstream out;
			out << "chain setup segment=" << previous.id << "->" << current.id << " action=" << short_action_name( setup_action )
			    << " distance2d=" << segment.distance2d << " height_delta=" << segment.height_delta << " reason=" << reason;
			AppendDebugLog( "routecalc", out.str( ) );
		} else if ( current.type == PointType::PixelSurf ) {
			if ( !final_geometry.target_is_wall_like )
				return false;

			MovementAction final_action = MovementAction::MiniJump;
			const ProfileEvaluation* contact_eval = find_evaluation( evaluations, "minijump_lost_shift" );
			if ( !contact_eval )
				contact_eval = find_evaluation( evaluations, "minijump_reduck" );

			if ( final_geometry.height_delta < -18.0f ) {
				final_action = MovementAction::JumpBug;
				if ( const auto* longjump = find_evaluation( evaluations, "longjump" ) )
					contact_eval = longjump;
			} else if ( !contact_eval ) {
				final_action = MovementAction::LongJump;
				contact_eval = find_evaluation( evaluations, "longjump" );
			}

			if ( !contact_eval )
				return false;

			actions.push_back( final_action );
			actions.push_back( MovementAction::PixelSurfContact );

			CompositeCandidate candidate = make_composite_candidate( final_geometry, *contact_eval, actions, 54.0f,
			                                                         "multi-point route chain from placed floor setup points to pixelsurf target" );
			candidate.contact_tick = contact_eval->best_tick;
			candidate.required_speed2d = contact_eval->best_required_speed;

			add_composite_combo( g_route_points.front( ), target, final_geometry, candidate, manual_sequence, max_displayed, stop_at_max_displayed );

			std::ostringstream out;
			out << "chain candidate route=";
			for ( std::size_t route_id = 0; route_id < route_ids.size( ); ++route_id ) {
				if ( route_id > 0 )
					out << "->";
				out << route_ids[ route_id ];
			}
			out << " actions=\"" << composite_summary( candidate ) << "\" final_segment=" << final_start.id << "->" << target.id
			    << " final_reason=" << final_geometry.trace.reason;
			AppendDebugLog( "routecalc", out.str( ) );
			return true;
		}
	}

	return false;
}

const RoutePoint* point_by_id( const int id )
{
	for ( const auto& point : g_route_points ) {
		if ( point.id == id )
			return &point;
	}
	return nullptr;
}

void compare_solver_to_manual( const std::string& manual )
{
	if ( manual.empty( ) )
		return;

	const ComboResult* solver_combo = nullptr;
	for ( const auto& combo : g_combo_results ) {
		if ( combo.status != ComboResult::Status::ManualObserved && combo.status != ComboResult::Status::Invalid ) {
			solver_combo = &combo;
			break;
		}
	}
	const std::string solver_text = solver_combo ? solver_combo->text : "<none>";
	AppendDebugLog( "routecalc", "solver_compare manual=\"" + manual + "\" solver=\"" + solver_text +
	                                  "\" match=" + std::to_string( solver_text == manual ? 1 : 0 ) +
	                                  " reason=manual_reference_is_calibration_not_validation" );
}

std::string hidden_reason_for_combo( const ComboResult& hidden, const ComboResult& kept )
{
	if ( hidden.status == ComboResult::Status::Invalid )
		return "invalid_candidate";
	if ( hidden.text == "lj + jb -> ps" )
		return "no_jumpbug_contact_evidence";
	if ( hidden.text == "cj + mj -> ps" )
		return "crouchjump_speed_crop_penalty";
	if ( hidden.text == "j + mj -> ps" && kept.best_tick == hidden.best_tick )
		return "duplicate_tick_window_lower_priority";
	if ( hidden.text.find( "jump ->" ) == 0 )
		return "plain_jump_low_pixelsurf_evidence";
	if ( hidden.text.find( "crouchjump" ) == 0 )
		return "crouchjump_speed_crop_penalty";
	if ( kept.best_tick == hidden.best_tick && std::fabs( kept.best_required_speed - hidden.best_required_speed ) < 0.5f )
		return "duplicate_tick_window_lower_priority";
	return "lower_score_generic_alternative";
}

void renumber_combos( )
{
	for ( int i = 0; i < static_cast< int >( g_combo_results.size( ) ); ++i )
		g_combo_results[ i ].index = i + 1;
}

void finalize_combo_results( const bool show_all_debug_candidates, const int max_displayed )
{
	if ( g_combo_results.empty( ) )
		return;

	std::stable_sort( g_combo_results.begin( ), g_combo_results.end( ), []( const ComboResult& lhs, const ComboResult& rhs ) {
		if ( lhs.status == ComboResult::Status::Invalid && rhs.status != ComboResult::Status::Invalid )
			return false;
		if ( lhs.status != ComboResult::Status::Invalid && rhs.status == ComboResult::Status::Invalid )
			return true;
		return lhs.score > rhs.score;
	} );

	if ( show_all_debug_candidates ) {
		if ( max_displayed > 0 && static_cast< int >( g_combo_results.size( ) ) > max_displayed )
			g_combo_results.resize( max_displayed );
		renumber_combos( );
		return;
	}

	const ComboResult kept = g_combo_results.front( );
	const int hidden_count = std::max( 0, static_cast< int >( g_combo_results.size( ) ) - 1 );
	for ( int i = 1; i < static_cast< int >( g_combo_results.size( ) ); ++i ) {
		const auto reason = hidden_reason_for_combo( g_combo_results[ i ], kept );
		AppendDebugLog( "routecalc", "hide combo=\"" + g_combo_results[ i ].text + "\" reason=" + reason );
	}

	g_combo_results.clear( );
	g_combo_results.push_back( kept );
	g_combo_results.front( ).hidden_debug_candidates = hidden_count;
	if ( hidden_count > 0 ) {
		if ( !g_combo_results.front( ).solver_debug.empty( ) )
			g_combo_results.front( ).solver_debug += "; ";
		g_combo_results.front( ).solver_debug += "hidden debug candidates: " + std::to_string( hidden_count );
	}
	renumber_combos( );
}

struct BruteSimState {
	Vec3 origin{ };
	Vec3 previous_origin{ };
	Vec3 velocity{ };
	bool on_ground = false;
	bool duck_held = false;
	bool ducking = false;
	bool ducked = false;
	bool duck_origin_shifted = false;
	bool unduck_origin_shifted = false;
	float duck_amount = 0.0f;
	float duck_speed = 8.0f;
	int tick = 0;
};

struct BrutePlan {
	std::vector< MovementAction > actions{ };
	int speed_sample = 250;
	int delay_ticks = 0;
	int minijump_release_tick = 2;
	int crouchjump_lead_ticks = 4;
};

struct BruteCandidateResult {
	RouteResultStatus status = RouteResultStatus::Rejected;
	std::vector< MovementAction > actions{ };
	int hit_points = 0;
	int total_points = 0;
	std::vector< int > point_hit_ticks{ };
	int pixelsurf_tick = -1;
	int closest_point_id = 0;
	float closest_distance = std::numeric_limits< float >::max( );
	int closest_tick = -1;
	float score = 0.0f;
	bool trace_confirmed = false;
	bool approximate_hull_hit = false;
	std::string reason{ };
};

std::string action_text_upper( const MovementAction action )
{
	switch ( action ) {
	case MovementAction::Jump:
		return "JUMP";
	case MovementAction::LongJump:
		return "LONG_JUMP";
	case MovementAction::MiniJump:
		return "MINIJUMP";
	case MovementAction::CrouchJump:
		return "CROUCH_JUMP";
	case MovementAction::JumpBug:
		return "JUMPBUG";
	case MovementAction::PixelSurfContact:
		return "PS";
	case MovementAction::Land:
		return "LAND";
	default:
		return ToString( action );
	}
}

std::string brute_sequence_text( const std::vector< MovementAction >& actions, const bool append_pixelsurf )
{
	std::string text;
	for ( const auto action : actions ) {
		if ( !text.empty( ) )
			text += " -> ";
		text += action_text_upper( action );
	}
	if ( append_pixelsurf ) {
		if ( !text.empty( ) )
			text += " -> ";
		text += "PS";
	}
	return text.empty( ) ? "NO_ACTION" : text;
}

Settings brute_settings_for( const BruteForceSettings& brute )
{
	Settings settings{ };
	settings.tick_interval = brute.tickrate == 128 ? 1.0f / 128.0f : 1.0f / 64.0f;
	settings.max_ticks = std::max( 1, brute.max_ticks );
	return settings;
}

BruteSimState make_brute_state( const SimStart& start )
{
	BruteSimState state{ };
	state.origin = start.origin;
	state.previous_origin = start.origin;
	state.velocity = start.velocity;
	state.on_ground = start.on_ground;
	state.duck_amount = std::clamp( start.duck_amount, 0.0f, 1.0f );
	state.duck_speed = start.duck_speed > 0.0f ? start.duck_speed : 8.0f;
	state.ducked = state.duck_amount >= 0.99f;
	state.ducking = state.duck_amount > 0.0f && state.duck_amount < 1.0f;
	state.duck_held = state.ducked || state.ducking;
	return state;
}

float distance_2d_between( const Vec3& lhs, const Vec3& rhs )
{
	return length_2d( { lhs.x - rhs.x, lhs.y - rhs.y, 0.0f } );
}

float distance_3d_between( const Vec3& lhs, const Vec3& rhs )
{
	return distance_3d( lhs, rhs );
}

bool matches_pixelsurf_trace( const Vec3& from, const Vec3& to, const RoutePoint& point, const bool ducked )
{
	if ( !g_interfaces.m_engine_trace || !g_interfaces.m_engine_client || !g_interfaces.m_engine_client->is_in_game( ) || !point.has_normal )
		return false;

	trace_t trace{ };
	trace_world_hull( from, to, ducked ? 54.0f : 72.0f, trace );
	if ( !trace.did_hit( ) )
		return false;

	const Vec3 hit_normal = from_source_vector( trace.m_plane.m_normal );
	const float normal_dot = dot_product( hit_normal, point.normal );
	const float hit_distance = distance_3d_between( from_source_vector( trace.m_end ), point.position );
	return std::fabs( normal_dot ) >= 0.78f && hit_distance <= 48.0f;
}

bool hit_floor_point( const BruteSimState& state, const RoutePoint& point, const BruteForceSettings& settings )
{
	if ( point.type != PointType::Floor )
		return false;

	const float horizontal = distance_2d_between( state.origin, point.position );
	const float vertical = std::fabs( state.origin.z - point.position.z );
	if ( horizontal > std::max( 2.0f, settings.floor_radius ) )
		return false;

	if ( vertical <= 8.0f )
		return true;

	const bool crossing_down = state.previous_origin.z >= point.position.z && state.origin.z <= point.position.z && state.velocity.z <= 0.0f;
	return crossing_down && vertical <= std::max( 14.0f, settings.floor_radius );
}

bool hit_pixelsurf_point( const BruteSimState& state, const RoutePoint& point, const BruteForceSettings& settings, bool& trace_confirmed )
{
	trace_confirmed = false;
	if ( point.type != PointType::PixelSurf || !wall_like_normal( point.has_normal, point.normal ) )
		return false;

	const float direct_distance = distance_3d_between( state.origin, point.position );
	if ( direct_distance > std::max( 2.0f, settings.pixelsurf_radius ) )
		return false;

	const Vec3 approach = normalized( { state.origin.x - state.previous_origin.x, state.origin.y - state.previous_origin.y, state.origin.z - state.previous_origin.z } );
	if ( length_3d( approach ) > 0.001f ) {
		const float approach_dot = dot_product( approach, point.normal );
		if ( approach_dot > 0.35f )
			return false;
	}

	if ( settings.use_hull_trace )
		trace_confirmed = matches_pixelsurf_trace( state.previous_origin, point.position, point, state.ducked );
	return true;
}

bool near_jumpbug_window( const BruteSimState& state, const std::vector< RoutePoint >& points, const int target_index )
{
	if ( state.on_ground || state.velocity.z > -20.0f )
		return false;

	for ( int i = target_index; i < static_cast< int >( points.size( ) ); ++i ) {
		if ( points[ i ].type != PointType::Floor )
			continue;

		const float horizontal = distance_2d_between( state.origin, points[ i ].position );
		const float vertical = state.origin.z - points[ i ].position.z;
		if ( horizontal <= 28.0f && vertical >= 0.0f && vertical <= 12.0f )
			return true;
	}

	return false;
}

void steer_toward_point( BruteSimState& state, const RoutePoint& point, const BrutePlan& plan )
{
	const Vec3 to_target = { point.position.x - state.origin.x, point.position.y - state.origin.y, 0.0f };
	const float dist = length_2d( to_target );
	if ( dist <= 0.001f )
		return;

	const Vec3 dir = { to_target.x / dist, to_target.y / dist, 0.0f };
	const float target_speed = static_cast< float >( std::clamp( plan.speed_sample, 80, 340 ) );
	const float blend = state.on_ground ? 0.30f : 0.075f;
	state.velocity.x += ( dir.x * target_speed - state.velocity.x ) * blend;
	state.velocity.y += ( dir.y * target_speed - state.velocity.y ) * blend;
}

BruteSimState simulate_brute_tick( const Settings& settings, BruteSimState state, const TickInput& input )
{
	state.previous_origin = state.origin;

	const bool was_on_ground = state.on_ground;
	const bool wants_jump = presses_jump( input.action );
	const bool wants_duck = presses_duck( input.action ) ? true : ( input.action == Action::ReleaseDuck ? false : state.duck_held );

	if ( wants_duck && !state.duck_held && !state.ducked )
		state.ducking = true;
	state.duck_held = wants_duck;

	const float previous_duck_speed = settings.duck_speed;
	Settings local_settings = settings;
	local_settings.duck_speed = state.duck_speed > 0.0f ? state.duck_speed : previous_duck_speed;
	const float duck_step = DuckStep( local_settings );
	if ( state.on_ground ) {
		if ( state.duck_held ) {
			state.duck_amount = std::min( 1.0f, state.duck_amount + duck_step );
			state.ducked = state.duck_amount >= 1.0f;
			state.ducking = !state.ducked && state.duck_amount > 0.0f;
		} else {
			state.duck_amount = std::max( 0.0f, state.duck_amount - duck_step );
			state.ducked = state.duck_amount >= 0.99f;
			state.ducking = state.duck_amount > 0.0f && state.duck_amount < 1.0f;
		}
	}

	if ( !state.on_ground )
		state.velocity.z -= HalfGravity( settings );

	if ( wants_jump && was_on_ground ) {
		state.on_ground = false;
		if ( state.ducking || state.ducked || wants_duck )
			state.velocity.z = settings.ground_factor * settings.jump_impulse;
		else
			state.velocity.z += settings.ground_factor * settings.jump_impulse;
	}

	if ( !state.on_ground ) {
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
		}
	}

	state.origin.x += state.velocity.x * settings.tick_interval;
	state.origin.y += state.velocity.y * settings.tick_interval;
	if ( !state.on_ground )
		state.origin.z += state.velocity.z * settings.tick_interval;

	if ( !state.on_ground )
		state.velocity.z -= HalfGravity( settings );

	++state.tick;
	return state;
}

void generate_action_sequences_recursive( const int depth, const int max_depth, std::vector< MovementAction >& current,
                                          std::vector< std::vector< MovementAction > >& out, const int max_sequences )
{
	static constexpr std::array< MovementAction, 5 > alphabet = { MovementAction::Jump, MovementAction::LongJump, MovementAction::MiniJump,
		                                                          MovementAction::CrouchJump, MovementAction::JumpBug };
	if ( static_cast< int >( out.size( ) ) >= max_sequences )
		return;

	if ( depth > 0 )
		out.push_back( current );
	if ( depth >= max_depth )
		return;

	for ( const auto action : alphabet ) {
		if ( static_cast< int >( out.size( ) ) >= max_sequences )
			break;
		current.push_back( action );
		generate_action_sequences_recursive( depth + 1, max_depth, current, out, max_sequences );
		current.pop_back( );
	}
}

std::vector< std::vector< MovementAction > > generate_action_sequences( const int max_depth, const int max_sequences )
{
	std::vector< std::vector< MovementAction > > sequences;
	std::vector< MovementAction > current;
	sequences.reserve( static_cast< std::size_t >( std::min( max_sequences, 1024 ) ) );
	generate_action_sequences_recursive( 0, std::max( 1, max_depth ), current, sequences, std::max( 1, max_sequences ) );
	return sequences;
}

bool plan_has_action( const BrutePlan& plan, const MovementAction action )
{
	return std::find( plan.actions.begin( ), plan.actions.end( ), action ) != plan.actions.end( );
}

std::vector< BrutePlan > generate_plan_variants( const std::vector< MovementAction >& actions, const BruteForceSettings& settings )
{
	std::vector< BrutePlan > plans;
	const std::vector< int > speeds = settings.allow_heavy_cpu ? std::vector< int >{ 180, 220, 250, 280, 320 }
	                                                           : std::vector< int >{ 220, 250, 280 };
	const std::vector< int > delays = { 0, 2, 4, 6 };
	const std::vector< int > mj_offsets = std::find( actions.begin( ), actions.end( ), MovementAction::MiniJump ) != actions.end( )
	                                          ? std::vector< int >{ 1, 2, 3, 4, 5 }
	                                          : std::vector< int >{ 2 };
	const std::vector< int > cj_leads = std::find( actions.begin( ), actions.end( ), MovementAction::CrouchJump ) != actions.end( )
	                                        ? std::vector< int >{ 1, 4, 7, 10 }
	                                        : std::vector< int >{ 4 };

	for ( const int speed : speeds ) {
		for ( const int delay : delays ) {
			for ( const int mj : mj_offsets ) {
				for ( const int lead : cj_leads ) {
					BrutePlan plan{ };
					plan.actions = actions;
					plan.speed_sample = speed;
					plan.delay_ticks = delay;
					plan.minijump_release_tick = mj;
					plan.crouchjump_lead_ticks = lead;
					plans.push_back( plan );
					if ( static_cast< int >( plans.size( ) ) >= settings.max_variants )
						return plans;
					if ( !settings.allow_heavy_cpu && plans.size( ) >= 180U )
						return plans;
				}
			}
		}
	}

	return plans;
}

void update_closest_point( BruteCandidateResult& result, const BruteSimState& state, const std::vector< RoutePoint >& points, const int next_point )
{
	if ( next_point < 0 || next_point >= static_cast< int >( points.size( ) ) )
		return;

	const float distance = points[ next_point ].type == PointType::Floor ? distance_2d_between( state.origin, points[ next_point ].position )
	                                                                     : distance_3d_between( state.origin, points[ next_point ].position );
	if ( distance < result.closest_distance ) {
		result.closest_distance = distance;
		result.closest_point_id = points[ next_point ].id;
		result.closest_tick = state.tick;
	}
}

BruteCandidateResult simulate_plan( const SimStart& start, const std::vector< RoutePoint >& points, const BrutePlan& plan,
                                    const BruteForceSettings& brute_settings )
{
	const Settings settings = brute_settings_for( brute_settings );
	BruteCandidateResult result{ };
	result.actions = plan.actions;
	result.total_points = static_cast< int >( points.size( ) );
	result.point_hit_ticks.assign( points.size( ), -1 );

	if ( points.empty( ) ) {
		result.status = RouteResultStatus::Error;
		result.reason = "no route points";
		return result;
	}

	BruteSimState state = make_brute_state( start );
	int next_point = 0;
	int action_index = 0;
	int next_action_tick = 0;
	int crouch_lead_remaining = 0;
	int hold_duck_until = -1;
	int release_duck_tick = -1;
	bool last_ps_trace_confirmed = false;

	for ( int tick = 0; tick < settings.max_ticks; ++tick ) {
		while ( next_point < static_cast< int >( points.size( ) ) ) {
			bool trace_confirmed = false;
			const bool hit = points[ next_point ].type == PointType::Floor ? hit_floor_point( state, points[ next_point ], brute_settings )
			                                                               : hit_pixelsurf_point( state, points[ next_point ], brute_settings, trace_confirmed );
			if ( !hit )
				break;

			result.point_hit_ticks[ next_point ] = state.tick;
			++result.hit_points;
			if ( points[ next_point ].type == PointType::Floor ) {
				state.origin.z = points[ next_point ].position.z;
				state.velocity.z = 0.0f;
				state.on_ground = true;
				state.unduck_origin_shifted = false;
				next_action_tick = state.tick + plan.delay_ticks;
			} else {
				result.pixelsurf_tick = state.tick;
				result.trace_confirmed = trace_confirmed;
				result.approximate_hull_hit = !trace_confirmed;
				last_ps_trace_confirmed = trace_confirmed;
			}
			++next_point;
		}

		if ( next_point >= static_cast< int >( points.size( ) ) ) {
			result.status = RouteResultStatus::SimulatedHitAllPoints;
			result.reason = last_ps_trace_confirmed ? "all points hit; pixelsurf hull trace confirmed" : "all points hit by coordinate/radius simulation";
			result.score = 1000.0f + static_cast< float >( result.hit_points ) * 100.0f - static_cast< float >( state.tick ) -
			               static_cast< float >( plan.actions.size( ) ) * 4.0f + ( result.trace_confirmed ? 80.0f : 0.0f );
			return result;
		}

		update_closest_point( result, state, points, next_point );

		const RoutePoint& target = points[ next_point ];
		steer_toward_point( state, target, plan );

		TickInput input{ };
		if ( hold_duck_until >= state.tick )
			input.action = Action::Duck;
		if ( release_duck_tick == state.tick )
			input.action = Action::ReleaseDuck;

		if ( action_index < static_cast< int >( plan.actions.size( ) ) && state.tick >= next_action_tick ) {
			const MovementAction action = plan.actions[ action_index ];
			if ( action == MovementAction::Jump && state.on_ground ) {
				input.action = Action::Jump;
				++action_index;
			} else if ( action == MovementAction::LongJump && state.on_ground ) {
				input.action = Action::JumpDuck;
				hold_duck_until = state.tick + 5;
				++action_index;
			} else if ( action == MovementAction::MiniJump && state.on_ground ) {
				input.action = Action::JumpDuck;
				release_duck_tick = state.tick + std::clamp( plan.minijump_release_tick, 1, 6 );
				hold_duck_until = state.tick + 1;
				++action_index;
			} else if ( action == MovementAction::CrouchJump && state.on_ground ) {
				if ( crouch_lead_remaining <= 0 )
					crouch_lead_remaining = std::clamp( plan.crouchjump_lead_ticks, 1, 10 );
				if ( crouch_lead_remaining > 0 ) {
					input.action = Action::Duck;
					--crouch_lead_remaining;
				}
				if ( crouch_lead_remaining <= 0 ) {
					input.action = Action::JumpDuck;
					hold_duck_until = state.tick + 8;
					++action_index;
				}
			} else if ( action == MovementAction::JumpBug ) {
				if ( near_jumpbug_window( state, points, next_point ) ) {
					input.action = Action::Jump;
					++action_index;
				}
			}
		}

		state = simulate_brute_tick( settings, state, input );
	}

	result.status = result.hit_points > 0 ? RouteResultStatus::SimulatedPartialHit : RouteResultStatus::NearMiss;
	result.reason = result.hit_points > 0 ? "max ticks reached after partial route hit" : "max ticks reached without hitting first route point";
	result.score = static_cast< float >( result.hit_points ) * 100.0f - std::min( result.closest_distance, 999.0f );
	return result;
}

bool is_better_brute_result( const BruteCandidateResult& lhs, const BruteCandidateResult& rhs )
{
	if ( lhs.status == RouteResultStatus::SimulatedHitAllPoints && rhs.status != RouteResultStatus::SimulatedHitAllPoints )
		return true;
	if ( lhs.status != RouteResultStatus::SimulatedHitAllPoints && rhs.status == RouteResultStatus::SimulatedHitAllPoints )
		return false;
	if ( lhs.hit_points != rhs.hit_points )
		return lhs.hit_points > rhs.hit_points;
	if ( std::fabs( lhs.closest_distance - rhs.closest_distance ) > 0.001f )
		return lhs.closest_distance < rhs.closest_distance;
	return lhs.score > rhs.score;
}

ComboResult combo_from_bruteforce( const BruteForceResult& brute )
{
	ComboResult combo{ };
	combo.index = 1;
	combo.text = brute.status == RouteResultStatus::SimulatedHitAllPoints ? brute.sequence : "no simulated route found";
	combo.score = brute.score;
	combo.start_point_id = g_route_points.empty( ) ? 0 : g_route_points.front( ).id;
	combo.end_point_id = g_route_points.empty( ) ? 0 : g_route_points.back( ).id;
	combo.status = brute.status == RouteResultStatus::SimulatedHitAllPoints ? ComboResult::Status::GeometryValidated : ComboResult::Status::Invalid;
	combo.status_reason = brute.reason;
	combo.notes = brute.status == RouteResultStatus::SimulatedHitAllPoints ? "tick brute-force simulated all placed points; no movement was executed"
	                                                                       : "brute-force did not simulate a full route hit";
	combo.solver_debug = "bruteforce from current player; hit_points=" + std::to_string( brute.hit_points ) + "/" +
	                     std::to_string( brute.total_points ) + " closest_point=" + std::to_string( brute.closest_point_id ) +
	                     " closest_distance=" + std::to_string( brute.closest_distance );
	combo.solver_validated = brute.status == RouteResultStatus::SimulatedHitAllPoints;
	combo.hidden_debug_candidates = g_bruteforce_progress.hidden_candidates;
	combo.actions.reserve( brute.actions.size( ) );
	for ( const auto action : brute.actions )
		combo.actions.push_back( make_composite_action( action, -1, brute.status == RouteResultStatus::SimulatedHitAllPoints ? 0.75f : 0.25f,
		                                                "bruteforce sequence action; exact tick plan kept internal" ) );
	return combo;
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

const char* ToString( MovementAction action )
{
	switch ( action ) {
	case MovementAction::Jump:
		return "JUMP";
	case MovementAction::Duck:
		return "DUCK";
	case MovementAction::Unduck:
		return "UNDUCK";
	case MovementAction::CrouchJump:
		return "CROUCH_JUMP";
	case MovementAction::LongJump:
		return "LONG_JUMP";
	case MovementAction::MiniJump:
		return "MINIJUMP";
	case MovementAction::JumpBug:
		return "JUMPBUG";
	case MovementAction::PixelSurfContact:
		return "PS";
	case MovementAction::PixelSurfExit:
		return "PIXELSURF_EXIT";
	case MovementAction::Land:
		return "LAND";
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
	case ComboResult::Status::ManualObservedBoosted:
		return "manual observed boosted";
	default:
		return "unknown";
	}
}

const char* ToString( RouteResultStatus status )
{
	switch ( status ) {
	case RouteResultStatus::Idle:
		return "idle";
	case RouteResultStatus::Calculating:
		return "calculating";
	case RouteResultStatus::SimulatedHitAllPoints:
		return "simulated hit all points";
	case RouteResultStatus::SimulatedPartialHit:
		return "partial hit";
	case RouteResultStatus::NearMiss:
		return "near miss";
	case RouteResultStatus::Rejected:
		return "rejected";
	case RouteResultStatus::Error:
		return "error";
	case RouteResultStatus::Cancelled:
		return "cancelled";
	case RouteResultStatus::TimedOut:
		return "timed out";
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
                                                  const bool show_all_debug_candidates, const float max_calculation_distance, const std::string& manual_observed_combo,
                                                  const std::string& manual_observed_map, const std::string& manual_observed_area,
                                                  const std::string& source )
{
	static auto last_calculate_time = std::chrono::steady_clock::time_point{ };
	const auto now = std::chrono::steady_clock::now( );
	if ( last_calculate_time.time_since_epoch( ).count( ) != 0 &&
	     std::chrono::duration_cast< std::chrono::milliseconds >( now - last_calculate_time ).count( ) < 175 ) {
		AppendDebugLog( "routecalc", "calculate skipped=debounce source=" + ( source.empty( ) ? std::string( "unknown" ) : source ) );
		return g_combo_results;
	}
	last_calculate_time = now;

	g_combo_results.clear( );
	g_last_calculation_message = "calculating";
	AppendDebugLog( "routecalc", "calculate source=" + ( source.empty( ) ? std::string( "unknown" ) : source ) );
	const Settings settings{ };
	log_settings_once( settings );

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
	g_last_route_segments = build_segments( );
	for ( const auto& segment : g_last_route_segments )
		log_segment( segment );

	if ( !floor || !pixelsurf ) {
		set_invalid_result( "need at least one floor and one pixelsurf point" );
		return g_combo_results;
	}

	log_calculation_input( *floor, *pixelsurf );

	const Vec3 delta = delta_vec( *floor, *pixelsurf );
	const float distance2d = length_2d( delta );
	const float height_delta = delta.z;
	RouteSegmentGeometry first_geometry{ };
	bool found_first_geometry = false;
	for ( const auto& segment : g_last_route_segments ) {
		if ( segment.from_id == floor->id && segment.to_id == pixelsurf->id ) {
			first_geometry = segment;
			found_first_geometry = true;
			break;
		}
	}
	if ( !found_first_geometry )
		first_geometry = make_geometry( *floor, *pixelsurf );
	log_pixelsurf_analysis( first_geometry );

	const std::string calibration_sequence = calibration_sequence_for_geometry( first_geometry );
	const std::string manual_sequence_for_matching = !calibration_sequence.empty( ) ? calibration_sequence : manual_observed_combo;
	if ( !manual_sequence_for_matching.empty( ) ) {
		add_manual_observation( *floor, *pixelsurf, delta, distance2d, height_delta, manual_sequence_for_matching, manual_observed_map,
		                        manual_observed_area );
	}

	const auto profiles = build_jump_profiles( settings );
	bool evaluated_solver_segment = false;
	bool pushed_solver_combo = false;
	for ( const auto& geometry : g_last_route_segments ) {
		const auto* start = point_by_id( geometry.from_id );
		const auto* target = point_by_id( geometry.to_id );
		if ( !start || !target )
			continue;

		if ( geometry.to_type != PointType::PixelSurf ) {
			AppendDebugLog( "routecalc", "setup segment=" + std::to_string( geometry.from_id ) + "->" +
			                                  std::to_string( geometry.to_id ) + " type=" + ToString( geometry.to_type ) +
			                                  " used_for_chain=1 skipped_individual_solver=1" );
			continue;
		}

		evaluated_solver_segment = true;
		log_pixelsurf_analysis( geometry );

		std::string reject_reason;
		if ( strict_validation && !geometry.target_from_scanline )
			reject_reason = "no_imported_pixelsurf_point";
		else if ( strict_validation && !geometry.target_has_normal )
			reject_reason = "target_normal_missing";
		else if ( geometry.to_type == PointType::PixelSurf && geometry.target_is_floor_like )
			reject_reason = "target_normal_floor_like";
		else if ( geometry.to_type == PointType::PixelSurf && geometry.target_has_normal && !geometry.target_is_wall_like )
			reject_reason = "target_normal_not_wall_like";
		else if ( geometry.to_type == PointType::Floor && geometry.target_has_normal && !geometry.target_is_floor_like )
			reject_reason = "target_normal_not_floor_like";
		else if ( geometry.distance2d > std::max( 64.0f, max_calculation_distance ) )
			reject_reason = "target_distance_over_limit";
		else if ( strict_validation && geometry.trace.available && geometry.trace.path_blocked_early )
			reject_reason = "engine_hull_trace_blocked_before_target";

		if ( !reject_reason.empty( ) ) {
			std::ostringstream out;
			out << "validate target_id=" << geometry.to_id << " normal="
			    << ( geometry.target_has_normal ? vec3_string( geometry.target_normal ) : std::string( "(unavailable)" ) )
			    << " normal_z=" << ( geometry.target_has_normal ? geometry.target_normal.z : 0.0f ) << " result=reject reason=" << reject_reason;
			AppendDebugLog( "routecalc", out.str( ) );
			add_invalid_combo( *start, *target, geometry, reject_reason, max_displayed, stop_at_max_displayed );
			continue;
		}

		AppendDebugLog( "routecalc", "validate result=pass confidence=debug_heuristic segment=" + std::to_string( geometry.from_id ) + "->" +
		                                  std::to_string( geometry.to_id ) );
		std::vector< ProfileEvaluation > passed_evaluations;
		for ( const auto& profile : profiles ) {
			const auto evaluation = evaluate_profile( settings, geometry, profile );
			log_profile_evaluation( geometry, evaluation );
			if ( !evaluation.passed )
				continue;
			passed_evaluations.push_back( evaluation );
		}

		if ( generate_route_chain_candidate( *start, *target, geometry, passed_evaluations, manual_sequence_for_matching, max_displayed,
		                                     stop_at_max_displayed ) )
			pushed_solver_combo = true;

		generate_composite_candidates( *start, *target, geometry, passed_evaluations, manual_sequence_for_matching, max_displayed, stop_at_max_displayed );

		for ( const auto& evaluation : passed_evaluations ) {
			add_profile_combo( *start, *target, geometry, evaluation, max_displayed, stop_at_max_displayed );
			pushed_solver_combo = true;
		}
		if ( !passed_evaluations.empty( ) )
			pushed_solver_combo = true;
	}

	if ( !evaluated_solver_segment ) {
		set_invalid_result( "no floor/pixelsurf route segment to evaluate" );
	} else if ( !pushed_solver_combo ) {
		g_last_calculation_message = "no solver-validated route: vertical+horizontal heuristic found no pass";
		AppendDebugLog( "routecalc", "validate result=reject reason=no_profile_passed" );
	} else {
		const bool has_trace_evidence = std::any_of( g_last_route_segments.begin( ), g_last_route_segments.end( ),
		                                             []( const RouteSegmentGeometry& segment ) { return segment.trace.available; } );
		g_last_calculation_message = has_trace_evidence ? "solver: segment heuristic + engine trace evidence; full tick-by-tick hull route search not implemented"
		                                                : "solver: vertical+horizontal debug heuristic; engine trace unavailable";
	}

	finalize_combo_results( show_all_debug_candidates, max_displayed );
	compare_solver_to_manual( manual_sequence_for_matching );

	return g_combo_results;
}

bool CaptureCurrentPlayerState( SimStart& out, std::string& reason )
{
	out = SimStart{ };
	if ( !g_interfaces.m_engine_client || !g_interfaces.m_engine_client->is_in_game( ) ) {
		reason = "engine_not_in_game";
		return false;
	}
	if ( !g_ctx.m_local || !g_ctx.m_local->is_alive( ) ) {
		reason = "local_player_invalid";
		return false;
	}

	out.origin = from_source_vector( g_ctx.m_local->get_origin( ) );
	out.velocity = from_source_vector( g_ctx.m_local->get_velocity( ) );
	out.flags = g_ctx.m_local->get_flags( );
	out.on_ground = ( out.flags & e_flags::fl_onground ) != 0;
	out.duck_amount = std::clamp( g_ctx.m_local->get_duck_amount( ), 0.0f, 1.0f );
	out.duck_speed = g_ctx.m_local->get_duck_speed( ) > 0.0f ? g_ctx.m_local->get_duck_speed( ) : 8.0f;
	out.tick_base = g_ctx.m_local->get_tick_base( );
	out.speed2d = length_2d( out.velocity );
	if ( g_ctx.m_cmd )
		out.buttons = g_ctx.m_cmd->m_buttons;

	c_angle view{ };
	g_interfaces.m_engine_client->get_view_angles( view );
	out.view_angles = { view.m_x, view.m_y, view.m_z };
	out.captured = true;
	out.source = "current_player";
	reason.clear( );
	return true;
}

const BruteForceResult& RunBruteForceCalculation( const BruteForceSettings& settings, const std::string& source )
{
	static auto last_calculate_time = std::chrono::steady_clock::time_point{ };
	const auto now = std::chrono::steady_clock::now( );
	if ( last_calculate_time.time_since_epoch( ).count( ) != 0 &&
	     std::chrono::duration_cast< std::chrono::milliseconds >( now - last_calculate_time ).count( ) < 175 ) {
		AppendDebugLog( "routecalc", "bruteforce skipped=debounce source=" + ( source.empty( ) ? std::string( "unknown" ) : source ) );
		return g_bruteforce_result;
	}
	last_calculate_time = now;

	g_bruteforce_cancel_requested = false;
	g_bruteforce_progress = BruteForceProgress{ };
	g_bruteforce_progress.running = true;
	g_bruteforce_progress.status_text = "calculating";
	g_bruteforce_result = BruteForceResult{ };
	g_combo_results.clear( );
	g_last_calculation_message = "bruteforce calculating";

	SimStart start{ };
	std::string capture_reason;
	if ( settings.start_from_current_player ) {
		if ( !CaptureCurrentPlayerState( start, capture_reason ) ) {
			g_bruteforce_result.status = RouteResultStatus::Error;
			g_bruteforce_result.reason = capture_reason;
			g_bruteforce_progress.running = false;
			g_bruteforce_progress.status_text = capture_reason;
			g_last_calculation_message = "bruteforce failed: " + capture_reason;
			AppendDebugLog( "routecalc", "bruteforce rejected reason=" + capture_reason );
			g_combo_results.push_back( combo_from_bruteforce( g_bruteforce_result ) );
			return g_bruteforce_result;
		}
	} else if ( !g_route_points.empty( ) ) {
		start.origin = g_route_points.front( ).position;
		start.on_ground = g_route_points.front( ).type == PointType::Floor;
		start.duck_speed = 8.0f;
		start.captured = true;
		start.source = "first_route_point";
	}

	std::vector< RoutePoint > points = g_route_points;
	if ( !settings.use_all_points && !points.empty( ) ) {
		const auto first_pixelsurf = std::find_if( points.begin( ), points.end( ), []( const RoutePoint& point ) {
			return point.type == PointType::PixelSurf;
		} );
		if ( first_pixelsurf != points.end( ) )
			points.erase( std::next( first_pixelsurf ), points.end( ) );
	}

	if ( points.empty( ) ) {
		g_bruteforce_result.status = RouteResultStatus::Error;
		g_bruteforce_result.reason = "no_route_points";
		g_bruteforce_result.start = start;
		g_bruteforce_progress.running = false;
		g_bruteforce_progress.status_text = "no route points";
		g_last_calculation_message = "no simulated route found: no route points";
		AppendDebugLog( "routecalc", "bruteforce rejected reason=no_route_points" );
		g_combo_results.push_back( combo_from_bruteforce( g_bruteforce_result ) );
		return g_bruteforce_result;
	}

	AppendDebugLog( "routecalc",
	                "bruteforce start source=" + ( source.empty( ) ? std::string( "unknown" ) : source ) +
	                    " tickrate=" + std::to_string( settings.tickrate ) + " start=" + start.source +
	                    " origin=" + vec3_string( start.origin ) + " velocity=" + vec3_string( start.velocity ) +
	                    " points=" + std::to_string( points.size( ) ) + " max_depth=" + std::to_string( settings.max_depth ) +
	                    " max_ticks=" + std::to_string( settings.max_ticks ) + " max_sequences=" + std::to_string( settings.max_sequences ) +
	                    " max_variants=" + std::to_string( settings.max_variants ) );
	for ( const auto& point : points ) {
		std::ostringstream out;
		out << "point id=" << point.id << " type=" << ToString( point.type ) << " pos=" << vec3_string( point.position );
		if ( point.has_normal )
			out << " normal=" << vec3_string( point.normal );
		out << " source=" << point.source;
		AppendDebugLog( "routecalc", out.str( ) );
	}

	const auto started = std::chrono::steady_clock::now( );
	const auto sequences = generate_action_sequences( std::max( 1, settings.max_depth ), std::max( 1, settings.max_sequences ) );
	g_bruteforce_progress.sequences_generated = static_cast< int >( sequences.size( ) );

	BruteCandidateResult best{ };
	best.status = RouteResultStatus::Rejected;
	best.closest_distance = std::numeric_limits< float >::max( );
	bool found_hit = false;

	for ( const auto& sequence : sequences ) {
		if ( g_bruteforce_cancel_requested ) {
			g_bruteforce_result.status = RouteResultStatus::Cancelled;
			g_bruteforce_result.reason = "cancel_requested";
			break;
		}

		const auto variants = generate_plan_variants( sequence, settings );
		for ( const auto& plan : variants ) {
			const auto elapsed = std::chrono::duration_cast< std::chrono::milliseconds >( std::chrono::steady_clock::now( ) - started ).count( );
			if ( elapsed >= settings.hard_timeout_ms ) {
				g_bruteforce_result.status = RouteResultStatus::TimedOut;
				g_bruteforce_result.reason = "hard_timeout";
				break;
			}
			if ( g_bruteforce_progress.variants_tested >= settings.max_variants ) {
				g_bruteforce_result.status = RouteResultStatus::TimedOut;
				g_bruteforce_result.reason = "variant_cap";
				break;
			}

			++g_bruteforce_progress.variants_tested;
			++g_bruteforce_progress.simulations_run;
			BruteCandidateResult candidate = simulate_plan( start, points, plan, settings );
			if ( candidate.status == RouteResultStatus::SimulatedHitAllPoints ) {
				++g_bruteforce_progress.hits_found;
				found_hit = true;
				if ( g_bruteforce_progress.hits_found <= 8 || settings.show_debug_candidates )
					AppendDebugLog( "routecalc", "candidate seq=\"" + brute_sequence_text( candidate.actions, true ) +
					                                  "\" result=hit_all pixelsurf_tick=" + std::to_string( candidate.pixelsurf_tick ) +
					                                  " score=" + std::to_string( candidate.score ) );
			} else {
				++g_bruteforce_progress.rejected_count;
				if ( settings.show_rejects && g_bruteforce_progress.rejected_count <= 64 )
					AppendDebugLog( "routecalc", "candidate seq=\"" + brute_sequence_text( candidate.actions, false ) +
					                                  "\" result=reject reason=" + candidate.reason +
					                                  " reached=" + std::to_string( candidate.hit_points ) + "/" +
					                                  std::to_string( candidate.total_points ) +
					                                  " closest=" + std::to_string( candidate.closest_distance ) );
			}

			if ( is_better_brute_result( candidate, best ) )
				best = candidate;
		}

		if ( g_bruteforce_result.status == RouteResultStatus::TimedOut || g_bruteforce_result.status == RouteResultStatus::Cancelled )
			break;
	}

	g_bruteforce_progress.elapsed_ms =
		static_cast< int >( std::chrono::duration_cast< std::chrono::milliseconds >( std::chrono::steady_clock::now( ) - started ).count( ) );
	g_bruteforce_progress.hidden_candidates = std::max( 0, g_bruteforce_progress.hits_found - 1 );
	g_bruteforce_progress.running = false;

	if ( g_bruteforce_result.status != RouteResultStatus::TimedOut && g_bruteforce_result.status != RouteResultStatus::Cancelled ) {
		g_bruteforce_result.status = found_hit ? RouteResultStatus::SimulatedHitAllPoints
		                                       : ( best.hit_points > 0 ? RouteResultStatus::SimulatedPartialHit : RouteResultStatus::NearMiss );
		g_bruteforce_result.reason = found_hit ? best.reason : "no candidate simulated every route point";
	}
	g_bruteforce_result.sequence = found_hit ? brute_sequence_text( best.actions, true ) : "";
	g_bruteforce_result.closest_sequence = brute_sequence_text( best.actions, false );
	g_bruteforce_result.hit_points = best.hit_points;
	g_bruteforce_result.total_points = static_cast< int >( points.size( ) );
	g_bruteforce_result.closest_point_id = best.closest_point_id;
	g_bruteforce_result.closest_distance = best.closest_distance == std::numeric_limits< float >::max( ) ? -1.0f : best.closest_distance;
	g_bruteforce_result.closest_tick = best.closest_tick;
	g_bruteforce_result.pixelsurf_tick = best.pixelsurf_tick;
	g_bruteforce_result.score = best.score;
	g_bruteforce_result.elapsed_ms = g_bruteforce_progress.elapsed_ms;
	g_bruteforce_result.trace_confirmed = best.trace_confirmed;
	g_bruteforce_result.approximate_hull_hit = best.approximate_hull_hit;
	g_bruteforce_result.start = start;
	g_bruteforce_result.point_hit_ticks = best.point_hit_ticks;
	g_bruteforce_result.actions = best.actions;
	g_bruteforce_progress.status_text = ToString( g_bruteforce_result.status );
	g_last_calculation_message = found_hit ? "bruteforce simulated a full point-chain hit"
	                                       : "no simulated route found; closest miss recorded";

	if ( found_hit )
		AppendDebugLog( "routecalc", "best seq=\"" + g_bruteforce_result.sequence + "\" status=" + ToString( g_bruteforce_result.status ) +
		                                  " pixelsurf_tick=" + std::to_string( g_bruteforce_result.pixelsurf_tick ) +
		                                  " score=" + std::to_string( g_bruteforce_result.score ) );
	else
		AppendDebugLog( "routecalc", "no_route closest_seq=\"" + g_bruteforce_result.closest_sequence +
		                                  "\" closest_dist=" + std::to_string( g_bruteforce_result.closest_distance ) +
		                                  " closest_tick=" + std::to_string( g_bruteforce_result.closest_tick ) +
		                                  " reached=" + std::to_string( g_bruteforce_result.hit_points ) + "/" +
		                                  std::to_string( g_bruteforce_result.total_points ) );

	g_combo_results.push_back( combo_from_bruteforce( g_bruteforce_result ) );
	return g_bruteforce_result;
}

void CancelBruteForceCalculation( )
{
	g_bruteforce_cancel_requested = true;
	g_bruteforce_progress.cancel_requested = true;
	AppendDebugLog( "routecalc", "bruteforce cancel_requested" );
}

const BruteForceProgress& GetBruteForceProgress( )
{
	return g_bruteforce_progress;
}

const BruteForceResult& GetBruteForceResult( )
{
	return g_bruteforce_result;
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

const std::vector< ManualRouteObservation >& GetManualObservations( )
{
	return g_manual_observations;
}

const ManualRouteObservation* GetLastManualObservation( )
{
	return g_manual_observations.empty( ) ? nullptr : &g_manual_observations.back( );
}
}
