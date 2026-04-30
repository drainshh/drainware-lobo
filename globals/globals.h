#pragma once

class c_user_cmd;
class c_base_entity;
class c_angle;
#include "../hacks/lagcomp/lagcomp.h"

namespace n_ctx
{
	struct impl_t {
		c_user_cmd* m_cmd                     = nullptr;
		c_base_entity* m_local                = nullptr;
		
		// STILL TODO.
		struct {
			int m_lag_layer_amt   = 0;
			int m_vis_layer_amt   = 0;
			int m_invis_layer_amt = 0;
		} m_chams;

		bool m_unloading{ }, m_is_window_focused{ };

		// fps lower then server tickrate
		bool m_low_fps = false;

		bool m_is_glow_being_drawn = false;

		bool m_is_console_being_drawn = false;

		float m_width{ }, m_height{ };

		float m_last_tick_yaw = 0.f;

		int m_max_allocations = 0;

		int m_last_spectators_y = 5;

		char m_windows_directory[ 64 ]{ };
		c_angle old_view_point{ };
		c_angle last_view_point{ };
		bool m_bNeedFullUpdate;
		float serverTime( c_user_cmd* cmd = nullptr ); 
		bool force_full_update = false;
	};

} // namespace n_globals

inline n_ctx::impl_t g_ctx;
