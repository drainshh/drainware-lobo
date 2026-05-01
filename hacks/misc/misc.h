#pragma once
#include "../../globals/includes/includes.h"
#include <string>
#include <vector>

struct IDirect3DTexture9;
inline std::string oldtitle;
inline std::string oldartist;
inline IDirect3DTexture9* albumArtTexture = nullptr;
inline std::string strartist;
inline std::string strtitle;
namespace n_misc
{
	struct spectator_data_t {
		std::string m_text          = { };
		IDirect3DTexture9* m_avatar = { };
		c_color m_color             = { };
	};

	struct impl_t {
		struct health_snapshot_t {
			std::string particles;
			std::string sound_guard;
			std::string prediction_guard;
			std::string world_modulation;
			std::string inventory;
			std::string config;
			std::string last_error;
			std::string last_action;
			std::string last_suppressed_sound;
			int suppressed_sounds = 0;
			bool custom_sounds_muted = false;
		};

		struct session_hub_snapshot_t {
			std::string run_status = "idle";
			int run_max_speed = 0;
			std::string run_combo = "none";
			std::string last_tech = "none";
			std::string last_fail_reason = "none";
			std::string last_run_summary = "none";
			std::string coach_feedback = "no timing data";
			std::string pixelsurf_assist_status = "off";
			std::string edgebug_status = "off";
			std::string jumpbug_status = "off";
			std::string px_database_status = "disabled";
			int px_lines_current_map = 0;
			std::string current_map_profile = "none";
			int particle_table_count = 0;
			std::string particle_mode = "working";
			std::string last_particle_error = "none";
		};

		struct practice_t {
			c_angle saved_angles    = { };
			c_vector saved_position = { };
		} practice;

		void on_create_move_pre( );

		void on_end_scene( );
		void on_fire_event( );

		void on_paint_traverse( );
		void request_px_delete_nearest( );
		void request_px_clear_current_map( );
		void clear_debug_log( );
		void request_particle_debug_test( int mode );
		void request_ambient_light_restore( );
		void request_panic_restore( );
		void panic_restore( );
		health_snapshot_t get_health_snapshot( );
		std::string debug_summary( );
		bool save_stable_snapshot( );
		bool restore_stable_snapshot( );
		void restore_defaults( );
		std::vector< std::string > bind_conflicts( bool only_active );
		session_hub_snapshot_t get_session_hub_snapshot( );
		bool save_current_map_profile( );
		bool reload_current_map_profile( );
		void dump_particle_table( );

		session_hub_snapshot_t m_session_hub_snapshot{ };

	private:
		void practice_window_think( );
		void apply_game_visual_convars( );
		void disable_post_processing( );
		void remove_panorama_blur( );
		void clantag_changer( );
		void draw_visual_cosmetics( );
		void deagle_spinner( );
		void blockbot( );
		void apply_bloom_settings( );
		void apply_ambient_light_settings( );

		void draw_spectator_list( );
		void draw_spectating_local( );
	};
} // namespace n_misc

inline n_misc::impl_t g_misc{ };
