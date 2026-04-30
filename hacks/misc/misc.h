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

	struct particle_menu_item_t {
		int m_index = 0;
		std::string m_label;
		std::string m_name;
		bool m_available = false;
		bool m_local = false;
		std::string m_reason;
	};

	struct impl_t {
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
		void revalidate_particles( );
		void dump_particle_table( );
		const std::vector< particle_menu_item_t >& particle_menu_items( bool tracer );
		const char* particle_mode_name( int mode );
		bool particle_mode_available( int mode );
		const char* particle_mode_reason( int mode );
		int resolve_particle_fallback_mode( );

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
