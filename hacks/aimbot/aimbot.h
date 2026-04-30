#pragma once
#include "../lagcomp/lagcomp.h"
#include <array>

class c_base_entity;

namespace n_aimbot
{
	struct impl_t {
		void on_create_move_post( );
		enum hitboxex_form_t {
			hb_head,
			hb_neck,
			hb_body,
			hb_legs,
			max_hitboxes
		};
		struct aimbot_settings_t {
			bool m_enable{ false };
			bool m_silent{ false };
			bool m_visibility_check{ false };
			bool m_friendly_fire{ false };
			bool m_recoil_control{false };
			bool m_aim_to_backtrack{ false };
			bool m_hitboxes[ hitboxex_form_t::max_hitboxes ]{ };
			float m_fov{ 0.f};
			float m_smooth{ 1.f};
		};
		std::array< aimbot_settings_t, 5 > m_aimbot_settings{ };
		bool save( );
		bool load( );
		aimbot_settings_t get_aimbot_settings( );
	private:
		// find closest player within the given method
		// TODO: add these methods for scanning;
		//		- distance
		//		- best simtime(maybe?)
		//		- best hitchance

		c_base_entity* find_closest_player( );

	
	

		bool can_aimbot( );
	};
} // namespace n_aimbot

inline n_aimbot::impl_t g_aimbot{ };
