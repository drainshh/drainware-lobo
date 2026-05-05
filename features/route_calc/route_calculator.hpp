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
	int source_pixel_assist_index = -1;
	std::string source{ };
	float confidence = 0.0f;
	float distance = -1.0f;
};

struct ComboResult {
	enum class Status : std::uint8_t {
		Invalid,
		Unvalidated,
		DebugHeuristic,
		GeometryValidated,
		ManualObserved,
	};

	int index = 0;
	std::string text{ };
	float score = 0.0f;
	int start_point_id = 0;
	int end_point_id = 0;
	std::string notes{ };
	Status status = Status::Unvalidated;
	std::string status_reason{ };
};

const char* ToString( Action action );
const char* ToString( RouteEvent::Type type );
const char* ToString( PointType type );
const char* ToString( ComboResult::Status status );

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
                                                  float max_calculation_distance = 1000.0f, const std::string& manual_observed_combo = "",
                                                  const std::string& manual_observed_map = "", const std::string& manual_observed_area = "" );
const std::vector< ComboResult >& GetComboResults( );
const ComboResult* GetPrimaryCombo( );
const std::string& GetLastCalculationMessage( );
}
