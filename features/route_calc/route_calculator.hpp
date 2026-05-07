#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace route_calc
{
struct Vec3 {
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
};

enum class RouteCalcPreset : std::uint8_t {
	CSGO_64_Normal,
	CSGO_64_Heavy,
	CSGO_64_InsaneDebug,
};

enum class PixelSurfValidationMode : std::uint8_t {
	StrictTrace,
	CoordinateNearDebug,
	PixelAssistOnly,
};

struct Settings {
	float tick_interval = 1.0f / 64.0f;
	float gravity = 800.0f;
	float ent_gravity = 1.0f;
	float jump_impulse = 301.993377f;
	float ground_factor = 1.0f;
	float duck_origin_shift = 9.0f;
	float in_air_unduck_origin_shift = -9.0f;
	float duck_speed = 8.0f;
	float duck_step_scale = 0.8f;
	float ground_z = 0.0f;
	int max_ticks = 96;
};

enum class Action : std::uint8_t {
	None,
	Jump,
	Duck,
	JumpDuck,
	ReleaseDuck,
};

struct TickInput {
	Action action = Action::None;
};

struct PlayerState {
	Vec3 origin{ };
	Vec3 velocity{ };

	bool on_ground = true;
	bool duck_held = false;
	bool ducking = false;
	bool ducked = false;
	bool duck_origin_shifted = false;
	bool unduck_origin_shifted = false;
	float duck_amount = 0.0f;

	int tick = 0;
};

struct RouteEvent {
	enum class Type : std::uint8_t {
		None,
		StandingJump,
		LongJump,
		CrouchJump,
		MiniJumpRisk,
		Apex,
		Landed,
	};

	Type type = Type::None;
	int tick = 0;
	PlayerState state{ };
	std::string note{ };
};

struct RouteCandidate {
	std::string name{ };
	std::vector< TickInput > inputs{ };
	std::vector< PlayerState > states{ };
	std::vector< RouteEvent > events{ };

	float apex_z = 0.0f;
	float score = 0.0f;
};

enum class PointType : std::uint8_t {
	Floor,
	PixelSurf,
};

struct RoutePoint {
	int id = 0;
	PointType type = PointType::Floor;
	Vec3 position{ };
	Vec3 normal{ };
	bool has_normal = false;
	bool source_trace_hit = false;
	float source_trace_fraction = 1.0f;
	Vec3 source_trace_normal{ };
	int source_pixel_assist_index = -1;
	std::string source{ };
	float confidence = 0.0f;
	float distance = -1.0f;
};

enum class MovementAction : std::uint8_t {
	Jump,
	Duck,
	Unduck,
	CrouchJump,
	LongJump,
	MiniJump,
	JumpBug,
	PixelSurfContact,
	PixelSurfExit,
	Land,
};

struct ActionTiming {
	MovementAction action = MovementAction::Jump;
	int earliest_tick = -1;
	int latest_tick = -1;
	float confidence = 0.0f;
	std::string reason{ };
};

struct TraceEvidence {
	bool attempted = false;
	bool available = false;
	bool line_hit = false;
	bool line_wall_like = false;
	bool line_matches_target = false;
	bool path_blocked_early = false;
	bool path_reaches_contact = false;
	float line_fraction = 1.0f;
	float line_distance_to_target = 0.0f;
	float line_normal_dot = 0.0f;
	Vec3 line_normal{ };

	bool standing_hull_hit = false;
	bool duck_hull_hit = false;
	float standing_hull_fraction = 1.0f;
	float duck_hull_fraction = 1.0f;
	Vec3 standing_hull_normal{ };
	Vec3 duck_hull_normal{ };
	std::string reason{ };
};

struct RouteSegmentGeometry {
	int from_id = 0;
	int to_id = 0;
	PointType from_type = PointType::Floor;
	PointType to_type = PointType::Floor;
	Vec3 from{ };
	Vec3 to{ };
	Vec3 delta{ };
	float distance2d = 0.0f;
	float height_delta = 0.0f;
	Vec3 target_normal{ };
	bool target_has_normal = false;
	bool target_is_wall_like = false;
	bool target_is_floor_like = false;
	bool target_from_scanline = false;
	float approach_dot = 0.0f;
	std::string target_source{ };
	TraceEvidence trace{ };
};

struct JumpProfile {
	std::string name{ };
	MovementAction action = MovementAction::Jump;
	float initial_z_velocity = 0.0f;
	float origin_shift = 0.0f;
	float apex_height = 0.0f;
	int apex_tick = -1;
	int land_tick_estimate = -1;
};

struct TickCandidate {
	int tick = -1;
	float required_speed2d = 0.0f;
	float z_error = 0.0f;
	float score = 0.0f;
};

struct CompositeCandidate {
	std::vector< MovementAction > actions{ };
	int start_tick = 0;
	int contact_tick = -1;
	float required_speed2d = 0.0f;
	float score = 0.0f;
	std::string reason{ };
};

struct ScoreBreakdown {
	float base = 0.0f;
	float z_score = 0.0f;
	float speed_score = 0.0f;
	float approach_score = 0.0f;
	float type_score = 0.0f;
	float speed_crop_bonus = 0.0f;
	float manual_match_bonus = 0.0f;
	float penalties = 0.0f;
	float final_score = 0.0f;
};

struct ComboResult {
	enum class Status : std::uint8_t {
		Invalid,
		Unvalidated,
		DebugHeuristic,
		GeometryValidated,
		ManualObserved,
		ManualObservedBoosted,
	};

	int index = 0;
	std::string text{ };
	float score = 0.0f;
	int start_point_id = 0;
	int end_point_id = 0;
	std::string notes{ };
	Status status = Status::Unvalidated;
	std::string status_reason{ };
	RouteSegmentGeometry geometry{ };
	std::vector< ActionTiming > actions{ };
	std::vector< JumpProfile > tested_profiles{ };
	int z_window_start = -1;
	int z_window_end = -1;
	float required_speed_min = 0.0f;
	float required_speed_max = 0.0f;
	int valid_tick_start = -1;
	int valid_tick_end = -1;
	int best_tick = -1;
	float best_required_speed = 0.0f;
	std::vector< TickCandidate > valid_ticks{ };
	CompositeCandidate composite{ };
	ScoreBreakdown score_breakdown{ };
	int hidden_debug_candidates = 0;
	float approach_dot = 0.0f;
	std::string reject_reason{ };
	std::string solver_debug{ };
	bool solver_validated = false;
};

struct ManualRouteObservation {
	std::string map{ };
	std::string area{ };
	int floor_id = 0;
	int target_id = 0;
	Vec3 start{ };
	Vec3 target{ };
	Vec3 target_normal{ };
	Vec3 delta{ };
	float distance2d = 0.0f;
	float height_delta = 0.0f;
	std::string sequence{ };
	std::string result{ };
};

struct SimStart {
	Vec3 origin{ };
	Vec3 velocity{ };
	Vec3 view_angles{ };
	int flags = 0;
	int tick_base = 0;
	int buttons = 0;
	bool on_ground = false;
	float duck_amount = 0.0f;
	float duck_speed = 8.0f;
	float speed2d = 0.0f;
	bool captured = false;
	std::string source{ };
};

struct MoveSample {
	float forwardmove = 0.0f;
	float sidemove = 0.0f;
};

struct BruteForceSettings {
	bool enabled = true;
	bool start_from_current_player = true;
	bool use_all_points = true;
	bool use_hull_trace = true;
	bool show_debug_candidates = false;
	bool show_rejects = false;
	bool allow_heavy_cpu = true;
	int tickrate = 64;
	int max_depth = 4;
	int max_ticks = 256;
	int max_sequences = 5000;
	int max_variants = 60000;
	int hard_timeout_ms = 4500;
	float floor_radius = 18.0f;
	float floor_z_tolerance = 12.0f;
	float pixelsurf_radius = 8.0f;
	PixelSurfValidationMode pixelsurf_validation_mode = PixelSurfValidationMode::StrictTrace;
	std::vector< float > yaw_offsets_deg{ };
	std::vector< MoveSample > move_samples{ };
	std::vector< int > speed_samples{ };
	std::vector< int > delay_samples{ };
	std::vector< int > minijump_release_offsets{ };
	std::vector< int > crouchjump_lead_samples{ };
	bool include_current_view_yaw = true;
	int log_top_candidates = 10;
	bool log_all_candidate_summaries = false;
	std::string manual_sequence{ };
	std::string preset_name{ "CSGO_64_Normal" };
};

enum class RouteResultStatus : std::uint8_t {
	Idle,
	Calculating,
	SimulatedHitAllPoints,
	SimulatedFinalPixelsurfHit,
	TraceConfirmedContact,
	CoordinateNearTarget,
	SimulatedPartialHit,
	NearMiss,
	Rejected,
	Error,
	Cancelled,
	TimedOut,
};

struct BruteForceProgress {
	bool running = false;
	bool cancel_requested = false;
	int sequences_generated = 0;
	int variants_tested = 0;
	int simulations_run = 0;
	int hits_found = 0;
	int rejected_count = 0;
	int elapsed_ms = 0;
	int hidden_candidates = 0;
	std::string status_text{ "idle" };
};

struct BruteForceResult {
	RouteResultStatus status = RouteResultStatus::Idle;
	std::string sequence{ };
	std::string reason{ };
	std::string manual_sequence{ };
	std::string manual_closest_sequence{ };
	std::string manual_issue{ };
	int manual_closest_point_id = 0;
	std::string manual_closest_point_type{ };
	std::string manual_closest_quality{ };
	std::string manual_closest_source{ };
	float manual_closest_distance = -1.0f;
	int manual_closest_tick = -1;
	int manual_legal_variants = 0;
	int manual_illegal_variants = 0;
	std::string closest_sequence{ };
	int hit_points = 0;
	int total_points = 0;
	int closest_point_id = 0;
	std::string closest_point_type{ };
	std::string closest_quality{ };
	std::string closest_source{ };
	float closest_distance = 0.0f;
	int closest_tick = -1;
	int pixelsurf_tick = -1;
	float score = 0.0f;
	int elapsed_ms = 0;
	bool trace_confirmed = false;
	bool approximate_hull_hit = false;
	SimStart start{ };
	std::vector< int > point_hit_ticks{ };
	std::vector< MovementAction > actions{ };
};

const char* ToString( Action action );
const char* ToString( RouteEvent::Type type );
const char* ToString( PointType type );
const char* ToString( MovementAction action );
const char* ToString( ComboResult::Status status );
const char* ToString( RouteResultStatus status );

float HalfGravity( const Settings& settings );
float DuckStep( const Settings& settings );
int DuckTicks( const Settings& settings );
void AppendDebugLog( const char* feature, const std::string& message );

PlayerState SimulateTick( const Settings& settings, PlayerState state, const TickInput& input );
RouteCandidate SimulateRoute( const Settings& settings, const std::string& name, const std::vector< TickInput >& inputs );

std::vector< RouteCandidate > FindSimpleJumpRoutes( const Settings& settings );

void ClearRoutePoints( );
int AddRoutePoint( const RoutePoint& point );
const std::vector< RoutePoint >& GetRoutePoints( );
bool DeleteRoutePoint( int index );
bool DeleteSelectedOrLastRoutePoint( );
void SetRoutePointType( int index, PointType type );
void SetSelectedRoutePointIndex( int index );
int GetSelectedRoutePointIndex( );
const RoutePoint* GetSelectedRoutePoint( );
RoutePoint* GetMutableSelectedRoutePoint( );
void SetTargetRoutePointIndex( int index );
int GetTargetRoutePointIndex( );
const RoutePoint* GetTargetRoutePoint( );

void SetAimedPixelSurfAssistCandidate( const RoutePoint& point );
void ClearAimedPixelSurfAssistCandidate( );
bool HasAimedPixelSurfAssistCandidate( );
const RoutePoint* GetAimedPixelSurfAssistCandidate( );

std::vector< ComboResult > CalculateSimpleCombos( int max_displayed, bool stop_at_max_displayed, bool strict_validation = false,
                                                  bool show_all_debug_candidates = false, float max_calculation_distance = 1000.0f,
                                                  const std::string& manual_observed_combo = "",
                                                  const std::string& manual_observed_map = "", const std::string& manual_observed_area = "",
                                                  const std::string& source = "" );
const std::vector< ComboResult >& GetComboResults( );
const ComboResult* GetPrimaryCombo( );
const std::string& GetLastCalculationMessage( );
const std::vector< ManualRouteObservation >& GetManualObservations( );
const ManualRouteObservation* GetLastManualObservation( );

bool CaptureCurrentPlayerState( SimStart& out, std::string& reason );
const BruteForceResult& RunBruteForceCalculation( const BruteForceSettings& settings, const std::string& source = "" );
void CancelBruteForceCalculation( );
const BruteForceProgress& GetBruteForceProgress( );
const BruteForceResult& GetBruteForceResult( );

BruteForceSettings MakeCsgo64Preset( RouteCalcPreset preset );
}
