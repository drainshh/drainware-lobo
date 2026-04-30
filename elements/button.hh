#pragma once
#include "../instalisation fonts/fonts2.h"
#include <unordered_map>
#include <cfloat>

// Функция линейной интерполяции для float
float lerp23( float a, float b, float t )
{
	return a + ( b - a ) * t;
}

// Функция для линейной интерполяции между двумя цветами
ImColor lerpColor23( const ImColor& a, const ImColor& b, float t )
{
	// Получаем нормированные компоненты цвета
	float ra = a.Value.x, ga = a.Value.y, ba = a.Value.z, aa = a.Value.w;
	float rb = b.Value.x, gb = b.Value.y, bb_val = b.Value.z, ab = b.Value.w;
	float r     = ra + ( rb - ra ) * t;
	float g     = ga + ( gb - ga ) * t;
	float b_col = ba + ( bb_val - ba ) * t;
	float a_col = aa + ( ab - aa ) * t;
	return ImColor( r, g, b_col, a_col );
}

bool button( const char* name, ImVec2 pos = ImVec2( 180, 0 ) )
{
	if ( ImGui::GetCurrentWindow( )->SkipItems )
		return false;

	// Вычисляем размер текста через шрифт
	const auto name_text_size = fonts_for_gui::regular->CalcTextSizeA( fonts_for_gui::regular->FontSize, FLT_MAX, NULL, name );

	// Задаём размеры кнопки: ширина – pos.x, а высота вычисляется через размер текста плюс вертикальные отступы
	const float width            = pos.x;
	const float vertical_padding = 8.0f; // отступ сверху и снизу
	const float height           = name_text_size.y + vertical_padding;
	const float rounding         = 4.0f;

	// Определяем прямоугольник кнопки
	const ImVec2 position = ImGui::GetCurrentWindow( )->DC.CursorPos;
	ImRect total_bb( position, position + ImVec2( width, height ) );

	// Регистрируем пространство для элемента
	ImGui::ItemSize( total_bb, height );
	if ( !ImGui::ItemAdd( total_bb, ImGui::GetID( name ) ) )
		return false;

	// Получаем состояние кнопки (hover, held, pressed)
	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior( total_bb, ImGui::GetID( name ), &hovered, &held );

	// Плавная анимация нажатия
	float dt = ImGui::GetIO( ).DeltaTime;
	// Статический контейнер для хранения состояния анимации по ID кнопки
	static std::unordered_map< ImGuiID, float > pressAnim;
	ImGuiID id = ImGui::GetID( name );
	if ( pressAnim.find( id ) == pressAnim.end( ) )
		pressAnim[ id ] = 0.0f;
	// Если кнопка удерживается, целевое значение равно 1.0, иначе – 0.0
	float target = held ? 1.0f : 0.0f;
	// Обновляем анимационное значение с использованием линейной интерполяции
	pressAnim[ id ] = lerp23( pressAnim[ id ], target, dt * 10.0f );

	// Определяем базовый цвет фона кнопки и акцентный цвет (используя m_accent)
	ImColor baseColor( 25, 25, 25, int( 255 * g_menu.m_animation_progress ) );
	ImColor accentColor( GET_VARIABLE( g_variables.m_accent, c_color ).get_u32( float( 1.f * g_menu.m_animation_progress ) ) );
	// Итоговый цвет кнопки получается линейной интерполяцией между базовым и акцентным
	ImColor finalColor = lerpColor23( baseColor, accentColor, pressAnim[ id ] );

	/*
	 * Отрисовка кнопки
	 */
	ImGui::PushClipRect( total_bb.Min, total_bb.Max, false );
	// Отрисовка фона кнопки с плавным изменением цвета
	ImGui::GetWindowDrawList( )->AddRectFilled( total_bb.Min, total_bb.Max, finalColor, rounding, ImDrawFlags_RoundCornersAll );

	// Вычисляем положение текста, центрируя его по горизонтали и вертикали
	const float text_x = total_bb.Min.x + ( width - name_text_size.x ) * 0.5f;
	const float text_y = total_bb.Min.y + ( height - name_text_size.y ) * 0.5f;
	ImGui::GetWindowDrawList( )->AddText( fonts_for_gui::regular, fonts_for_gui::regular->FontSize, ImVec2( text_x, text_y ),
	                                      ImColor( 255, 255, 255, int( 255 * g_menu.m_animation_progress ) ), name );
	ImGui::PopClipRect( );

	return pressed;
}