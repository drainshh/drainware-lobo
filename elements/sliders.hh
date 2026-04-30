#pragma once
#include "../instalisation fonts/fonts2.h"
#include <unordered_map> // Не забудьте подключить для работы unordered_map

#include <unordered_map> // Для использования unordered_map

bool draw_slider( const char* name, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format,
                  float width = 180.f )
{
	if ( ImGui::GetCurrentWindow( )->SkipItems )
		return false;

	ImGuiContext& g = *GImGui;

	char value_buf[ 64 ];
	const char* value_buf_end = value_buf + ImGui::DataTypeFormatString( value_buf, IM_ARRAYSIZE( value_buf ), data_type, p_data, format );

	const static float height{ 23.0f };
	const static float size{ 2.0f };
	const static float slide_size{ height - size };
	const static float rounding{ 0.0f };
	const static auto name_text_size = ImGui::CalcTextSize( name, NULL, true );
	const auto value_text_size       = ImGui::CalcTextSize( value_buf, value_buf_end, true );

	const ImRect frame_bb( ImGui::GetCurrentWindow( )->DC.CursorPos, ImGui::GetCurrentWindow( )->DC.CursorPos + ImVec2( width, height ) );
	const ImRect total_bb( ImGui::GetCurrentWindow( )->DC.CursorPos + ImVec2( 0.0f, slide_size ),
	                       ImGui::GetCurrentWindow( )->DC.CursorPos + ImVec2( width, height ) );

	ImGui::ItemSize( frame_bb, height );

	if ( !ImGui::ItemAdd( frame_bb, ImGui::GetID( name ), &total_bb ) )
		return false;

	const bool hovered = ImGui::ItemHoverable( frame_bb, ImGui::GetID( name ) );
	const bool clicked = hovered && ImGui::IsMouseClicked( ImGuiMouseButton_::ImGuiMouseButton_Left, false );

	if ( clicked ) {
		ImGui::SetActiveID( ImGui::GetID( name ), ImGui::GetCurrentWindow( ) );
		ImGui::SetFocusID( ImGui::GetID( name ), ImGui::GetCurrentWindow( ) );
		ImGui::FocusWindow( ImGui::GetCurrentWindow( ) );
		g.ActiveIdUsingNavDirMask |= ( 1 << ImGuiDir_Left ) | ( 1 << ImGuiDir_Right );
	}
	int percentage = 0;
	ImRect grab_bb;
	const bool value_changed =
		ImGui::SliderBehavior( total_bb, ImGui::GetID( name ), data_type, p_data, p_min, p_max, format,
	                           ImGuiSliderFlags_::ImGuiSliderFlags_NoInput | ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp, &grab_bb, &percentage );

	if ( value_changed )
		ImGui::MarkItemEdited( ImGui::GetID( name ) );

	// Определяем, что значение больше минимума
	int delta_values = 0;
	switch ( data_type ) {
	case ImGuiDataType_Float:
		delta_values = +*static_cast< float* >( p_data ) > *static_cast< const float* >( p_min );
		break;
	case ImGuiDataType_S32:
		delta_values = +*static_cast< int* >( p_data ) > *static_cast< const int* >( p_min );
		break;
	case ImGuiDataType_Double:
		delta_values = +*static_cast< double* >( p_data ) > *static_cast< const double* >( p_min );
		break;
	default:
		delta_values = +*static_cast< int* >( p_data ) > *static_cast< const int* >( p_min );
		break;
	}

	/*
	 * Интерполяция движения ползунка с учётом смещения меню
	 *
	 * Для этого вводим дополнительную статическую карту, сохраняющую предыдущий
	 * "origin" (точку начала прямоугольника слайдера) для каждого слайдера.
	 */
	static std::unordered_map< ImGuiID, ImVec2 > s_prevGrabPos;
	static std::unordered_map< ImGuiID, ImVec2 > s_lastSliderOrigin;

	ImGuiID sliderID     = ImGui::GetID( name );
	ImVec2 currentOrigin = total_bb.Min;

	// Если меню (или сам слайдер) сдвинулось, сбрасываем интерполяцию
	if ( s_lastSliderOrigin.find( sliderID ) == s_lastSliderOrigin.end( ) || s_lastSliderOrigin[ sliderID ].x != currentOrigin.x ||
	     s_lastSliderOrigin[ sliderID ].y != currentOrigin.y ) {
		s_prevGrabPos[ sliderID ]      = grab_bb.Min;
		s_lastSliderOrigin[ sliderID ] = currentOrigin;
	}

	ImVec2 targetGrabPos = grab_bb.Min;
	ImVec2 smoothGrabPos = targetGrabPos; // инициализация

	auto it = s_prevGrabPos.find( sliderID );
	if ( it != s_prevGrabPos.end( ) ) {
		// Коэффициент сглаживания; регулируется временем между кадрами и множителем
		float smoothing_factor = ImClamp( ImGui::GetIO( ).DeltaTime * 12.0f, 0.0f, 1.0f );
		smoothGrabPos.x        = it->second.x + ( targetGrabPos.x - it->second.x ) * smoothing_factor;
		smoothGrabPos.y        = it->second.y + ( targetGrabPos.y - it->second.y ) * smoothing_factor;
	}
	s_prevGrabPos[ sliderID ] = smoothGrabPos;

	// Вычисляем размеры для отрисовки интерполированного граба
	ImVec2 grab_size = grab_bb.GetSize( );
	ImRect smoothGrabRect( smoothGrabPos, smoothGrabPos + grab_size );

	/*
	 * Отрисовка элементов слайдера
	 */

	// Фон слайдера
	ImGui::GetWindowDrawList( )->AddRectFilled( total_bb.Min, total_bb.Max, ImColor( 25, 25, 25, int( 255 * g_menu.m_animation_progress ) ), rounding,
	                                            ImDrawFlags_::ImDrawFlags_RoundCornersAll );

	// Название
	ImGui::GetWindowDrawList( )->AddText( fonts_for_gui::regular, fonts_for_gui::regular->FontSize, frame_bb.Min,
	                                      ImColor( 255, 255, 255, int( 255 * g_menu.m_animation_progress ) ), name );

	// Значение
	ImGui::GetWindowDrawList( )->AddText( fonts_for_gui::regular, fonts_for_gui::regular->FontSize,
	                                      frame_bb.Min + ImVec2( width - value_text_size.x, 0.0f ),
	                                      ImColor( 255, 255, 255, int( 255 * g_menu.m_animation_progress ) ), value_buf, value_buf_end );

	// Отрисовка ползунка (grabber) с применением интерполированной позиции
	 {
		ImGui::GetWindowDrawList( )->AddRectFilled(
			total_bb.Min, smoothGrabRect.Max + ImVec2( 2, 2 ),
			ImColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( float( 1.f * g_menu.m_animation_progress ) ) ), rounding,
			ImDrawFlags_::ImDrawFlags_RoundCornersAll );
		ImGui::GetWindowDrawList( )->AddRectFilled(
			ImVec2( smoothGrabRect.Min.x - 2.0f, smoothGrabRect.Min.y - 4.0f ), smoothGrabRect.Max + ImVec2( 2.0f, 4.0f ),
			ImColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( float( 1.f * g_menu.m_animation_progress ) ) ), 2.0f,
			ImDrawFlags_::ImDrawFlags_RoundCornersAll );
	}

	return value_changed;
}


bool slider_float(const char* label, float* v, float v_min, float v_max, const char* format, float width = 180.f)
{
	return draw_slider( label, ImGuiDataType_Float, v, &v_min, &v_max, format, width  );
}

bool slider_int( const char* label, int* v, int v_min, int v_max, const char* format, float width = 180.f )
{
    return draw_slider(label, ImGuiDataType_S32, v, &v_min, &v_max, format,width);
}

bool slider_double( const char* label, double* v, double v_min, double v_max, const char* format, float width = 180.f )
{
	return draw_slider( label, ImGuiDataType_Double, v, &v_min, &v_max, format,  width );
}
