#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace routecalc {

struct Vec3 {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};
};

struct Settings {
    // CS:GO-like defaults. These are intentionally exposed so you can tune
    // them per server/tickrate instead of hardcoding mystery Discord numbers.
    float tick_interval{1.0f / 64.0f};
    float gravity{800.0f};
    float jump_impulse{301.993377f};
    float duck_origin_shift{9.0f};
    float ground_z{0.0f};
    int max_ticks{96};
};

enum class Action : std::uint8_t {
    None,
    Jump,
    Duck,
    JumpDuck,
    ReleaseDuck,
};

struct TickInput {
    Action action{Action::None};
};

struct PlayerState {
    Vec3 origin{};
    Vec3 velocity{};

    bool on_ground{true};
    bool duck_held{false};
    bool ducking{false};
    bool ducked{false};

    int tick{0};
};

struct RouteEvent {
    enum class Type : std::uint8_t {
        None,
        LongJump,
        CrouchJump,
        MiniJumpRisk,
        Apex,
        Landed,
    } type{Type::None};

    int tick{0};
    PlayerState state{};
    std::string note{};
};

struct RouteCandidate {
    std::vector<TickInput> inputs{};
    std::vector<PlayerState> states{};
    std::vector<RouteEvent> events{};

    float apex_z{0.0f};
    float score{0.0f};
};

const char* ToString(Action action);
const char* ToString(RouteEvent::Type type);

PlayerState SimulateTick(const Settings& settings, PlayerState state, const TickInput& input);
RouteCandidate SimulateRoute(const Settings& settings, const std::vector<TickInput>& inputs);

// Tiny brute-force planner for local/offline experiments.
// It only explores the first two ticks and ranks sequences by apex height.
// No game-memory reads, no command injection, no online automation. Shocking restraint.
std::vector<RouteCandidate> FindSimpleJumpRoutes(const Settings& settings);

} // namespace routecalc
