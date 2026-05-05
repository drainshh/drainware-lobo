#pragma once
#include "../../game/sdk/classes/c_color.h"
#include "config.h"
#include "structs/font_setting_t.h"

// TODO: add this
enum e_player_flags {
	player_flag_money = 0,
	player_flag_armor,
	player_flag_reloading,
	player_flag_bomb,
	player_flags_max
};

enum e_free_type_font_flags : int {
	font_flag_nohinting = 0,
	font_flag_noautohint,
	font_flag_forceautohint,
	font_flag_lighthinting,
	font_flag_monohinting,
	font_flag_bold,
	font_flag_oblique,
	font_flag_monochrome,
	font_flag_max
};

enum e_log_types {
	log_type_hit_enemy,
	log_type_hit_teammate,
	log_type_purchase,
	log_type_votes,
	log_type_max,
};

enum e_keybind_indicators {
	key_eb = 0,
	key_ps,
	key_ej,
	key_lj,
	key_mj,
	key_jb,
	key_ast,
	key_bast,
	key_fr,
	key_lb,
	key_max
};

namespace n_variables
{
	struct impl_t {
		/* menu */
		ADD_VARIABLE( c_color, m_accent, c_color( 174, 255, 0, 255 ) );
		ADD_VARIABLE( bool, m_debug_logging, false );
		ADD_VARIABLE( bool, m_debug_log_particles, true );
		ADD_VARIABLE( bool, m_debug_log_chat, true );
		ADD_VARIABLE( bool, m_debug_log_run, true );
		ADD_VARIABLE( bool, m_debug_log_sound, true );
		ADD_VARIABLE( int, m_particle_debug_feature, 0 );
		ADD_VARIABLE( int, m_particle_debug_particle, 13 );
		ADD_VARIABLE( std::string, m_particle_debug_name, "" );
		ADD_VARIABLE( key_bind_t, m_panic_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( bool, m_bind_conflicts_only_active, true );
		ADD_VARIABLE( float, m_menu_scale, 1.f );
		ADD_VARIABLE( float, m_menu_section_spacing, 8.f );
		ADD_VARIABLE( float, m_ui_animation_speed, 2.5f );
		ADD_VARIABLE( bool, m_session_inspector_mode, false );
		ADD_VARIABLE( bool, m_session_movement_coach, true );
		ADD_VARIABLE( int, m_session_movement_coach_output, 0 );
		ADD_VARIABLE( int, m_session_pending_preset, -1 );
		ADD_VARIABLE( int, m_theme_preset, 0 );
		ADD_VARIABLE( float, m_theme_background_alpha, 1.f );
		ADD_VARIABLE( float, m_theme_border_alpha, 1.f );
		ADD_VARIABLE( float, m_theme_corner_radius, 6.f );
		ADD_VARIABLE( float, m_theme_column_spacing, 10.f );
		ADD_VARIABLE( float, m_theme_font_scale, 1.f );
		ADD_VARIABLE( float, m_theme_glow_strength, 0.35f );
		ADD_VARIABLE( float, m_theme_scrollbar_size, 8.f );
		ADD_VARIABLE( int, m_theme_layout_mode, 0 );
		ADD_VARIABLE( int, m_theme_columns, 0 );
		ADD_VARIABLE( bool, m_theme_accent_gradient, false );
		ADD_VARIABLE( c_color, m_theme_gradient_start, c_color( 174, 255, 0, 255 ) );
		ADD_VARIABLE( c_color, m_theme_gradient_end, c_color( 70, 180, 255, 255 ) );
		ADD_VARIABLE( int, m_theme_curve_preset, 1 );
		ADD_VARIABLE( std::vector< float >, m_theme_animation_curve, std::vector< float >( { 0.f, 0.f, 0.28f, 0.88f, 1.f, 1.f } ) );
		ADD_VARIABLE( std::string, m_particle_favorites, "" );

		ADD_VARIABLE( bool, m_aimbot_enable, false );
		ADD_VARIABLE( bool, m_backtrack_enable, false );
		ADD_VARIABLE( bool, m_backtrack_extend, false );
		ADD_VARIABLE( int, m_backtrack_time_limit, 200 );
		ADD_VARIABLE( int, fakeLatencyAmount, 200 );
		/* visuals - players */
		ADD_VARIABLE( bool, m_players, false );
		ADD_VARIABLE( int, m_players_entity_type, 0 );
		ADD_VARIABLE( bool, m_players_only_audible, false );
		ADD_VARIABLE( bool, m_players_box, false );
		ADD_VARIABLE( bool, m_players_box_corner, false );
		ADD_VARIABLE( bool, m_players_box_outline, false );
		ADD_VARIABLE( c_color, m_players_box_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_players_box_outline_color, c_color( 0, 0, 0, 255 ) );
		ADD_VARIABLE( c_color, m_players_box_color_in, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_players_box_outline_color_in, c_color( 0, 0, 0, 255 ) );
		ADD_VARIABLE( bool, m_players_health_bar, false );
		ADD_VARIABLE( bool, m_players_health_bar_custom_color, false );
		ADD_VARIABLE( bool, m_players_health_text, false );
		ADD_VARIABLE( bool, m_players_health_suffix, false );
		ADD_VARIABLE( int, m_players_health_text_style, 0 );
		ADD_VARIABLE( c_color, m_players_health_bar_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_players_health_bar_color_in, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( bool, m_players_name, false );
		ADD_VARIABLE( c_color, m_players_name_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_players_name_color_in, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( bool, m_weapon_name, false );
		ADD_VARIABLE( c_color, m_weapon_name_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_weapon_name_color_in, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( bool, m_weapon_icon, false );
		ADD_VARIABLE( c_color, m_weapon_icon_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_weapon_icon_color_in, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( bool, m_player_ammo_bar, false );
		ADD_VARIABLE( bool, m_player_ammo_text, false );
		ADD_VARIABLE( c_color, m_player_ammo_bar_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_player_ammo_bar_color_in, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( bool, m_players_skeleton, false );
		ADD_VARIABLE( c_color, m_players_skeleton_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_players_skeleton_color_in, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( int, m_players_skeleton_type, 0 );
		ADD_VARIABLE( bool, m_glow_enable, false );
		ADD_VARIABLE( bool, m_visible_glow, false );
		ADD_VARIABLE( bool, m_invisible_glow, false );
		ADD_VARIABLE( bool, m_visible_stencil_glow, false );
		ADD_VARIABLE( bool, m_invisible_stencil_glow, false );
		ADD_VARIABLE( c_color, m_glow_vis_color, c_color( 224, 175, 86, 153 ) );
		ADD_VARIABLE( c_color, m_glow_invis_color, c_color( 114, 155, 221, 153 ) );
		ADD_VARIABLE( bool, m_players_backtrack_trail, false );
		ADD_VARIABLE( bool, m_players_flags, false );
		ADD_VARIABLE( bool, m_players_distance, false );
		ADD_VARIABLE( bool, m_players_emitted_sounds, false );
		ADD_VARIABLE( bool, m_players_damage_indicators, false );
		ADD_VARIABLE( bool, m_players_backtrack_eye_trail, false );
		ADD_VARIABLE( bool, m_players_snaplines, false );
		ADD_VARIABLE( bool, m_player_preview, true );
		ADD_VARIABLE( bool, m_players_avatar, false );
		ADD_VARIABLE( bool, m_players_radar, false );
		ADD_VARIABLE( bool, m_out_of_fov_arrows, false );
		ADD_VARIABLE( c_color, m_out_of_fov_arrows_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( float, m_out_of_fov_arrows_size, 20.f );
		ADD_VARIABLE( int, m_out_of_fov_arrows_distance, 200 );

		/* visuals - chams*/

		// TODO ADD OVERLAYS USING g_ctx.m_chams.m_vis_layer_amt

		// vis
		ADD_VARIABLE( bool, m_player_visible_chams, false );
		ADD_VARIABLE( bool, m_player_visible_attachment_chams, false );
		ADD_VARIABLE( int, m_player_visible_chams_material, 0 );
		ADD_VARIABLE( c_color, m_player_visible_chams_color, c_color( 114, 0, 221, 153 ) );

		// invis
		ADD_VARIABLE( bool, m_player_invisible_chams, false );
		ADD_VARIABLE( bool, m_player_invisible_attachment_chams, false );
		ADD_VARIABLE( int, m_player_invisible_chams_material, 0 );
		ADD_VARIABLE( c_color, m_player_invisible_chams_color, c_color( 0, 200, 221, 153 ) );

		// lag
		ADD_VARIABLE( bool, m_player_lag_chams, false );
		ADD_VARIABLE( bool, m_player_lag_chams_xqz, false );
		ADD_VARIABLE( int, m_player_lag_chams_type, 0 );
		ADD_VARIABLE( int, m_player_lag_chams_material, 0 );
		ADD_VARIABLE( c_color, m_player_lag_chams_color, c_color( 114, 155, 221, 153 ) );

		/* visuals - edicts */
		ADD_VARIABLE( bool, m_dropped_weapons, false );
		ADD_VARIABLE( bool, m_dropped_weapons_box, false );
		ADD_VARIABLE( bool, m_dropped_weapons_box_corner, false );
		ADD_VARIABLE( bool, m_dropped_weapons_box_outline, false );
		ADD_VARIABLE( c_color, m_dropped_weapons_box_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_dropped_weapons_box_outline_color, c_color( 0, 0, 0, 255 ) );
		ADD_VARIABLE( c_color, m_dropped_weapons_box_color_in, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_dropped_weapons_box_outline_color_in, c_color( 0, 0, 0, 255 ) );
		ADD_VARIABLE( bool, m_dropped_weapons_name, false );
		ADD_VARIABLE( c_color, m_dropped_weapons_name_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_dropped_weapons_name_color_in, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( bool, m_dropped_weapons_icon, false );
		ADD_VARIABLE( c_color, m_dropped_weapons_icon_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_dropped_weapons_icon_color_in, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( bool, m_hit_sound, false );
		ADD_VARIABLE( bool, m_thrown_objects, false );
		ADD_VARIABLE( bool, m_thrown_objects_name, false );
		ADD_VARIABLE( c_color, m_thrown_objects_name_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_thrown_objects_name_color_in, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( bool, m_thrown_objects_icon, false );
		ADD_VARIABLE( c_color, m_thrown_objects_icon_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_thrown_objects_icon_color_in, c_color( 255, 255, 255, 255 ) );
		/* visuals - world */
		ADD_VARIABLE( bool, m_sun_set, false );
		ADD_VARIABLE( float, m_sun_set_rot_x, 0.f );
		ADD_VARIABLE( float, m_sun_set_rot_y, 0.f );
		ADD_VARIABLE( float, m_sun_set_rot_z, 0.f );
		ADD_VARIABLE( bool, m_precipitation, false );
		ADD_VARIABLE( int, m_precipitation_type, 0 );
		ADD_VARIABLE( c_color, m_precipitation_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( bool, m_world_modulation, false );
		ADD_VARIABLE( bool, m_world_modulate_world, true );
		ADD_VARIABLE( bool, m_world_modulate_props, true );
		ADD_VARIABLE( bool, m_world_modulate_sky, false );
		ADD_VARIABLE( c_color, m_world_modulation_world_color, c_color( 174, 255, 0, 255 ) );
		ADD_VARIABLE( c_color, m_world_modulation_prop_color, c_color( 124, 160, 95, 255 ) );
		ADD_VARIABLE( c_color, m_world_modulation_sky_color, c_color( 96, 120, 150, 255 ) );
		ADD_VARIABLE( float, m_world_modulation_intensity, 0.75f );
		ADD_VARIABLE( float, m_world_modulation_brightness, 1.0f );
		ADD_VARIABLE( float, m_world_modulation_exposure, 1.0f );
		ADD_VARIABLE( bool, m_shadows, false );
		ADD_VARIABLE( int, m_world_material_changer, 0 );
		ADD_VARIABLE( bool, m_ragdolls, true );
		ADD_VARIABLE( bool, m_fullbright, false );
		ADD_VARIABLE( bool, m_world_glow_lights, false );
		ADD_VARIABLE( bool, m_fog, false );
		ADD_VARIABLE( float, m_fog_start, 0.f );
		ADD_VARIABLE( float, m_fog_end, 0.f );
		ADD_VARIABLE( c_color, m_fog_color, c_color( 255, 255, 255, 255 ) );

		ADD_VARIABLE( bool, m_custom_smoke, false );
		ADD_VARIABLE( c_color, m_custom_smoke_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( bool, m_custom_molotov, false );
		ADD_VARIABLE( c_color, m_custom_molotov_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( bool, m_custom_blood, false );
		ADD_VARIABLE( c_color, m_custom_blood_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( bool, m_custom_precipitation, false );
		ADD_VARIABLE( c_color, m_custom_precipitation_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( bool, m_disable_post_processing, false );
		ADD_VARIABLE( bool, m_remove_panorama_blur, false );

		/* visuals - screen */
		ADD_VARIABLE( bool, m_motion_blur, false );
		ADD_VARIABLE(bool,  m_motion_forward_move_blur  , false);
		ADD_VARIABLE(float, m_motion_falling_minimum   , 10.0f);
		ADD_VARIABLE(float, m_motion_falling_maximum   , 20.0f);
		ADD_VARIABLE(float, m_motion_falling_intensity , 1.0f);
		ADD_VARIABLE(float, m_motion_rotate_intensity  , 1.0f);
		ADD_VARIABLE(float, m_motion_strength          , 1.0f);
		ADD_VARIABLE( bool, m_pts_meter, false );
		ADD_VARIABLE( c_color, m_pts_meter_color, c_color( 174, 255, 0, 255 ) );
		ADD_VARIABLE( int, m_pts_meter_x, 22 );
		ADD_VARIABLE( int, m_pts_meter_y, 120 );
		ADD_VARIABLE( bool, m_velocity_trail, false );
		ADD_VARIABLE( bool, m_speed_pulse_overlay, false );
		ADD_VARIABLE( float, m_speed_pulse_threshold, 275.f );
		ADD_VARIABLE( bool, m_strafe_aura, false );
		ADD_VARIABLE( bool, m_velocity_graph, false );
		ADD_VARIABLE( int, m_velocity_graph_x, 22 );
		ADD_VARIABLE( int, m_velocity_graph_y, 520 );
		ADD_VARIABLE( int, m_velocity_graph_w, 240 );
		ADD_VARIABLE( int, m_velocity_graph_h, 88 );
		ADD_VARIABLE( int, m_velocity_graph_samples, 120 );
		ADD_VARIABLE( float, m_velocity_graph_smoothing, 0.35f );
		ADD_VARIABLE( bool, m_velocity_graph_compact, false );
		ADD_VARIABLE( bool, m_velocity_graph_show_velocity, true );
		ADD_VARIABLE( bool, m_velocity_graph_show_accel, true );
		ADD_VARIABLE( bool, m_velocity_graph_show_vertical, false );
		ADD_VARIABLE( bool, m_velocity_graph_show_dir_labels, true );
		ADD_VARIABLE( bool, m_velocity_graph_show_tech_markers, true );
		ADD_VARIABLE( bool, m_velocity_graph_show_gain_loss, true );
		ADD_VARIABLE( bool, m_velocity_graph_use_accent, true );
		ADD_VARIABLE( c_color, m_velocity_graph_color, c_color( 174, 255, 0, 220 ) );
		ADD_VARIABLE( bool, m_velocity_graph_reset, false );
		ADD_VARIABLE( bool, m_keystrokes_overlay, false );
		ADD_VARIABLE( int, m_keystrokes_x, 34 );
		ADD_VARIABLE( int, m_keystrokes_y, 330 );
		ADD_VARIABLE( float, m_keystrokes_scale, 1.0f );
		ADD_VARIABLE( float, m_keystrokes_spacing, 6.f );
		ADD_VARIABLE( float, m_keystrokes_bg_alpha, 0.52f );
		ADD_VARIABLE( bool, m_keystrokes_use_accent, true );
		ADD_VARIABLE( c_color, m_keystrokes_color, c_color( 174, 255, 0, 230 ) );
		ADD_VARIABLE( float, m_keystrokes_anim_speed, 8.0f );
		ADD_VARIABLE( bool, m_keystrokes_show_mouse, true );
		ADD_VARIABLE( bool, m_keystrokes_show_movement_binds, true );
		ADD_VARIABLE( bool, m_keystrokes_show_assist_binds, false );
		ADD_VARIABLE( bool, m_keystrokes_draggable, true );
		ADD_VARIABLE( int, m_keystrokes_style, 0 );
		ADD_VARIABLE( bool, m_jump_stats_enable, false );
		ADD_VARIABLE( bool, m_jump_stats_panel, true );
		ADD_VARIABLE( bool, m_jump_stats_chat, false );
		ADD_VARIABLE( bool, m_jump_stats_debug_log, false );
		ADD_VARIABLE( bool, m_jump_stats_play_sound, false );
		ADD_VARIABLE( std::string, m_jump_stats_sound_path, "C:\\drainware\\sounds\\jump_good.wav" );
		ADD_VARIABLE( float, m_jump_stats_sound_volume, 1.0f );
		ADD_VARIABLE( float, m_jump_stats_sound_threshold, 245.f );
		ADD_VARIABLE( float, m_jump_stats_sound_cooldown, 0.65f );
		ADD_VARIABLE( int, m_jump_stats_x, 22 );
		ADD_VARIABLE( int, m_jump_stats_y, 310 );
		ADD_VARIABLE( int, m_jump_stats_label_style, 0 );
		ADD_VARIABLE( int, m_jump_stats_typography, 1 );
		ADD_VARIABLE( bool, m_px_sound_enable, false );
		ADD_VARIABLE( int, m_px_sound_type, 0 );
		ADD_VARIABLE( std::string, m_px_sound_custom_path, "C:\\drainware\\sounds\\ownage.wav" );
		ADD_VARIABLE( float, m_px_sound_volume, 0.85f );
		ADD_VARIABLE( float, m_px_sound_pitch_min, 0.9f );
		ADD_VARIABLE( float, m_px_sound_pitch_max, 1.35f );
		ADD_VARIABLE( float, m_px_sound_vel_min, 180.f );
		ADD_VARIABLE( float, m_px_sound_vel_max, 480.f );
		ADD_VARIABLE( float, m_px_sound_retrigger, 0.12f );
		ADD_VARIABLE( float, m_px_sound_fade_out, 0.2f );
		ADD_VARIABLE( bool, m_px_sound_debug, false );
		ADD_VARIABLE( bool, m_local_afterimage, false );
		ADD_VARIABLE( bool, m_jump_strafe_feedback, false );
		ADD_VARIABLE( bool, m_jump_feedback_sound, false );
		ADD_VARIABLE( bool, m_aura_debt_counter, false );
		ADD_VARIABLE( int, m_aura_debt_x, 22 );
		ADD_VARIABLE( int, m_aura_debt_y, 190 );
		ADD_VARIABLE( bool, m_drain_check_widget, false );
		ADD_VARIABLE( int, m_drain_check_x, 22 );
		ADD_VARIABLE( int, m_drain_check_y, 260 );
		ADD_VARIABLE( bool, m_schizo_vision, false );
		ADD_VARIABLE( float, m_schizo_vision_intensity, 0.25f );
		ADD_VARIABLE( bool, m_npc_roast, false );
		ADD_VARIABLE( bool, m_based_radar, false );
		ADD_VARIABLE( bool, m_cringe_detector, false );
		ADD_VARIABLE( bool, m_speedometer, false );
		ADD_VARIABLE( int, m_speedometer_x, 0 );
		ADD_VARIABLE( int, m_speedometer_y, 0 );
		ADD_VARIABLE( int, m_speedometer_style, 0 );
		ADD_VARIABLE( int, m_speedometer_units, 0 );
		ADD_VARIABLE( bool, m_speedometer_peak, true );
		ADD_VARIABLE( bool, m_speedometer_state, true );
		ADD_VARIABLE( bool, m_speedometer_jump_quality, true );
		ADD_VARIABLE( bool, m_fumo_spin, false );
		ADD_VARIABLE( int, m_fumo_spin_x, 80 );
		ADD_VARIABLE( int, m_fumo_spin_y, 300 );
		ADD_VARIABLE( float, m_fumo_spin_size, 74.f );
		ADD_VARIABLE( float, m_fumo_spin_speed, 1.35f );
		ADD_VARIABLE( float, m_fumo_spin_intensity, 0.65f );
		ADD_VARIABLE( int, m_fumo_spin_direction, 0 );
		ADD_VARIABLE( bool, m_jump_trail, false );
		ADD_VARIABLE( bool, m_bullet_tracer, false );
		ADD_VARIABLE( int, m_bullet_tracer_type, 0 );
		ADD_VARIABLE( bool, m_bullet_tracer_use_accent, true );
		ADD_VARIABLE( c_color, m_bullet_tracer_color, c_color( 174, 255, 0, 220 ) );
		ADD_VARIABLE( float, m_bullet_tracer_intensity, 1.0f );
		ADD_VARIABLE( float, m_bullet_tracer_lifetime, 0.28f );
		ADD_VARIABLE( bool, m_predicted_grenade_path, false );
		ADD_VARIABLE( c_color, m_predicted_grenade_path_color, c_color( 174, 255, 0, 220 ) );
		ADD_VARIABLE( bool, m_footstep_fx_enabled, false );
		ADD_VARIABLE( int, m_footstep_fx_mode, 0 );
		ADD_VARIABLE( float, m_footstep_fx_intensity, 1.0f );
		ADD_VARIABLE( float, m_footstep_fx_cooldown, 0.18f );
		ADD_VARIABLE( bool, m_edgebug_particles, false );
		ADD_VARIABLE( int, m_edgebug_particle_type, 1 );
		ADD_VARIABLE( float, m_edgebug_particle_intensity, 1.0f );
		ADD_VARIABLE( float, m_edgebug_particle_cooldown, 0.45f );
		ADD_VARIABLE( bool, m_force_crosshair, false );
		ADD_VARIABLE( bool, m_recoil_crosshair, false );
		ADD_VARIABLE( bool, m_penetration_reticle, false );
		ADD_VARIABLE( bool, m_spread_circle, false );
		ADD_VARIABLE( c_color, m_spread_circle_color, c_color( 174, 255, 0, 160 ) );
		ADD_VARIABLE( bool, m_reduce_flash, false );
		ADD_VARIABLE( float, m_flash_alpha, 0.f );
		ADD_VARIABLE( bool, m_remove_fire_smoke, false );
		ADD_VARIABLE( bool, m_thirdperson, false );
		ADD_VARIABLE( int, m_thirdperson_distance, 120 );
		ADD_VARIABLE( bool, m_viewmodel_offsets, false );
		ADD_VARIABLE( float, m_viewmodel_offset_x, 0.f );
		ADD_VARIABLE( float, m_viewmodel_offset_y, 0.f );
		ADD_VARIABLE( float, m_viewmodel_offset_z, 0.f );
		ADD_VARIABLE( bool, m_viewmodel_fov, false );
		ADD_VARIABLE( int, m_viewmodel_fov_value, 68 );
		ADD_VARIABLE( bool, m_weapon_sway_scale, false );
		ADD_VARIABLE( float, m_weapon_sway_scale_value, 0.f );
		ADD_VARIABLE( bool, m_bloom, false );
		ADD_VARIABLE( float, m_bloom_intensity, 1.0f );
		ADD_VARIABLE( float, m_bloom_exposure, 1.0f );
		ADD_VARIABLE( bool, m_engine_ambient_light, false );
		ADD_VARIABLE( c_color, m_engine_ambient_light_color, c_color( 174, 255, 0, 255 ) );
		ADD_VARIABLE( float, m_engine_ambient_light_intensity, 1.0f );
		ADD_VARIABLE( bool, m_dof, false );
		ADD_VARIABLE( float, m_dof_focus_distance, 120.f );
		ADD_VARIABLE( float, m_dof_blur_amount, 2.0f );
		ADD_VARIABLE( float, m_dof_near_depth, 32.f );
		ADD_VARIABLE( float, m_dof_far_depth, 260.f );
		ADD_VARIABLE( bool, m_dof_smooth, true );


		/* misc - movement */
		ADD_VARIABLE( bool, m_bunny_hop, false );
		ADD_VARIABLE( bool, m_no_crouch_cooldown, false );
		ADD_VARIABLE( bool, m_edge_jump, false );
		ADD_VARIABLE( key_bind_t, m_edge_jump_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( bool, m_edge_jump_ladder, false );
		ADD_VARIABLE( bool, m_long_jump, false );
		ADD_VARIABLE( key_bind_t, m_long_jump_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( bool, m_mini_jump, false );
		ADD_VARIABLE( key_bind_t, m_mini_jump_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( bool, m_mini_jump_hold_duck, false );
		ADD_VARIABLE( bool, m_jump_bug, false );
		ADD_VARIABLE( key_bind_t, m_jump_bug_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( bool, m_pixel_surf, false );
		ADD_VARIABLE( key_bind_t, m_pixel_surf_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( bool, m_pixel_surf_fix, false );
		ADD_VARIABLE( bool, m_auto_align, false );
		ADD_VARIABLE( bool, m_auto_duck, false );

	
		ADD_VARIABLE( key_bind_t, edge_bug_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( int, EdgeBugTicks, 1 );
		ADD_VARIABLE( bool, edge_bug, false );
		ADD_VARIABLE(bool, EdgeBugAdvanceSearch,false);
		ADD_VARIABLE( bool, EdgeBugDetectChat, false );
		ADD_VARIABLE(float, EdgeBugMouseLock,0.f);
		ADD_VARIABLE( float, deltaScaler, 0.f );
		ADD_VARIABLE(int, DeltaType,0);
		ADD_VARIABLE( bool, MegaEdgeBug, false );
		ADD_VARIABLE(bool, AutoStrafeEdgeBug, false );
		ADD_VARIABLE(bool, SiletEdgeBug     , false );

		ADD_VARIABLE( bool, m_velocity_indicator, false );
		ADD_VARIABLE( bool, m_velocity_indicator_show_pre_speed, false );
		ADD_VARIABLE( bool, m_velocity_indicator_fade_alpha, false );
		ADD_VARIABLE( bool, m_velocity_indicator_custom_color, false );
		ADD_VARIABLE( bool, m_velocity_indicator_rgb, false );
		ADD_VARIABLE( float, m_velocity_indicator_rgb_speed, 0.25f );
		ADD_VARIABLE( int, m_velocity_indicator_padding, 125 );

		ADD_VARIABLE( bool, m_fire_man, false );
		ADD_VARIABLE( key_bind_t, m_fire_man_key, key_bind_t( 0, 1 ) );

		ADD_VARIABLE( key_bind_t, m_pixel_finder_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( bool, m_hit_marker, false );
		ADD_VARIABLE( int, m_sky_box, 0 );
		ADD_VARIABLE( bool, m_pixel_surf_assist, false );
		ADD_VARIABLE( bool, m_pixel_surf_assist_brokehop, false );
		ADD_VARIABLE( bool, m_pixel_surf_assist_render, false );
		ADD_VARIABLE( float, m_pixel_surf_assist_radius, 300.f );
		ADD_VARIABLE( key_bind_t, m_pixel_surf_assist_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( key_bind_t, m_pixel_surf_assist_point_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( key_bind_t, m_pixel_jump_point_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( bool, m_route_calculator, false );
		ADD_VARIABLE( bool, m_routecalc_scanline, false );
		ADD_VARIABLE( bool, m_routecalc_show_assist_candidates, true );
		ADD_VARIABLE( bool, m_routecalc_show_points, true );
		ADD_VARIABLE( bool, m_routecalc_stop_at_max_displayed, true );
		ADD_VARIABLE( bool, m_routecalc_helper_boxes, true );
		ADD_VARIABLE( bool, m_routecalc_manual_pixelsurf_debug, false );
		ADD_VARIABLE( bool, m_routecalc_strict_validation, true );
		ADD_VARIABLE( int, m_routecalc_max_displayed_combos, 4 );
		ADD_VARIABLE( float, m_routecalc_max_render_distance, 1000.f );
		ADD_VARIABLE( std::string, m_routecalc_manual_combo, "mj + j -> pixelsurf" );
		ADD_VARIABLE( std::string, m_routecalc_observed_map, "de_nuke" );
		ADD_VARIABLE( std::string, m_routecalc_observed_area, "window" );
		ADD_VARIABLE( key_bind_t, m_routecalc_add_floor_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( key_bind_t, m_routecalc_add_pixelsurf_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( key_bind_t, m_routecalc_calculate_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( key_bind_t, m_routecalc_delete_point_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( key_bind_t, m_routecalc_clear_points_key, key_bind_t( 0, 1 ) );

		ADD_VARIABLE( bool, m_bouncee_assist, false );
		ADD_VARIABLE( bool, m_bouncee_assist_brokehop, false );
		ADD_VARIABLE( bool, m_bouncee_assist_render, false );
		ADD_VARIABLE( float, m_bouncee_assist_radius, 300.f );
		ADD_VARIABLE( key_bind_t, m_bounce_assist_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( key_bind_t, m_bounce_assist_point_key, key_bind_t( 0, 1 ) );

		ADD_VARIABLE( key_bind_t, m_ladder_bug_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( bool, m_ladder_bug, false );


		ADD_VARIABLE( key_bind_t, m_air_stuck_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( bool, m_air_stuck, false );
		float test_air_stuck = 0.f;
		ADD_VARIABLE( bool, m_deagle_spinner, false );
		ADD_VARIABLE( key_bind_t, m_deagle_spinner_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( int, m_deagle_spinner_mode, 0 );
		ADD_VARIABLE( float, m_deagle_spinner_speed, 1.0f );
		ADD_VARIABLE( bool, m_blockbot, false );
		ADD_VARIABLE( key_bind_t, m_blockbot_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( int, m_blockbot_target_mode, 0 );
		ADD_VARIABLE( int, m_blockbot_team_filter, 0 );
		ADD_VARIABLE( float, m_blockbot_strength, 1.0f );
		ADD_VARIABLE( float, m_blockbot_smoothing, 0.35f );
		ADD_VARIABLE( bool, m_px_database, false );
		ADD_VARIABLE( bool, m_px_database_auto_record, true );
		ADD_VARIABLE( bool, m_px_database_current_map_only, true );
		ADD_VARIABLE( bool, m_px_database_use_accent, true );
		ADD_VARIABLE( c_color, m_px_database_color, c_color( 174, 255, 0, 220 ) );
		ADD_VARIABLE( float, m_px_database_thickness, 1.5f );
		ADD_VARIABLE( float, m_px_database_distance, 1800.f );
		ADD_VARIABLE( bool, m_px_database_draw_through_walls, false );

		/* misc - movement - indicators */
		ADD_VARIABLE( c_color, m_velocity_indicator_color1, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_velocity_indicator_color2, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_velocity_indicator_color3, c_color( 255, 199, 89, 255 ) );
		ADD_VARIABLE( c_color, m_velocity_indicator_color4, c_color( 255, 119, 119, 255 ) );
		ADD_VARIABLE( c_color, m_velocity_indicator_color5, c_color( 30, 255, 109, 255 ) );
		ADD_VARIABLE( float, m_sun_set_rotation_speed, 1.0f );

		ADD_VARIABLE( bool, m_stamina_indicator, false );
		ADD_VARIABLE( bool, m_stamina_indicator_show_pre_speed, false );
		ADD_VARIABLE( c_color, m_stamina_indicator_color1, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_stamina_indicator_color2, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( bool, m_stamina_indicator_fade_alpha, false );
		ADD_VARIABLE( int, m_stamina_indicator_padding, 125 );

		ADD_VARIABLE( c_color, m_key_color, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_key_color_success, c_color( 0, 255, 0, 255 ) );
		ADD_VARIABLE( bool, m_key_indicators_success_toggle, true );
		ADD_VARIABLE( bool, m_key_indicators_enable, false );
		ADD_VARIABLE( int, m_key_indicators_type, 0 );
		ADD_VARIABLE( int, m_key_indicators_position, 100 );
		ADD_VARIABLE_VECTOR( bool, e_keybind_indicators::key_max, m_key_indicators, false );
		ADD_VARIABLE( bool, m_bind_overlay, false );
		ADD_VARIABLE( int, m_bind_overlay_position, 0 );
		ADD_VARIABLE( int, m_bind_overlay_style, 0 );
		ADD_VARIABLE( bool, m_bind_overlay_use_accent, true );
		ADD_VARIABLE( c_color, m_bind_overlay_color, c_color( 174, 255, 0, 255 ) );
		ADD_VARIABLE( float, m_bind_overlay_background_alpha, 0.62f );
		ADD_VARIABLE( float, m_bind_overlay_animation_speed, 1.0f );
		ADD_VARIABLE( float, m_bind_overlay_font_scale, 1.0f );

		/* misc - game */
		ADD_VARIABLE( bool, m_spectators_list, false );
		ADD_VARIABLE( c_color, m_spectators_list_text_color_one, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, m_spectators_list_text_color_two, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( int, m_spectators_list_type, 0 );
		ADD_VARIABLE( bool, m_spectators_avatar, false );
		ADD_VARIABLE( bool, m_scaleform, false );
		ADD_VARIABLE( int, m_clantag_mode, 0 );
		ADD_VARIABLE( std::string, m_custom_clantag, "drainware" );
		ADD_VARIABLE( float, m_clantag_speed, 1.f );
		ADD_VARIABLE( int, m_watermark_mode, 1 );
		ADD_VARIABLE( int, m_menu_logo_mode, 0 );
		ADD_VARIABLE( std::string, m_watermark_nickname, "verionfe" );
		ADD_VARIABLE( int, m_watermark_accent_mode, 0 );
		ADD_VARIABLE( c_color, m_watermark_custom_accent, c_color( 174, 255, 0, 255 ) );
		ADD_VARIABLE( float, m_watermark_background_alpha, 0.86f );
		ADD_VARIABLE( float, m_watermark_chroma_speed, 1.0f );
		ADD_VARIABLE( float, m_watermark_chroma_strength, 0.85f );
		ADD_VARIABLE( bool, m_movement_chat_prints, false );
		ADD_VARIABLE( bool, m_chat_print_edgebug, true );
		ADD_VARIABLE( bool, m_chat_print_pixelsurf, true );
		ADD_VARIABLE( bool, m_chat_print_new_px, true );
		ADD_VARIABLE( bool, m_chat_print_jumpbug, true );
		ADD_VARIABLE( std::string, m_chat_print_prefix, "dgware" );
		ADD_VARIABLE( c_color, m_chat_print_color, c_color( 174, 255, 0, 255 ) );
		ADD_VARIABLE( bool, m_chat_print_combine_accent, true );
		ADD_VARIABLE( int, m_chat_print_color_mode, 1 );
		ADD_VARIABLE( float, m_chat_print_cooldown, 0.55f );
		ADD_VARIABLE( bool, m_run_analyzer, false );
		ADD_VARIABLE( float, m_run_analyzer_min_speed, 230.f );
		ADD_VARIABLE( bool, m_run_analyzer_show_failed, true );
		ADD_VARIABLE( bool, m_run_analyzer_use_accent, true );
		ADD_VARIABLE( c_color, m_run_analyzer_color, c_color( 174, 255, 0, 255 ) );
		ADD_VARIABLE( float, m_run_analyzer_end_timeout, 0.75f );
		ADD_VARIABLE( bool, m_run_analyzer_include_raw_binds, false );
		ADD_VARIABLE( bool, m_old_edit_vibes, false );
		ADD_VARIABLE( float, m_old_edit_vibes_chance, 0.05f );
		ADD_VARIABLE( int, m_old_edit_vibes_min_combo, 3 );
		ADD_VARIABLE( bool, m_old_edit_vibes_text, true );
		ADD_VARIABLE( bool, m_old_edit_vibes_sound, false );
		ADD_VARIABLE( std::string, m_old_edit_vibes_sound_path, "" );

		ADD_VARIABLE( bool, m_sniper_crosshair, false );

		ADD_VARIABLE( bool, m_practice_window, false );
		ADD_VARIABLE( bool, m_practice_mode, false );
		ADD_VARIABLE( bool, m_practice_basic_setup, true );
		ADD_VARIABLE( bool, m_practice_give_weapons, true );
		ADD_VARIABLE( bool, m_practice_disable_fall_damage, true );
		ADD_VARIABLE( bool, m_practice_ignore_round_win, true );
		ADD_VARIABLE( bool, m_practice_restart_after_apply, true );
		ADD_VARIABLE( std::string, m_practice_custom_commands, "" );
		ADD_VARIABLE( bool, m_practice_apply_requested, false );
		ADD_VARIABLE( bool, m_practice_reset_requested, false );
		ADD_VARIABLE( key_bind_t, m_practice_cp_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( key_bind_t, m_practice_tp_key, key_bind_t( 0, 1 ) );
		ADD_VARIABLE( float, m_pixel_surf_assist_lockfactor, 0.f );
		ADD_VARIABLE( int, m_pixel_surf_assist_ticks, 12 );
		ADD_VARIABLE( int, m_pixel_surf_assist_type, 0 );
		ADD_VARIABLE( float, m_pixelsurf_assist_delta_scale, 0.f );
		ADD_VARIABLE_VECTOR( bool, e_log_types::log_type_max, m_log_types, false );

		/* fonts */
		ADD_VARIABLE_VECTOR( bool, e_free_type_font_flags::font_flag_max, m_indicator_font_flags, false );
		ADD_VARIABLE( font_setting_t, m_indicator_font_settings, font_setting_t( "Verdanab", 29 ) );

		// NEW: ESP font selection
		ADD_VARIABLE_VECTOR( bool, e_free_type_font_flags::font_flag_max, m_esp_font_flags, false );
		ADD_VARIABLE( font_setting_t, m_esp_font_settings, font_setting_t( "Verdanab", 13 ) );

		// NEW: Render Assistant (addwindow) font selection
		ADD_VARIABLE_VECTOR( bool, e_free_type_font_flags::font_flag_max, m_addwindow_font_flags, false );
		ADD_VARIABLE( font_setting_t, m_addwindow_font_settings, font_setting_t( "Verdanab", 11 ) );
		ADD_VARIABLE( bool, m_debugger_visual, true );
		ADD_VARIABLE( bool, m_disable_interp, false );
		ADD_VARIABLE( bool, trackdispay, false );
		ADD_VARIABLE( bool, esp_enable, false );
		ADD_VARIABLE( bool,    colors_custom, false );
		
		ADD_VARIABLE( c_color, colors_health_bar				, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, colors_health_bar_upper			, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, colors_health_bar_lower			, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, colors_health_bar_outline		, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, colors_health_bar_in, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, colors_health_bar_upper_in, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, colors_health_bar_lower_in, c_color( 255, 255, 255, 255 ) );
		ADD_VARIABLE( c_color, colors_health_bar_outline_in, c_color( 255, 255, 255, 255 ) );
		
		ADD_VARIABLE( int, m_visible_alpha, 255 );
		ADD_VARIABLE( int, m_invisuals_alpha, 255 );
		
			ADD_VARIABLE( bool, health_bar_enable       , false);
			ADD_VARIABLE( bool, health_bar_baseonhealth , false);
			ADD_VARIABLE( bool, health_bar_gradient     , false);
			ADD_VARIABLE( bool, health_bar_background   , false);
			ADD_VARIABLE( bool, health_bar_text         , false);
			ADD_VARIABLE( int,  health_bar_size          ,    1);
		

		
			
				
		

		std::wstring artist, title;
		bool pause    = false;
		bool next     = false;
		bool pervious = false;
		std::string albumArtPath;
		int64_t trackPosition;
		int64_t trackDuration = 0;
		std::string m_custom_sky_box{ };

	

	

	};
}; // namespace n_variables

inline n_variables::impl_t g_variables{ };
