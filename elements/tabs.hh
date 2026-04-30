#pragma once
#include <vector>

#include "../instalisation fonts/fonts2.h"

struct tab_t {
	std::string name;
    std::string icon;
	std::vector<std::string> sub_tabs;
};
inline int tab_number    = 0;
inline int subtab_number = 0;
float lerp( float a, float b, float t )
{
	return a + ( b - a ) * t;
}

// Линейная интерполяция для ImColor (аналогично можно добавить отдельную функцию для ImU32)
// Здесь выполняется побитовая разбивка на компоненты
ImColor lerpColor( const ImColor& a, const ImColor& b, float t )
{
	int r     = int( lerp( a.Value.x * 255.0f, b.Value.x * 255.0f, t ) );
	int g     = int( lerp( a.Value.y * 255.0f, b.Value.y * 255.0f, t ) );
	int b_col = int( lerp( a.Value.z * 255.0f, b.Value.z * 255.0f, t ) );
	int a_col = int( lerp( a.Value.w * 255.0f, b.Value.w * 255.0f, t ) );
	return ImColor( r, g, b_col, a_col );
}

#include <unordered_map>



void custom_tabs( std::vector< tab_t > tabs )
{
	float y_offset_adjustment = 0.0f;

	const static float spacing[ 2 ]{ 11.0f, 125.0f }; // LEFT (0) – UP (1)
	const static float size[ 2 ]{ 141.0f, 25.0f };    // WIDTH (0) – HEIGHT (1)
	const static float text_spacing{ 25.0f };
	const static float rounding{ 4.0f };

	// Для анимации раскрытия сабтабов – ключ: индекс основного таба, значение: [0..1]
	static std::unordered_map< size_t, float > subTabsAnimProgress;
	static std::unordered_map< size_t, float > tabHoverProgress;
	static std::unordered_map< size_t, float > subTabHoverProgress;
	float dt = ImGui::GetIO( ).DeltaTime;
	// Множитель сглаживания – настраивается экспериментально
	float smoothing_factor = dt * 10.0f;

	for ( size_t i = 0; i < tabs.size( ); ++i ) {
		auto windowPos = ImGui::GetWindowPos( );

		float x_offset     = windowPos.x + spacing[ 0 ] + text_spacing;
		float y_offset     = windowPos.y + spacing[ 1 ] + ( i * size[ 1 ] ) + 5 + y_offset_adjustment;
		auto text_position = ImVec2( x_offset, y_offset );

		auto tab_min = ImVec2( windowPos.x + spacing[ 0 ], windowPos.y + spacing[ 1 ] + i * size[ 1 ] + y_offset_adjustment );
		auto tab_max = ImVec2( windowPos.x + size[ 0 ], windowPos.y + spacing[ 1 ] + ( i + 1 ) * size[ 1 ] + y_offset_adjustment );

		bool hovered  = ImGui::IsMouseHoveringRect( tab_min, tab_max );
		bool selected = ( i == tab_number );
		if ( selected && subtab_number >= static_cast< int >( tabs[ i ].sub_tabs.size( ) ) )
			subtab_number = 0;
		tabHoverProgress[ i ] = ImClamp( tabHoverProgress[ i ] + ( hovered ? 1.f : -1.f ) * dt * 9.f, 0.f, 1.f );
		if ( tabHoverProgress[ i ] > 0.01f && !selected ) {
			const float alpha = tabHoverProgress[ i ] * 0.08f * g_menu.m_animation_progress;
			ImGui::GetWindowDrawList( )->AddRectFilled( tab_min, tab_max,
			                                            ImColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( alpha ) ), rounding,
			                                            ImDrawFlags_RoundCornersAll );
		}

		if ( selected ) {
			ImGui::GetWindowDrawList( )->AddRectFilled( tab_min, tab_max, ImColor( 25, 25, 25, int( 255 * g_menu.m_animation_progress ) ), rounding,
			                                            ImDrawFlags_RoundCornersTop );
		}

		// Отрисовка названия и иконки основного таба
		ImGui::GetWindowDrawList( )->AddText( fonts_for_gui::regular, fonts_for_gui::regular->FontSize, text_position,
		                                      ImColor( 255, 255, 255, int( 255 * g_menu.m_animation_progress ) ), tabs[ i ].name.c_str( ) );
		ImGui::GetWindowDrawList( )->AddText(
			fonts_for_gui::icons, fonts_for_gui::icons->FontSize, ImVec2( text_position.x - 20.0f, text_position.y ),
			ImColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( int( 255 * g_menu.m_animation_progress ) ) ), tabs[ i ].icon.c_str( ) );

		if ( hovered && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
			tab_number    = i;
			subtab_number = 0;
		}

		// Отрисовка сабтабов только если таковые имеются
		// Выполняется отрисовка только если таб выбран или анимация раскрытия ещё не завершена
		if ( !tabs[ i ].sub_tabs.empty( ) && ( selected || subTabsAnimProgress[ i ] > 0.001f ) ) {
			// Целевое значение анимации: если выбран – раскрываем (1.0f), иначе – сворачиваем (0.0f)
			float targetAnim = selected ? 1.0f : 0.0f;
			if ( subTabsAnimProgress.find( i ) == subTabsAnimProgress.end( ) ) {
				subTabsAnimProgress[ i ] = targetAnim;
			} else {
				// Плавно приближаем текущее значение к целевому
				subTabsAnimProgress[ i ] = lerp( subTabsAnimProgress[ i ], targetAnim, smoothing_factor );
			}
			// Вычисляем высоту группы сабтабов с учётом анимации
			float groupHeight = tabs[ i ].sub_tabs.size( ) * size[ 1 ] * subTabsAnimProgress[ i ];

			auto group_min = ImVec2( windowPos.x + spacing[ 0 ], windowPos.y + spacing[ 1 ] + ( i + 1 ) * size[ 1 ] + y_offset_adjustment );
			auto group_max =
				ImVec2( windowPos.x + size[ 0 ], windowPos.y + spacing[ 1 ] + ( i + 1 ) * size[ 1 ] + y_offset_adjustment + groupHeight );

			// Фон группы сабтабов
			ImGui::GetWindowDrawList( )->AddRectFilled( group_min, group_max, ImColor( 10, 10, 10, int( 255 * g_menu.m_animation_progress ) ),
			                                            rounding, ImDrawFlags_RoundCornersBottom );

			// Рендеринг каждого сабтаба
			for ( size_t j = 0; j < tabs[ i ].sub_tabs.size( ); ++j ) {
				auto sub_tab_name      = tabs[ i ].sub_tabs[ j ];
				auto sub_tab_min       = ImVec2( group_min.x, group_min.y + j * size[ 1 ] );
				auto sub_tab_max       = ImVec2( group_max.x, group_min.y + ( j + 1 ) * size[ 1 ] );
				auto sub_text_position = ImVec2( x_offset, group_min.y + j * size[ 1 ] + 5 );

				bool sub_hovered  = ImGui::IsMouseHoveringRect( sub_tab_min, sub_tab_max );
				bool sub_selected = ( subtab_number == j );
				const size_t sub_key = ( i << 8U ) | j;
				subTabHoverProgress[ sub_key ] = ImClamp( subTabHoverProgress[ sub_key ] + ( sub_hovered ? 1.f : -1.f ) * dt * 10.f, 0.f, 1.f );
				if ( selected && subTabHoverProgress[ sub_key ] > 0.01f && !sub_selected )
					ImGui::GetWindowDrawList( )->AddRectFilled( sub_tab_min, sub_tab_max,
					                                            ImColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32(
						                                            subTabHoverProgress[ sub_key ] * 0.06f * g_menu.m_animation_progress ) ),
					                                            2.0f );
				if ( selected )
				ImGui::GetWindowDrawList( )->AddText(
					fonts_for_gui::regular, fonts_for_gui::regular->FontSize, sub_text_position,
					sub_selected ? ImColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( int( 255 * g_menu.m_animation_progress ) ) )
								 : ImColor( 255, 255, 255, int( 255 * g_menu.m_animation_progress ) ),
					sub_tab_name.c_str( ) );

				if ( sub_hovered && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) )
					subtab_number = j;
			}

			// Корректировка вертикального смещения для последующих табов
			y_offset_adjustment += tabs[ i ].sub_tabs.size( ) * size[ 1 ] * subTabsAnimProgress[ i ];
		}
	}
}
