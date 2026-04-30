#pragma once

constexpr const char* base_material_names[ 2 ]    = { "flat", "textured" };
constexpr const char* overlay_material_names[ 1 ] = { "snow" };

namespace n_menu
{
	struct impl_t {
		void on_end_scene( );
		bool m_opened              = false;
		bool m_is_opening          = false; // ‘лаг направлени€ анимации (true - открытие, false - закрытие)
		float m_animation_speed    = 2.5f;  // —корость анимации (можно настроить)
		bool connect_to_server     = false;
		int m_selected_config    = 0;
		char m_config_file[ 32 ] = { };
		float m_animation_progress = 0.f; // ѕрогресс анимации (0.0 - закрыто, 1.0 - полностью открыто)
	};
} // namespace n_menu

inline n_menu::impl_t g_menu{ };