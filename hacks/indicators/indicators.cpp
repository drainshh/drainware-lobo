#include "indicators.h"

#include "../../game/sdk/includes/includes.h"
#include "../../globals/includes/includes.h"
#include "../movement/movement.h"
#include "../instalisation fonts/fonts2.h"
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdarg>
#include <type_traits>
#include <cstddef>
#include <any>
void n_indicators::impl_t::on_paint_traverse( )
{
	const bool on_ground = g_ctx.m_local->get_flags( ) & e_flags::fl_onground;

	if ( GET_VARIABLE( g_variables.m_velocity_indicator, bool ) )
		this->velocity( on_ground );

	if ( GET_VARIABLE( g_variables.m_stamina_indicator, bool ) )
		this->stamina( on_ground );

	if ( GET_VARIABLE( g_variables.m_key_indicators_enable, bool ) && GET_VARIABLE(g_variables.m_key_indicators_type,int) == 0 )
		this->keybind_indicators( );

	

	if ( GET_VARIABLE( g_variables.m_sniper_crosshair, bool ) )
		this->sniper_crosshair( );

	this->fps_warning( );
}

void n_indicators::impl_t::on_end_scene()
{
	if ( GET_VARIABLE( g_variables.m_key_indicators_enable, bool ) && GET_VARIABLE( g_variables.m_key_indicators_type, int ) == 1 )
		this->keybind_indicators_hor( );
}

void n_indicators::impl_t::fps_warning( )
{
	if ( !g_ctx.m_local || !g_interfaces.m_engine_client->is_in_game( ) )
		return;

	[ & ]( const char* fps_warning, const c_color& color, const bool active ) {
		ImAnimationHelper fps_animation = ImAnimationHelper( HASH_RT( fps_warning ), ImGui::GetIO( ).DeltaTime );
		fps_animation.Update( 2.f, active ? 2.f : -2.f );

		if ( fps_animation.AnimationData->second <= 0.f )
			return;

		const auto text_size =
			fonts_for_gui::bolder->CalcTextSizeA(
			fonts_for_gui::bolder->FontSize, FLT_MAX, 0.f, fps_warning );

		// render 'LOW FPS'
		g_render.m_draw_data.emplace_back(
			e_draw_type::draw_type_text,
			std::make_any< text_draw_object_t >(
				fonts_for_gui::bolder, c_vector_2d( ( g_ctx.m_width / 2 ) - ( text_size.x / 2 ), 200 ),
				fps_warning, color.get_u32( fps_animation.AnimationData->second ),
				ImColor( 0.f, 0.f, 0.f, color.base< e_color_type::color_type_a >( ) * fps_animation.AnimationData->second ),
				e_text_flags::text_flag_dropshadow ) );

		// render '!' to the left
		g_render.m_draw_data.emplace_back(
			e_draw_type::draw_type_text,
			std::make_any< text_draw_object_t >(
				fonts_for_gui::bolder, c_vector_2d( ( g_ctx.m_width / 2 ) - text_size.x, 200 ), "!",
				ImColor( 1.f, 0.f, 0.f, 1.f ),
				ImColor( 0.f, 0.f, 0.f, color.base< e_color_type::color_type_a >( ) * fps_animation.AnimationData->second ),
				e_text_flags::text_flag_dropshadow ) );

		// render '!' to the right
		g_render.m_draw_data.emplace_back(
			e_draw_type::draw_type_text,
			std::make_any< text_draw_object_t >(
				fonts_for_gui::bolder, c_vector_2d( ( g_ctx.m_width / 2 ) + text_size.x, 200 ), "!",
				ImColor( 1.f, 0.f, 0.f, 1.f ),
				ImColor( 0.f, 0.f, 0.f, color.base< e_color_type::color_type_a >( ) * fps_animation.AnimationData->second ),
				e_text_flags::text_flag_dropshadow ) );

		const std::string current_fps_text =
			std::format( "current fps: {} needed fps: {}+",
		                   static_cast< int >( ImGui::GetIO( ).Framerate + 0.5f ),
		                                         static_cast< int >( 1.f / g_interfaces.m_global_vars_base->m_interval_per_tick )  );

		const auto current_fps_text_size =
			fonts_for_gui::regular->CalcTextSizeA(
			fonts_for_gui::regular->FontSize, FLT_MAX, 0.f, current_fps_text.c_str( ) );

		// render current fps and needed fps
		g_render.m_draw_data.emplace_back(
			e_draw_type::draw_type_text,
			std::make_any< text_draw_object_t >(
				fonts_for_gui::regular, c_vector_2d( ( g_ctx.m_width / 2 ) - ( current_fps_text_size.x / 2 ), 215 ),
				current_fps_text, ImColor( .7f, .7f, .7f, fps_animation.AnimationData->second ),
				ImColor( 0.f, 0.f, 0.f, color.base< e_color_type::color_type_a >( ) * fps_animation.AnimationData->second ),
				e_text_flags::text_flag_dropshadow ) );
	}( "LOW FPS", c_color( 1.f, 1.f, 1.f, 1.f ), g_ctx.m_low_fps );
}

void n_indicators::impl_t::sniper_crosshair( )
{
	if ( !g_ctx.m_local->is_alive( ) )
		return;

	auto weapon_handle = g_ctx.m_local->get_active_weapon_handle( );
	if ( !weapon_handle )
		return;

	const auto active_weapon = g_interfaces.m_client_entity_list->get< c_base_entity >( weapon_handle );

	if ( !active_weapon )
		return;

	const auto definition_index = active_weapon->get_item_definition_index( );

	if ( g_ctx.m_local->is_scoped( ) )
		return;

	if ( g_utilities.is_in< short >( definition_index, { e_item_definition_index::weapon_awp, e_item_definition_index::weapon_ssg08,
	                                                     e_item_definition_index::weapon_scar20, e_item_definition_index::weapon_g3sg1 } ) ) {
		g_render.m_draw_data.emplace_back(
			e_draw_type::draw_type_rect, std::make_any< rect_draw_object_t >( c_vector_2d( ( g_ctx.m_width / 2 ) - 1, ( g_ctx.m_height / 2 ) - 1 ),
		                                                                      c_vector_2d( ( g_ctx.m_width / 2 ) + 1, ( g_ctx.m_height / 2 ) + 1 ),
		                                                                      ImColor( 1.f, 1.f, 1.f, 1.f ), ImColor( 0.f, 0.f, 0.f, 0.f ), true, 0.f,
		                                                                      ImDrawFlags_::ImDrawFlags_None, 4.f, e_rect_flags::rect_flag_none ) );
	}
}
extern bool HITGODA;
extern bool HITGODA2;
extern bool EdgeBug_Founded;

struct Indicator {
	std::string label;
	ImVec2 size;

	// Анимация появления/исчезания с плавным lerp
	float opacity       = 0.0f; // текущая прозрачность [0 ... 1]
	float targetOpacity = 1.0f; // целевая прозрачность

	float posX    = 0.0f; // текущая позиция X
	float targetX = 0.0f; // целевая позиция X
	bool removing = false;

	// Анимация дропшадовс
	bool detect         = false;
	float shadowIntensity = 0.0f; // текущая интенсивность дропшадовс [0 ... 1]
	float targetShadow    = 0.0f; // целевая интенсивность дропшадовс

	// Сглаживающий коэффициент для различных анимаций
	static constexpr float OPACITY_SMOOTH  = 16.0f; // выше = быстрее
	static constexpr float POSITION_SMOOTH = 14.0f; // выше = быстрее
	static constexpr float SHADOW_SMOOTH   = 12.0f; // выше = быстрее
};

static constexpr float SPACING = 5.0f;
static constexpr float OFFSET  = 120.0f; // Появление чуть ближе к центру

static std::vector< Indicator > g_indicators_keys;
struct CallData {
	std::string label;
	bool active;
	bool detect;
};
static std::vector< CallData > g_calls;

// Внешний API: без цвета, только с detect
void RenderInd( const std::string& label, bool active, bool detect )
{
	g_calls.push_back( { label, active, detect } );
}

// Плавный lerp между двумя значениями
float LerpFloat( float current, float target, float smoothFactor, float dt )
{
	// Более плавная, iOS-подобная анимация с экспоненциальным сглаживанием
	return current + ( target - current ) * ( 1.0f - expf( -smoothFactor * dt ) );
}

// Пересчитать новые targetX для "живых" индикаторов
static void RecalculateTargets( )
{
	ImGuiIO& io = ImGui::GetIO( );
	float W     = io.DisplaySize.x;

	std::vector< ImVec2 > sizes;
	for ( auto& I : g_indicators_keys )
		if ( !I.removing )
			sizes.push_back( I.size );

	float totalW = -SPACING;
	for ( auto& s : sizes )
		totalW += s.x + SPACING;
	float baseX = W * 0.5f - totalW * 0.5f;

	size_t idx = 0;
	for ( auto& I : g_indicators_keys ) {
		if ( I.removing )
			continue;
		I.targetX = baseX;
		baseX += sizes[ idx++ ].x + SPACING;
	}
}

void RenderIndicatorsOverlay( )
{
	ImGuiIO& io = ImGui::GetIO( );
	float dt    = io.DeltaTime;
	auto draw   = ImGui::GetForegroundDrawList( );
	float H     = io.DisplaySize.y;
	float W     = io.DisplaySize.x;

	// 1) Разбираем все вызовы RenderInd
	for ( auto& cmd : g_calls ) {
		auto it = std::find_if( g_indicators_keys.begin( ), g_indicators_keys.end( ), [ & ]( auto& I ) { return I.label == cmd.label; } );

		if ( cmd.active ) {
			if ( it == g_indicators_keys.end( ) ) {
				// Новый индикатор
				float originX = W + OFFSET;
				if ( !g_indicators_keys.empty( ) ) {
					float maxX = 0;
					for ( auto& J : g_indicators_keys )
						if ( !J.removing )
							maxX = std::max( maxX, J.targetX + J.size.x );
					originX = maxX + OFFSET;
				}

				Indicator I;
				I.label = cmd.label;
				I.size  = g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_indicator ]->CalcTextSizeA(
                    g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_indicator ]->FontSize, FLT_MAX, NULL, I.label.c_str( ) );
				I.opacity       = 0.0f;
				I.targetOpacity = 1.0f;
				I.removing      = false;
				I.detect        = cmd.detect;
				I.shadowIntensity = 0.0f;
				I.targetShadow    = 1.0f; // Всегда есть базовый дропшадовс
				I.posX          = originX;
				I.targetX       = originX;

				g_indicators_keys.push_back( I );
				RecalculateTargets( );
			} else if ( !it->removing ) {
				// Обновляем флаг detect
				it->detect = cmd.detect;
			}
		} else if ( it != g_indicators_keys.end( ) && !it->removing ) {
			// Удаление
			it->removing      = true;
			it->targetOpacity = 0.0f;
			it->targetX       = it->posX + OFFSET;
			it->targetShadow  = 0.0f;
			RecalculateTargets( );
		}
	}
	g_calls.clear( );

	// 2) Анимация + рендер с плавным lerp
	for ( auto it = g_indicators_keys.begin( ); it != g_indicators_keys.end( ); ) {
		auto& I = *it;

		// Плавное изменение прозрачности
		I.opacity = LerpFloat( I.opacity, I.targetOpacity, I.OPACITY_SMOOTH, dt );

		// Плавное изменение позиции
		I.posX = LerpFloat( I.posX, I.targetX, I.POSITION_SMOOTH, dt );

		// Плавное изменение дропшадовс
		I.shadowIntensity = LerpFloat( I.shadowIntensity, I.targetShadow, I.SHADOW_SMOOTH, dt );

		float x = I.posX;
		float y = H - GET_VARIABLE( g_variables.m_key_indicators_position, int );

		// Рендер с дропшадовс
		if ( I.shadowIntensity > 0.01f ) {
			// Определяем цвет дропшадовс
			ImVec4 shadowColor;
			if ( I.detect && GET_VARIABLE( g_variables.m_key_indicators_success_toggle, bool ) ) {
				// Акцент цвет при срабатывании из конфига
				c_color& accentColor = GET_VARIABLE( g_variables.m_key_color_success, c_color );
				shadowColor = ImVec4( accentColor[0], accentColor[1], accentColor[2], I.shadowIntensity * I.opacity * 0.7f );
			} else {
				// Обычный черный дропшадовс
				shadowColor = ImVec4( 0.0f, 0.0f, 0.0f, I.shadowIntensity * I.opacity * 0.6f );
			}

			// Рендер дропшадовс (смещение вниз и вправо)
			draw->AddText( g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_indicator ],
			               g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_indicator ]->FontSize, 
			               ImVec2( x + 1.0f, y + 1.0f ), 
			               ImGui::GetColorU32( shadowColor ), 
			               I.label.c_str( ) );
		}

		// Основной текст (белый)
		ImU32 textColor = ImGui::GetColorU32( ImVec4( 1.0f, 1.0f, 1.0f, I.opacity ) );
		draw->AddText( g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_indicator ],
		               g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_indicator ]->FontSize, 
		               ImVec2( x, y ), 
		               textColor, 
		               I.label.c_str( ) );

		// Удаляем индикатор, когда он полностью прозрачный (анимация завершена)
		if ( I.removing && I.opacity < 0.01f )
			it = g_indicators_keys.erase( it );
		else
			++it;
	}
}

// Пример вставки в ваш кадр:
void MyImGuiFrame( )
{
	// Pixel Surf (ps)
	if ( GET_VARIABLE( g_variables.m_pixel_surf, bool ) &&
	     g_config.get< std::vector< bool > >( g_variables.m_key_indicators )[ e_keybind_indicators::key_ps ] ) {
		bool active = g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_surf_key, key_bind_t ) );
		bool detect = g_movement.m_pixelsurf_data.m_predicted_succesful || g_movement.m_pixelsurf_data.m_in_pixel_surf;
		RenderInd( "ps", active, detect );
	}

	// Edge Jump (ej)
	if ( GET_VARIABLE( g_variables.m_edge_jump, bool ) &&
	     g_config.get< std::vector< bool > >( g_variables.m_key_indicators )[ e_keybind_indicators::key_ej ] ) {
		bool active = g_input.check_input( &GET_VARIABLE( g_variables.m_edge_jump_key, key_bind_t ) );
		bool detect = false;
		RenderInd( "ej", active, detect );
	}

	// Long Jump (lj)
	if ( GET_VARIABLE( g_variables.m_long_jump, bool ) &&
	     g_config.get< std::vector< bool > >( g_variables.m_key_indicators )[ e_keybind_indicators::key_lj ] ) {
		bool active = g_input.check_input( &GET_VARIABLE( g_variables.m_long_jump_key, key_bind_t ) );
		bool detect = false;
		RenderInd( "lj", active, detect );
	}

	// Mini Jump (mj)
	if ( GET_VARIABLE( g_variables.m_mini_jump, bool ) &&
	     g_config.get< std::vector< bool > >( g_variables.m_key_indicators )[ e_keybind_indicators::key_mj ] ) {
		bool active = g_input.check_input( &GET_VARIABLE( g_variables.m_mini_jump_key, key_bind_t ) );
		bool detect = false;
		RenderInd( "mj", active, detect );
	}

	// Jump Bug (jb)
	if ( GET_VARIABLE( g_variables.m_jump_bug, bool ) &&
	     g_config.get< std::vector< bool > >( g_variables.m_key_indicators )[ e_keybind_indicators::key_jb ] ) {
		bool active = g_input.check_input( &GET_VARIABLE( g_variables.m_jump_bug_key, key_bind_t ) );
		bool detect = g_movement.m_jumpbug_data.m_can_jb;
		RenderInd( "jb", active, detect );
	}

	// Объединенный Assist (ast) - объединение ast и bast
	if ( ( GET_VARIABLE( g_variables.m_pixel_surf_assist, bool ) || GET_VARIABLE( g_variables.m_bouncee_assist, bool ) ) &&
	     g_config.get< std::vector< bool > >( g_variables.m_key_indicators )[ e_keybind_indicators::key_ast ] ) {
		bool active = g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_surf_assist_key, key_bind_t ) ) ||
		              g_input.check_input( &GET_VARIABLE( g_variables.m_bounce_assist_key, key_bind_t ) );
		bool detect = HITGODA || HITGODA2; // Объединяем условия
		RenderInd( "ast", active, detect );
	}

	// Edge Bug (eb)
	if ( GET_VARIABLE( g_variables.edge_bug, bool ) &&
	     g_config.get< std::vector< bool > >( g_variables.m_key_indicators )[ e_keybind_indicators::key_eb ] ) {
		bool active = g_input.check_input( &GET_VARIABLE( g_variables.edge_bug_key, key_bind_t ) );
		bool detect = EdgeBug_Founded;
		RenderInd( "eb", active, detect );
	}

	// Fireman (fr)
	if ( GET_VARIABLE( g_variables.m_fire_man, bool ) &&
	     g_config.get< std::vector< bool > >( g_variables.m_key_indicators )[ e_keybind_indicators::key_fr ] ) {
		bool active = g_input.check_input( &GET_VARIABLE( g_variables.m_fire_man_key, key_bind_t ) );
		bool detect = g_movement.m_fireman_data.is_ladder;
		RenderInd( "fm", active, detect );
	}

	// Ladder Bug (lb)
	if ( GET_VARIABLE( g_variables.m_ladder_bug, bool ) &&
	     g_config.get< std::vector< bool > >( g_variables.m_key_indicators )[ e_keybind_indicators::key_lb ] ) {
		bool active = g_input.check_input( &GET_VARIABLE( g_variables.m_ladder_bug_key, key_bind_t ) );
		bool detect = g_movement.m_ladder_bug_data.founded;
		RenderInd( "lb", active, detect );
	}

	RenderIndicatorsOverlay( );
}

void n_indicators::impl_t::keybind_indicators( )
{
	float offset = 0.f;
	const bool success_enabled = GET_VARIABLE( g_variables.m_key_indicators_success_toggle, bool );
	const c_color key_success_color = GET_VARIABLE( g_variables.m_key_color_success, c_color );
	const auto success_color = [ & ]( const bool success ) {
		return success && success_enabled ? key_success_color : GET_VARIABLE( g_variables.m_key_color, c_color );
	};
	const auto success_color_custom = [ & ]( const bool success, const c_color& color ) {
		return success && success_enabled ? color : GET_VARIABLE( g_variables.m_key_color, c_color );
	};

	const auto render_indicator = [ & ]( const char* indicator_name, const c_color& color, const bool active ) {
		ImAnimationHelper indicator_animation = ImAnimationHelper( HASH_RT( indicator_name ), ImGui::GetIO( ).DeltaTime );
		indicator_animation.Update( 3.f, active ? 3.f : -3.f );

		if ( indicator_animation.AnimationData->second <= 0.f )
			return;

		const auto text_size = g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_indicator ]->CalcTextSizeA(
			g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_indicator ]->FontSize, FLT_MAX, 0.f, indicator_name );

		g_render.m_draw_data.emplace_back(
			e_draw_type::draw_type_text,
			std::make_any< text_draw_object_t >(
				g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_indicator ],
				c_vector_2d( ( g_ctx.m_width - text_size.x ) / 2.f,
		                     g_ctx.m_height - offset - GET_VARIABLE( g_variables.m_key_indicators_position, int ) ),

				indicator_name, color.get_u32( indicator_animation.AnimationData->second ),
				ImColor( 0.f, 0.f, 0.f, color.base< e_color_type::color_type_a >( ) * indicator_animation.AnimationData->second ),
				e_text_flags::text_flag_dropshadow ) );

		offset -= text_size.y * indicator_animation.AnimationData->second;
	};

	

	if ( GET_VARIABLE( g_variables.m_pixel_surf, bool ) &&
	     g_config.get< std::vector< bool > >( g_variables.m_key_indicators )[ e_keybind_indicators::key_ps ] )
		render_indicator( "ps",
		                  success_color( g_movement.m_pixelsurf_data.m_predicted_succesful || g_movement.m_pixelsurf_data.m_in_pixel_surf ),
		                  g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_surf_key, key_bind_t ) ) );

	if ( GET_VARIABLE( g_variables.m_edge_jump, bool ) &&
	     g_config.get< std::vector< bool > >( g_variables.m_key_indicators )[ e_keybind_indicators::key_ej ] )
		render_indicator( "ej", GET_VARIABLE( g_variables.m_key_color, c_color ),
		                  g_input.check_input( &GET_VARIABLE( g_variables.m_edge_jump_key, key_bind_t ) ) );

	if ( GET_VARIABLE( g_variables.m_long_jump, bool ) &&
	     g_config.get< std::vector< bool > >( g_variables.m_key_indicators )[ e_keybind_indicators::key_lj ] )
		render_indicator( "lj", GET_VARIABLE( g_variables.m_key_color, c_color ),
		                  g_input.check_input( &GET_VARIABLE( g_variables.m_long_jump_key, key_bind_t ) ) );

	// TODO: re-add delayhop

	/* if ( GET_CONFIG_BOOL( g_variables.m_delay_hop ) &&
	     config.get< std::vector< bool > >( g_variables.m_key_indicators )[ e_keybind_indicators::key_dh ] )
	    render_indicator( "dh", GET_VARIABLE( g_variables.m_key_color, c_color ),
	                      g_input.check_input( &GET_VARIABLE( g_variables.m_delay_hop_key ) ) );*/

	if ( GET_VARIABLE( g_variables.m_mini_jump, bool ) &&
	     g_config.get< std::vector< bool > >( g_variables.m_key_indicators )[ e_keybind_indicators::key_mj ] )
		render_indicator( "mj", GET_VARIABLE( g_variables.m_key_color, c_color ),
		                  g_input.check_input( &GET_VARIABLE( g_variables.m_mini_jump_key, key_bind_t ) ) );

	if ( GET_VARIABLE( g_variables.m_jump_bug, bool ) &&
	     g_config.get< std::vector< bool > >( g_variables.m_key_indicators )[ e_keybind_indicators::key_jb ] )
		render_indicator( "jb", success_color_custom( g_movement.m_jumpbug_data.m_can_jb, c_color( 1.f, 0.f, 0.f, 1.f ) ),
		                  g_input.check_input( &GET_VARIABLE( g_variables.m_jump_bug_key, key_bind_t ) ) );

	if ( GET_VARIABLE( g_variables.m_pixel_surf_assist, bool ) &&
	     g_config.get< std::vector< bool > >( g_variables.m_key_indicators )[ e_keybind_indicators::key_ast ] )
		render_indicator( "ast", success_color( HITGODA ),
		                  g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_surf_assist_key, key_bind_t ) ) );
	if ( GET_VARIABLE( g_variables.m_bouncee_assist, bool ) &&
	     g_config.get< std::vector< bool > >( g_variables.m_key_indicators )[ e_keybind_indicators::key_ast ] )
		render_indicator( "bast", success_color( HITGODA2 ),
		                  g_input.check_input( &GET_VARIABLE( g_variables.m_bounce_assist_key, key_bind_t ) ) );
	if ( GET_VARIABLE( g_variables.edge_bug, bool ) &&
	     g_config.get< std::vector< bool > >( g_variables.m_key_indicators )[ e_keybind_indicators::key_eb ] )
		render_indicator(
			"eb", success_color( EdgeBug_Founded ),
		                  g_input.check_input( &GET_VARIABLE( g_variables.edge_bug_key, key_bind_t ) ) );
	if ( GET_VARIABLE( g_variables.m_fire_man, bool ) &&
	     g_config.get< std::vector< bool > >( g_variables.m_key_indicators )[ e_keybind_indicators::key_fr ] )
		render_indicator( "fr", success_color( g_movement.m_fireman_data.is_ladder ),
		                  g_input.check_input( &GET_VARIABLE( g_variables.m_fire_man_key, key_bind_t ) ) );
	if ( GET_VARIABLE( g_variables.m_fire_man, bool ) &&
	     g_config.get< std::vector< bool > >( g_variables.m_key_indicators )[ e_keybind_indicators::key_lb ] )
		render_indicator( "lb", success_color( g_movement.m_ladder_bug_data.founded ),
		                  g_input.check_input( &GET_VARIABLE( g_variables.m_ladder_bug_key, key_bind_t ) ) );
}

void n_indicators::impl_t::keybind_indicators_hor( ) {


	MyImGuiFrame( );
}

struct VelocityIndicator {
	std::string label;
	ImVec2 size;

	// Анимация появления/исчезания с плавным lerp
	float opacity       = 0.0f; // текущая прозрачность [0 ... 1]
	float targetOpacity = 1.0f; // целевая прозрачность

	float posX    = 0.0f; // текущая позиция X
	float targetX = 0.0f; // целевая позиция X
	bool removing = false;

	// Анимация цвета с плавным lerp
	bool detect         = false;
	ImVec4 currentColor = ImVec4( 1.0f, 1.0f, 1.0f, 1.0f ); // текущий цвет
	ImVec4 detectColor;                                     // целевой цвет при detect

	// Сглаживающий коэффициент для различных анимаций
	static constexpr float OPACITY_SMOOTH  = 16.0f; // выше = быстрее
	static constexpr float POSITION_SMOOTH = 18.0f; // выше = быстрее (увеличен для более плавной анимации)
	static constexpr float COLOR_SMOOTH    = 18.0f; // выше = быстрее
};

static constexpr float VELOCITY_SPACING = 6.0f; // уменьшено для меньшего расстояния
static constexpr float VELOCITY_OFFSET  = 12.0f; // уменьшено для меньшего отступа

static std::vector< VelocityIndicator > g_velocity_indicators;
struct VelocityCallData {
	std::string label;
	bool active;
	bool detect;
	ImColor detect_color;
};
static std::vector< VelocityCallData > g_velocity_calls;

// Внешний API для добавления velocity индикаторов
void RenderVelocityInd( const std::string& label, bool active, bool detect, ImColor detect_color )
{
	g_velocity_calls.push_back( { label, active, detect, detect_color.Value } );
}

// Плавный lerp между двумя значениями
float VelocityLerpFloat( float current, float target, float smoothFactor, float dt )
{
	return current + ( target - current ) * ( 1.0f - expf( -smoothFactor * dt ) );
}

// Плавный lerp между двумя цветами
ImVec4 VelocityLerpColor( const ImVec4& current, const ImVec4& target, float smoothFactor, float dt )
{
	ImVec4 result;
	result.x = VelocityLerpFloat( current.x, target.x, smoothFactor, dt );
	result.y = VelocityLerpFloat( current.y, target.y, smoothFactor, dt );
	result.z = VelocityLerpFloat( current.z, target.z, smoothFactor, dt );
	result.w = VelocityLerpFloat( current.w, target.w, smoothFactor, dt );
	return result;
}

// Структура для основного velocity индикатора
struct MainVelocityIndicator {
	float posX    = 0.0f; // текущая позиция X
	float targetX = 0.0f; // целевая позиция X

	static constexpr float POSITION_SMOOTH = 18.0f; // выше = быстрее
};

static MainVelocityIndicator g_main_velocity;

// Пересчитать позиции для всех velocity индикаторов (включая основной)
static void RecalculateVelocityTargets( float main_velocity_width )
{
	// Найти первый (самый левый) take-off индикатор (даже если removing)
	VelocityIndicator* first_takeoff = nullptr;
	for (auto& I : g_velocity_indicators) {
		if (!first_takeoff || I.posX < first_takeoff->posX)
			first_takeoff = &I;
	}

	float takeoff_width = first_takeoff ? first_takeoff->size.x : 0.f;
	float spacing = (first_takeoff ? VELOCITY_SPACING : 0.f);
	float total_width = main_velocity_width + spacing + takeoff_width;

	float group_left = (g_ctx.m_width - total_width) / 2.0f;

	// velocity всегда слева в группе
	g_main_velocity.targetX = group_left;

	// take-off всегда справа от velocity + spacing
	if (first_takeoff) {
		first_takeoff->targetX = group_left + main_velocity_width + spacing;
	}

	// Если есть другие take-off индикаторы (редко, но вдруг), выстраиваем их дальше справа
	float next_x = group_left + main_velocity_width + spacing + takeoff_width;
	for (auto& I : g_velocity_indicators) {
		if (&I == first_takeoff) continue;
		I.targetX = next_x;
		next_x += I.size.x + VELOCITY_SPACING;
	}
}

void n_indicators::impl_t::velocity( const bool on_ground )
{
	const float velocity_len = g_ctx.m_local->get_velocity( ).length_2d( );
	const int velocity = static_cast< int >( std::round( velocity_len ) );

	// fade alpha logic
	float alpha = 1.0f;
	if (GET_VARIABLE(g_variables.m_velocity_indicator_fade_alpha, bool)) {
		if (velocity_len < 100.f)
			alpha = velocity_len / 100.f;
		else
			alpha = 1.0f;
	}

	// Проверяем настройку take-off в меню перед созданием индикатора
	const bool take_off_enabled = GET_VARIABLE( g_variables.m_velocity_indicator_show_pre_speed, bool );

	// Определяем, нужно ли обновить скорость при отрыве от земли
	if ( take_off_enabled && this->m_indicator_data.m_last_on_ground_velocity && !on_ground ) {
		this->m_indicator_data.m_take_off_velocity      = velocity;
		this->m_indicator_data.m_take_off_time_velocity = g_interfaces.m_global_vars_base->m_current_time + 2.f;

		// Создаём или обновляем take-off индикатор только если функция включена
		std::string take_off_text = "(" + std::to_string( velocity ) + ")";
		ImColor take_off_color    = GET_VARIABLE( g_variables.m_velocity_indicator_color1, c_color ).get_u32( );
		RenderVelocityInd( take_off_text, true, false, take_off_color );
	}

	// Скрываем take-off индикатор, когда время истекло или функция отключена
	if ( this->m_indicator_data.m_take_off_time_velocity < g_interfaces.m_global_vars_base->m_current_time || !take_off_enabled ) {
		// Помечаем все индикаторы для удаления
		for ( auto& ind : g_velocity_indicators ) {
			if ( !ind.removing ) {
				ind.removing      = true;
				ind.targetOpacity = 0.0f;
				ind.targetX       = ind.posX + VELOCITY_OFFSET;
			}
		}
	}

	// Обработка анимаций velocity индикаторов
	float dt = g_interfaces.m_global_vars_base->m_frame_time;

	if ( take_off_enabled ) {
		// Разбираем все вызовы RenderVelocityInd
		for ( auto& cmd : g_velocity_calls ) {
			auto it = std::find_if( g_velocity_indicators.begin( ), g_velocity_indicators.end( ),
			                        [ &cmd ]( const auto& I ) { return I.label == cmd.label; } );

			if ( cmd.active ) {
				if ( it == g_velocity_indicators.end( ) ) {
					// Ищем существующий индикатор взлёта (метка начинается с "(")
					auto takeoff_it = std::find_if( g_velocity_indicators.begin( ), g_velocity_indicators.end( ),
					                                []( const auto& I ) { return I.label.find( "(") == 0 && !I.removing; } );

					if ( takeoff_it != g_velocity_indicators.end( ) ) {
						// Обновляем существующий индикатор
						takeoff_it->label = cmd.label;
						takeoff_it->size  = g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_indicator ]->CalcTextSizeA(
                            g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_indicator ]->FontSize, FLT_MAX, NULL,
                            takeoff_it->label.c_str( ) );
					} else {
						// Создаём новый индикатор с анимацией появления
						VelocityIndicator I;
						I.label = cmd.label;
						I.size  = g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_indicator ]->CalcTextSizeA(
                            g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_indicator ]->FontSize, FLT_MAX, NULL, I.label.c_str( ) );
						I.opacity       = 0.0f;
						I.targetOpacity = 1.0f;
						I.removing      = false;
						I.detect        = cmd.detect;
						I.detectColor   = cmd.detect_color;
						I.currentColor  = ImVec4( 1.0f, 1.0f, 1.0f, 0.0f );
						I.posX          = g_ctx.m_width + VELOCITY_OFFSET;
						I.targetX       = g_ctx.m_width + VELOCITY_OFFSET;
						g_velocity_indicators.push_back( I );
					}
				} else if ( !it->removing ) {
					// Обновляем существующий индикатор
					it->detect      = cmd.detect;
					it->detectColor = cmd.detect_color;
				}
			}
		}
	}
	g_velocity_calls.clear( );

	// Цвета
	const ImColor blended_color = ImColor::Blend( GET_VARIABLE( g_variables.m_velocity_indicator_color1, c_color ).get_u32( ),
	                                              GET_VARIABLE( g_variables.m_velocity_indicator_color2, c_color ).get_u32( ), alpha );

	const ImColor speed_based_color = velocity == this->m_indicator_data.m_last_velocity
	                                      ? GET_VARIABLE( g_variables.m_velocity_indicator_color3, c_color )
	                                            .get_u32( alpha )
	                                  : velocity < this->m_indicator_data.m_last_velocity
	                                      ? GET_VARIABLE( g_variables.m_velocity_indicator_color4, c_color )
	                                            .get_u32( alpha )
	                                      : GET_VARIABLE( g_variables.m_velocity_indicator_color5, c_color )
	                                            .get_u32( alpha );

	// Отрисовка основного индикатора скорости
	const std::string velocity_text = std::to_string( velocity );
	const auto text_size            = g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_indicator ]->CalcTextSizeA(
        g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_indicator ]->FontSize, FLT_MAX, 0.f, velocity_text.c_str( ) );

	// Всегда пересчитываем позиции при изменении размеров
	RecalculateVelocityTargets( text_size.x );

	// Инициализируем позицию основного velocity при первом запуске
	if ( g_main_velocity.posX == 0.0f && g_main_velocity.targetX == 0.0f ) {
		g_main_velocity.posX = g_main_velocity.targetX;
	}

	// Анимируем позицию основного velocity
	g_main_velocity.posX = VelocityLerpFloat( g_main_velocity.posX, g_main_velocity.targetX, g_main_velocity.POSITION_SMOOTH, dt );

	float main_y = g_ctx.m_height * ( 1.f - GET_VARIABLE( g_variables.m_velocity_indicator_padding, int ) / ( 100.f ) );

	// Определяем финальный цвет для основного индикатора скорости
	ImColor final_main_color =
		GET_VARIABLE( g_variables.m_velocity_indicator_custom_color, bool )
			? ImColor( blended_color.Value.x, blended_color.Value.y, blended_color.Value.z, alpha )
			: ImColor( speed_based_color.Value.x, speed_based_color.Value.y, speed_based_color.Value.z, alpha );

	if ( GET_VARIABLE( g_variables.m_velocity_indicator_rgb, bool ) ) {
		const float speed = std::clamp( GET_VARIABLE( g_variables.m_velocity_indicator_rgb_speed, float ), 0.05f, 2.5f );
		const float hue = std::fmod( g_interfaces.m_global_vars_base->m_real_time * speed, 1.f );
		final_main_color = c_color::from_hsb( hue, 1.f, 1.f, 1.f ).get_u32( alpha );
	}

	// Отрисовка дропшадовс для основного индикатора скорости
	ImColor shadow_color = ImColor( 0.0f, 0.0f, 0.0f, final_main_color.Value.w * 0.6f );
	g_render.m_draw_data.emplace_back(
		e_draw_type::draw_type_text,
		std::make_any< text_draw_object_t >( g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_indicator ],
	                                         c_vector_2d( g_main_velocity.posX + 1.0f, main_y + 1.0f ), velocity_text, shadow_color ) );

	// Отрисовка основного индикатора скорости
	g_render.m_draw_data.emplace_back(
		e_draw_type::draw_type_text,
		std::make_any< text_draw_object_t >( g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_indicator ],
	                                         c_vector_2d( g_main_velocity.posX, main_y ), velocity_text, final_main_color ) );

	// Анимация и рендер take-off индикаторов
	if ( take_off_enabled ) {
		for ( auto it = g_velocity_indicators.begin( ); it != g_velocity_indicators.end( ); ) {
			auto& I = *it;

			// Плавное изменение прозрачности
			I.opacity = VelocityLerpFloat( I.opacity, I.targetOpacity, I.OPACITY_SMOOTH, dt );

			// Плавное изменение позиции
			I.posX = VelocityLerpFloat( I.posX, I.targetX, I.POSITION_SMOOTH, dt );

			// Плавное изменение цвета - используем тот же alpha что и основной velocity
			ImVec4 targetColor = final_main_color.Value;
			targetColor.w      = I.opacity * alpha;
			I.currentColor     = VelocityLerpColor( I.currentColor, targetColor, I.COLOR_SMOOTH, dt );

			// Рендер take-off индикатора с дропшадовс
			if ( I.opacity > 0.01f ) {
				ImColor take_off_color = ImColor( I.currentColor );

				// Дропшадовс для take-off индикатора
				ImColor take_off_shadow = ImColor( 0.0f, 0.0f, 0.0f, take_off_color.Value.w * 0.6f );
				g_render.m_draw_data.emplace_back(
					e_draw_type::draw_type_text,
					std::make_any< text_draw_object_t >( g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_indicator ],
					                                     c_vector_2d( I.posX + 1.0f, main_y + 1.0f ), I.label, take_off_shadow ) );

				// Основной текст take-off индикатора
				g_render.m_draw_data.emplace_back(
					e_draw_type::draw_type_text,
					std::make_any< text_draw_object_t >( g_render.m_custom_fonts[ e_custom_font_names::custom_font_name_indicator ],
					                                     c_vector_2d( I.posX, main_y ), I.label, take_off_color ) );
			}

			// Удаляем индикатор, когда он полностью прозрачный
			if ( I.removing && I.opacity < 0.01f ) {
				it = g_velocity_indicators.erase( it );
			} else {
				++it;
			}
		}
	} else {
		// Если функция отключена, очищаем все индикаторы
		if ( !g_velocity_indicators.empty( ) ) {
			g_velocity_indicators.clear( );
		}
	}

	// Обновляем данные о последней скорости
	if ( this->m_indicator_data.m_tick_prev_velocity + 5 < g_interfaces.m_global_vars_base->m_tick_count ) {
		this->m_indicator_data.m_last_velocity      = velocity;
		this->m_indicator_data.m_tick_prev_velocity = g_interfaces.m_global_vars_base->m_tick_count;
	}

	this->m_indicator_data.m_last_on_ground_velocity = on_ground;
}

void n_indicators::impl_t::stamina(const bool on_ground) {
    // TODO: Реализовать отображение stamina-индикатора
}
