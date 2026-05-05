#include "route_calculator.hpp"

#include <algorithm>
#include <cmath>

namespace routecalc {
namespace {

bool PressesDuck(Action action) {
    return action == Action::Duck || action == Action::JumpDuck;
}

bool PressesJump(Action action) {
    return action == Action::Jump || action == Action::JumpDuck;
}

float HalfGravity(const Settings& settings) {
    return settings.gravity * 0.5f * settings.tick_interval;
}

void AddEvent(RouteCandidate& candidate, RouteEvent::Type type, const PlayerState& state, const char* note) {
    RouteEvent event{};
    event.type = type;
    event.tick = state.tick;
    event.state = state;
    event.note = note ? note : "";
    candidate.events.push_back(event);
}

} // namespace

const char* ToString(Action action) {
    switch (action) {
    case Action::None: return "none";
    case Action::Jump: return "jump";
    case Action::Duck: return "duck";
    case Action::JumpDuck: return "jump+duck";
    case Action::ReleaseDuck: return "release_duck";
    default: return "unknown";
    }
}

const char* ToString(RouteEvent::Type type) {
    switch (type) {
    case RouteEvent::Type::None: return "none";
    case RouteEvent::Type::LongJump: return "longjump";
    case RouteEvent::Type::CrouchJump: return "crouchjump";
    case RouteEvent::Type::MiniJumpRisk: return "minijump_risk";
    case RouteEvent::Type::Apex: return "apex";
    case RouteEvent::Type::Landed: return "landed";
    default: return "unknown";
    }
}

PlayerState SimulateTick(const Settings& settings, PlayerState state, const TickInput& input) {
    const bool was_on_ground = state.on_ground;
    const bool was_duck_held = state.duck_held;
    const bool wants_duck = PressesDuck(input.action) ? true : (input.action == Action::ReleaseDuck ? false : state.duck_held);
    const bool wants_jump = PressesJump(input.action);

    // Simplified Duck(): this preserves the parts that matter for route planning:
    // duck intent, in-air origin shift, and the same-tick jump+duck longjump branch.
    if (wants_duck && !state.duck_held) {
        state.ducking = true;
    }

    state.duck_held = wants_duck;

    // StartGravity(): Source applies half gravity before jump handling.
    if (!state.on_ground) {
        state.velocity.z -= HalfGravity(settings);
    } else {
        // Keeping this here models the classic jump impulse overwrite difference.
        state.velocity.z -= HalfGravity(settings);
    }

    if (wants_jump && was_on_ground) {
        state.on_ground = false;

        if (state.ducking || state.ducked || wants_duck) {
            // Duck/longjump branch overwrites the negative half-gravity value.
            state.velocity.z = settings.jump_impulse;
        } else {
            // Standing jump branch adds impulse onto the already reduced z velocity.
            state.velocity.z += settings.jump_impulse;
        }
    }

    // FinishDuck()/FinishUnDuck() in air. This is intentionally minimal.
    if (!state.on_ground) {
        if (state.duck_held && !state.ducked) {
            state.origin.z += settings.duck_origin_shift;
            state.ducked = true;
            state.ducking = false;
        } else if (!state.duck_held && state.ducked) {
            state.origin.z -= settings.duck_origin_shift;
            state.ducked = false;
            state.ducking = false;
        } else if (!state.duck_held && state.ducking && was_duck_held) {
            // Released duck right after jump before FinishDuck() meaningfully helps.
            // This is the minijump danger case from the compendium.
            state.origin.z -= settings.duck_origin_shift;
            state.ducking = false;
        }
    }

    state.origin.x += state.velocity.x * settings.tick_interval;
    state.origin.y += state.velocity.y * settings.tick_interval;
    state.origin.z += state.velocity.z * settings.tick_interval;

    if (!state.on_ground) {
        state.velocity.z -= HalfGravity(settings);
    }

    if (state.origin.z <= settings.ground_z && state.velocity.z <= 0.0f) {
        state.origin.z = settings.ground_z;
        state.velocity.z = 0.0f;
        state.on_ground = true;
    }

    ++state.tick;
    return state;
}

RouteCandidate SimulateRoute(const Settings& settings, const std::vector<TickInput>& inputs) {
    RouteCandidate candidate{};
    PlayerState state{};

    candidate.states.push_back(state);
    candidate.apex_z = state.origin.z;

    bool saw_apex = false;
    float previous_z_velocity = state.velocity.z;

    for (int i = 0; i < settings.max_ticks; ++i) {
        const TickInput input = i < static_cast<int>(inputs.size()) ? inputs[i] : TickInput{};
        const PlayerState before = state;
        state = SimulateTick(settings, state, input);

        if (i == 0 && input.action == Action::JumpDuck && before.on_ground) {
            AddEvent(candidate, RouteEvent::Type::LongJump, state, "+jump and +duck on the same ground tick");
        } else if (i == 0 && input.action == Action::Duck) {
            // The route can become a crouchjump if jump follows while ducking.
        } else if (i > 0 && PressesJump(input.action) && before.on_ground && before.ducking) {
            AddEvent(candidate, RouteEvent::Type::CrouchJump, state, "jump while duck transition is active");
        }

        if (before.duck_held && !state.duck_held && !before.on_ground) {
            AddEvent(candidate, RouteEvent::Type::MiniJumpRisk, state, "duck released in air; height may be lost");
        }

        if (!saw_apex && previous_z_velocity > 0.0f && state.velocity.z <= 0.0f) {
            AddEvent(candidate, RouteEvent::Type::Apex, state, "vertical velocity crossed zero");
            saw_apex = true;
        }

        if (!before.on_ground && state.on_ground) {
            AddEvent(candidate, RouteEvent::Type::Landed, state, "returned to ground plane");
            candidate.states.push_back(state);
            break;
        }

        candidate.apex_z = std::max(candidate.apex_z, state.origin.z);
        previous_z_velocity = state.velocity.z;
        candidate.states.push_back(state);
    }

    candidate.score = candidate.apex_z;
    return candidate;
}

std::vector<RouteCandidate> FindSimpleJumpRoutes(const Settings& settings) {
    const std::vector<std::vector<TickInput>> seeds = {
        { {Action::Jump} },
        { {Action::JumpDuck}, {Action::Duck} },
        { {Action::Duck}, {Action::Jump}, {Action::Duck} },
        { {Action::JumpDuck}, {Action::ReleaseDuck} },
        { {Action::Duck}, {Action::Jump}, {Action::ReleaseDuck} },
    };

    std::vector<RouteCandidate> routes{};
    routes.reserve(seeds.size());

    for (const auto& seed : seeds) {
        routes.push_back(SimulateRoute(settings, seed));
    }

    std::sort(routes.begin(), routes.end(), [](const RouteCandidate& lhs, const RouteCandidate& rhs) {
        return lhs.score > rhs.score;
    });

    return routes;
}

} // namespace routecalc
