#pragma once
#define WINDOWS_IGNORE_PACKING_MISMATCH
#include <vector>
#include <Windows.h>
#include <string>
class c_user_cmd;
class c_angle;
class c_vector;

enum edgebug_type_t : int {
	eb_standing = 0,
	eb_ducking,
};

namespace n_movement
{
	struct impl_t {
		struct edgebug_data_t {
			edgebug_type_t m_edgebug_method{ };

			bool m_will_edgebug{ };
			bool m_will_fail{ };
			bool m_strafing{ };

			float m_yaw_delta{ };
			float m_starting_yaw{ };

			float m_side_move{ };
			float m_forward_move{ };

			float m_saved_mousedx{ };
			int m_ticks_to_stop{ };
			int m_last_tick{ };

			void reset( );
		} m_edgebug_data;

		struct jumpbug_data_t {
			int m_height_diff                    = 0.f;
			float m_vertical_velocity_at_landing = 0.f;
			float m_abs_height_diff              = 0.f;
			int m_ticks_till_land                = 0;
			bool m_can_jb                        = false;
		
			
          
		} m_jumpbug_data;

		struct pixelsurf_data_t {
			bool m_predicted_succesful = false, m_in_pixel_surf = false, m_should_duck = false;
			int m_prediction_ticks      = 0;
			c_user_cmd* m_simulated_cmd = { };
			bool should_pixel_surf      = false;
			void reset( );
		} m_pixelsurf_data;

		struct autoduck_data_t {
			bool m_did_land_ducking  = false;
			bool m_did_land_standing = false;

			float m_ducking_vert  = 0.f;
			float m_standing_vert = 0.f;

			void reset( );
		} m_autoduck_data;
		struct fireman_data_t {
			bool is_ladder = false;
			bool fr_hit_1  = false;
			bool fr_hit    = false;
			bool awall     = false;
		} m_fireman_data;

		struct pixelsurf_assist_t {
			bool in_crosshair = false;
			bool set_point    = false;
			

		} m_pixelsurf_assist_t;
		
		struct ladder_bug_t {
			bool founded = false;
		} m_ladder_bug_data;

		struct air_stuck_t {
			bool m_air_stuck = false;
		
		} m_air_stuck_data;
		void on_create_move_pre( );
		void on_create_move_post( );
		void on_paint_traverse( );
		void on_frame_stage_notify( int stage );
		void on_end_scene( );
		void rotate_movement( c_user_cmd* cmd, c_angle& angle );

		void pixel_finder( c_user_cmd* cmd );
	private:
		void bunny_hop( );

		void edge_jump( );

		void air_stuck( c_user_cmd* cmd );

		void edge_bug( );

		void long_jump( );

		void mini_jump( );

		void jump_bug( );

		void auto_duck( );

		void pixel_surf_locking( float target_ps_velocity );

		void pixel_surf_fix( );

		void pixel_surf( float target_ps_velocity );

		void detect_edgebug( c_user_cmd* cmd );

		void auto_align( c_user_cmd* cmd );

		void strafe_to_yaw( c_user_cmd* cmd, c_angle& angle, const float yaw );

		void movement_fix( const c_angle& old_view_point );

		void autobounce_assist( c_user_cmd* cmd );

		void pixelsurf_assist( c_user_cmd* cmd );
		
		void pixelsurf_assist_render( );

		void fire_man( c_user_cmd* cmd );
		void ladder_bug( c_user_cmd* cmd );
		void anti_ladder( c_user_cmd* cmd );
	};
} // namespace n_movement



inline n_movement::impl_t g_movement{ };
