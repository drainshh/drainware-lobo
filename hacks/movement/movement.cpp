#include "movement.h"
#include "../../game/sdk/includes/includes.h"
#include "../../globals/includes/includes.h"
#include "../prediction/prediction.h"

bool AlertJB                      = false;
bool alert_mini_crouch_hop        = false;
bool bounce_alert_mini_crouch_hop = false;
bool AlertJB2                     = false;
int b_buttons                     = 0;
void n_movement::impl_t::on_create_move_pre( )
{
	if ( GET_VARIABLE( g_variables.m_no_crouch_cooldown, bool ) )
		g_ctx.m_cmd->m_buttons |= e_command_buttons::in_bullrush;
	this->pixel_finder( g_ctx.m_cmd );
	const auto move_type = g_ctx.m_local->get_move_type( );
	if ( move_type == e_move_types::move_type_ladder || move_type == e_move_types::move_type_noclip || move_type == e_move_types::move_type_fly ||
	     move_type == e_move_types::move_type_observer )
		return;

	if ( GET_VARIABLE( g_variables.m_bunny_hop, bool ) &&
	     !( GET_VARIABLE( g_variables.m_jump_bug, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.m_jump_bug_key, key_bind_t ) ) ) )
		this->bunny_hop( );

	if ( g_ctx.m_local ) {
		if ( g_ctx.m_local->get_health( ) != 0 && g_ctx.m_local->is_alive( ) ) {
			b_buttons = g_ctx.m_cmd->m_buttons;

			g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
			this->pixelsurf_assist( g_ctx.m_cmd );
			g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
			this->autobounce_assist( g_ctx.m_cmd );
			g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
			static bool off_nahui = false;
			if ( g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_surf_assist_key, key_bind_t ) ) ) {
				if ( alert_mini_crouch_hop ) {
					if ( off_nahui ) {
						if ( g_prediction.backup_data.m_velocity.m_z == -6.25f ) {
							off_nahui             = false;
							alert_mini_crouch_hop = false;
						}
						if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground ) {
							off_nahui             = false;
							alert_mini_crouch_hop = false;
						}
					}
					if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground ) {
						g_ctx.m_cmd->m_buttons &= ~e_command_buttons::in_duck;
						g_ctx.m_cmd->m_buttons |= in_jump;
						off_nahui = true;
					} else {
						g_ctx.m_cmd->m_buttons |= e_command_buttons::in_duck;
					}
				}

			} else {
				alert_mini_crouch_hop = false;
				off_nahui             = false;
			}
			static bool off_nahui2 = false;
			if ( g_input.check_input( &GET_VARIABLE( g_variables.m_bounce_assist_key, key_bind_t ) ) ) {
				if ( bounce_alert_mini_crouch_hop ) {
					if ( off_nahui2 ) {
						if ( g_prediction.backup_data.m_velocity.m_z == -6.25f ) {
							off_nahui2                   = false;
							bounce_alert_mini_crouch_hop = false;
						}
						if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground ) {
							off_nahui2                   = false;
							bounce_alert_mini_crouch_hop = false;
						}
					}
					if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground ) {
						g_ctx.m_cmd->m_buttons &= ~e_command_buttons::in_duck;
						g_ctx.m_cmd->m_buttons |= in_jump;
						off_nahui2 = true;
					} else {
						g_ctx.m_cmd->m_buttons |= e_command_buttons::in_duck;
					}
				}
			} else {
				bounce_alert_mini_crouch_hop = false;
				off_nahui2                   = false;
			}
			if ( AlertJB || AlertJB2 )
				g_ctx.m_cmd->m_buttons |= in_jump;

			if ( m_pixelsurf_data.should_pixel_surf )
				g_ctx.m_cmd->m_buttons |= in_duck;

			if ( GET_VARIABLE( g_variables.m_auto_align, bool ) && !( g_prediction.backup_data.m_flags & 1 ) )
				this->auto_align( g_ctx.m_cmd );
			g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );

			this->air_stuck( g_ctx.m_cmd );
			m_pixelsurf_assist_t.set_point = false;

			if ( g_ctx.m_local->get_flags( ) & 1 )
				m_pixelsurf_data.should_pixel_surf = false;
		}
	}
}

void n_movement::impl_t::bunny_hop( )
{
	if ( !( g_ctx.m_local->get_flags( ) & e_flags::fl_onground ) )
		g_ctx.m_cmd->m_buttons &= ~e_command_buttons::in_jump;
}

bool can = false;

float LerpFloat( float a, float b, float t )
{
	return a + ( b - a ) * t;
}
struct points_check_t {
	c_vector pos;
	std::string map;
	float currentScale     = 0.f;
	bool open_settings     = false;
	bool jump              = true;
	bool minijump          = true;
	bool longjump          = true;
	bool jumpbug           = true;
	bool crouch_hop        = true;
	bool mini_crouch_hop   = true;
	bool c_jump            = true;
	bool c_minijump        = true;
	bool c_longjump        = true;
	bool c_jumpbug         = true;
	bool c_crouch_hop      = true;
	bool c_mini_crouch_hop = true;
	bool active            = true;
	float radius           = 300.f; // Предполагается, что уже добавлено
	float delta_strafe     = 0.5f;  // Новое поле для индивидуального delta_strafe
	points_check_t( const c_vector& Pos = { }, const std::string& Map = "" ) : pos( Pos ), map( Map ) { }
};
std::vector< points_check_t > m_bounce_points_check{ };
std::vector< points_check_t > m_points_check{ };

bool point_menu_is_opened( )
{
	for ( size_t i = 0; i < m_bounce_points_check.size( ); ++i ) {
		points_check_t& point = m_bounce_points_check[ i ];
		if ( point.open_settings )
			return true;
	}
	for ( size_t i = 0; i < m_points_check.size( ); ++i ) {
		points_check_t& point = m_points_check[ i ];
		if ( point.open_settings )
			return true;
	}
	return false;
}
ImColor LerpColor( const ImColor& a, const ImColor& b, float t )
{
	t      = std::clamp( t, 0.0f, 1.0f );
	int r  = static_cast< int >( a.Value.x + ( b.Value.x - a.Value.x ) * t );
	int g  = static_cast< int >( a.Value.y + ( b.Value.y - a.Value.y ) * t );
	int b_ = static_cast< int >( a.Value.z + ( b.Value.z - a.Value.z ) * t );
	int a_ = static_cast< int >( a.Value.w + ( b.Value.w - a.Value.w ) * t );
	return ImColor( r, g, b_, a_ );
}
bool InCrosshair( float x, float y, float radius )
{
	float centerX = g_ctx.m_width / 2;
	float centerY = g_ctx.m_height / 2;

	// Проверяем расстояние от центра экрана до центра круга
	float dx = centerX - x;
	float dy = centerY - y;

	// Если расстояние меньше или равно радиусу, значит цель в прицеле
	return ( dx * dx + dy * dy ) <= ( radius * radius );
}

bool detect_opened_points  = false;
int gloabal_settings_timer = 0;
#include "../../instalisation fonts/fonts2.h"
#include <conio.h> // Для использования _kbhit() и _getch()
extern void checkbox(
	const char* name, bool* variable, bool disabled = false, bool popup = false, const ImVec2& popup_size = ImVec2( ),
	const std::function< void( ) >& popup_function = [] { } );

bool isEnterPressed( )
{
	static bool keyWasDown = false;
	// Если клавиша VK_RETURN нажата (старший бит равен 1)
	if ( GetAsyncKeyState( VK_RETURN ) & 0x8000 ) {
		keyWasDown = true;
	}
	// Если клавиша была нажата, а теперь отпущена, фиксируем событие
	else if ( keyWasDown ) {
		keyWasDown = false;
		return true;
	}
	return false;
}
void RenderPoints( std::vector< points_check_t >& points, const c_vector& playerPos, const std::string& currentMap )
{
	float dt                       = ImGui::GetIO( ).DeltaTime;
	const float scaleSpeed         = 10.0f;
	static bool isBackspacePressed = false;

	// Обработка нажатия клавиши Backspace
	if ( GetAsyncKeyState( VK_BACK ) & 0x8000 ) {
		isBackspacePressed = true;
	} else {
		isBackspacePressed = false;
	}

	auto accentclr = ImGui::GetStyle( ).Colors[ ImGuiCol_::ImGuiCol_Accent ];

	// Рендеринг точек для pixel surf assist
	if ( GET_VARIABLE( g_variables.m_pixel_surf_assist, bool ) ) {
		for ( size_t i = 0; i < points.size( ); ++i ) {
			points_check_t& point = points[ i ];

			if ( point.map != currentMap )
				continue;

			c_vector_2d screenPos;
			if ( !g_render.world_to_screen( point.pos, screenPos ) )
				continue;

			float distance = playerPos.dist_to( point.pos );
			if ( distance > 1500.f )
				continue;

			float baseRadius = 11.0f;
			if ( distance > 300.0f ) {
				float t    = std::clamp( ( distance - 300.0f ) / 300.0f, 0.0f, 1.0f );
				baseRadius = LerpFloat( 11.0f, 4.0f, t );
			}
			baseRadius = std::max( baseRadius, 3.0f );

			float outlineThickness = 0.5f;

			// Альфа исчезает только в последние 100 юнитов (от 1400 до 1500)
			float alphaMultiplier = 1.0f;
			if ( distance > 1400.0f ) {
				alphaMultiplier = std::clamp( ( 1500.0f - distance ) / 100.0f, 0.0f, 1.0f );
			}

			// Цвет всегда свой, не меняется от расстояния
			unsigned int outlineColor;
			if ( point.active ) {
				int outlineR = static_cast< int >( accentclr.x * 255.f );
				int outlineG = static_cast< int >( accentclr.y * 255.f );
				int outlineB = static_cast< int >( accentclr.z * 255.f );
				int outlineA = static_cast< int >( 255.0f * alphaMultiplier );
				outlineColor = c_color( outlineR, outlineG, outlineB, outlineA ).get_u32( );
			} else {
				int gray     = 100;
				int outlineA = static_cast< int >( 255.0f * alphaMultiplier );
				outlineColor = c_color( gray, gray, gray, outlineA ).get_u32( );
			}

			// Цвет заливки
			int fillAlpha          = static_cast< int >( 100.0f * alphaMultiplier );
			unsigned int fillColor = c_color( 0, 0, 0, fillAlpha ).get_u32( );

			bool isHovered        = InCrosshair( screenPos.m_x, screenPos.m_y, baseRadius );
			float targetScale     = isHovered ? 1.5f : 1.0f;
			point.currentScale    = LerpFloat( point.currentScale, targetScale, dt * scaleSpeed );
			float effectiveRadius = baseRadius * point.currentScale;

			if ( isHovered ) {
				if ( isEnterPressed( ) ) {
					gloabal_settings_timer = GetTickCount64( ) + 500;
					point.open_settings    = true;
				}
			} else {
				point.open_settings = false;
			}

			filled_circle_draw_object_t filledCircle{ screenPos, effectiveRadius, fillColor, 32 };
			g_render.m_draw_data.emplace_back( e_draw_type::draw_type_filled_circle, std::make_any< filled_circle_draw_object_t >( filledCircle ) );

			// Основная обводка (3/4 круга) - рисуем линиями
			unsigned int grayColor = c_color( 128, 128, 128, 255 ).get_u32( );
			const int segments     = 32;
			const float angleStep  = 2.0f * 3.14159f / segments;

			// Рисуем основную обводку (3/4 круга)
			for ( int j = 0; j < segments; ++j ) {
				float angle1 = j * angleStep;
				float angle2 = ( j + 1 ) * angleStep;

				// Для pixel surf - серая обводка сверху (1/4 круга)
				// Центр: 270° (или 4.71 радиан), по 45° в каждую сторону
				// От 225° до 315° (или от 3.93 до 5.50 радиан)
				bool isGraySegment = ( angle1 >= 3.93f && angle1 <= 5.50f ) || ( angle2 >= 3.93f && angle2 <= 5.50f );

				c_vector_2d pos1, pos2;
				pos1.m_x = screenPos.m_x + cos( angle1 ) * effectiveRadius;
				pos1.m_y = screenPos.m_y + sin( angle1 ) * effectiveRadius;
				pos2.m_x = screenPos.m_x + cos( angle2 ) * effectiveRadius;
				pos2.m_y = screenPos.m_y + sin( angle2 ) * effectiveRadius;

				unsigned int segmentColor = isGraySegment ? grayColor : outlineColor;

				line_draw_object_t outlineLine{ pos1, pos2, segmentColor, outlineThickness };
				g_render.m_draw_data.emplace_back( e_draw_type::draw_type_line, std::make_any< line_draw_object_t >( outlineLine ) );
			}

			float animSpeed     = 2.0f;
			float anim          = ( std::sin( ImGui::GetTime( ) * animSpeed ) + 1.0f ) / 2.0f;
			float maxCrossAlpha = 200.0f;
			if ( distance > 300.0f ) {
				float fade    = std::clamp( ( distance - 300.0f ) / 300.0f, 0.0f, 1.0f );
				maxCrossAlpha = 200.0f * ( 1.0f - fade );
			}
			int crossAlpha = static_cast< int >( maxCrossAlpha * anim );

			// Расчет цвета креста в зависимости от активности точки
			unsigned int crossColor;
			if ( point.active ) {
				crossColor = c_color( int( accentclr.x * 255.f ), int( accentclr.y * 255.f ), int( accentclr.z * 255.f ), crossAlpha ).get_u32( );
			} else {
				int gray   = 150; // Светло-серый для креста неактивных точек
				crossColor = c_color( gray, gray, gray, crossAlpha ).get_u32( );
			}

			float baseLineLength      = 4.0f;
			float effectiveLineLength = baseLineLength * point.currentScale;

			auto screenDistance = []( const c_vector_2d& a, const c_vector_2d& b ) -> float {
				float dx = a.m_x - b.m_x;
				float dy = a.m_y - b.m_y;
				return std::sqrt( dx * dx + dy * dy );
			};

			auto rotateVector = [ & ]( const c_vector& v ) -> c_vector {
				float x1 = v.m_x * cos( 0.f ) + v.m_z * sin( 0.f );
				float y1 = v.m_y;
				float z1 = -v.m_x * sin( 0.f ) + v.m_z * cos( 0.f );
				float x2 = x1;
				float y2 = y1 * cos( 0.f ) - z1 * sin( 0.f );
				float z2 = y1 * sin( 0.f ) + z1 * cos( 0.f );
				return c_vector( x2, y2, z2 );
			};

			float maxProjectedOffset = 0.0f;
			auto getScreenOffset     = [ & ]( const c_vector& offsetVec ) -> float {
                c_vector_2d projected;
                if ( g_render.world_to_screen( point.pos + offsetVec, projected ) )
                    return screenDistance( screenPos, projected );
                return 0.0f;
			};

			{
				c_vector off       = rotateVector( c_vector( effectiveLineLength, 0, 0 ) );
				maxProjectedOffset = std::max( maxProjectedOffset, getScreenOffset( off ) );
			}
			{
				c_vector off       = rotateVector( c_vector( 0, effectiveLineLength, 0 ) );
				maxProjectedOffset = std::max( maxProjectedOffset, getScreenOffset( off ) );
			}
			{
				c_vector off       = rotateVector( c_vector( 0, 0, effectiveLineLength ) );
				maxProjectedOffset = std::max( maxProjectedOffset, getScreenOffset( off ) );
			}

			if ( maxProjectedOffset > effectiveRadius && effectiveRadius > 0.0f ) {
				float scaleFactor = effectiveRadius / maxProjectedOffset;
				effectiveLineLength *= scaleFactor;
			}

			c_vector offsetX = rotateVector( c_vector( effectiveLineLength, 0, 0 ) );
			c_vector offsetY = rotateVector( c_vector( 0, effectiveLineLength, 0 ) );
			c_vector offsetZ = rotateVector( c_vector( 0, 0, effectiveLineLength ) );

			c_vector center = point.pos;
			c_vector xPos   = center + offsetX;
			c_vector xNeg   = center - offsetX;
			c_vector yPos   = center + offsetY;
			c_vector yNeg   = center - offsetY;
			c_vector zPos   = center + offsetZ;
			c_vector zNeg   = center - offsetZ;

			c_vector_2d screenX1, screenX2, screenY1, screenY2, screenZ1, screenZ2;
			bool validX1 = g_render.world_to_screen( xPos, screenX1 );
			bool validX2 = g_render.world_to_screen( xNeg, screenX2 );
			bool validY1 = g_render.world_to_screen( yPos, screenY1 );
			bool validY2 = g_render.world_to_screen( yNeg, screenY2 );
			bool validZ1 = g_render.world_to_screen( zPos, screenZ1 );
			bool validZ2 = g_render.world_to_screen( zNeg, screenZ2 );

			if ( validX1 && validX2 ) {
				line_draw_object_t lineX{ screenX1, screenX2, crossColor, 1.0f };
				g_render.m_draw_data.emplace_back( e_draw_type::draw_type_line, std::make_any< line_draw_object_t >( lineX ) );
			}
			if ( validY1 && validY2 ) {
				line_draw_object_t lineY{ screenY1, screenY2, crossColor, 1.0f };
				g_render.m_draw_data.emplace_back( e_draw_type::draw_type_line, std::make_any< line_draw_object_t >( lineY ) );
			}
			if ( validZ1 && validZ2 ) {
				line_draw_object_t lineZ{ screenZ1, screenZ2, crossColor, 1.0f };
				g_render.m_draw_data.emplace_back( e_draw_type::draw_type_line, std::make_any< line_draw_object_t >( lineZ ) );
			}

			if ( isHovered && isBackspacePressed ) {
				points.erase( points.begin( ) + i );
				break;
			}
		}
	}

	// Рендеринг точек для bounce assist
	if ( GET_VARIABLE( g_variables.m_bouncee_assist, bool ) ) {
		for ( size_t i = 0; i < m_bounce_points_check.size( ); ++i ) {
			points_check_t& point = m_bounce_points_check[ i ];

			if ( point.map != currentMap )
				continue;

			c_vector_2d screenPos;
			if ( !g_render.world_to_screen( point.pos, screenPos ) )
				continue;

			float distance = playerPos.dist_to( point.pos );
			if ( distance > 1500.f )
				continue;

			float baseRadius = 11.0f;
			if ( distance > 300.0f ) {
				float t    = std::clamp( ( distance - 300.0f ) / 300.0f, 0.0f, 1.0f );
				baseRadius = LerpFloat( 11.0f, 4.0f, t );
			}
			baseRadius = std::max( baseRadius, 5.0f );

			float outlineThickness = 0.5f;

			// Альфа исчезает только в последние 100 юнитов (от 1400 до 1500)
			float alphaMultiplier = 1.0f;
			if ( distance > 1400.0f ) {
				alphaMultiplier = std::clamp( ( 1500.0f - distance ) / 100.0f, 0.0f, 1.0f );
			}

			// Цвет всегда свой, не меняется от расстояния
			unsigned int outlineColor;
			if ( point.active ) {
				int outlineR = static_cast< int >( accentclr.x * 255.f );
				int outlineG = static_cast< int >( accentclr.y * 255.f );
				int outlineB = static_cast< int >( accentclr.z * 255.f );
				int outlineA = static_cast< int >( 255.0f * alphaMultiplier );
				outlineColor = c_color( outlineR, outlineG, outlineB, outlineA ).get_u32( );
			} else {
				int gray     = 100;
				int outlineA = static_cast< int >( 255.0f * alphaMultiplier );
				outlineColor = c_color( gray, gray, gray, outlineA ).get_u32( );
			}

			// Цвет заливки
			int fillAlpha          = static_cast< int >( 100.0f * alphaMultiplier );
			unsigned int fillColor = c_color( 0, 0, 0, fillAlpha ).get_u32( );

			bool isHovered = InCrosshair( screenPos.m_x, screenPos.m_y, baseRadius );
			if ( isHovered ) {
				if ( isEnterPressed( ) ) {
					gloabal_settings_timer = GetTickCount64( ) + 500;
					point.open_settings    = true;
				}
			} else {
				point.open_settings = false;
			}

			float targetScale     = isHovered ? 1.5f : 1.0f;
			point.currentScale    = LerpFloat( point.currentScale, targetScale, dt * scaleSpeed );
			float effectiveRadius = baseRadius * point.currentScale;

			filled_circle_draw_object_t filledCircle{ screenPos, effectiveRadius, fillColor, 32 };
			g_render.m_draw_data.emplace_back( e_draw_type::draw_type_filled_circle, std::make_any< filled_circle_draw_object_t >( filledCircle ) );

			// Основная обводка для bounce точек - серая обводка снизу
			unsigned int grayColor = c_color( 128, 128, 128, 255 ).get_u32( );
			const int segments     = 32;
			const float angleStep  = 2.0f * 3.14159f / segments;

			// Рисуем основную обводку (3/4 круга)
			for ( int j = 0; j < segments; ++j ) {
				float angle1 = j * angleStep;
				float angle2 = ( j + 1 ) * angleStep;

				// Для bounce - серая обводка снизу (1/4 круга)
				// Центр: 90° (или 1.57 радиан), по 45° в каждую сторону
				// От 45° до 135° (или от 0.79 до 2.36 радиан)
				bool isGraySegment = ( angle1 >= 0.79f && angle1 <= 2.36f ) || ( angle2 >= 0.79f && angle2 <= 2.36f );

				c_vector_2d pos1, pos2;
				pos1.m_x = screenPos.m_x + cos( angle1 ) * effectiveRadius;
				pos1.m_y = screenPos.m_y + sin( angle1 ) * effectiveRadius;
				pos2.m_x = screenPos.m_x + cos( angle2 ) * effectiveRadius;
				pos2.m_y = screenPos.m_y + sin( angle2 ) * effectiveRadius;

				unsigned int segmentColor = isGraySegment ? grayColor : outlineColor;

				line_draw_object_t outlineLine{ pos1, pos2, segmentColor, outlineThickness };
				g_render.m_draw_data.emplace_back( e_draw_type::draw_type_line, std::make_any< line_draw_object_t >( outlineLine ) );
			}

			float animSpeed     = 2.0f;
			float anim          = ( std::sin( ImGui::GetTime( ) * animSpeed ) + 1.0f ) / 2.0f;
			float maxCrossAlpha = 200.0f;
			if ( distance > 300.0f ) {
				float fade    = std::clamp( ( distance - 300.0f ) / 300.0f, 0.0f, 1.0f );
				maxCrossAlpha = 200.0f * ( 1.0f - fade );
			}
			int crossAlpha = static_cast< int >( maxCrossAlpha * anim );

			// Расчет цвета креста в зависимости от активности точки
			unsigned int crossColor;
			if ( point.active ) {
				crossColor = c_color( int( accentclr.x * 255.f ), int( accentclr.y * 255.f ), int( accentclr.z * 255.f ), crossAlpha ).get_u32( );
			} else {
				int gray   = 150; // Светло-серый для креста неактивных точек
				crossColor = c_color( gray, gray, gray, crossAlpha ).get_u32( );
			}

			if ( isHovered && isBackspacePressed ) {
				m_bounce_points_check.erase( m_bounce_points_check.begin( ) + i );
				break;
			}
		}
	}
}

void n_movement::impl_t::on_paint_traverse( )
{
	can = true;

	this->pixel_finder( nullptr );
	this->pixelsurf_assist_render( );
	if ( g_ctx.m_local )
		if ( g_interfaces.m_engine_client->is_in_game( ) )
			if ( g_ctx.m_local->is_alive( ) ) {
				RenderPoints( m_points_check, g_ctx.m_local->get_origin( ), g_interfaces.m_engine_client->get_level_name_short( ) );
			}
	can = false;
}

void n_movement::impl_t::on_create_move_post( )
{
	// if in any type of obs mode dont run movement features
	if ( g_ctx.m_local->get_observer_mode( ) > 0 )
		return;
	if ( g_ctx.m_local ) {
		if ( g_ctx.m_local->get_health( ) != 0 && g_ctx.m_local->is_alive( ) ) {
			const float target_ps_velocity =
				-g_convars[ HASH_BT( "sv_gravity" ) ]->get_float( ) * 0.5f * g_interfaces.m_global_vars_base->m_interval_per_tick;

			if ( GET_VARIABLE( g_variables.m_edge_jump, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.m_edge_jump_key, key_bind_t ) ) )
				this->edge_jump( );

			// run this before movetype check so we can auto ladder bug up stairs(temporary)
			if ( GET_VARIABLE( g_variables.m_jump_bug, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.m_jump_bug_key, key_bind_t ) ) ||
			     AlertJB || AlertJB2 )
				this->jump_bug( );

			g_movement.m_pixelsurf_data.m_in_pixel_surf =
				g_prediction.backup_data.m_velocity.m_z == -6.25f && g_ctx.m_local->get_velocity( ).m_z == -6.25f;

			const auto move_type = g_prediction.backup_data.m_move_type;
			if ( move_type != e_move_types::move_type_ladder && move_type != e_move_types::move_type_noclip &&
			     move_type != e_move_types::move_type_fly && move_type != e_move_types::move_type_observer ) {
				if ( GET_VARIABLE( g_variables.m_long_jump, bool ) &&
				     g_input.check_input( &GET_VARIABLE( g_variables.m_long_jump_key, key_bind_t ) ) )
					this->long_jump( );

				if ( GET_VARIABLE( g_variables.m_mini_jump, bool ) &&
				     g_input.check_input( &GET_VARIABLE( g_variables.m_mini_jump_key, key_bind_t ) ) )
					this->mini_jump( );

				if ( GET_VARIABLE( g_variables.m_pixel_surf_fix, bool ) )
					this->pixel_surf_fix( );
				this->fire_man( g_ctx.m_cmd );
				this->pixel_surf( target_ps_velocity );

				this->auto_duck( );

				this->movement_fix( g_prediction.backup_data.m_view_angles );
			}
			this->ladder_bug( g_ctx.m_cmd );
			this->anti_ladder( g_ctx.m_cmd );
		}
	}
}

void n_movement::impl_t::edge_jump( )
{
	const auto move_type = g_prediction.backup_data.m_move_type;
	if ( move_type == e_move_types::move_type_noclip || move_type == e_move_types::move_type_fly || move_type == e_move_types::move_type_observer )
		return;
	static int saved_tick{ };
	if ( g_interfaces.m_global_vars_base->m_tick_count - saved_tick > 1 && g_interfaces.m_global_vars_base->m_tick_count - saved_tick < 15 ) {
		g_ctx.m_cmd->m_forward_move = 0.f;
		g_ctx.m_cmd->m_side_move    = 0.f;
		g_ctx.m_cmd->m_buttons      = g_ctx.m_cmd->m_buttons & ~( in_forward | in_back | in_moveleft | in_moveleft );
		g_ctx.m_cmd->m_buttons |= in_duck;
	}

	// is on ladder and wont be on ladder on next tick
	if ( move_type == e_move_types::move_type_ladder ) {
		const int pre = g_ctx.m_local->get_move_type( );
		g_prediction.begin( g_ctx.m_local, g_ctx.m_cmd );
		g_prediction.end( g_ctx.m_local );

		const int post = g_ctx.m_local->get_move_type( );
		if ( pre == e_move_types::move_type_ladder ) {
			if ( post != e_move_types::move_type_ladder ) {
				saved_tick = g_interfaces.m_global_vars_base->m_tick_count;
				g_ctx.m_cmd->m_buttons |= in_jump;
				g_ctx.m_cmd->m_forward_move = 0.f;
				g_ctx.m_cmd->m_side_move    = 0.f;
				g_ctx.m_cmd->m_buttons      = g_ctx.m_cmd->m_buttons & ~( in_forward | in_back | in_moveleft | in_moveleft );
			}
		}

		if ( g_interfaces.m_global_vars_base->m_tick_count - saved_tick > 1 && g_interfaces.m_global_vars_base->m_tick_count - saved_tick < 15 ) {
			g_ctx.m_cmd->m_forward_move = 0.f;
			g_ctx.m_cmd->m_side_move    = 0.f;
			g_ctx.m_cmd->m_buttons      = g_ctx.m_cmd->m_buttons & ~( in_forward | in_back | in_moveleft | in_moveleft );
			g_ctx.m_cmd->m_buttons |= in_duck;
		}

		return;
	}

	if ( ( g_prediction.backup_data.m_flags & e_flags::fl_onground ) && !( g_ctx.m_local->get_flags( ) & e_flags::fl_onground ) )
		g_ctx.m_cmd->m_buttons |= e_command_buttons::in_jump;
}

void n_movement::impl_t::air_stuck( c_user_cmd* cmd )
{
	if ( !GET_VARIABLE( g_variables.m_air_stuck, bool ) )
		return;
	if ( !g_input.check_input( &GET_VARIABLE( g_variables.m_air_stuck_key, key_bind_t ) ) )
		return;
	if ( !g_ctx.m_local || !g_ctx.m_local->is_alive( ) )
		return;
	if ( g_prediction.backup_data.m_velocity.m_z > 0.f || g_prediction.backup_data.m_flags & e_flags::fl_onground )
		return;

	static bool wall_detected = false;
	static float start_circle = 0.f;
	float max_radias          = std::numbers::pi_v< float > * 2.f;
	float step                = max_radias / 128.f; // Увеличена точность детекции стен
	c_vector start_pos        = g_ctx.m_local->get_abs_origin( );
	const auto mins           = g_ctx.m_local->get_collideable( )->get_obb_mins( );
	const auto maxs           = g_ctx.m_local->get_collideable( )->get_obb_maxs( );
	c_trace_filter_world fil;
	wall_detected = false;
	trace_t trace;

	static c_angle original_viewangle;
	static bool first_tick = true;
	if ( first_tick ) {
		original_viewangle = cmd->m_view_point;
		first_tick         = false;
	}

	for ( float a = start_circle; a < max_radias; a += step ) {
		c_vector end_pos;
		end_pos.m_x = cos( a ) * 64.f + start_pos.m_x;
		end_pos.m_y = sin( a ) * 64.f + start_pos.m_y;
		end_pos.m_z = start_pos.m_z;
		ray_t ray( start_pos, end_pos, mins, maxs );
		g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &fil, &trace );
		if ( trace.m_fraction != 1.f && trace.m_plane.m_normal.m_z == 0.f ) {
			wall_detected = true;
			start_circle  = a;
			break;
		}
	}
	if ( !wall_detected ) {
		start_circle = 0.f;
		first_tick   = true;
		return;
	}

	static bool hit = false;
	static c_vector_2d move_directions;
	const float target_z_velocity    = -6.25f; // Целевая скорость по Z
	const float z_velocity_tolerance = 0.01f;  // Допуск для точности

	if ( wall_detected ) {
		c_vector wall_normal = { trace.m_plane.m_normal.m_x, trace.m_plane.m_normal.m_y, 0.f };
		wall_normal.normalized( );
		c_vector wish_dir = { -wall_normal.m_x, -wall_normal.m_y, 0.f };
		wish_dir.normalized( );

		if ( !hit ) {
			float best_z_vel_diff = 1000.f;
			float best_angle      = 0.f;
			c_vector_2d best_move_directions;

			// Первый этап: грубый поиск
			for ( float i = -30.f; i <= 30.f; i += 1.f ) {
				g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
				float theta_rad   = deg2rad( i );
				c_vector move_dir = { wish_dir.m_x * cos( theta_rad ) - wish_dir.m_y * sin( theta_rad ),
					                  wish_dir.m_x * sin( theta_rad ) + wish_dir.m_y * cos( theta_rad ), 0.f };
				move_dir.normalized( );
				float move_amount = 450.f;
				c_angle move_ang  = move_dir.to_angle2( );
				move_ang.normalize( );
				float rot               = deg2rad( move_ang.m_y - original_viewangle.m_y );
				float test_forward      = cos( rot ) * move_amount;
				float test_side         = -sin( rot ) * move_amount;
				c_user_cmd test_cmd     = *cmd;
				test_cmd.m_forward_move = test_forward;
				test_cmd.m_side_move    = test_side;
				test_cmd.m_view_point   = original_viewangle;
				c_vector old_velocity   = g_ctx.m_local->get_velocity( );
				g_prediction.begin( g_ctx.m_local, &test_cmd );
				g_prediction.end( g_ctx.m_local );
				c_vector new_velocity = g_ctx.m_local->get_velocity( );
				if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground )
					continue;
				float z_vel_diff = std::abs( new_velocity.m_z - target_z_velocity );
				if ( z_vel_diff < best_z_vel_diff ) {
					best_z_vel_diff          = z_vel_diff;
					best_angle               = i;
					best_move_directions.m_x = test_forward;
					best_move_directions.m_y = test_side;
				}
			}

			// Второй этап: точный поиск
			for ( float i = best_angle - 1.f; i <= best_angle + 1.f; i += 0.1f ) {
				g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
				float theta_rad   = deg2rad( i );
				c_vector move_dir = { wish_dir.m_x * cos( theta_rad ) - wish_dir.m_y * sin( theta_rad ),
					                  wish_dir.m_x * sin( theta_rad ) + wish_dir.m_y * cos( theta_rad ), 0.f };
				move_dir.normalized( );
				float move_amount = 450.f;
				c_angle move_ang  = move_dir.to_angle2( );
				move_ang.normalize( );
				float rot               = deg2rad( move_ang.m_y - original_viewangle.m_y );
				float test_forward      = cos( rot ) * move_amount;
				float test_side         = -sin( rot ) * move_amount;
				c_user_cmd test_cmd     = *cmd;
				test_cmd.m_forward_move = test_forward;
				test_cmd.m_side_move    = test_side;
				test_cmd.m_view_point   = original_viewangle;
				c_vector old_velocity   = g_ctx.m_local->get_velocity( );
				g_prediction.begin( g_ctx.m_local, &test_cmd );
				g_prediction.end( g_ctx.m_local );
				c_vector new_velocity = g_ctx.m_local->get_velocity( );
				if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground )
					continue;
				float z_vel_diff = std::abs( new_velocity.m_z - target_z_velocity );
				if ( z_vel_diff < best_z_vel_diff ) {
					best_z_vel_diff          = z_vel_diff;
					best_angle               = i;
					best_move_directions.m_x = test_forward;
					best_move_directions.m_y = test_side;
				}
			}

			move_directions = best_move_directions;
			hit             = ( best_z_vel_diff < z_velocity_tolerance );
		}

		if ( hit ) {
			cmd->m_forward_move = move_directions.m_x;
			cmd->m_side_move    = move_directions.m_y;
			float current_yaw   = cmd->m_view_point.m_y;
			float rot           = deg2rad( original_viewangle.m_y - current_yaw );
			float cos_r         = std::cos( rot );
			float sin_r         = std::sin( rot );
			float old_forward   = cmd->m_forward_move;
			float old_side      = cmd->m_side_move;
			cmd->m_forward_move = cos_r * old_forward - sin_r * old_side;
			cmd->m_side_move    = sin_r * old_forward + cos_r * old_side;
		}
	} else {
		hit        = false;
		first_tick = true;
	}

	// Проверка стабильности air stuck
	if ( hit && std::abs( g_ctx.m_local->get_velocity( ).m_z - target_z_velocity ) > 0.1f ) {
		hit = false;
	}
}

void n_movement::impl_t::pixel_surf_fix( )
{
	if ( !g_ctx.m_local )
		return;
	if ( !g_ctx.m_local->is_alive( ) )
		return;
	if ( g_prediction.backup_data.m_velocity.m_z > 0.f )
		return;
	if ( g_input.check_input( &GET_VARIABLE( g_variables.m_bounce_assist_key, key_bind_t ) ) )
		return;
	if ( g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_surf_assist_key, key_bind_t ) ) )
		return;

	if ( g_prediction.backup_data.m_velocity.length_2d( ) >= 285.91f ) {
		if ( g_ctx.m_local->get_flags( ) & 1 ) {
			auto sv_airAcelerate = g_interfaces.m_convar->find_var( "sv_airaccelerate" );
			float Razn           = ( ( g_prediction.backup_data.m_velocity.length_2d( ) + 2.f - 285.91f ) / 12.f * 64.f );
			c_vector velocity    = g_prediction.backup_data.m_velocity * -1.f;
			velocity.m_z         = 0.f;
			float rotation       = deg2rad( velocity.to_angle2( ).m_y - g_ctx.m_cmd->m_view_point.m_y );
			float cos_rot        = cos( rotation );
			float sin_rot        = sin( rotation );

			float forwardmove           = cos_rot * Razn;
			float sidemove              = -sin_rot * Razn;
			g_ctx.m_cmd->m_forward_move = forwardmove;
			g_ctx.m_cmd->m_side_move    = sidemove;
		}
	}
}

void n_movement::impl_t::long_jump( )
{
	static int saved_tick = 0;

	if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground )
		saved_tick = g_interfaces.m_global_vars_base->m_tick_count;

	if ( !( g_interfaces.m_global_vars_base->m_tick_count - saved_tick > 2 ) && !( g_ctx.m_local->get_flags( ) & e_flags::fl_onground ) )
		g_ctx.m_cmd->m_buttons |= e_command_buttons::in_duck;
}

void n_movement::impl_t::mini_jump( )
{
	static bool should_duck = false;

	if ( g_prediction.backup_data.m_flags && g_ctx.m_local->get_flags( ) & e_flags::fl_onground )
		should_duck = false;

	if ( g_prediction.backup_data.m_flags & e_flags::fl_onground && !( g_ctx.m_local->get_flags( ) & e_flags::fl_onground ) ) {
		g_ctx.m_cmd->m_buttons |= e_command_buttons::in_jump;
		g_ctx.m_cmd->m_buttons |= e_command_buttons::in_duck;
		if ( GET_VARIABLE( g_variables.m_mini_jump_hold_duck, bool ) )
			should_duck = true;
	}

	if ( should_duck )
		g_ctx.m_cmd->m_buttons |= e_command_buttons::in_duck;
}

void n_movement::impl_t::auto_duck( )
{
	if ( !GET_VARIABLE( g_variables.m_auto_duck, bool ) || g_prediction.backup_data.m_flags & e_flags::fl_onground ||
	     g_movement.m_edgebug_data.m_will_edgebug || g_movement.m_pixelsurf_data.m_in_pixel_surf ||
	     ( GET_VARIABLE( g_variables.m_jump_bug, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.m_jump_bug_key, key_bind_t ) ) ) ||
	     ( GET_VARIABLE( g_variables.m_edge_jump, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.m_edge_jump_key, key_bind_t ) ) ) ) {
		g_movement.m_autoduck_data.reset( );

		return;
	}

	g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );

	for ( int i = 0; i < 2; i++ ) {
		if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground )
			break;

		c_user_cmd* simulated_cmd = new c_user_cmd( *g_ctx.m_cmd );

		simulated_cmd->m_buttons |= e_command_buttons::in_bullrush;
		simulated_cmd->m_buttons |= e_command_buttons::in_duck;

		g_prediction.begin( g_ctx.m_local, simulated_cmd );
		g_prediction.end( g_ctx.m_local );

		if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground ) {
			g_movement.m_autoduck_data.m_did_land_ducking = true;
			g_movement.m_autoduck_data.m_ducking_vert     = g_ctx.m_local->get_abs_origin( ).m_z;
			break;
		}
		delete simulated_cmd;
	}

	g_prediction.begin( g_ctx.m_local, g_ctx.m_cmd );
	g_prediction.end( g_ctx.m_local );

	g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );

	if ( !g_movement.m_autoduck_data.m_did_land_ducking )
		return;

	for ( int i = 0; i < 2; i++ ) {
		if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground )
			break;

		c_user_cmd* simulated_cmd = new c_user_cmd( *g_ctx.m_cmd );

		simulated_cmd->m_buttons &= ~e_command_buttons::in_duck;

		g_prediction.begin( g_ctx.m_local, simulated_cmd );
		g_prediction.end( g_ctx.m_local );

		if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground ) {
			g_movement.m_autoduck_data.m_did_land_standing = true;
			g_movement.m_autoduck_data.m_standing_vert     = g_ctx.m_local->get_abs_origin( ).m_z;
			break;
		}
		delete simulated_cmd;
	}

	g_prediction.begin( g_ctx.m_local, g_ctx.m_cmd );
	g_prediction.end( g_ctx.m_local );

	g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );

	if ( g_movement.m_autoduck_data.m_did_land_ducking && g_movement.m_autoduck_data.m_did_land_standing ) {
		if ( g_movement.m_autoduck_data.m_ducking_vert > g_movement.m_autoduck_data.m_standing_vert )
			g_ctx.m_cmd->m_buttons |= e_command_buttons::in_duck;
	} else if ( g_movement.m_autoduck_data.m_did_land_ducking && !g_movement.m_autoduck_data.m_did_land_standing )
		g_ctx.m_cmd->m_buttons |= e_command_buttons::in_duck;
}

void n_movement::impl_t::pixel_surf_locking( float target_ps_velocity )
{
	if ( g_movement.m_pixelsurf_data.m_in_pixel_surf ) {
		if ( g_movement.m_pixelsurf_data.m_should_duck )
			g_ctx.m_cmd->m_buttons |= e_command_buttons::in_duck;
		else
			g_ctx.m_cmd->m_buttons &= ~e_command_buttons::in_duck;

		if ( g_prediction.backup_data.m_velocity.m_z == -6.25f ) {
			if ( b_buttons & in_duck )
				g_ctx.m_cmd->m_buttons |= in_duck;
		}
		if ( g_movement.m_pixelsurf_data.m_predicted_succesful && !( g_ctx.m_local->get_velocity( ).m_z == target_ps_velocity ) ) {
			g_movement.m_pixelsurf_data.m_in_pixel_surf       = false;
			g_movement.m_pixelsurf_data.m_predicted_succesful = false;
			return;
		}
		return;
	}
}
bool wall_detected = false;
void n_movement::impl_t::pixel_surf( float target_ps_velocity )
{
	if ( !( GET_VARIABLE( g_variables.m_pixel_surf, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_surf_key, key_bind_t ) ) ) ) {
		return;
	}

	{
		static int ticks = 0;
		if ( !g_ctx.m_local )
			return;
		if ( !g_ctx.m_local->is_alive( ) ) {
			ticks = 0;
			return;
		}
		if ( g_ctx.m_local->get_move_type( ) == move_type_noclip || g_ctx.m_local->get_move_type( ) == move_type_ladder )
			return;
		if ( g_ctx.m_local->get_flags( ) & 1 )
			return;

		if ( !wall_detected )
			return;

		if ( !m_pixelsurf_data.should_pixel_surf ) {
			int BackupButtons = g_ctx.m_cmd->m_buttons;
			for ( int i = 0; i < 2; i++ ) {
				g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
				if ( i == 0 )
					g_ctx.m_cmd->m_buttons &= ~in_duck;
				else
					g_ctx.m_cmd->m_buttons |= in_duck;
				for ( int z = 0; z < 8; z++ ) {
					float un_pred_velo = g_ctx.m_local->get_velocity( ).m_z;
					g_prediction.begin( g_ctx.m_local, g_ctx.m_cmd );
					g_prediction.end( g_ctx.m_local );
					if ( g_ctx.m_local->get_flags( ) & 1 ) {
						break;
					}
					float zVelo                        = g_ctx.m_local->get_velocity( ).m_z;
					m_pixelsurf_data.should_pixel_surf = zVelo == -6.25f;
					if ( m_pixelsurf_data.should_pixel_surf && i == 0 ) {
						m_pixelsurf_data.should_pixel_surf = false;
						g_ctx.m_cmd->m_buttons             = BackupButtons;
						return;
					}
					if ( m_pixelsurf_data.should_pixel_surf ) {
						ticks         = g_ctx.m_cmd->m_tick_count + z + 16;
						BackupButtons = g_ctx.m_cmd->m_buttons;
						break;
					}
				}
			}
			g_ctx.m_cmd->m_buttons = BackupButtons;
			g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
		} else {
			g_ctx.m_cmd->m_buttons |= in_duck;
			if ( g_ctx.m_cmd->m_tick_count > ticks ) {
				if ( g_prediction.backup_data.m_velocity.m_z != -6.25f ) {
					m_pixelsurf_data.should_pixel_surf = false;
				}
			}
		}
	}
}

void n_movement::impl_t::jump_bug( )
{
	if ( g_prediction.backup_data.m_flags & 1 )
		g_movement.m_jumpbug_data.m_can_jb = false;
	static float btime;
	if ( btime < g_interfaces.m_global_vars_base->m_current_time )
		g_movement.m_jumpbug_data.m_can_jb = false;

	if ( ( GET_VARIABLE( g_variables.m_jump_bug, bool ) && !m_edgebug_data.m_will_edgebug &&
	       g_ctx.m_local->get_velocity( ).m_z > g_prediction.backup_data.m_velocity.m_z && !( g_prediction.backup_data.m_flags & 1 ) &&
	       !( g_ctx.m_local->get_flags( ) & 1 ) && g_ctx.m_local->get_move_type( ) != move_type_ladder &&
	       g_ctx.m_local->get_move_type( ) != move_type_noclip && g_ctx.m_local->get_move_type( ) != move_type_observer ) ) {
		btime                              = g_interfaces.m_global_vars_base->m_current_time + .2f;
		g_movement.m_jumpbug_data.m_can_jb = true;
	}

	[[unlikely]] if ( !( g_ctx.m_cmd->m_buttons & e_command_buttons::in_jump ) ) {
		static bool ducked = false;

		if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground && !( g_prediction.backup_data.m_flags & e_flags::fl_onground ) && !ducked ) {
			g_ctx.m_cmd->m_buttons |= e_command_buttons::in_duck;
			ducked = true;
		} else
			ducked = false;

		if ( g_prediction.backup_data.m_flags & e_flags::fl_onground && ducked )
			ducked = false;
	} else {
		if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground && !( g_prediction.backup_data.m_flags & e_flags::fl_onground ) )
			g_ctx.m_cmd->m_buttons |= e_command_buttons::in_duck;

		if ( g_ctx.m_local->get_flags( ) & e_flags::fl_onground )
			g_ctx.m_cmd->m_buttons &= ~e_command_buttons::in_jump;

		if ( !( g_ctx.m_local->get_flags( ) & fl_onground ) && g_prediction.backup_data.m_flags & fl_onground )
			g_ctx.m_cmd->m_buttons &= ~e_command_buttons::in_duck;
	}
}

void n_movement::impl_t::rotate_movement( c_user_cmd* cmd, c_angle& angle )
{
	if ( angle.m_x == 0 && angle.m_y == 0 && angle.m_z == 0 )
		g_interfaces.m_engine_client->get_view_angles( angle );

	c_vector vec_move = c_vector( cmd->m_forward_move, cmd->m_side_move, 0.f );

	const float speed = vec_move.length_2d( );

	const float rotation = deg2rad( cmd->m_view_point.m_y - angle.m_y );

	cmd->m_forward_move = std::cosf( rotation ) * speed;
	cmd->m_side_move    = std::sinf( rotation ) * speed;
}

constexpr float RAY_LENGTH        = 6000.f;
constexpr float ALIGN_OFFSET      = 15.97803f;
constexpr float MOVE_SPEED        = 10.f;
constexpr float HEIGHT_OFFSET_NEG = -0.972092f;
constexpr float HEIGHT_OFFSET_POS = 0.0287018f;

// Функция для вычисления смещения по нормали стены
c_vector GetOffsetFromWallNormal( const c_vector& wallNormal, float offset )
{
	// Если wallNormal.x == 0, смещение по X будет 0, иначе берем знак
	float offsetX = ( wallNormal.m_x < 0 ? -offset : ( wallNormal.m_x > 0 ? offset : 0.f ) );
	float offsetY = ( wallNormal.m_y < 0 ? -offset : ( wallNormal.m_y > 0 ? offset : 0.f ) );
	return c_vector( offsetX, offsetY, 0.f );
}
struct AnimatedPoint {
	c_vector position;                // Позиция точки в мировых координатах
	float animation_progress = 0.f;   // Прогресс анимации (0.0 до 1.0)
	bool is_appearing        = true;  // Точка появляется
	bool is_removing         = false; // Точка исчезает
	float current_size       = 0.f;   // Текущий размер для рисования
};
std::vector< AnimatedPoint > animated_points;
void UpdateAndRenderPoints( std::vector< c_vector >& Points )
{
	float delta_time                   = ImGui::GetIO( ).DeltaTime; // Время между кадрами
	const float appearance_duration    = 0.5f;                      // Длительность появления (в секундах)
	const float disappearance_duration = 0.5f;                      // Длительность исчезновения (в секундах)
	const float size_animation_speed   = 5.0f;                      // Скорость анимации размера
	const float normal_size            = 9.f;                       // Нормальный размер круга
	const float in_crosshair_size      = 12.f;                      // Размер круга в прицеле

	// Шаг 1: Пометить все animated_points как потенциально удаляемые
	for ( auto& ap : animated_points ) {
		ap.is_removing = true;
	}

	// Шаг 2: Обновить animated_points на основе текущих Points
	for ( const auto& p : Points ) {
		auto it = std::find_if( animated_points.begin( ), animated_points.end( ), [ &p ]( const AnimatedPoint& ap ) { return ap.position == p; } );
		if ( it == animated_points.end( ) ) {
			// Новая точка, добавить с начальной анимацией появления
			animated_points.push_back( { p, 0.f, true, false, 0.f } );
		} else {
			// Точка уже есть, отменить удаление
			it->is_removing = false;
		}
	}

	// Шаг 3: Обновить анимации для всех точек
	for ( auto& ap : animated_points ) {
		if ( ap.is_appearing ) {
			ap.animation_progress += delta_time / appearance_duration;
			if ( ap.animation_progress >= 1.f ) {
				ap.animation_progress = 1.f;
				ap.is_appearing       = false;
			}
		} else if ( ap.is_removing ) {
			ap.animation_progress -= delta_time / disappearance_duration;
			if ( ap.animation_progress <= 0.f ) {
				ap.animation_progress = 0.f; // Точка готова к удалению
			}
		} else {
			ap.animation_progress = 1.f; // Нормальное состояние
		}

		// Определить, находится ли точка в прицеле
		c_vector_2d screenPos;
		bool in_crosshair = false;
		if ( g_render.world_to_screen( ap.position, screenPos ) ) {
			in_crosshair = InCrosshair( screenPos.m_x, screenPos.m_y, 9.f );
		}

		// Вычислить целевой размер
		float target_size;
		if ( ap.is_removing ) {
			target_size = 0.f; // При исчезновении размер уменьшается до 0
		} else {
			target_size = in_crosshair ? in_crosshair_size : normal_size;
			if ( ap.is_appearing ) {
				target_size *= ap.animation_progress; // Увеличивать от 0 до целевого размера
			}
		}

		// Плавно интерполировать текущий размер к целевому
		ap.current_size += ( target_size - ap.current_size ) * ( 1.f - std::exp( -size_animation_speed * delta_time ) );
	}

	// Шаг 4: Удалить полностью исчезнувшие точки
	animated_points.erase( std::remove_if( animated_points.begin( ), animated_points.end( ),
	                                       []( const AnimatedPoint& ap ) { return ap.is_removing && ap.animation_progress <= 0.f; } ),
	                       animated_points.end( ) );
	static int stop = 0;
	// Шаг 5: Отрисовка и обработка выбора
	bool point_selected = false;
	for ( const auto& ap : animated_points ) {
		c_vector_2d screenPos;
		if ( g_render.world_to_screen( ap.position, screenPos ) ) {
			bool in_crosshair                            = InCrosshair( screenPos.m_x, screenPos.m_y, 9.f );
			g_movement.m_pixelsurf_assist_t.in_crosshair = in_crosshair; // Обновляем глобальное состояние

			// Вычислить альфа-канал для заполненного круга и контура
			float filled_alpha  = ap.animation_progress * 100.f; // От 0 до 100
			float outline_alpha = ap.animation_progress * 255.f; // От 0 до 255

			// Отрисовка заполненного круга
			g_render.m_draw_data.emplace_back( e_draw_type::draw_type_filled_circle,
			                                   std::make_any< filled_circle_draw_object_t >(
												   screenPos, ap.current_size, c_color( 0, 0, 0, static_cast< int >( filled_alpha ) ).get_u32( ) ) );

			// Отрисовка контура круга
			g_render.m_draw_data.emplace_back(
				e_draw_type::draw_type_circle,
				std::make_any< circle_draw_object_t >( screenPos, ap.current_size,
			                                           c_color( 0, 255, 0, static_cast< int >( outline_alpha ) ).get_u32( ) ) );

			// Обработка выбора точки (только одна за кадр)
			if ( !point_selected && in_crosshair && g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_surf_assist_point_key, key_bind_t ) ) ) {
				if ( stop < GetTickCount64( ) ) {
					m_points_check.emplace_back( ap.position, g_interfaces.m_engine_client->get_level_name_short( ) );
					g_movement.m_pixelsurf_assist_t.set_point = true;
					Points.clear( ); // Очищаем Points, как в исходном коде
					point_selected = true;
					stop           = GetTickCount64( ) + 3000;
				}
			}
		}
	}
}
std::vector< AnimatedPoint > animated_points2;
c_vector round_pos2( c_vector point )
{
	if ( point.m_z < 0.f )
		return c_vector( point.m_x, point.m_y, ( int )point.m_z - 0.969644f );
	else
		return c_vector( point.m_x, point.m_y, ( int )point.m_z + 0.04f );
}
bool bounce_set_point = false;
void UpdateAndRenderPoints2( std::vector< c_vector >& Points )
{
	float delta_time                   = ImGui::GetIO( ).DeltaTime; // Время между кадрами
	const float appearance_duration    = 0.5f;                      // Длительность появления (в секундах)
	const float disappearance_duration = 0.5f;                      // Длительность исчезновения (в секундах)
	const float size_animation_speed   = 5.0f;                      // Скорость анимации размера
	const float normal_size            = 9.f;                       // Нормальный размер круга
	const float in_crosshair_size      = 12.f;                      // Размер круга в прицеле

	// Шаг 1: Пометить все animated_points как потенциально удаляемые
	for ( auto& ap : animated_points2 ) {
		ap.is_removing = true;
	}

	// Шаг 2: Обновить animated_points на основе текущих Points
	for ( const auto& p : Points ) {
		auto it = std::find_if( animated_points2.begin( ), animated_points2.end( ), [ &p ]( const AnimatedPoint& ap ) { return ap.position == p; } );
		if ( it == animated_points2.end( ) ) {
			// Новая точка, добавить с начальной анимацией появления
			animated_points2.push_back( { p, 0.f, true, false, 0.f } );
		} else {
			// Точка уже есть, отменить удаление
			it->is_removing = false;
		}
	}

	// Шаг 3: Обновить анимации для всех точек
	for ( auto& ap : animated_points2 ) {
		if ( ap.is_appearing ) {
			ap.animation_progress += delta_time / appearance_duration;
			if ( ap.animation_progress >= 1.f ) {
				ap.animation_progress = 1.f;
				ap.is_appearing       = false;
			}
		} else if ( ap.is_removing ) {
			ap.animation_progress -= delta_time / disappearance_duration;
			if ( ap.animation_progress <= 0.f ) {
				ap.animation_progress = 0.f; // Точка готова к удалению
			}
		} else {
			ap.animation_progress = 1.f; // Нормальное состояние
		}

		// Определить, находится ли точка в прицеле
		c_vector_2d screenPos;
		bool in_crosshair = false;
		if ( g_render.world_to_screen( ap.position, screenPos ) ) {
			in_crosshair = InCrosshair( screenPos.m_x, screenPos.m_y, 9.f );
		}

		// Вычислить целевой размер
		float target_size;
		if ( ap.is_removing ) {
			target_size = 0.f; // При исчезновении размер уменьшается до 0
		} else {
			target_size = in_crosshair ? in_crosshair_size : normal_size;
			if ( ap.is_appearing ) {
				target_size *= ap.animation_progress; // Увеличивать от 0 до целевого размера
			}
		}

		// Плавно интерполировать текущий размер к целевому
		ap.current_size += ( target_size - ap.current_size ) * ( 1.f - std::exp( -size_animation_speed * delta_time ) );
	}

	// Шаг 4: Удалить полностью исчезнувшие точки
	animated_points2.erase( std::remove_if( animated_points2.begin( ), animated_points2.end( ),
	                                        []( const AnimatedPoint& ap ) { return ap.is_removing && ap.animation_progress <= 0.f; } ),
	                        animated_points2.end( ) );
	static int stop = 0;
	// Шаг 5: Отрисовка и обработка выбора
	bool point_selected = false;
	for ( const auto& ap : animated_points2 ) {
		c_vector_2d screenPos;
		if ( g_render.world_to_screen( ap.position, screenPos ) ) {
			bool in_crosshair                            = InCrosshair( screenPos.m_x, screenPos.m_y, 9.f );
			g_movement.m_pixelsurf_assist_t.in_crosshair = in_crosshair; // Обновляем глобальное состояние

			// Вычислить альфа-канал для заполненного круга и контура
			float filled_alpha  = ap.animation_progress * 100.f; // От 0 до 100
			float outline_alpha = ap.animation_progress * 255.f; // От 0 до 255

			// Отрисовка заполненного круга
			g_render.m_draw_data.emplace_back( e_draw_type::draw_type_filled_circle,
			                                   std::make_any< filled_circle_draw_object_t >(
												   screenPos, ap.current_size, c_color( 0, 0, 0, static_cast< int >( filled_alpha ) ).get_u32( ) ) );

			// Отрисовка контура круга
			g_render.m_draw_data.emplace_back(
				e_draw_type::draw_type_circle,
				std::make_any< circle_draw_object_t >( screenPos, ap.current_size,
			                                           c_color( 255, 0, 0, static_cast< int >( outline_alpha ) ).get_u32( ) ) );

			// Обработка выбора точки (только одна за кадр)
			if ( !point_selected && in_crosshair && g_input.check_input( &GET_VARIABLE( g_variables.m_bounce_assist_point_key, key_bind_t ) ) ) {
				if ( stop < GetTickCount64( ) ) {
					m_bounce_points_check.emplace_back( round_pos2( c_vector( ap.position.m_x, ap.position.m_y, ap.position.m_z + 1.f ) ),
					                                    g_interfaces.m_engine_client->get_level_name_short( ) );
					bounce_set_point = true;
					Points.clear( ); // Очищаем Points, как в исходном коде
					point_selected = true;
					stop           = GetTickCount64( ) + 3000;
				}
			}
		}
	}
}

void n_movement::impl_t::pixel_finder( c_user_cmd* cmd )
{
	static c_vector StartPos, EndPos{ };
	static c_vector ViewAngles{ };
	static c_vector pixelsurfPoint{ };
	static c_vector WallNormal{ };
	static bool redLine = false;
	static std::vector< c_vector > Points{ };
	static std::vector< c_vector > Point_bounce{ };
	int to_muchPoints           = 0;
	static bool is_displacement = false;
	if ( cmd != nullptr ) {
		ViewAngles             = c_vector( cmd->m_view_point.m_x, cmd->m_view_point.m_y, 0.f );
		static bool FirstPoint = FALSE;
		if ( g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_finder_key, key_bind_t ) ) ) {
			if ( !FirstPoint ) {
				if ( Points.size( ) > 0 )
					Points.clear( );
				if ( Point_bounce.size( ) > 0 )
					Point_bounce.clear( );
				const auto endPos = g_ctx.m_local->get_eye_position( ) + c_vector::fromAngle( ViewAngles ) * 6000;

				trace_t trace = { };
				ray_t ray( g_ctx.m_local->get_eye_position( ), endPos );

				c_trace_filter flt( g_ctx.m_local );

				g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &flt, &trace );

				WallNormal      = trace.m_plane.m_normal;
				is_displacement = strstr( trace.surface.m_name, "displacement" );

				StartPos   = g_ctx.m_local->get_eye_position( ) + ( endPos - g_ctx.m_local->get_eye_position( ) ) * trace.m_fraction;
				EndPos     = g_ctx.m_local->get_eye_position( ) + ( endPos - g_ctx.m_local->get_eye_position( ) ) * trace.m_fraction;
				FirstPoint = true;
			} else {
				const auto endPos = g_ctx.m_local->get_eye_position( ) + c_vector::fromAngle( ViewAngles ) * 6000;

				trace_t trace = { };
				ray_t ray( g_ctx.m_local->get_eye_position( ), endPos );

				c_trace_filter flt( g_ctx.m_local );

				g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &flt, &trace );

				c_vector SecPlane = trace.m_plane.m_normal;
				// 0x5fff3298 is displacement

				EndPos = c_vector(
					StartPos.m_x, StartPos.m_y,
					c_vector( g_ctx.m_local->get_eye_position( ) + ( endPos - g_ctx.m_local->get_eye_position( ) ) * trace.m_fraction ).m_z );
			}

		} else if ( !g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_finder_key, key_bind_t ) ) && FirstPoint ) {
			if ( redLine ) {
				redLine    = false;
				FirstPoint = false;

				return;
			}

			c_vector origin   = g_ctx.m_local->get_origin( );
			int buttons       = cmd->m_buttons;
			float ForwardMove = cmd->m_forward_move;
			float Lerp        = 0;

			int PredictionFrames = g_interfaces.m_prediction->m_commands_predicted - 1;
			float test_al        = 15.97803f;
			if ( is_displacement )
				test_al = 16.001f;
			auto colid = g_ctx.m_local->get_collideable( );

			for ( int v = 0; v < 4; v++ ) {
				g_prediction.restore_entity_to_predicted_frame( PredictionFrames );
				bool done_and_break = false;
				for ( int i = 0; i < 10000; i++ ) {
					float align                                                       = 15.97803f;
					*( int* )( ( uintptr_t )( c_base_entity* )g_ctx.m_local + 0x25C ) = 2;
					// if (to_muchPoints > 4)
					//{
					//	Points.clear();
					//	return;
					// }

					cmd->m_buttons |= in_jump;
					cmd->m_buttons |= in_duck;

					c_vector Angles{ WallNormal.m_x * -1.f, WallNormal.m_y * -1.f, 0.f };
					c_angle to_wall     = Angles.to_angle( );
					float rotation      = deg2rad( to_wall.m_y - cmd->m_view_point.m_y );
					float cos_rot       = cos( rotation );
					float sin_rot       = sin( rotation );
					float forwardmove   = cos_rot * 10.f;
					float sidemove      = -sin_rot * 10.f;
					cmd->m_forward_move = forwardmove;
					cmd->m_side_move    = sidemove;
					/*
					if ( g_ctx.m_local->get_velocity( ).m_z < 0 ) {
					    static float Height = 0.f;
					    if ( StartPos.m_z > EndPos.m_z ) {
					        if ( StartPos.m_z < 0.f )
					            Height = ( int )StartPos.m_z - 0.972092f - Lerp;
					        else
					            Height = ( int )StartPos.m_z + 0.0287018f - Lerp;
					    }
					    else
					    {
					        if ( EndPos.m_z < 0.f )
					            Height = ( int )EndPos.m_z - 0.972092f - Lerp;
					        else
					            Height = ( int )EndPos.m_z + 0.0287018f - Lerp;
					    }
					    int maxLerp;
					    if ( StartPos.m_z > EndPos.m_z ) {
					        maxLerp = StartPos.m_z - EndPos.m_z;
					    }
					    else
					    {
					        maxLerp = EndPos.m_z - StartPos.m_z;
					    }

					    if ( Lerp > maxLerp ) {
					        cmd->m_buttons      = buttons;
					        cmd->m_forward_move = ForwardMove;
					        FirstPoint          = false;

					        break;
					    } else {
					        Lerp += 1.f;
					    }

					    if ( WallNormal.m_x < 0 && WallNormal.m_y < 0.f ) {
					        g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x - test_al, StartPos.m_y - test_al, Height );
					        g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
					    } else if ( WallNormal.m_x < 0 && WallNormal.m_y > 0.f ) {
					        g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x - test_al, StartPos.m_y + test_al, Height );
					        g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
					    } else if ( WallNormal.m_x > 0 && WallNormal.m_y < 0.f ) {
					        g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x + test_al, StartPos.m_y - test_al, Height );
					        g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
					    } else if ( WallNormal.m_x > 0 && WallNormal.m_y > 0.f ) {
					        g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x + test_al, StartPos.m_y + test_al, Height );
					        g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
					    } else if ( WallNormal.m_x == 0.f && WallNormal.m_y > 0.f ) {
					        g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x, StartPos.m_y + test_al, Height );
					        g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
					    } else if ( WallNormal.m_x == 0.f && WallNormal.m_y < 0.f ) {
					        g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x, StartPos.m_y - test_al, Height );
					        g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
					    } else if ( WallNormal.m_x < 0 && WallNormal.m_y == 0.f ) {
					        g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x - test_al, StartPos.m_y, Height );
					        g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
					    } else if ( WallNormal.m_x > 0 && WallNormal.m_y == 0.f ) {
					        g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x + test_al, StartPos.m_y, Height );
					        g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
					    }
					}

					*/

					if ( g_ctx.m_local->get_velocity( ).m_z > 0 ) {
						static float Height = 0.f;
						if ( StartPos.m_z > EndPos.m_z ) {
							if ( StartPos.m_z < 0.f )
								Height = ( int )StartPos.m_z - 0.02f - Lerp - 54.f;
							else
								Height = ( int )StartPos.m_z + 0.02f + Lerp - 54.f;
						} else {
							if ( EndPos.m_z < 0.f )
								Height = ( int )EndPos.m_z - 0.02f - Lerp - 54.f;
							else
								Height = ( int )EndPos.m_z + 0.02f + Lerp - 54.f;
						}
						int maxLerp;
						if ( StartPos.m_z > EndPos.m_z ) {
							maxLerp = StartPos.m_z - EndPos.m_z;
						} else {
							maxLerp = EndPos.m_z - StartPos.m_z;
						}

						if ( Lerp > maxLerp ) {
							cmd->m_buttons      = buttons;
							cmd->m_forward_move = ForwardMove;
							FirstPoint          = false;
							done_and_break      = true;
							break;
						} else {
							Lerp += 1.f;
						}

						if ( WallNormal.m_x < 0 && WallNormal.m_y < 0.f ) {
							g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x - test_al, StartPos.m_y - test_al, Height );
							g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
						} else if ( WallNormal.m_x < 0 && WallNormal.m_y > 0.f ) {
							g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x - test_al, StartPos.m_y + test_al, Height );
							g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
						} else if ( WallNormal.m_x > 0 && WallNormal.m_y < 0.f ) {
							g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x + test_al, StartPos.m_y - test_al, Height );
							g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
						} else if ( WallNormal.m_x > 0 && WallNormal.m_y > 0.f ) {
							g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x + test_al, StartPos.m_y + test_al, Height );
							g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
						} else if ( WallNormal.m_x == 0.f && WallNormal.m_y > 0.f ) {
							g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x, StartPos.m_y + test_al, Height );
							g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
						} else if ( WallNormal.m_x == 0.f && WallNormal.m_y < 0.f ) {
							g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x, StartPos.m_y - test_al, Height );
							g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
						} else if ( WallNormal.m_x < 0 && WallNormal.m_y == 0.f ) {
							g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x - test_al, StartPos.m_y, Height );
							g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
						} else if ( WallNormal.m_x > 0 && WallNormal.m_y == 0.f ) {
							g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x + test_al, StartPos.m_y, Height );
							g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
						}

						trace_t trace;
						c_trace_filter flt( g_ctx.m_local );
						ray_t ray(
							c_vector( g_ctx.m_local->get_origin( ).m_x, g_ctx.m_local->get_origin( ).m_y, g_ctx.m_local->get_origin( ).m_z + 54.f ),
							c_vector( g_ctx.m_local->get_origin( ).m_x, g_ctx.m_local->get_origin( ).m_y,
						              g_ctx.m_local->get_origin( ).m_z - 1000.f ) );
						g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &flt, &trace );
						c_vector down_point = trace.m_end;
						trace_t trace2;
						c_trace_filter flt2( g_ctx.m_local );
						ray_t ray2(
							c_vector( g_ctx.m_local->get_origin( ).m_x, g_ctx.m_local->get_origin( ).m_y, g_ctx.m_local->get_origin( ).m_z + 54.f ),
							c_vector( g_ctx.m_local->get_origin( ).m_x, g_ctx.m_local->get_origin( ).m_y,
						              g_ctx.m_local->get_origin( ).m_z + 1000.f ) );
						g_interfaces.m_engine_trace->trace_ray( ray2, mask_playersolid, &flt2, &trace2 );
						c_vector upper_point = trace2.m_end;

						if ( g_ctx.m_local->get_origin( ).m_z + 1.f < down_point.m_z )
							continue;
						if ( g_ctx.m_local->get_origin( ).m_z + 1.f > upper_point.m_z )
							continue;
					}

					c_vector un_pred_velo = g_ctx.m_local->get_velocity( );

					g_prediction.begin( g_ctx.m_local, g_ctx.m_cmd );
					g_prediction.end( g_ctx.m_local );

					c_vector pred_velo = g_ctx.m_local->get_velocity( );
					{
						if ( g_ctx.m_local->get_velocity( ).m_z == -6.25f ) {
							if ( WallNormal.m_x < 0 && WallNormal.m_y < 0.f ) {
								//	pointvec = c_vector(g_ctx.m_local->get_origin().m_x + test_al, g_ctx.m_local->get_origin().m_y + test_al,
								// g_ctx.m_local->get_origin().m_z); 	g_Routes.emplace_back(routes(c_vector(g_ctx.m_local->get_origin().m_x +
								// test_al, g_ctx.m_local->get_origin().m_y+ test_al, g_ctx.m_local->get_origin().m_z), "ps"));
								Point_bounce.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x + test_al,
								                                     g_ctx.m_local->get_origin( ).m_y + test_al,
								                                     g_ctx.m_local->get_origin( ).m_z + 54.f ) );
								to_muchPoints += 1;
								FirstPoint = false;
							} else if ( WallNormal.m_x < 0 && WallNormal.m_y > 0.f ) {
								//	pointvec = c_vector(g_ctx.m_local->get_origin().m_x + test_al, g_ctx.m_local->get_origin().m_y - test_al,
								// g_ctx.m_local->get_origin().m_z); 	g_Routes.emplace_back(routes(c_vector(g_ctx.m_local->get_origin().m_x +
								// test_al, g_ctx.m_local->get_origin().m_y - test_al, g_ctx.m_local->get_origin().m_z), "ps"));
								Point_bounce.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x + test_al,
								                                     g_ctx.m_local->get_origin( ).m_y - test_al,
								                                     g_ctx.m_local->get_origin( ).m_z + 54.f ) );
								to_muchPoints += 1;
								FirstPoint = false;
							} else if ( WallNormal.m_x > 0 && WallNormal.m_y < 0.f ) {
								//	pointvec = c_vector(g_ctx.m_local->get_origin().m_x - test_al, g_ctx.m_local->get_origin().m_y + test_al,
								// g_ctx.m_local->get_origin().m_z); 	g_Routes.emplace_back(routes(c_vector(g_ctx.m_local->get_origin().m_x -
								// test_al, g_ctx.m_local->get_origin().m_y + test_al, g_ctx.m_local->get_origin().m_z), "ps"));
								Point_bounce.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x - test_al,
								                                     g_ctx.m_local->get_origin( ).m_y + test_al,
								                                     g_ctx.m_local->get_origin( ).m_z + 54.f ) );
								to_muchPoints += 1;
								FirstPoint = false;
							} else if ( WallNormal.m_x > 0 && WallNormal.m_y > 0.f ) {
								//	pointvec = c_vector(g_ctx.m_local->get_origin().m_x - test_al, g_ctx.m_local->get_origin().m_y - test_al,
								// g_ctx.m_local->get_origin().m_z); 	g_Routes.emplace_back(routes(c_vector(g_ctx.m_local->get_origin().m_x -
								// test_al, g_ctx.m_local->get_origin().m_y - test_al, g_ctx.m_local->get_origin().m_z), "ps"));
								Point_bounce.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x - test_al,
								                                     g_ctx.m_local->get_origin( ).m_y - test_al,
								                                     g_ctx.m_local->get_origin( ).m_z + 54.f ) );
								to_muchPoints += 1;
								FirstPoint = false;
							} else if ( WallNormal.m_x == 0.f && WallNormal.m_y > 0.f ) {
								//	pointvec = c_vector(g_ctx.m_local->get_origin().m_x, g_ctx.m_local->get_origin().m_y - test_al,
								// g_ctx.m_local->get_origin().m_z); 	g_Routes.emplace_back(routes(c_vector(g_ctx.m_local->get_origin().m_x,
								// g_ctx.m_local->get_origin().m_y - test_al, g_ctx.m_local->get_origin().m_z), "ps"));
								Point_bounce.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x, g_ctx.m_local->get_origin( ).m_y - test_al,
								                                     g_ctx.m_local->get_origin( ).m_z + 54.f ) );
								to_muchPoints += 1;
								FirstPoint = false;
							} else if ( WallNormal.m_x == 0.f && WallNormal.m_y < 0.f ) {
								//	pointvec = c_vector(g_ctx.m_local->get_origin().m_x, g_ctx.m_local->get_origin().m_y + test_al,
								// g_ctx.m_local->get_origin().m_z); 	g_Routes.emplace_back(routes(c_vector(g_ctx.m_local->get_origin().m_x,
								// g_ctx.m_local->get_origin().m_y + test_al, g_ctx.m_local->get_origin().m_z), "ps"));
								Point_bounce.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x, g_ctx.m_local->get_origin( ).m_y + test_al,
								                                     g_ctx.m_local->get_origin( ).m_z + 54.f ) );
								to_muchPoints += 1;
								FirstPoint = false;
							} else if ( WallNormal.m_x < 0 && WallNormal.m_y == 0.f ) {
								//		pointvec = c_vector(g_ctx.m_local->get_origin().m_x + test_al, g_ctx.m_local->get_origin().m_y ,
								// g_ctx.m_local->get_origin().m_z); 		g_Routes.emplace_back(routes(c_vector(g_ctx.m_local->get_origin().m_x+
								// test_al, g_ctx.m_local->get_origin().m_y , g_ctx.m_local->get_origin().m_z), "ps"));
								Point_bounce.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x + test_al, g_ctx.m_local->get_origin( ).m_y,
								                                     g_ctx.m_local->get_origin( ).m_z + 54.f ) );
								to_muchPoints += 1;
								FirstPoint = false;
							} else if ( WallNormal.m_x > 0 && WallNormal.m_y == 0.f ) {
								//	pointvec = c_vector(g_ctx.m_local->get_origin().m_x - test_al, g_ctx.m_local->get_origin().m_y,
								// g_ctx.m_local->get_origin().m_z); 	g_Routes.emplace_back(routes(c_vector(g_ctx.m_local->get_origin().m_x -
								// test_al, g_ctx.m_local->get_origin().m_y, g_ctx.m_local->get_origin().m_z), "ps"));
								Point_bounce.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x - test_al, g_ctx.m_local->get_origin( ).m_y,
								                                     g_ctx.m_local->get_origin( ).m_z + 54.f ) );
								to_muchPoints += 1;
								FirstPoint = false;
							}
						}
					}
					if ( un_pred_velo.m_z < 0.f && pred_velo.m_z < 0.f )
						break;
				}
				if ( done_and_break )
					break;
			}
			cmd->m_buttons      = buttons;
			cmd->m_forward_move = ForwardMove;

			g_prediction.restore_entity_to_predicted_frame( PredictionFrames );
			Lerp = 0.f;
			for ( int i = 0; i < 10000; i++ ) {
				float align                                                       = 15.97803f;
				*( int* )( ( uintptr_t )( c_base_entity* )g_ctx.m_local + 0x25C ) = 2;
				// if (to_muchPoints > 4)
				//{
				//	Points.clear();
				//	return;
				// }

				cmd->m_buttons |= in_jump;
				cmd->m_buttons |= in_duck;

				c_vector Angles{ WallNormal.m_x * -1.f, WallNormal.m_y * -1.f, 0.f };
				c_angle to_wall     = Angles.to_angle( );
				float rotation      = deg2rad( to_wall.m_y - cmd->m_view_point.m_y );
				float cos_rot       = cos( rotation );
				float sin_rot       = sin( rotation );
				float forwardmove   = cos_rot * 10.f;
				float sidemove      = -sin_rot * 10.f;
				cmd->m_forward_move = forwardmove;
				cmd->m_side_move    = sidemove;

				if ( g_ctx.m_local->get_velocity( ).m_z < 0 ) {
					static float Height = 0.f;
					if ( StartPos.m_z > EndPos.m_z ) {
						if ( StartPos.m_z < 0.f )
							Height = ( int )StartPos.m_z - 0.972092f - Lerp;
						else
							Height = ( int )StartPos.m_z + 0.0287018f - Lerp;
					} else {
						if ( EndPos.m_z < 0.f )
							Height = ( int )EndPos.m_z - 0.972092f - Lerp;
						else
							Height = ( int )EndPos.m_z + 0.0287018f - Lerp;
					}
					int maxLerp;
					if ( StartPos.m_z > EndPos.m_z ) {
						maxLerp = StartPos.m_z - EndPos.m_z;
					} else {
						maxLerp = EndPos.m_z - StartPos.m_z;
					}

					if ( Lerp > maxLerp ) {
						cmd->m_buttons      = buttons;
						cmd->m_forward_move = ForwardMove;
						FirstPoint          = false;

						break;
					} else {
						Lerp += 1.f;
					}

					if ( WallNormal.m_x < 0 && WallNormal.m_y < 0.f ) {
						g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x - test_al, StartPos.m_y - test_al, Height );
						g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
					} else if ( WallNormal.m_x < 0 && WallNormal.m_y > 0.f ) {
						g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x - test_al, StartPos.m_y + test_al, Height );
						g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
					} else if ( WallNormal.m_x > 0 && WallNormal.m_y < 0.f ) {
						g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x + test_al, StartPos.m_y - test_al, Height );
						g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
					} else if ( WallNormal.m_x > 0 && WallNormal.m_y > 0.f ) {
						g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x + test_al, StartPos.m_y + test_al, Height );
						g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
					} else if ( WallNormal.m_x == 0.f && WallNormal.m_y > 0.f ) {
						g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x, StartPos.m_y + test_al, Height );
						g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
					} else if ( WallNormal.m_x == 0.f && WallNormal.m_y < 0.f ) {
						g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x, StartPos.m_y - test_al, Height );
						g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
					} else if ( WallNormal.m_x < 0 && WallNormal.m_y == 0.f ) {
						g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x - test_al, StartPos.m_y, Height );
						g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
					} else if ( WallNormal.m_x > 0 && WallNormal.m_y == 0.f ) {
						g_ctx.m_local->get_origin( ) = c_vector( StartPos.m_x + test_al, StartPos.m_y, Height );
						g_ctx.m_local->set_abs_origin( c_vector( StartPos.m_x, StartPos.m_y, Height ) );
					}
					trace_t trace;
					c_trace_filter flt( g_ctx.m_local );
					ray_t ray(
						c_vector( g_ctx.m_local->get_origin( ).m_x, g_ctx.m_local->get_origin( ).m_y, g_ctx.m_local->get_origin( ).m_z + 54.f ),
						c_vector( g_ctx.m_local->get_origin( ).m_x, g_ctx.m_local->get_origin( ).m_y, g_ctx.m_local->get_origin( ).m_z - 1000.f ) );
					g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &flt, &trace );
					c_vector down_point = trace.m_end;
					trace_t trace2;
					c_trace_filter flt2( g_ctx.m_local );
					ray_t ray2(
						c_vector( g_ctx.m_local->get_origin( ).m_x, g_ctx.m_local->get_origin( ).m_y, g_ctx.m_local->get_origin( ).m_z + 54.f ),
						c_vector( g_ctx.m_local->get_origin( ).m_x, g_ctx.m_local->get_origin( ).m_y, g_ctx.m_local->get_origin( ).m_z + 1000.f ) );
					g_interfaces.m_engine_trace->trace_ray( ray2, mask_playersolid, &flt2, &trace2 );
					c_vector upper_point = trace2.m_end;

					if ( g_ctx.m_local->get_origin( ).m_z + 1.f < down_point.m_z )
						continue;
					if ( g_ctx.m_local->get_origin( ).m_z + 1.f > upper_point.m_z )
						continue;
				}

				c_vector un_pred_velo = g_ctx.m_local->get_velocity( );

				g_prediction.begin( g_ctx.m_local, g_ctx.m_cmd );
				g_prediction.end( g_ctx.m_local );

				c_vector pred_velo = g_ctx.m_local->get_velocity( );

				if ( g_ctx.m_local->get_velocity( ).m_z == -6.25f ) {
					if ( WallNormal.m_x < 0 && WallNormal.m_y < 0.f ) {
						//	pointvec = c_vector(g_ctx.m_local->get_origin().m_x + test_al, g_ctx.m_local->get_origin().m_y + test_al,
						// g_ctx.m_local->get_origin().m_z); 	g_Routes.emplace_back(routes(c_vector(g_ctx.m_local->get_origin().m_x +
						// test_al, g_ctx.m_local->get_origin().m_y+ test_al, g_ctx.m_local->get_origin().m_z), "ps"));
						Points.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x + test_al, g_ctx.m_local->get_origin( ).m_y + test_al,
						                               g_ctx.m_local->get_origin( ).m_z ) );
						to_muchPoints += 1;
						FirstPoint = false;
					} else if ( WallNormal.m_x < 0 && WallNormal.m_y > 0.f ) {
						//	pointvec = c_vector(g_ctx.m_local->get_origin().m_x + test_al, g_ctx.m_local->get_origin().m_y - test_al,
						// g_ctx.m_local->get_origin().m_z); 	g_Routes.emplace_back(routes(c_vector(g_ctx.m_local->get_origin().m_x +
						// test_al, g_ctx.m_local->get_origin().m_y - test_al, g_ctx.m_local->get_origin().m_z), "ps"));
						Points.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x + test_al, g_ctx.m_local->get_origin( ).m_y - test_al,
						                               g_ctx.m_local->get_origin( ).m_z ) );
						to_muchPoints += 1;
						FirstPoint = false;
					} else if ( WallNormal.m_x > 0 && WallNormal.m_y < 0.f ) {
						//	pointvec = c_vector(g_ctx.m_local->get_origin().m_x - test_al, g_ctx.m_local->get_origin().m_y + test_al,
						// g_ctx.m_local->get_origin().m_z); 	g_Routes.emplace_back(routes(c_vector(g_ctx.m_local->get_origin().m_x -
						// test_al, g_ctx.m_local->get_origin().m_y + test_al, g_ctx.m_local->get_origin().m_z), "ps"));
						Points.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x - test_al, g_ctx.m_local->get_origin( ).m_y + test_al,
						                               g_ctx.m_local->get_origin( ).m_z ) );
						to_muchPoints += 1;
						FirstPoint = false;
					} else if ( WallNormal.m_x > 0 && WallNormal.m_y > 0.f ) {
						//	pointvec = c_vector(g_ctx.m_local->get_origin().m_x - test_al, g_ctx.m_local->get_origin().m_y - test_al,
						// g_ctx.m_local->get_origin().m_z); 	g_Routes.emplace_back(routes(c_vector(g_ctx.m_local->get_origin().m_x -
						// test_al, g_ctx.m_local->get_origin().m_y - test_al, g_ctx.m_local->get_origin().m_z), "ps"));
						Points.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x - test_al, g_ctx.m_local->get_origin( ).m_y - test_al,
						                               g_ctx.m_local->get_origin( ).m_z ) );
						to_muchPoints += 1;
						FirstPoint = false;
					} else if ( WallNormal.m_x == 0.f && WallNormal.m_y > 0.f ) {
						//	pointvec = c_vector(g_ctx.m_local->get_origin().m_x, g_ctx.m_local->get_origin().m_y - test_al,
						// g_ctx.m_local->get_origin().m_z); 	g_Routes.emplace_back(routes(c_vector(g_ctx.m_local->get_origin().m_x,
						// g_ctx.m_local->get_origin().m_y - test_al, g_ctx.m_local->get_origin().m_z), "ps"));
						Points.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x, g_ctx.m_local->get_origin( ).m_y - test_al,
						                               g_ctx.m_local->get_origin( ).m_z ) );
						to_muchPoints += 1;
						FirstPoint = false;
					} else if ( WallNormal.m_x == 0.f && WallNormal.m_y < 0.f ) {
						//	pointvec = c_vector(g_ctx.m_local->get_origin().m_x, g_ctx.m_local->get_origin().m_y + test_al,
						// g_ctx.m_local->get_origin().m_z); 	g_Routes.emplace_back(routes(c_vector(g_ctx.m_local->get_origin().m_x,
						// g_ctx.m_local->get_origin().m_y + test_al, g_ctx.m_local->get_origin().m_z), "ps"));
						Points.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x, g_ctx.m_local->get_origin( ).m_y + test_al,
						                               g_ctx.m_local->get_origin( ).m_z ) );
						to_muchPoints += 1;
						FirstPoint = false;
					} else if ( WallNormal.m_x < 0 && WallNormal.m_y == 0.f ) {
						//		pointvec = c_vector(g_ctx.m_local->get_origin().m_x + test_al, g_ctx.m_local->get_origin().m_y ,
						// g_ctx.m_local->get_origin().m_z); 		g_Routes.emplace_back(routes(c_vector(g_ctx.m_local->get_origin().m_x+
						// test_al, g_ctx.m_local->get_origin().m_y , g_ctx.m_local->get_origin().m_z), "ps"));
						Points.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x + test_al, g_ctx.m_local->get_origin( ).m_y,
						                               g_ctx.m_local->get_origin( ).m_z ) );
						to_muchPoints += 1;
						FirstPoint = false;
					} else if ( WallNormal.m_x > 0 && WallNormal.m_y == 0.f ) {
						//	pointvec = c_vector(g_ctx.m_local->get_origin().m_x - test_al, g_ctx.m_local->get_origin().m_y,
						// g_ctx.m_local->get_origin().m_z); 	g_Routes.emplace_back(routes(c_vector(g_ctx.m_local->get_origin().m_x -
						// test_al, g_ctx.m_local->get_origin().m_y, g_ctx.m_local->get_origin().m_z), "ps"));
						Points.emplace_back( c_vector( g_ctx.m_local->get_origin( ).m_x - test_al, g_ctx.m_local->get_origin( ).m_y,
						                               g_ctx.m_local->get_origin( ).m_z ) );
						to_muchPoints += 1;
						FirstPoint = false;
					}
				}
			}

			cmd->m_buttons      = buttons;
			cmd->m_forward_move = ForwardMove;
			FirstPoint          = false;
			if ( Points.size( ) > 0 ) {
				for ( int i = 0; i < Points.size( ) - 1; ) {
					if ( ( int )Points[ i ].m_z - ( int )Points[ i + 1 ].m_z == 1 ) {
						Points.erase( Points.begin( ) + i );
					} else {
						++i;
					}
				}
			}
		}
	} else {
		// Отрисовка точек, когда cmd равен nullptr
		UpdateAndRenderPoints( Points );
		UpdateAndRenderPoints2( Point_bounce );
		// Отрисовка линии между StartPos и EndPos при зажатой клавише
		if ( g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_finder_key, key_bind_t ) ) ) {
			if ( !StartPos.is_zero( ) && !EndPos.is_zero( ) ) {
				c_vector_2d start2D, end2D;
				if ( g_render.world_to_screen( StartPos, start2D ) && g_render.world_to_screen( EndPos, end2D ) ) {
					// Параметры
					float outline_thickness = 3.f;                            // Толщина линии для контура
					c_color outline_color   = c_color( 0.f, 0.f, 0.f, 0.5f ); // Черный с прозрачностью 50%
					float s                 = 10.f;                           // Длина одного сегмента в пикселях
					float speed             = 50.f;                           // Скорость анимации в пикселях/сек

					// Вычисление длины линии в экранных координатах
					float l = start2D.distance( end2D );

					if ( l > 0.f ) {
						// Накопление смещения для анимации
						static float offset = 0.f;
						offset              = fmod( offset + speed * ImGui::GetIO( ).DeltaTime, 2 * s );

						// Отрисовка контура (толстая черная линия)
						g_render.m_draw_data.emplace_back(
							e_draw_type::draw_type_line,
							std::make_any< line_draw_object_t >( start2D, end2D, outline_color.get_u32( ), outline_thickness ) );

						// Вычисление точек перехода между сегментами
						std::vector< float > transitions;
						transitions.push_back( 0.f );

						int m_start = static_cast< int >( ceil( offset / s ) );
						for ( int m = m_start;; m++ ) {
							float d = -offset + m * s;
							if ( d > l )
								break;
							float u = d / l;
							if ( u >= 0.f && u <= 1.f ) {
								transitions.push_back( u );
							}
						}
						transitions.push_back( 1.f );

						// Отрисовка цветных сегментов
						for ( size_t i = 0; i < transitions.size( ) - 1; i++ ) {
							float u0 = transitions[ i ];
							float u1 = transitions[ i + 1 ];
							if ( u1 > u0 ) {
								c_vector_2d p0 = start2D + ( end2D - start2D ) * u0; // Работает с существующим оператором
								c_vector_2d p1 = start2D + ( end2D - start2D ) * u1; // Работает с существующим оператором
								// Определение цвета сегмента
								float u_mid   = ( u0 + u1 ) / 2.f;
								float d_mid   = u_mid * l;
								int phase     = static_cast< int >( floor( ( d_mid + offset ) / s ) ) % 2;
								c_color color = ( phase == 0 ) ? c_color( 1.f, 1.f, 1.f, 1.f )     // Белый
								                               : c_color( 0.5f, 0.5f, 0.5f, 1.f ); // Серый

								// Отрисовка сегмента
								g_render.m_draw_data.emplace_back( e_draw_type::draw_type_line,
								                                   std::make_any< line_draw_object_t >( p0, p1, color.get_u32( ), 1.f ) );
							}
						}
					}
				}
			}
		}
	}
}

bool should_align{ };
bool hb = false;
bool should_ps_standing{ };

float difference( float a, float b )
{
	return std::max( abs( a ), abs( b ) ) - std::min( abs( a ), abs( b ) );
}

void n_movement::impl_t::auto_align( c_user_cmd* cmd )
{
	if ( const auto mt = g_ctx.m_local->get_move_type( ); mt == e_move_types::move_type_ladder || mt == e_move_types::move_type_noclip ) {
		should_ps_standing = false;
		return;
	}
	if ( g_input.check_input( &GET_VARIABLE( g_variables.m_ladder_bug_key, key_bind_t ) ) )
		return;
	static float start_circle = 0.f;
	float max_radias          = std::numbers::pi_v< float > * 2.f;
	float step                = max_radias / 16.f;
	c_vector start_pos        = g_ctx.m_local->get_abs_origin( );
	const auto mins           = g_ctx.m_local->get_collideable( )->get_obb_mins( );
	const auto maxs           = g_ctx.m_local->get_collideable( )->get_obb_maxs( );
	c_trace_filter_world fil;
	wall_detected = false;
	trace_t trace;
	c_vector save_start_pos = { };
	c_vector save_end_pos   = { };
	float saved_cos         = 0.f;
	float saved_sin         = 0.f;
	for ( float a = start_circle; a < max_radias; a += step ) {
		c_vector end_pos;
		end_pos.m_x = cos( a ) + start_pos.m_x;
		end_pos.m_y = sin( a ) + start_pos.m_y;
		end_pos.m_z = start_pos.m_z;
		saved_cos   = cos( a ) * 64.f;
		saved_sin   = sin( a ) * 64.f;
		ray_t ray( start_pos, end_pos, mins, maxs );

		// traceraying the whole circle around us trying to find a wall
		g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &fil, &trace );
		if ( trace.m_fraction != 1.f && trace.m_plane.m_normal.m_z == 0.f ) {
			wall_detected  = true;
			start_circle   = a;
			save_start_pos = start_pos;
			save_end_pos   = end_pos;
			break;
		}
	}
	if ( !wall_detected ) {
		start_circle = 0.f;
		return;
	}
	c_vector normal_plane = c_vector( trace.m_plane.m_normal.m_x * -0.005f, trace.m_plane.m_normal.m_y * -0.005f, 0.f );

	const c_vector end_pos2 = c_vector( save_start_pos.m_x, save_start_pos.m_y, trace.m_end.m_z ) + c_vector( saved_cos, saved_sin, 0.f );
	c_angle wall_angle      = normal_plane.to_angle2( );
	wall_angle.normalize( );

	float rotation = deg2rad( wall_angle.m_y - cmd->m_view_point.m_y );
	float cos_rot  = cos( rotation );
	float sin_rot  = sin( rotation );
	bool detect    = false;
	trace_t trace_2;
	ray_t ray_2( c_vector( save_start_pos.m_x, save_start_pos.m_y, trace.m_end.m_z ), end_pos2 );
	g_interfaces.m_engine_trace->trace_ray( ray_2, mask_playersolid, &fil, &trace_2 );

	float backup_forward_move                   = cmd->m_forward_move;
	float backup_side_move                      = cmd->m_side_move;
	constexpr static float distance_till_adjust = 0.03125f;
	constexpr auto has_to_align                 = []( const c_vector& origin ) -> bool {
        constexpr static float distance_to_stop = 0.00750f;

        const c_vector_2d remainder1 = c_vector_2d( 1.f - ( origin.m_x - floor( origin.m_x ) ), 1.f - ( origin.m_y - floor( origin.m_y ) ) );
        const c_vector_2d remainder2 = c_vector_2d( ( origin.m_x - floor( origin.m_x ) ), ( origin.m_y - floor( origin.m_y ) ) );

        return ( ( remainder1.m_x >= distance_to_stop && remainder1.m_x <= distance_till_adjust ) ||
                 ( remainder1.m_y >= distance_to_stop && remainder1.m_y <= distance_till_adjust ) ) ||
               ( ( remainder2.m_x >= distance_to_stop && remainder2.m_x <= distance_till_adjust ) ||
                 ( remainder2.m_y >= distance_to_stop && remainder2.m_y <= distance_till_adjust ) );
	};
	for ( float multiplayer = 0.f; multiplayer < 100.f; multiplayer += 10.f ) {
		g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );

		float forwardmove   = cos_rot * multiplayer;
		float sidemove      = -sin_rot * multiplayer;
		cmd->m_forward_move = forwardmove;
		cmd->m_side_move    = sidemove;
		float backup_zspeed = g_ctx.m_local->get_velocity( ).m_z;

		g_prediction.begin( g_ctx.m_local, g_ctx.m_cmd );
		g_prediction.end( g_ctx.m_local );
		float new_zspeed = g_ctx.m_local->get_velocity( ).m_z;
		if ( new_zspeed == -6.25f ) {
			cmd->m_forward_move = forwardmove;
			cmd->m_side_move    = sidemove;
			detect              = true;
			break;
		}
	}
	if ( !detect ) {
		float forwardmove   = cos_rot * 10.f;
		float sidemove      = -sin_rot * 10.f;
		cmd->m_forward_move = forwardmove;
		cmd->m_side_move    = sidemove;
	}

	float if_not_slide_fw = cmd->m_forward_move;
	float if_not_slide_sw = cmd->m_side_move;
	if ( !has_to_align( g_prediction.backup_data.m_origin ) && g_prediction.backup_data.m_velocity.m_z != -6.25f ) {
		if ( ( cmd->m_buttons & in_forward ) || ( cmd->m_buttons & in_back ) || ( cmd->m_buttons & in_moveleft ) ||
		     ( cmd->m_buttons & in_moveright ) ) {
			cmd->m_forward_move = backup_forward_move;
			cmd->m_side_move    = backup_side_move;
		}
	}
	if ( g_prediction.backup_data.m_velocity.m_z == -6.25f ) {
		c_vector wishdir;

		bool done = false;

		if ( ( cmd->m_buttons & in_forward ) && !( cmd->m_buttons & in_back ) && !( cmd->m_buttons & in_moveleft ) &&
		     !( cmd->m_buttons & in_moveright ) ) {
			wishdir = { std::cos( deg2rad( cmd->m_view_point.m_y ) ) * 128.f, std::sin( deg2rad( cmd->m_view_point.m_y ) ) * 128.f, 0.f };
			done    = true;
		}
		if ( ( cmd->m_buttons & in_back ) && !( cmd->m_buttons & in_forward ) && !( cmd->m_buttons & in_moveleft ) &&
		     !( cmd->m_buttons & in_moveright ) ) {
			wishdir = { std::cos( deg2rad( cmd->m_view_point.m_y + 180.f ) ) * 128.f, std::sin( deg2rad( cmd->m_view_point.m_y + 180.f ) ) * 128.f,
				        0.f };
			done    = true;
		}
		if ( ( cmd->m_buttons & in_moveleft ) && !( cmd->m_buttons & in_back ) && !( cmd->m_buttons & in_forward ) &&
		     !( cmd->m_buttons & in_moveright ) ) {
			wishdir = { std::cos( deg2rad( cmd->m_view_point.m_y + 90.f ) ) * 128.f, std::sin( deg2rad( cmd->m_view_point.m_y + 90.f ) ) * 128.f,
				        0.f };
			done    = true;
		}
		if ( ( cmd->m_buttons & in_moveright ) && !( cmd->m_buttons & in_back ) && !( cmd->m_buttons & in_moveleft ) &&
		     !( cmd->m_buttons & in_forward ) ) {
			wishdir = { std::cos( deg2rad( cmd->m_view_point.m_y - 90.f ) ) * 128.f, std::sin( deg2rad( cmd->m_view_point.m_y - 90.f ) ) * 128.f,
				        0.f };
			done    = true;
		}
		if ( done ) {
			trace_t trace_4;
			c_vector st = c_vector( save_start_pos.m_x, save_start_pos.m_y, trace.m_end.m_z );
			ray_t ray_4( st, st + wishdir );
			g_interfaces.m_engine_trace->trace_ray( ray_4, mask_playersolid, &fil, &trace_4 );
			if ( trace_4.m_fraction < 1.f ) {
				int buttons_2         = cmd->m_buttons;
				float forwardmove_2   = cmd->m_forward_move;
				float sidemove_2      = cmd->m_side_move;
				int i_backup_velo     = g_ctx.m_local->get_velocity( ).length_2d( );
				bool do_surf_detected = false;
				for ( int i = 0; i < 450; i += 45 ) {
					g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
					if ( buttons_2 & in_forward )
						cmd->m_forward_move = i;
					if ( buttons_2 & in_back )
						cmd->m_forward_move = -i;
					if ( buttons_2 & in_moveleft )
						cmd->m_side_move = -i;
					if ( buttons_2 & in_moveright )
						cmd->m_side_move = i;

					g_prediction.begin( g_ctx.m_local, g_ctx.m_cmd );
					g_prediction.end( g_ctx.m_local );
					float zvelo = g_ctx.m_local->get_velocity( ).m_z;
					if ( zvelo == -6.25f ) {
						forwardmove_2 = cmd->m_forward_move;
						sidemove_2    = cmd->m_side_move;
					}
				}

				int i_preed_velo = g_ctx.m_local->get_velocity( ).length_2d( );

				cmd->m_forward_move = forwardmove_2;
				cmd->m_side_move    = sidemove_2;

				if ( g_prediction.backup_data.m_velocity.length_2d( ) > 250.f ) {
					std::vector< c_vector_2d > direction{ };
					std::vector< float > max_speed{ };
					if ( difference( i_backup_velo, i_preed_velo ) < 5 ) {
						for ( float angle = 15.f; angle < 30.f; angle += 5.f ) {
							g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
							float mVel         = hypotf( g_prediction.backup_data.m_velocity.m_x, g_prediction.backup_data.m_velocity.m_y );
							float ideal        = rad2deg( atanf( angle / mVel ) );
							c_vector dvelo     = g_ctx.m_local->get_velocity( );
							dvelo.m_z          = 0.f;
							c_angle velo_angle = dvelo.to_angle2( );
							c_angle delta =
								velo_angle - c_vector( trace.m_plane.m_normal.m_x * -1.f, trace.m_plane.m_normal.m_y * -1.f, 0.f ).to_angle2( );
							delta.normalize( );
							if ( delta.m_y >= 0.f )
								wall_angle.m_y += ideal;
							else
								wall_angle.m_y -= ideal;
							float rotation2     = deg2rad( wall_angle.m_y - cmd->m_view_point.m_y );
							float cos_rot2      = cos( rotation2 );
							float sin_rot2      = sin( rotation2 );
							float forwardmove2  = cos_rot2 * 450.f;
							float sidemove2     = -sin_rot2 * 450.f;
							cmd->m_forward_move = forwardmove2;
							cmd->m_side_move    = sidemove2;
							c_vector b_velo     = g_ctx.m_local->get_velocity( );
							g_prediction.begin( g_ctx.m_local, cmd );
							g_prediction.end( g_ctx.m_local );
							c_vector p_velo = g_ctx.m_local->get_velocity( );
							if ( b_velo.m_z == -6.25f && p_velo.m_z == -6.25f ) {
								if ( p_velo.length_2d( ) > b_velo.length_2d( ) ) {
									direction.emplace_back( c_vector_2d( forwardmove2, sidemove2 ) );
									max_speed.emplace_back( p_velo.length_2d( ) - b_velo.length_2d( ) );
								}
							}
						}
					}
					cmd->m_forward_move = forwardmove_2;
					cmd->m_side_move    = sidemove_2;
					float mxsp          = 0;
					int index_max_speed = -1;
					for ( int k = 0; k < max_speed.size( ); k++ ) {
						if ( max_speed.at( k ) > mxsp ) {
							index_max_speed = k;
							mxsp            = max_speed.at( k );
						}
					}
					g_console.print( std::format( "Velocity gained: {}", mxsp ).c_str( ) );
					if ( index_max_speed != -1 ) {
						cmd->m_forward_move = direction.at( index_max_speed ).m_x;
						cmd->m_side_move    = direction.at( index_max_speed ).m_y;
					}
				}
				if ( difference( trace_4.m_end.m_x, trace_2.m_end.m_x ) > 1.f && difference( trace_4.m_end.m_y, trace_2.m_end.m_y ) > 1.f ) {
					if ( cmd->m_forward_move < 0.f && cmd->m_buttons & in_forward )
						cmd->m_forward_move = 450.f;
					if ( cmd->m_forward_move > 0.f && cmd->m_buttons & in_back )
						cmd->m_forward_move = -450.f;
					if ( cmd->m_side_move < 0.f && cmd->m_buttons & in_moveright )
						cmd->m_side_move = 450.f;
					if ( cmd->m_side_move > 0.f && cmd->m_buttons & in_moveleft )
						cmd->m_side_move = -450.f;
				}

			} else {
				if ( cmd->m_forward_move < 0.f && cmd->m_buttons & in_forward )
					cmd->m_forward_move = 450.f;
				if ( cmd->m_forward_move > 0.f && cmd->m_buttons & in_back )
					cmd->m_forward_move = -450.f;
				if ( cmd->m_side_move < 0.f && cmd->m_buttons & in_moveright )
					cmd->m_side_move = 450.f;
				if ( cmd->m_side_move > 0.f && cmd->m_buttons & in_moveleft )
					cmd->m_side_move = -450.f;
			}
		}
	}
}

void n_movement::impl_t::fire_man( c_user_cmd* cmd )
{
	m_fireman_data.is_ladder = false;

	// Предварительные проверки.
	if ( !g_ctx.m_local )
		return;
	if ( !g_interfaces.m_engine_client->is_in_game( ) )
		return;
	if ( !g_ctx.m_local->is_alive( ) )
		return;

	if ( !GET_VARIABLE( g_variables.m_fire_man, bool ) || !g_input.check_input( &GET_VARIABLE( g_variables.m_fire_man_key, key_bind_t ) ) ) {
		m_fireman_data.fr_hit   = false;
		m_fireman_data.fr_hit_1 = false;
		return;
	}

	if ( m_fireman_data.fr_hit_1 ) {
		if ( g_prediction.backup_data.m_move_type == move_type_ladder ) {
			cmd->m_buttons |= in_jump;
			cmd->m_forward_move = 0.f;
			cmd->m_side_move    = 0.f;
		}
	}

	auto colidable = g_ctx.m_local->get_collideable( );
	if ( !colidable )
		return;

	// Восстановление предыдущего состояния.
	g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
	m_fireman_data.awall = false;
	trace_t trace;
	float step                = std::numbers::pi_v< float > * 2.0f / 16.f;
	static float start_circle = 0.f;

	if ( g_ctx.m_local->get_velocity( ).m_z != -6.25f && g_prediction.backup_data.m_velocity != -6.25f )
		start_circle = 0.f;

	// --- Этап поиска стены: только поиск, без выполнения основной логики ---
	bool foundWall   = false;
	float foundAngle = 0.f;
	c_vector wall_position;

	for ( float a = start_circle; a < std::numbers::pi_v< float > * 2.0f; a += step ) {
		c_vector wishdir( cosf( a ), sinf( a ), 0.f );
		auto start_pos = g_ctx.m_local->get_abs_origin( );
		auto end_pos   = start_pos + wishdir;
		c_trace_filter flt( g_ctx.m_local );
		ray_t ray( start_pos, end_pos, colidable->get_obb_mins( ), colidable->get_obb_maxs( ) );

		g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &flt, &trace );

		if ( trace.m_fraction < 1.f && trace.m_plane.m_normal.m_z < 0.4f ) {
			wall_position = trace.m_end;
			foundWall     = true;
			foundAngle    = a;
			break; // Выходим из цикла сразу после нахождения подходящей стены.
		}
	}

	// --- Если стена найдена, выполняем основную логику (ladder, предсказание, перемещение) ---
	if ( foundWall ) {
		m_fireman_data.awall   = true;
		int commands_predicted = g_interfaces.m_prediction->m_commands_predicted;

		// Вычисление направления стены.
		c_vector normal_plane( trace.m_plane.m_normal.m_x * -1.f, trace.m_plane.m_normal.m_y * -1.f, 0.f );
		c_angle wall_angle = normal_plane.to_angle( );
		c_vector d_velo    = g_ctx.m_local->get_velocity( );
		d_velo.m_z         = 0.f;
		c_angle velo_angle = d_velo.to_angle( );
		c_angle delta      = velo_angle - wall_angle;
		delta.normalize( );
		float rotation            = deg2rad( wall_angle.m_y - cmd->m_view_point.m_y );
		float cos_rot             = cos( rotation );
		float sin_rot             = sin( rotation );
		float multiplayer         = 450.f;
		float backup_forward_move = cmd->m_forward_move;
		float backup_side_move    = cmd->m_side_move;
		float forward_move        = cos_rot * multiplayer;
		float side_move           = -sin_rot * multiplayer;
		int backup_buttons        = cmd->m_buttons;

		g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
		cmd->m_forward_move = forward_move;
		cmd->m_side_move    = side_move;
		cmd->m_buttons &= ~in_jump;

		g_prediction.begin( g_ctx.m_local, g_ctx.m_cmd );
		g_prediction.end( g_ctx.m_local );

		// Проверка на ladder.
		if ( !m_fireman_data.is_ladder ) {
			if ( ( g_prediction.backup_data.m_move_type != move_type_ladder && g_ctx.m_local->get_move_type( ) == move_type_ladder ) ||
			     g_prediction.backup_data.m_move_type == move_type_ladder ) {
				m_fireman_data.is_ladder = true;
				start_circle             = foundAngle;
			} else {
				cmd->m_buttons      = backup_buttons;
				cmd->m_forward_move = backup_forward_move;
				cmd->m_side_move    = backup_side_move;
			}
		}

		if ( m_fireman_data.is_ladder ) {
			g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
			int old_flags       = g_ctx.m_local->get_flags( );
			int old_move_type   = g_ctx.m_local->get_move_type( );
			cmd->m_forward_move = 0.f;
			cmd->m_side_move    = 0.f;

			if ( !m_fireman_data.fr_hit && !m_fireman_data.fr_hit_1 ) {
				for ( int i = 0; i < 12; i++ ) {
					g_prediction.begin( g_ctx.m_local, g_ctx.m_cmd );
					g_prediction.end( g_ctx.m_local );
					if ( !( old_flags & 1 ) && ( g_ctx.m_local->get_flags( ) & 1 ) ) {
						if ( i < 10 ) {
							m_fireman_data.fr_hit_1 = true;
							break;
						}
						m_fireman_data.fr_hit = true;
					}
				}
			}
			if ( !m_fireman_data.fr_hit_1 ) {
				if ( !m_fireman_data.fr_hit ) {
					cmd->m_buttons &= ~in_jump;
					cmd->m_forward_move = 0.f;
					cmd->m_side_move    = 0.f;
				} else {
					if ( g_prediction.backup_data.m_move_type != move_type_ladder ) {
						cmd->m_forward_move = forward_move;
						cmd->m_side_move    = side_move;
						cmd->m_buttons |= in_jump;
					} else {
						cmd->m_forward_move = 0.f;
						cmd->m_side_move    = 0.f;
						cmd->m_buttons |= in_jump;
					}
				}
			} else {
				g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
				int oldflags        = g_ctx.m_local->get_flags( );
				int oldMoveType     = g_ctx.m_local->get_move_type( );
				cmd->m_forward_move = 0.f;
				cmd->m_side_move    = 0.f;

				g_prediction.begin( g_ctx.m_local, g_ctx.m_cmd );
				g_prediction.end( g_ctx.m_local );

				if ( !( oldflags & 1 ) && ( g_ctx.m_local->get_flags( ) & 1 ) ) {
					cmd->m_forward_move = forward_move;
					cmd->m_side_move    = side_move;
					cmd->m_buttons &= ~in_jump;
				} else {
					cmd->m_buttons |= in_jump;
					cmd->m_forward_move = 0.f;
					cmd->m_side_move    = 0.f;
				}
			}
		} else {
			m_fireman_data.fr_hit   = false;
			m_fireman_data.fr_hit_1 = false;
		}
	}
}
struct fake_user_move {
	float sidemove     = 0.f;
	float forwardmove  = 0.f;
	int buttons        = 0;
	c_angle viewangles = c_angle( 0.f, 0.f, 0.f );
};
fake_user_move g_moveuser[ 1000 ];
void n_movement::impl_t::ladder_bug( c_user_cmd* cmd )
{
	m_ladder_bug_data.founded = false;
	static int ticks_to_stop  = 0;
	if ( !g_ctx.m_local )
		return;
	if ( !g_interfaces.m_engine_client->is_in_game( ) )
		return;
	if ( !g_ctx.m_local->is_alive( ) )
		return;
	if ( !GET_VARIABLE( g_variables.m_ladder_bug, bool ) || !g_input.check_input( &GET_VARIABLE( g_variables.m_ladder_bug_key, key_bind_t ) ) ) {
		ticks_to_stop = 0;
		return;
	}
	auto cvar = g_interfaces.m_convar->find_var( "sv_autobunnyhopping" );
	if ( cvar->get_int( ) == 0 ) {
		cvar->set_value( 1 );
		return;
	}

	c_vector_2d direction_move = c_vector_2d( cmd->m_forward_move, cmd->m_side_move );
	c_vector_2d direction_view = c_vector_2d( cmd->m_view_point.m_x, cmd->m_view_point.m_y );
	int buttons                = cmd->m_buttons;
	bool detected_in_predicted = false;
	bool jump_do_it            = false;

	int max_ground = 0;

	g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
	int ticks_to_jb     = 0;
	int ticks_on_ladder = 0;
	int max_on_ladder   = 0;
	if ( !m_ladder_bug_data.founded && g_ctx.m_local->get_move_type( ) != move_type_ladder && !( g_ctx.m_local->get_flags( ) & 1 ) ) {
		g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
		for ( int i = 0; i < 64; i++ ) {
			{
				int old_flags         = g_ctx.m_local->get_flags( );
				c_vector old_velocity = g_ctx.m_local->get_velocity( );
				c_vector old_origin   = g_ctx.m_local->get_abs_origin( );
				int old_move_type     = g_ctx.m_local->get_move_type( );

				if ( !jump_do_it && old_move_type == move_type_ladder )
					break;
				if ( jump_do_it && old_move_type != move_type_ladder )
					break;

				if ( old_flags & 1 ) {
					cmd->m_buttons |= in_jump;
					jump_do_it = true;
				} else {
					cmd->m_buttons &= ~in_jump;
				}
				if ( jump_do_it ) {
					cmd->m_buttons |= in_jump;

					cmd->m_forward_move = direction_move.m_x;
					cmd->m_side_move    = direction_move.m_y;

				} else {
					cmd->m_forward_move = 0.f;
					cmd->m_side_move    = 0.f;
				}

				g_prediction.begin( g_ctx.m_local, cmd );
				g_prediction.end( g_ctx.m_local );
				int new_flags         = g_ctx.m_local->get_flags( );
				c_vector new_velocity = g_ctx.m_local->get_velocity( );
				c_vector new_origin   = g_ctx.m_local->get_abs_origin( );
				int new_move_type     = g_ctx.m_local->get_move_type( );
				if ( old_move_type == move_type_ladder && new_velocity.m_z == old_velocity.m_z ) {
					if ( old_velocity.m_z > 0.f ) {
						if ( max_on_ladder > 10 ) {
							m_ladder_bug_data.founded = false;
							ticks_to_stop             = 0;
							break;
						}
						m_ladder_bug_data.founded = true;
						ticks_to_stop             = i;

						max_on_ladder++;
					}
				}
				g_moveuser[ i ].sidemove    = cmd->m_side_move;
				g_moveuser[ i ].forwardmove = cmd->m_forward_move;
				g_moveuser[ i ].buttons     = cmd->m_buttons;
				g_moveuser[ i ].viewangles  = cmd->m_view_point;
			}
		}

		cmd->m_forward_move   = direction_move.m_x;
		cmd->m_side_move      = direction_move.m_y;
		cmd->m_buttons        = buttons;
		cmd->m_view_point.m_x = direction_view.m_x;
		cmd->m_view_point.m_y = direction_view.m_y;
	}
	if ( m_ladder_bug_data.founded ) {
		g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
		static int Hits       = 0;
		static bool hold_jump = false;
		if ( Hits > ticks_to_stop ) {
			Hits                      = 0;
			ticks_to_stop             = 0;
			m_ladder_bug_data.founded = false;
			hold_jump                 = false;
			return;
		}
		{
			cmd->m_buttons |= in_jump;
			cmd->m_side_move    = g_moveuser[ Hits ].sidemove;
			cmd->m_forward_move = g_moveuser[ Hits ].forwardmove;
		}

		Hits += 1;
	}
}

void n_movement::impl_t::anti_ladder( c_user_cmd* cmd )
{
	if ( !g_ctx.m_local )
		return;
	if ( !g_interfaces.m_engine_client->is_in_game( ) )
		return;
	if ( !g_ctx.m_local->is_alive( ) )
		return;

	if ( !GET_VARIABLE( g_variables.m_ladder_bug, bool ) || !g_input.check_input( &GET_VARIABLE( g_variables.m_ladder_bug_key, key_bind_t ) ) ) {
		return;
	}
	static int ticks_on_ladder = 0;

	if ( g_prediction.backup_data.m_move_type == move_type_ladder ) {
		if ( ticks_on_ladder > 10 ) {
			c_vector_2d direction_move = c_vector_2d( cmd->m_forward_move, cmd->m_side_move );
			for ( int i = 0; i < 8; i++ ) {
				g_prediction.restore_entity_to_predicted_frame( g_interfaces.m_prediction->m_commands_predicted - 1 );
				int old_move_type = g_ctx.m_local->get_move_type( );
				if ( i == 0 ) {
					cmd->m_forward_move = 450.f;
					cmd->m_side_move    = 0.f;
				} else if ( i == 1 ) {
					cmd->m_forward_move = -450.f;
					cmd->m_side_move    = 0.f;
				} else if ( i == 2 ) {
					cmd->m_forward_move = 0.f;
					cmd->m_side_move    = 450.f;
				} else if ( i == 3 ) {
					cmd->m_forward_move = 0.f;
					cmd->m_side_move    = -450.f;
				} else if ( i == 4 ) {
					cmd->m_forward_move = 0.f;
					cmd->m_side_move    = 0.f;
				} else if ( i == 5 ) {
					cmd->m_forward_move = 450.f;
					cmd->m_side_move    = -450.f;
				} else if ( i == 6 ) {
					cmd->m_side_move    = 450.f;
					cmd->m_forward_move = -450.f;
				} else if ( i == 7 ) {
					cmd->m_forward_move = 450.f;
					cmd->m_side_move    = 450.f;
				}
				g_prediction.begin( g_ctx.m_local, cmd );
				g_prediction.end( g_ctx.m_local );
				int new_move_type = g_ctx.m_local->get_move_type( );
				if ( old_move_type == move_type_ladder && new_move_type != move_type_ladder ) {
					direction_move = c_vector_2d( cmd->m_forward_move, cmd->m_side_move );
				}
			}
			cmd->m_forward_move = direction_move.m_x;
			cmd->m_side_move    = direction_move.m_y;
		}
		ticks_on_ladder++;
	} else {
		ticks_on_ladder = 0;
	}
}

void n_movement::impl_t::strafe_to_yaw( c_user_cmd* cmd, c_angle& angle, const float yaw )
{
	const static auto max_side_speed = g_convars[ HASH_BT( "cl_sidespeed" ) ]->get_float( );

	angle.m_y += yaw;
	cmd->m_side_move    = 0.f;
	cmd->m_forward_move = 0.f;

	const auto degrees = rad2deg( std::atan2f( g_ctx.m_local->get_velocity( ).m_y, g_ctx.m_local->get_velocity( ).m_x ) );

	const auto delta = g_math.normalize_angle( angle.m_y - degrees );

	cmd->m_side_move = delta > 0.f ? -max_side_speed : max_side_speed;
	angle.m_y        = g_math.normalize_angle( angle.m_y - delta );
}

void n_movement::impl_t::detect_edgebug( c_user_cmd* cmd )
{
	if ( g_prediction.backup_data.m_velocity.m_z > 0 || g_utilities.is_in< int >( g_ctx.m_local->get_move_type( ), invalid_move_types ) ) {
		m_edgebug_data.m_will_edgebug = false;
		m_edgebug_data.m_will_fail    = true;
		return;
	}

	if ( round( g_ctx.m_local->get_velocity( ).m_z ) == 0 || g_prediction.backup_data.m_flags & fl_onground ) {
		m_edgebug_data.m_will_edgebug = false;
		m_edgebug_data.m_will_fail    = true;
	} else if ( g_prediction.backup_data.m_velocity.m_z < -6.f && g_ctx.m_local->get_velocity( ).m_z > g_prediction.backup_data.m_velocity.m_z &&
	            g_ctx.m_local->get_velocity( ).m_z < -6.f && !( g_ctx.m_local->get_flags( ) & fl_onground ) &&
	            g_prediction.backup_data.m_origin.m_z > g_ctx.m_local->get_abs_origin( ).m_z ) {
		const auto gravity = g_convars[ HASH_BT( "sv_gravity" ) ]->get_float( );

		if ( std::floor( g_prediction.backup_data.m_velocity.m_z ) < -7 && std::floor( g_ctx.m_local->get_velocity( ).m_z ) == -7 &&
		     g_ctx.m_local->get_velocity( ).length_2d( ) >= g_prediction.backup_data.m_velocity.length_2d( ) ) {
			m_edgebug_data.m_will_edgebug = true;
			m_edgebug_data.m_will_fail    = false;
		} else {
			float previous_velocity = g_ctx.m_local->get_velocity( ).m_z;

			g_prediction.begin( g_ctx.m_local, cmd );
			g_prediction.end( g_ctx.m_local );

			float expected_vertical_velocity = std::roundf( ( -gravity ) * g_interfaces.m_global_vars_base->m_interval_per_tick + previous_velocity );

			m_edgebug_data.m_will_edgebug = expected_vertical_velocity == std::roundf( g_ctx.m_local->get_velocity( ).m_z );
			m_edgebug_data.m_will_fail    = !( expected_vertical_velocity == std::roundf( g_ctx.m_local->get_velocity( ).m_z ) );
		}
	}
}

void n_movement::impl_t::edgebug_data_t::reset( )
{
	g_movement.m_edgebug_data.m_edgebug_method = edgebug_type_t::eb_standing;
	g_movement.m_edgebug_data.m_last_tick      = 0;
	g_movement.m_edgebug_data.m_ticks_to_stop  = 0;
	g_movement.m_edgebug_data.m_will_edgebug   = false;
	g_movement.m_edgebug_data.m_will_fail      = false;
}

void n_movement::impl_t::autoduck_data_t::reset( )
{
	g_movement.m_autoduck_data.m_did_land_ducking  = false;
	g_movement.m_autoduck_data.m_did_land_standing = false;
	g_movement.m_autoduck_data.m_ducking_vert      = 0.f;
	g_movement.m_autoduck_data.m_standing_vert     = 0.f;
}

void n_movement::impl_t::pixelsurf_data_t::reset( )
{
	g_movement.m_pixelsurf_data.m_predicted_succesful = false;
	g_movement.m_pixelsurf_data.m_in_pixel_surf       = false;
	g_movement.m_pixelsurf_data.m_should_duck         = false;
	g_movement.m_pixelsurf_data.m_prediction_ticks    = 0;
}
c_vector round_pos( const c_vector& point )
{
	const float epsilon  = 0.001f;
	const float z_offset = point.m_z < 0.0f ? -0.97f : 0.031f;
	return c_vector( point.m_x, point.m_y, floorf( point.m_z + epsilon ) + z_offset );
}

void n_movement::impl_t::movement_fix( const c_angle& old_view_point )
{
	// Получаем направляющие векторы
	c_vector forward, right, up;
	g_math.angle_vectors( old_view_point, &forward, &right, &up );

	c_vector old_forward, old_right, old_up;
	g_math.angle_vectors( g_ctx.m_cmd->m_view_point, &old_forward, &old_right, &old_up );

	// Нормализация с учетом только горизонтальной плоскости
	forward.m_z = right.m_z = 0.0f;
	old_forward.m_z = old_right.m_z = 0.0f;
	up.m_x = up.m_y = old_up.m_x = old_up.m_y = 0.0f;

	const float forward_len     = sqrtf( forward.m_x * forward.m_x + forward.m_y * forward.m_y );
	const float right_len       = sqrtf( right.m_x * right.m_x + right.m_y * right.m_y );
	const float old_forward_len = sqrtf( old_forward.m_x * old_forward.m_x + old_forward.m_y * old_forward.m_y );
	const float old_right_len   = sqrtf( old_right.m_x * old_right.m_x + old_right.m_y * old_right.m_y );

	// Защита от деления на ноль
	if ( forward_len > 0.001f ) {
		forward.m_x /= forward_len;
		forward.m_y /= forward_len;
	}
	if ( right_len > 0.001f ) {
		right.m_x /= right_len;
		right.m_y /= right_len;
	}
	if ( old_forward_len > 0.001f ) {
		old_forward.m_x /= old_forward_len;
		old_forward.m_y /= old_forward_len;
	}
	if ( old_right_len > 0.001f ) {
		old_right.m_x /= old_right_len;
		old_right.m_y /= old_right_len;
	}
	if ( fabsf( up.m_z ) > 0.001f )
		up.m_z = up.m_z > 0.0f ? 1.0f : -1.0f;
	if ( fabsf( old_up.m_z ) > 0.001f )
		old_up.m_z = old_up.m_z > 0.0f ? 1.0f : -1.0f;

	// Сохраняем исходные значения движения
	const float original_forward = g_ctx.m_cmd->m_forward_move;
	const float original_side    = g_ctx.m_cmd->m_side_move;
	const float original_up      = g_ctx.m_cmd->m_up_move;

	// Вычисляем векторную проекцию движения
	const float input_magnitude = sqrtf( original_forward * original_forward + original_side * original_side );

	// Применяем движение к старым векторам направления
	const float world_forward_x = forward.m_x * original_forward + right.m_x * original_side;
	const float world_forward_y = forward.m_y * original_forward + right.m_y * original_side;
	const float world_up_z      = up.m_z * original_up;

	// Создаем матрицу поворота для компенсации изменения угла обзора
	const float cos_delta = old_forward.m_x * forward.m_x + old_forward.m_y * forward.m_y;
	const float sin_delta = old_forward.m_x * forward.m_y - old_forward.m_y * forward.m_x;

	// Поворачиваем вектор движения обратно в систему координат текущего угла
	const float rotated_x = world_forward_x * cos_delta + world_forward_y * sin_delta;
	const float rotated_y = -world_forward_x * sin_delta + world_forward_y * cos_delta;

	// Проецируем обратно на оси forward/side текущего угла обзора
	const float new_forward = old_forward.m_x * rotated_x + old_forward.m_y * rotated_y + old_up.m_z * world_up_z;
	const float new_side    = old_right.m_x * rotated_x + old_right.m_y * rotated_y;

	// Сохраняем исходную величину движения при возможности
	const float output_magnitude = sqrtf( new_forward * new_forward + new_side * new_side );
	const float scale_factor = ( output_magnitude > 0.001f && input_magnitude > 0.001f ) ? fminf( 1.0f, input_magnitude / output_magnitude ) : 1.0f;

	// Получаем лимиты скорости
	const float cached_forward_speed = g_convars[ HASH_BT( "cl_forwardspeed" ) ]->get_float( );
	const float cached_side_speed    = g_convars[ HASH_BT( "cl_sidespeed" ) ]->get_float( );

	// Применяем масштабирование и ограничения
	const float final_forward = new_forward * scale_factor;
	const float final_side    = new_side * scale_factor;

	g_ctx.m_cmd->m_forward_move = std::clamp( final_forward, -cached_forward_speed, cached_forward_speed );
	g_ctx.m_cmd->m_side_move    = std::clamp( final_side, -cached_side_speed, cached_side_speed );
}



class WindowDetect
{
public:
	// Window animation states
	enum WindowState {
		FADE_IN,
		VISIBLE,
		FADE_OUT
	};

	// Structure describing an animated window
	struct AnimatedWindow {
		int type;              // Window type
		std::string info;      // Window text
		float elapsed;         // Time elapsed in current state
		WindowState state;     // Current animation state
		float fadeInDuration;  // Fade-in animation duration
		float visibleDuration; // Full visibility duration
		float fadeOutDuration; // Fade-out animation duration
		float totalDuration;   // Total duration of window (fadeIn + visible + fadeOut)
		float alpha;           // Current opacity (0..1)

		// Positioning
		ImVec2 targetPosition;  // Target window position
		ImVec2 currentPosition; // Current window position

		// Accent line animation
		float accentProgress; // Accent line animation progress (0..1)
		float accentWidth;    // Current accent line width
		bool startFadeOut;    // Flag to start disappearance
	};

	// Animation constants (in seconds)
	static constexpr float NORMAL_FADEIN_DURATION  = 0.25f; // 250ms
	static constexpr float QUICK_FADEIN_DURATION   = 0.15f; // 150ms
	static constexpr float VISIBLE_DURATION        = 2.5f;  // 2500ms - increased by 1 second
	static constexpr float NORMAL_FADEOUT_DURATION = 0.2f;  // 200ms
	static constexpr float QUICK_FADEOUT_DURATION  = 0.12f; // 120ms

	// Constants for positioning
	static constexpr float WINDOW_SPACING = 5.0f / 1080.0f; // 5 pixels normalized for 1080p screen height

	// Adds a new window with animation.
	// When adding a new window, all previous ones move up and remain visible.
	void AddWindow( int type, const std::string& info )
	{
		// Base Y position for new window (% of screen height)
		const float baseYPosition = 0.8f;

		// Maximum number of simultaneously visible windows
		constexpr int MAX_VISIBLE_WINDOWS = 5;

		// Calculate window height for spacing (based on typical text size and padding)
		ImVec2 infoTextSize = fonts_for_gui::regular_11->CalcTextSizeA( 11.f, FLT_MAX, NULL, info.c_str( ) );
		float heightPadding = 22.0f;                                        // From Draw method
		float windowHeight  = ( infoTextSize.y + heightPadding ) / 1080.0f; // Normalized height

		// Create new window
		AnimatedWindow newWin;
		newWin.type            = type;
		newWin.info            = info;
		newWin.elapsed         = 0.f;
		newWin.state           = FADE_IN;
		newWin.fadeInDuration  = NORMAL_FADEIN_DURATION;
		newWin.visibleDuration = VISIBLE_DURATION;
		newWin.fadeOutDuration = NORMAL_FADEOUT_DURATION;
		newWin.totalDuration   = NORMAL_FADEIN_DURATION + VISIBLE_DURATION + NORMAL_FADEOUT_DURATION; // Total lifetime
		newWin.alpha           = 0.f;
		newWin.accentProgress  = 0.f;
		newWin.accentWidth     = 0.f;
		newWin.startFadeOut    = false;

		// Count visible windows
		int visibleCount = 0;
		for ( auto& win : windows ) {
			if ( win.state != FADE_OUT ) {
				visibleCount++;
			}
		}

		// Shift all existing active windows up
		for ( auto& win : windows ) {
			if ( win.state != FADE_OUT ) {
				// Shift target position up by window height + spacing
				win.targetPosition.y -= ( windowHeight + WINDOW_SPACING );
				win.currentPosition.y -= ( windowHeight + WINDOW_SPACING );

				// If window goes beyond top visibility boundary (too many windows),
				// transition it to fade-out mode
				if ( visibleCount >= MAX_VISIBLE_WINDOWS &&
				     win.targetPosition.y < baseYPosition - ( MAX_VISIBLE_WINDOWS - 1 ) * ( windowHeight + WINDOW_SPACING ) ) {
					win.state           = FADE_OUT;
					win.elapsed         = 0.f;
					win.fadeOutDuration = NORMAL_FADEOUT_DURATION;
				}
			}
		}

		// Set position for new window
		newWin.targetPosition  = ImVec2( 0.5f, baseYPosition );
		newWin.currentPosition = ImVec2( 0.5f, baseYPosition );

		windows.push_back( newWin );
	}

	// Linear interpolation function (lerp)
	float Lerp( float a, float b, float t )
	{
		return a + t * ( b - a );
	}

	// Smoothing function for accent line animation
	float EaseInOutCubic( float t )
	{
		return t < 0.5f ? 4.0f * t * t * t : 1.0f - pow( -2.0f * t + 2.0f, 3.0f ) / 2.0f;
	}

	// Updates all windows' state with deltaTime (in seconds)
	void Update( float deltaTime )
	{
		// Call cleanup method to remove old windows if necessary
		CleanupOldWindows( );

		// Sort windows by Y-position (bottom to top), so upper windows are drawn last
		std::sort( windows.begin( ), windows.end( ),
		           []( const AnimatedWindow& a, const AnimatedWindow& b ) { return a.targetPosition.y > b.targetPosition.y; } );

		for ( auto it = windows.begin( ); it != windows.end( ); ) {
			it->elapsed += deltaTime;

			// Update accent line animation (across entire window lifetime)
			float totalElapsed = 0.0f;
			if ( it->state == FADE_IN ) {
				totalElapsed = it->elapsed;
			} else if ( it->state == VISIBLE ) {
				totalElapsed = it->fadeInDuration + it->elapsed;
			} else if ( it->state == FADE_OUT ) {
				totalElapsed = it->fadeInDuration + it->visibleDuration + it->elapsed;
			}
			it->accentProgress = totalElapsed / it->totalDuration;
			if ( it->accentProgress > 1.0f )
				it->accentProgress = 1.0f;

			switch ( it->state ) {
			case FADE_IN:
				// Simple alpha animation - just linear increase
				it->alpha = it->elapsed / it->fadeInDuration;
				if ( it->alpha > 1.0f )
					it->alpha = 1.0f;

				if ( it->elapsed >= it->fadeInDuration ) {
					it->state   = VISIBLE;
					it->elapsed = 0.f;
					it->alpha   = 1.f;
				}
				break;
			case VISIBLE:
				it->alpha = 1.f;
				// Initiate transition to fade-out state when time is up
				if ( it->elapsed >= it->visibleDuration && !it->startFadeOut ) {
					it->startFadeOut = true;
					it->state        = FADE_OUT;
					it->elapsed      = 0.f;
				}
				break;
			case FADE_OUT:
				// Simple alpha animation - just linear decrease
				it->alpha = 1.0f - ( it->elapsed / it->fadeOutDuration );
				if ( it->alpha < 0.0f )
					it->alpha = 0.0f;

				if ( it->elapsed >= it->fadeOutDuration ) {
					// Remove window immediately when fade-out animation ends
					it = windows.erase( it );
					continue;
				}
				break;
			}
			++it;
		}
	}

	// Drawing windows using ImGui (pass ImDrawList pointer and screen size)
	void Draw( ImDrawList* draw, const ImVec2& displaySize )
	{
		// Iterate through all active windows, starting with older ones (draw from bottom to top)
		for ( const auto& win : windows ) {
			// Skip fully faded-out windows to prevent flickering
			if ( win.state == FADE_OUT && win.alpha <= 0.0f ) {
				continue;
			}

			// Calculate window dimensions based on info text size
			ImVec2 infoTextSize = fonts_for_gui::regular_11->CalcTextSizeA( 11.f, FLT_MAX, NULL, win.info.c_str( ) );

			// Window dimensions with padding
			float widthPadding  = 18.0f;
			float heightPadding = 22.0f;
			ImVec2 size         = ImVec2( infoTextSize.x + widthPadding, infoTextSize.y + heightPadding );

			// Calculate actual position
			ImVec2 pos = ImVec2( displaySize.x * win.currentPosition.x - size.x * 0.5f, displaySize.y * win.currentPosition.y - size.y * 0.5f );

			// Colors with current alpha transparency
			int a           = static_cast< int >( win.alpha * 255 );
			ImU32 bgColor   = IM_COL32( 14, 14, 14, a );
			ImU32 textColor = IM_COL32( 255, 255, 255, a );

			// Corner radius for rounded shape
			const float cornerRadius = 4.0f; // Reduced rounding

			// --- Background drawing ---
			// Fill entire window background with rounded corners
			draw->AddRectFilled( pos, ImVec2( pos.x + size.x, pos.y + size.y ), bgColor, cornerRadius );

			// --- Animated accent line drawing ---
			auto accentColor = ImGui::GetStyle( ).Colors[ ImGuiCol_::ImGuiCol_Accent ];
			accentColor.w    = win.alpha;

			// Accent line height set to 1 pixel
			const float accentHeight = 1.0f;
			// Calculate accent line width based on animation progress with 2px margin from edges
			float maxWidth = size.x - 4.0f; // 2px margin from each side
			float margin   = 2.0f;          // 2px margin from left edge

			// Animate line shrinking from right to left across entire window lifetime
			float currentWidth = maxWidth * ( 1.0f - EaseInOutCubic( win.accentProgress ) );
			if ( currentWidth < 0 )
				currentWidth = 0;

			// Accent line starts from left edge with margin
			float startX = pos.x + margin;

			// Draw simple accent line that shrinks to the center
			if ( currentWidth > 0 ) {
				float centerX   = pos.x + size.x * 0.5f;
				float halfWidth = currentWidth * 0.5f;
				float lineY     = pos.y + 0.0f; // or +1.0f for a small offset

				draw->AddRectFilled( ImVec2( centerX - halfWidth, lineY ), ImVec2( centerX + halfWidth, lineY + accentHeight ),
				                     ImGui::GetColorU32( accentColor ) );
			}

			// --- Text drawing with shadow ---
			ImVec2 textPos = ImVec2( pos.x + ( size.x - infoTextSize.x ) * 0.5f, pos.y + ( size.y - infoTextSize.y ) * 0.5f );

			// Subtle text shadow
			draw->AddText( fonts_for_gui::regular_11, 11.f, ImVec2( textPos.x + 1, textPos.y + 1 ), IM_COL32( 0, 0, 0, a / 4 ), win.info.c_str( ) );

			// Main text
			draw->AddText( fonts_for_gui::regular_11, 11.f, textPos, textColor, win.info.c_str( ) );
		}
	}

private:
	std::vector< AnimatedWindow > windows; // Vector of animated windows

	// Method for programmatically removing old windows when there are too many
	void CleanupOldWindows( )
	{
		constexpr int MAX_WINDOWS = 10; // Maximum total number of windows in collection

		// If there are more windows than the maximum
		if ( windows.size( ) > MAX_WINDOWS ) {
			// Find oldest windows (highest ones) and transition them to fade-out mode
			int windowsToRemove = windows.size( ) - MAX_WINDOWS;

			// Sort by Y position (topmost/oldest first)
			std::sort( windows.begin( ), windows.end( ),
			           []( const AnimatedWindow& a, const AnimatedWindow& b ) { return a.targetPosition.y < b.targetPosition.y; } );

			// Transition oldest windows to fade-out mode
			for ( int i = 0; i < windowsToRemove && i < windows.size( ); i++ ) {
				if ( windows[ i ].state != FADE_OUT ) {
					windows[ i ].state           = FADE_OUT;
					windows[ i ].elapsed         = 0.f;
					windows[ i ].fadeOutDuration = QUICK_FADEOUT_DURATION; // Use quick fadeout for cleanup
				}
			}
		}
	}
};

WindowDetect myWindowDetect;
int ignoretime = 0;


bool seccheckcros2 = false;
bool HITGODA2      = false;
bool check( float a, float b )
{
	int a1 = ( int )a;
	int b1 = ( int )b;
	if ( b < 0 ) {
		if ( a1 == b1 ) {
			int a2 = ( a - a1 ) * 100;
			int b2 = ( b - b1 ) * 100;
			int a3 = ( a - a1 ) * 1000;
			if ( b2 == a2 || a2 + 1 == b2 || a2 + 2 == b2 )
				return true;
			else
				return false;
		}
	} else {
		if ( a1 == b1 ) {
			int a2 = ( a - a1 ) * 100;
			int b2 = ( b - b1 ) * 100;
			int a3 = ( a - a1 ) * 1000;
			if ( b2 == a2 || a2 == b2 - 1 || a2 == b2 - 2 )
				return true;
			else
				return false;
		}
	}

	return false;
}

bool check2( float a, float b )
{
	int a1 = ( int )a;
	int b1 = ( int )b;

	if ( b < 0 ) {
		int aInt = static_cast< int >( a );
		int bInt = static_cast< int >( b );

		if ( aInt != bInt ) {
			return false;
		}

		float aFraction = std::fabs( a - aInt ); // fabs для float
		return ( aFraction >= 0.969644f && aFraction <= 0.988999f );

	} else {
		if ( a1 == b1 ) {
			double fracA          = ( double )a - a1;
			int fracA_thousandths = ( int )( fracA * 1000 ) % 1000;
			int a2;
			if ( fracA_thousandths == 969 ) {
				a2 = 97;
			} else {
				a2 = ( int )( fracA * 100 );
			}

			int b2 = ( int )( ( b - b1 ) * 100 );
			return ( b2 == a2 || a2 == b2 - 1 );
		}
	}

	return false;
}

bool self_coded( double a, double b )
{
	// Проверка для положительных чисел
	if ( a > 0 && b > 0 ) {
		double frac_a = a - std::floor( a ); // дробная часть a
		double frac_b = b - std::floor( b ); // дробная часть b
		if ( frac_a > 0 && frac_a < 0.3 && frac_b > 0 && frac_b < 0.3 ) {
			return true;
		} else {
			return false;
		}
	}
	// Проверка для отрицательных чисел (исходная логика)
	else if ( a < 0 && b < 0 ) {
		int i_a = ( int )a; // целая часть a
		int i_b = ( int )b; // целая часть b
		if ( i_a != i_b ) {
			return false;
		}
		float outofpoint_a = a - i_a; // дробная часть a (отрицательная)
		float outofpoint_b = b - i_b; // дробная часть b (отрицательная)
		int full_a         = std::abs( outofpoint_a * 1000000.f );
		int full_b         = std::abs( outofpoint_b * 1000000.f );
		if ( full_b <= full_a && full_a < 990000 ) {
			return true;
		} else {
			return false;
		}
	}
	// Все остальные случаи (одно число >= 0, другое < 0, или a = 0, b = 0)
	else {
		return false;
	}
}
struct NewCmd1 {
	c_angle viewangle;
	float forwardmove;
	float sideMove;
	int buttons;
	c_vector origin;
};
NewCmd1 NewCmd[ 128 ];
void n_movement::impl_t::autobounce_assist( c_user_cmd* cmd )
{
	seccheckcros2 = false;
	static bool HitJump{ false };
	static bool HitMiniJump{ false };
	static bool HitLongJump{ false };
	static bool HitCHop{ false };
	static bool HitMiniChop{ false };
	static bool HitJumpBug{ false };

	if ( !GET_VARIABLE( g_variables.m_bouncee_assist, bool ) )
		return;

	if ( !g_ctx.m_local )
		return;
	if ( !g_interfaces.m_engine_client->is_in_game( ) )
		return;
	if ( !g_ctx.m_local->is_alive( ) )
		return;
	if ( !g_ctx.m_local->get_collideable( ) )
		return;

	static int timer_to_point = 0;
	static bool off           = false;
	if ( g_input.check_input( &GET_VARIABLE( g_variables.m_bounce_assist_point_key, key_bind_t ) ) ) {
		if ( m_pixelsurf_assist_t.set_point )
			off = true;
		if ( bounce_set_point )
			off = true;
		if ( timer_to_point < GetTickCount64( ) && !m_pixelsurf_assist_t.set_point && !bounce_set_point ) {
			if ( !off ) {
				c_vector viewangle = c_vector( cmd->m_view_point.m_x, cmd->m_view_point.m_y, 0.f );
				const auto endPos  = g_ctx.m_local->get_eye_position( ) + c_vector::fromAngle( viewangle ) * 2000;
				trace_t trace;

				ray_t ray( g_ctx.m_local->get_eye_position( ), endPos );

				c_trace_filter flt( g_ctx.m_local );

				g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &flt, &trace );

				m_bounce_points_check.emplace_back(
					round_pos2( c_vector( g_ctx.m_local->get_eye_position( ) + ( endPos - g_ctx.m_local->get_eye_position( ) ) * trace.m_fraction ) ),
					g_interfaces.m_engine_client->get_level_name_short( ) );
				myWindowDetect.AddWindow(
					5, std::format( "point on: {}", round_pos2( c_vector( g_ctx.m_local->get_eye_position( ) +
				                                                          ( endPos - g_ctx.m_local->get_eye_position( ) ) * trace.m_fraction ) )
				                                        .m_z ) );
				timer_to_point = GetTickCount64( ) + 3000;
			}
		}
	} else {
		timer_to_point = GetTickCount64( ) + 50;
		off            = false;
	}

	static int longjump_ticks = 0;
	static int toggletime     = 0;
	static int skip_one_tick  = 0;
	static int ljtickshold    = 0;
	static int crtickshold    = 0;
	static float bfwd         = 0.f;
	static float bside        = 0.f;
	static int skip_ground    = 0;
	static int jumpcrhld      = 0;
	static int holdducklj     = 0;
	static int holdduckmj     = 0;
	static bool ChatStand{ };
	static int skip_mini_jump    = 0;
	static int duck_in_jump_tick = 0;

	if ( !g_input.check_input( &GET_VARIABLE( g_variables.m_bounce_assist_key, key_bind_t ) ) ) {
		AlertJB2          = false;
		HITGODA2          = false;
		HitJump           = false;
		HitMiniJump       = false;
		HitLongJump       = false;
		HitMiniChop       = false;
		HitCHop           = false;
		HitJumpBug        = false;
		skip_one_tick     = 0;
		ljtickshold       = 0;
		crtickshold       = 0;
		bfwd              = 0.f;
		bside             = 0.f;
		skip_ground       = 0;
		jumpcrhld         = 0;
		holdducklj        = 0;
		holdduckmj        = 0;
		ChatStand         = false;
		longjump_ticks    = 0;
		skip_mini_jump    = 0;
		duck_in_jump_tick = 0;
		return;
	}

	if ( m_bounce_points_check.size( ) > 0 ) {
		int index     = -1;      // Индекс ближайшей точки
		float Nearest = FLT_MAX; // Инициализируем максимальным значением
		std::vector< c_vector > surf{ };
		std::vector< int > surf_index{ };

		// Поиск активных точек в радиусе
		for ( int i = 0; i < m_bounce_points_check.size( ); i++ ) {
			if ( g_interfaces.m_engine_client->get_level_name_short( ) == m_bounce_points_check.at( i ).map &&
			     m_bounce_points_check.at( i ).active ) {
				float Distance = g_ctx.m_local->get_origin( ).dist_to_2d( m_bounce_points_check.at( i ).pos );
				if ( Distance <= m_bounce_points_check.at( i ).radius ) { // Используем индивидуальный радиус точки
					surf.emplace_back( m_bounce_points_check.at( i ).pos );
					surf_index.emplace_back( i );
					if ( Distance < Nearest ) {
						Nearest = Distance;
						index   = i;
					}
				}
			}
		}

		if ( index != -1 ) // Если найдена активная точка в пределах радиуса
		{
			c_vector Surf = m_bounce_points_check.at( index ).pos;

			c_vector Calculate{ };
			c_vector Calculate2{ };
			float iCalcilate{ };

			int BackupButtons = cmd->m_buttons;
			float ForwaMove   = cmd->m_forward_move;
			float SideMove    = cmd->m_side_move;

			c_vector BackupVelo = g_ctx.m_local->get_velocity( );
			c_vector Origin     = g_ctx.m_local->get_origin( );
			float stamina       = g_ctx.m_local->get_stamina( );
			int BackupPredicted = g_interfaces.m_prediction->m_commands_predicted;
			static int timer    = 0;
			static int ticks    = 0;
			static int ljticks  = 0;
			static int mjticks  = 0;

			// Добавляем поддержку delta_strafe
			static c_angle last{ };
			c_angle angViewPointBackup = cmd->m_view_point;
			c_angle DeltaAngle         = ( angViewPointBackup - last ) * m_bounce_points_check.at( index ).delta_strafe;
			last                       = cmd->m_view_point;

			if ( !GET_VARIABLE( g_variables.m_bouncee_assist_brokehop, bool ) ) {
				if ( cmd->m_tick_count > ticks && g_prediction.backup_data.m_flags & 1 && g_ctx.m_local->get_stamina( ) != 0.f )
					return;
			}
			if ( g_prediction.backup_data.m_velocity.m_z < 0.f )
				ljticks = 0;

			int g = ( g_prediction.backup_data.m_flags & 1 ) ? 3 : 5;
			int hitgroudtick{ };
			c_vector ChatPositions{ g_ctx.m_local->get_origin( ) };
			bool ChatDuck{ };
			int bheight = 0;

			if ( !HITGODA2 ) {
				for ( int v = 0; v < g; v++ ) {
					cmd->m_side_move    = 0.f;
					cmd->m_forward_move = 0.f;
					cmd->m_buttons      = BackupButtons;
					bool jb             = false;
					bool secjb          = false;
					bool thrjb          = false;
					bool forthjb        = false;
					g_prediction.restore_entity_to_predicted_frame( BackupPredicted - 1 );

					int BackupPredicted2 = 0;
					int once             = 0;
					int ticksljinpred    = 0;

					if ( !HitJump && !HitMiniJump && !HitLongJump && !HitCHop && !HitJumpBug && !( g_ctx.m_local->get_flags( ) & fl_ducking ) &&
					     !HitMiniChop ) {
						for ( int i = 0; i < 48; i++ ) {
							if ( HitJump || HitMiniJump || HitLongJump || HitCHop || HitJumpBug )
								break;
							int max_ground_ticks = 0;

							if ( g_ctx.m_local->get_flags( ) & 1 ) {
								max_ground_ticks += 1;
								once += 1;
							}
							if ( once == 1 )
								ChatPositions = g_ctx.m_local->get_origin( );

							if ( v == 0 ) {
								if ( g_ctx.m_local->get_flags( ) & 1 )
									cmd->m_buttons |= in_jump;
							}
							if ( v == 1 ) {
								if ( g_ctx.m_local->get_flags( ) & 1 ) {
									cmd->m_buttons |= in_jump;
									cmd->m_buttons |= in_duck;
								}
							}
							if ( v == 2 ) {
								if ( ticksljinpred > 4 )
									ticksljinpred = 0;

								if ( ticksljinpred == 0 ) {
									cmd->m_buttons &= ~in_duck;
									cmd->m_buttons &= ~in_jump;
								}

								if ( g_ctx.m_local->get_flags( ) & 1 ) {
									cmd->m_buttons |= in_jump;
									cmd->m_buttons |= in_duck;
									ticksljinpred += 1;
								}

								if ( ticksljinpred > 0 ) {
									cmd->m_buttons |= in_jump;
									cmd->m_buttons |= in_duck;
									ticksljinpred += 1;
								}
							}

							if ( !( g_ctx.m_local->get_flags( ) & 1 ) && v != 2 && v != 4 ) {
								cmd->m_buttons &= ~in_duck;
								cmd->m_buttons &= ~in_jump;
							}

							if ( v == 3 ) {
								if ( g_ctx.m_local->get_flags( ) & 1 ) {
									cmd->m_buttons |= in_jump;
								}
								cmd->m_buttons |= in_duck;
							}
							if ( v == 4 ) {
								if ( i == hitgroudtick ) {
									cmd->m_buttons |= in_duck;
									cmd->m_buttons &= ~in_jump;
									once += 1;
								} else {
									cmd->m_buttons &= ~in_duck;
									cmd->m_buttons |= in_jump;
								}
							}

							c_vector BackupVelo2 = g_ctx.m_local->get_velocity( );
							c_vector Origin2     = g_ctx.m_local->get_origin( );

							int backflags = g_ctx.m_local->get_flags( );
							g_prediction.begin( g_ctx.m_local, g_ctx.m_cmd );
							g_prediction.end( g_ctx.m_local );
							if ( g_ctx.m_local->get_flags( ) & 1 )
								max_ground_ticks += 1;
							if ( v == 0 ) {
								if ( g_ctx.m_local->get_flags( ) & 1 && !( backflags & 1 ) ) {
									if ( !jb )
										hitgroudtick = i;
									jb = true;
								}
							}
							if ( max_ground_ticks > 1 )
								break;
							if ( !( g_ctx.m_local->get_flags( ) & 1 ) && v != 3 && v != 4 ) {
								cmd->m_buttons &= ~in_duck;
								cmd->m_buttons &= ~in_jump;
							}

							c_vector PredictedLocalPlayerOrigin   = g_ctx.m_local->get_abs_origin( );
							c_vector PredictedLocalPlayerVelocity = g_ctx.m_local->get_velocity( );
							int flags                             = g_ctx.m_local->get_flags( );
							iCalcilate                            = 0.f;

							Calculate = PredictedLocalPlayerOrigin + g_ctx.m_local->get_collideable( )->get_obb_maxs( ) + c_vector( 0.f, 0.f, 0.04f );
							int heightlol = ( int )g_ctx.m_local->get_collideable( )->get_obb_maxs( ).m_z;

							if ( PredictedLocalPlayerVelocity.m_z > 0.f ) {
								if ( once == 1 ) {
									for ( int j = 0; j < surf.size( ); j++ ) {
										auto& point = m_bounce_points_check.at( surf_index[ j ] );

										if ( v == 0 ) {
											if ( point.jump && self_coded( Calculate.m_z, surf[ j ].m_z ) ) {
												HitJump   = true;
												ChatStand = true;
												ticks     = cmd->m_tick_count + i + 5;
												g_interfaces.m_engine_client->execute_client_cmd(
													std::format( "echo detected: {} \n", Calculate.m_z ).c_str( ) );
												break;
											}
											if ( point.c_jump && heightlol == 72 && self_coded( Calculate.m_z - 9.f, surf[ j ].m_z ) ) {
												HitJump   = true;
												ChatStand = false;
												ChatDuck  = true;
												ticks     = cmd->m_tick_count + i + 5;
												g_interfaces.m_engine_client->execute_client_cmd(
													std::format( "echo detected: {} \n", Calculate.m_z - 9.f ).c_str( ) );
												break;
											}
										} else if ( v == 1 ) {
											if ( point.minijump && self_coded( Calculate.m_z, surf[ j ].m_z ) ) {
												HitMiniJump = true;
												ChatStand   = true;
												ticks       = cmd->m_tick_count + i + 5;
												g_interfaces.m_engine_client->execute_client_cmd(
													std::format( "echo detected: {} \n", Calculate.m_z ).c_str( ) );
												break;
											}
											if ( point.c_minijump && heightlol == 72 && bheight == 72 &&
											     self_coded( Calculate.m_z - 9.f, surf[ j ].m_z ) ) {
												HitMiniJump = true;
												ChatStand   = false;
												ChatDuck    = true;
												ticks       = cmd->m_tick_count + i + 5;
												g_interfaces.m_engine_client->execute_client_cmd(
													std::format( "echo detected: {} \n", Calculate.m_z - 9.f ).c_str( ) );
												break;
											}
											bheight = heightlol;
										} else if ( v == 3 ) {
											if ( point.crouch_hop && self_coded( Calculate.m_z, surf[ j ].m_z ) ) {
												HitCHop   = true;
												ChatStand = true;
												ticks     = cmd->m_tick_count + i + 5;
												g_interfaces.m_engine_client->execute_client_cmd(
													std::format( "echo detected: {} \n", Calculate.m_z ).c_str( ) );
												break;
											}
											if ( point.c_crouch_hop && heightlol == 54 && self_coded( Calculate.m_z + 9.f, surf[ j ].m_z ) ) {
												HitCHop   = true;
												ChatStand = false;
												ChatDuck  = true;
												ticks     = cmd->m_tick_count + i + 5;
												g_interfaces.m_engine_client->execute_client_cmd(
													std::format( "echo detected: {} \n", Calculate.m_z + 9.f ).c_str( ) );
												break;
											}
											if ( point.c_mini_crouch_hop && self_coded( Calculate.m_z + 9.f, surf[ j ].m_z ) ) {
												HitMiniChop = true;
												ChatStand   = false;
												ChatDuck    = true;
												ticks       = cmd->m_tick_count + i + 5;
												g_interfaces.m_engine_client->execute_client_cmd(
													std::format( "echo detected: {} \n", Calculate.m_z ).c_str( ) );
												break;
											}
										} else if ( v == 4 ) {
											if ( point.jumpbug && self_coded( Calculate.m_z, surf[ j ].m_z ) ) {
												HitJumpBug = true;
												ChatStand  = true;
												ticks      = cmd->m_tick_count + i + 5;
												g_interfaces.m_engine_client->execute_client_cmd(
													std::format( "echo detected: {} \n", Calculate.m_z ).c_str( ) );
												break;
											}
										} else if ( v == 2 ) {
											if ( point.longjump && self_coded( Calculate.m_z, surf[ j ].m_z ) ) {
												HitLongJump = true;
												ChatStand   = true;
												ticks       = cmd->m_tick_count + i + 5;
												g_interfaces.m_engine_client->execute_client_cmd(
													std::format( "echo detected: {} \n", Calculate.m_z ).c_str( ) );
												break;
											}
											if ( point.c_longjump && heightlol == 72 && self_coded( Calculate.m_z - 9.f, surf[ j ].m_z ) ) {
												HitLongJump = true;
												ChatStand   = false;
												ChatDuck    = true;
												ticks       = cmd->m_tick_count + i + 5;
												g_interfaces.m_engine_client->execute_client_cmd(
													std::format( "echo detected: {} \n", Calculate.m_z - 9.f ).c_str( ) );
												break;
											}
										}
									}
								}
							}

							if ( HitJump || HitMiniJump || HitLongJump || HitCHop || HitJumpBug || HitMiniChop )
								break;
						}
					} else {
						break;
					}
				}
			}

			cmd->m_buttons      = BackupButtons;
			cmd->m_forward_move = ForwaMove;
			cmd->m_side_move    = SideMove;
			g_prediction.restore_entity_to_predicted_frame( BackupPredicted - 1 );
			g_prediction.restore_entity_to_predicted_frame( BackupPredicted - 1 );
			static int TicksOut = 0;

			if ( cmd->m_tick_count > ticks ) {
				ticks             = 0;
				HITGODA2          = false;
				skip_one_tick     = 0;
				skip_ground       = 0;
				HitJump           = false;
				jumpcrhld         = 0;
				holdducklj        = 0;
				holdduckmj        = 0;
				HitMiniJump       = false;
				HitMiniChop       = false;
				ChatStand         = false;
				longjump_ticks    = 0;
				skip_mini_jump    = 0;
				duck_in_jump_tick = 0;
			}
			if ( cmd->m_tick_count < ticks ) {
				HITGODA2 = true;
				if ( skip_ground == 0 ) {
					cmd->m_forward_move  = 0.f;
					cmd->m_side_move     = 0.f;
					cmd->m_buttons       = 0;
					cmd->m_mouse_delta_x = 0;
					cmd->m_mouse_delta_y = 0;
				}

				if ( g_prediction.backup_data.m_flags & 1 )
					skip_ground += 1;
				if ( g_prediction.backup_data.m_velocity.m_z > 0.f ) {
					if ( HitJumpBug ) {
						if ( !ChatPositions.is_zero( ) ) {
							if ( TicksOut < GetTickCount64( ) ) {
								TicksOut = GetTickCount64( ) + 500;
								if ( GET_VARIABLE( g_variables.m_bouncee_assist_render, bool ) )
									myWindowDetect.AddWindow( 5, std::string( "jb to head bounce " )
									                                 .append( std::to_string( Surf.m_z ) )
									                                 .append( " with " )
									                                 .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
									                                 .append( !ChatStand ? " (crouch)" : " (stand)" ) );
							}
						}
					}
					HitJumpBug = false;
				}

				if ( HitJump && ChatStand ) {
					if ( g_prediction.backup_data.m_flags & 1 ) {
						if ( GET_VARIABLE( g_variables.m_bouncee_assist_render, bool ) )
							myWindowDetect.AddWindow( 5, std::string( "jump to head bounce " )
							                                 .append( std::to_string( Surf.m_z ) )
							                                 .append( " with " )
							                                 .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
							                                 .append( !ChatStand ? " (crouch)" : " (stand)" ) );
						cmd->m_buttons |= in_jump;
					}
				}
				if ( HitJump && !ChatStand ) {
					if ( duck_in_jump_tick > g_interfaces.m_global_vars_base->m_tick_count )
						cmd->m_buttons |= in_duck;
					if ( g_prediction.backup_data.m_flags & 1 ) {
						if ( GET_VARIABLE( g_variables.m_bouncee_assist_render, bool ) )
							myWindowDetect.AddWindow( 5, std::string( "jump to head bounce " )
							                                 .append( std::to_string( Surf.m_z ) )
							                                 .append( " with " )
							                                 .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
							                                 .append( !ChatStand ? " (crouch)" : " (stand)" ) );
						cmd->m_buttons |= in_jump;
						duck_in_jump_tick = g_interfaces.m_global_vars_base->m_tick_count + 32;
					}
				}
				if ( HitMiniJump && ChatStand ) {
					if ( g_prediction.backup_data.m_flags & 1 ) {
						if ( GET_VARIABLE( g_variables.m_bouncee_assist_render, bool ) )
							myWindowDetect.AddWindow( 5, std::string( "mj to head bounce " )
							                                 .append( std::to_string( Surf.m_z ) )
							                                 .append( " with " )
							                                 .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
							                                 .append( !ChatStand ? " (crouch)" : " (stand)" ) );
						cmd->m_buttons |= in_jump;
						cmd->m_buttons |= in_duck;
					}
				}
				if ( HitMiniJump && !ChatStand ) {
					if ( skip_mini_jump > 1 )
						cmd->m_buttons |= in_duck;
					if ( skip_mini_jump > 0 )
						skip_mini_jump += 1;
					if ( g_prediction.backup_data.m_flags & 1 ) {
						if ( GET_VARIABLE( g_variables.m_bouncee_assist_render, bool ) )
							myWindowDetect.AddWindow( 5, std::string( "mj to head bounce " )
							                                 .append( std::to_string( Surf.m_z ) )
							                                 .append( " with " )
							                                 .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
							                                 .append( !ChatStand ? " (crouch)" : " (stand)" ) );
						cmd->m_buttons |= in_jump;
						cmd->m_buttons |= in_duck;
						skip_mini_jump += 1;
					}
				}
				if ( HitLongJump && ChatStand ) {
					if ( g_prediction.backup_data.m_flags & 1 ) {
						if ( GET_VARIABLE( g_variables.m_bouncee_assist_render, bool ) )
							myWindowDetect.AddWindow( 5, std::string( "lj to head bounce " )
							                                 .append( std::to_string( Surf.m_z ) )
							                                 .append( " with " )
							                                 .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
							                                 .append( !ChatStand ? " (crouch)" : " (stand)" ) );
						cmd->m_buttons |= in_jump;
						cmd->m_buttons |= in_duck;
						longjump_ticks = g_interfaces.m_global_vars_base->m_tick_count + 2;
					}
					if ( longjump_ticks > g_interfaces.m_global_vars_base->m_tick_count )
						cmd->m_buttons |= in_duck;
				}
				if ( HitLongJump && !ChatStand ) {
					if ( g_prediction.backup_data.m_flags & 1 ) {
						if ( GET_VARIABLE( g_variables.m_bouncee_assist_render, bool ) )
							myWindowDetect.AddWindow( 5, std::string( "lj to head bounce " )
							                                 .append( std::to_string( Surf.m_z ) )
							                                 .append( " with " )
							                                 .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
							                                 .append( !ChatStand ? " (crouch)" : " (stand)" ) );
						cmd->m_buttons |= in_jump;
						cmd->m_buttons |= in_duck;
						longjump_ticks = g_interfaces.m_global_vars_base->m_tick_count + 32;
					}
					if ( longjump_ticks > g_interfaces.m_global_vars_base->m_tick_count )
						cmd->m_buttons |= in_duck;
				}

				if ( HitCHop && ChatStand ) {
					if ( g_prediction.backup_data.m_flags & 1 ) {
						cmd->m_buttons |= in_jump;
						if ( GET_VARIABLE( g_variables.m_bouncee_assist_render, bool ) )
							myWindowDetect.AddWindow( 5, std::string( "crouch jump to head bounce " )
							                                 .append( std::to_string( Surf.m_z ) )
							                                 .append( " with " )
							                                 .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
							                                 .append( !ChatStand ? " (crouch)" : " (stand)" ) );
					}
					cmd->m_buttons |= in_duck;
				}
				if ( HitCHop && !ChatStand ) {
					if ( g_prediction.backup_data.m_flags & 1 ) {
						if ( GET_VARIABLE( g_variables.m_bouncee_assist_render, bool ) )
							myWindowDetect.AddWindow( 5, std::string( "crouch jump to head bounce " )
							                                 .append( std::to_string( Surf.m_z ) )
							                                 .append( " with " )
							                                 .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
							                                 .append( !ChatStand ? " (crouch)" : " (stand)" ) );
						cmd->m_buttons |= in_jump;
					}
					if ( !( g_prediction.backup_data.m_flags & 1 ) && g_prediction.backup_data.m_velocity.m_z < 0.f ) {
						cmd->m_buttons |= in_duck;
					}
				}

				if ( HitMiniChop ) {
					if ( GET_VARIABLE( g_variables.m_bouncee_assist_render, bool ) && !bounce_alert_mini_crouch_hop )
						myWindowDetect.AddWindow( 5, std::string( "high crouch jump to head bounce " )
						                                 .append( std::to_string( Surf.m_z ) )
						                                 .append( " with " )
						                                 .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
						                                 .append( !ChatStand ? " (crouch)" : " (stand)" ) );
					bounce_alert_mini_crouch_hop = true;
				}
				AlertJB2 = HitJumpBug;

				if ( skip_ground != 0 ) {
					auto colidable = g_ctx.m_local->get_collideable( );

					if ( !colidable )
						return;

					trace_t trace;
					float step         = std::numbers::pi_v< float > * 2.0f / 16.f;
					float start_circle = 0.f;

					c_vector wall_position{ };

					for ( float a = start_circle; a < std::numbers::pi_v< float > * 2.0f; a += step ) {
						c_vector wishdir     = c_vector( cosf( a ), sinf( a ), 0.f );
						const auto start_pos = g_ctx.m_local->get_abs_origin( );
						const auto end_pos   = start_pos + wishdir;
						c_trace_filter flt( g_ctx.m_local );
						ray_t ray( start_pos, end_pos, colidable->get_obb_mins( ), colidable->get_obb_maxs( ) );

						g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &flt, &trace );

						if ( trace.m_fraction < 1.f && trace.m_plane.m_normal.m_z == 0.f ) {
							c_vector NormalPlane = c_vector( trace.m_plane.m_normal.m_x * -1.f, trace.m_plane.m_normal.m_y * -1.f, 0.f );
							c_angle WallAngle    = NormalPlane.to_angle( );

							float rotation    = deg2rad( WallAngle.m_y - ( cmd->m_view_point.m_y + DeltaAngle.m_y ) );
							float cos_rot     = cos( rotation );
							float sin_rot     = sin( rotation );
							float multiplayer = 6.f;

							float forwardmove   = cos_rot * multiplayer;
							float sidemove      = -sin_rot * multiplayer;
							cmd->m_forward_move = forwardmove;
							cmd->m_side_move    = sidemove;
						}
					}
				}
			} else {
				HITGODA2 = false;
			}
		}
	}
}

bool seccheckcros = false;
float backupmyaw{ };
float prev_frametime  = -1.0f;  // Хранение предыдущего значения frametime
const float threshold = 0.016f; // Пороговое значение для изменения frametime
bool HITGODA          = false;
bool float_ground     = false;
static int cmd_i      = 0;
extern void start_movement_fix( c_user_cmd* cmd );
extern void end_movement_fix( c_user_cmd* cmd );
extern void CorrectMovement( c_user_cmd* cmd, c_angle wish_angle, c_angle old_angles );
static bool do_fucking_strafe = false;
c_angle aim_1{ };
c_angle back_ang = { };
c_angle save_delta{ };
c_angle save_cur{ };
void n_movement::impl_t::pixelsurf_assist( c_user_cmd* cmd )
{
	seccheckcros = false;
	static bool HitJump{ false };
	static bool HitMiniJump{ false };
	static bool HitLongJump{ false };
	static bool HitCHop{ false };
	static bool HitMiniChop{ false };
	static bool HitJumpBug{ false };

	if ( !g_ctx.m_local )
		return;
	back_ang = cmd->m_view_point;
	if ( !GET_VARIABLE( g_variables.m_pixel_surf_assist, bool ) )
		return;
	if ( !g_interfaces.m_engine_client->is_in_game( ) )
		return;
	if ( !g_ctx.m_local->is_alive( ) )
		return;
	static bool off           = false;
	static int timer_to_point = 0;
	if ( g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_surf_assist_point_key, key_bind_t ) ) ) {
		if ( m_pixelsurf_assist_t.set_point )
			off = true;

		if ( timer_to_point < GetTickCount64( ) && !m_pixelsurf_assist_t.set_point ) {
			if ( !off ) {
				c_vector viewangle = c_vector( cmd->m_view_point.m_x, cmd->m_view_point.m_y, 0.f );
				const auto endPos  = g_ctx.m_local->get_eye_position( ) + c_vector::fromAngle( viewangle ) * 2000;
				trace_t trace;

				ray_t ray( g_ctx.m_local->get_eye_position( ), endPos );

				c_trace_filter flt( g_ctx.m_local );

				g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &flt, &trace );

				m_points_check.emplace_back(
					round_pos( c_vector( g_ctx.m_local->get_eye_position( ) + ( endPos - g_ctx.m_local->get_eye_position( ) ) * trace.m_fraction ) ),
					g_interfaces.m_engine_client->get_level_name_short( ) );
				myWindowDetect.AddWindow(
					5, std::format( "point on: {}", round_pos( c_vector( g_ctx.m_local->get_eye_position( ) +
				                                                         ( endPos - g_ctx.m_local->get_eye_position( ) ) * trace.m_fraction ) )
				                                        .m_z ) );
				timer_to_point = GetTickCount64( ) + 3000;
			}
		}
	} else {
		timer_to_point = GetTickCount64( ) + 50;
		off            = false;
	}

	static int toggletime = 0;

	if ( !g_input.check_input( &GET_VARIABLE( g_variables.m_pixel_surf_assist_key, key_bind_t ) ) ) {
		AlertJB           = false;
		HITGODA           = false;
		HitJump           = false;
		HitMiniJump       = false;
		HitLongJump       = false;
		HitCHop           = false;
		HitJumpBug        = false;
		HitMiniChop       = false;
		float_ground      = false;
		cmd_i             = 0;
		do_fucking_strafe = false;
		return;
	}

	if ( m_points_check.size( ) > 0 ) {
		int index     = -1;      // Индекс ближайшей точки
		float Nearest = FLT_MAX; // Инициализируем максимальным значением

		// Поиск ближайшей активной точки в радиусе
		for ( int i = 0; i < m_points_check.size( ); i++ ) {
			if ( g_interfaces.m_engine_client->get_level_name_short( ) == m_points_check.at( i ).map && m_points_check.at( i ).active ) {
				float Distance = g_ctx.m_local->get_origin( ).dist_to( m_points_check.at( i ).pos );
				if ( Distance <= m_points_check.at( i ).radius ) { // Используем индивидуальный радиус точки
					if ( Distance < Nearest ) {
						Nearest = Distance;
						index   = i;
					}
				}
			}
		}

		if ( index != -1 ) // Если найдена активная точка в пределах радиуса
		{
			c_vector Surf = m_points_check.at( index ).pos;

			auto isLargeAngleDifference = []( float yaw1, float yaw2 ) -> bool {
				float yawDifference = std::abs( yaw1 - yaw2 );
				if ( yawDifference > 180.0f )
					yawDifference = 360.0f - yawDifference;
				return yawDifference >= 90.f;
			};

			int BackupButtons = cmd->m_buttons;
			float ForwaMove   = cmd->m_forward_move;
			float SideMove    = cmd->m_side_move;

			c_vector BackupVelo = g_ctx.m_local->get_velocity( );
			c_vector Origin     = g_ctx.m_local->get_origin( );
			float stamina       = g_ctx.m_local->get_stamina( );
			int BackupPredicted = g_interfaces.m_prediction->m_commands_predicted;
			static int timer    = 0;
			static int ticks    = 0;
			static int ljticks  = 0;

			if ( !GET_VARIABLE( g_variables.m_pixel_surf_assist_brokehop, bool ) ) {
				if ( cmd->m_tick_count > ticks && g_prediction.backup_data.m_flags & 1 && g_ctx.m_local->get_stamina( ) != 0.f )
					return;
			}
			if ( g_prediction.backup_data.m_velocity.m_z < 0.f )
				ljticks = 0;

			int g = ( g_prediction.backup_data.m_flags & 1 ) ? 2 : 4;
			int hitgroudtick{ };
			c_vector ChatPositions{ g_ctx.m_local->get_origin( ) };
			bool ChatStand{ };
			bool ChatDuck{ };
			static int TicksOut = 0;

			static c_angle last{ };

			float BackupForwardMove    = cmd->m_forward_move;
			float BackupSideMove       = cmd->m_side_move;
			c_angle angViewPointBackup = cmd->m_view_point;
			float DeltaSalier          = 0.5f;

			c_angle DeltaAngle = ( angViewPointBackup - last ) * m_points_check.at( index ).delta_strafe; // Используем индивидуальный delta_strafe
			last               = cmd->m_view_point;

			for ( int v = 0; v < g; v++ ) {
				cmd->m_side_move    = BackupSideMove;
				cmd->m_forward_move = BackupForwardMove;
				cmd->m_buttons      = BackupButtons;
				cmd->m_view_point   = angViewPointBackup;
				bool jb             = false;
				bool secjb          = false;
				bool thrjb          = false;
				bool forthjb        = false;
				g_prediction.restore_entity_to_predicted_frame( BackupPredicted - 1 );
				int wait             = 0;
				int BackupPredicted2 = 0;
				int once             = 0;
				c_vector Calculate{ };
				float iCalcilate{ };
				int once_fix         = 0;
				c_angle currentAngle = cmd->m_view_point;
				if ( !HitJump && !HitMiniJump && !HitLongJump && !HitCHop && !HitJumpBug && !HitMiniChop ) {
					for ( int i = 0; i < GET_VARIABLE( g_variables.m_pixel_surf_assist_ticks, int ); i++ ) {
						int max_tick_on_ground = 0;
						cmd->m_side_move       = BackupSideMove;
						cmd->m_forward_move    = BackupForwardMove;
						if ( !( g_ctx.m_local->get_flags( ) & 1 ) ) {
							if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_type, int ) == 1 ) {
								if ( !isLargeAngleDifference( currentAngle.m_y, angViewPointBackup.m_y ) )
									currentAngle = ( currentAngle + DeltaAngle ).normalize( ).clamp( );
								cmd->m_view_point = currentAngle;
								start_movement_fix( cmd );
								cmd->m_view_point = angViewPointBackup;
								end_movement_fix( cmd );
							}
						}
						if ( GET_VARIABLE( g_variables.m_pixel_surf_fix, bool ) ) {
							if ( g_prediction.backup_data.m_velocity.length_2d( ) >= 285.91f && once_fix == 0 ) {
								auto sv_airAcelerate = g_interfaces.m_convar->find_var( "sv_airaccelerate" );
								float Razn           = ( ( g_prediction.backup_data.m_velocity.length_2d( ) + 2.f - 285.91f ) / 12.f * 64.f );
								c_vector velocity    = g_prediction.backup_data.m_velocity * -1.f;
								velocity.m_z         = 0.f;
								float rotation       = deg2rad( velocity.to_angle2( ).m_y - g_ctx.m_cmd->m_view_point.m_y );
								float cos_rot        = cos( rotation );
								float sin_rot        = sin( rotation );

								float forwardmove   = cos_rot * Razn;
								float sidemove      = -sin_rot * Razn;
								cmd->m_forward_move = forwardmove;
								cmd->m_side_move    = sidemove;
								once_fix            = 1;
							}
						}

						if ( g_ctx.m_local->get_flags( ) & 1 ) {
							max_tick_on_ground += 1;
							once += 1;
							trace_t trace;
							ray_t ray( g_ctx.m_local->get_abs_origin( ), g_ctx.m_local->get_abs_origin( ) + c_vector( 0, 0, 64 ) );
							c_trace_filter_world flt;
							g_interfaces.m_engine_trace->trace_ray( ray, mask_playersolid, &flt, &trace );
							if ( ( trace.m_fraction < 1.f ) && ( trace.m_plane.m_normal.m_z == 1.f ) )
								float_ground = true;
						}
						if ( once == 1 )
							ChatPositions = g_ctx.m_local->get_origin( );

						if ( v == 0 ) {
							if ( g_ctx.m_local->get_flags( ) & 1 )
								cmd->m_buttons |= in_jump;
						}
						if ( v == 1 ) {
							if ( g_ctx.m_local->get_flags( ) & 1 ) {
								cmd->m_buttons |= in_jump;
								cmd->m_buttons |= in_duck;
							}
						}

						if ( !( g_ctx.m_local->get_flags( ) & 1 ) && v != 2 && v != 3 && v != 4 ) {
							cmd->m_buttons &= ~in_duck;
							cmd->m_buttons &= ~in_jump;
						}
						if ( v == 2 ) {
							if ( g_ctx.m_local->get_flags( ) & 1 ) {
								cmd->m_buttons |= in_jump;
							}
							cmd->m_buttons |= in_duck;
						}
						if ( v == 3 ) {
							if ( i == hitgroudtick ) {
								cmd->m_buttons |= in_duck;
								cmd->m_buttons &= ~in_jump;
								once += 1;
							} else {
								cmd->m_buttons &= ~in_duck;
								cmd->m_buttons |= in_jump;
							}
						}

						c_vector BackupVelo2 = g_ctx.m_local->get_velocity( );
						c_vector Origin2     = g_ctx.m_local->get_origin( );

						int backflags = g_ctx.m_local->get_flags( );
						g_prediction.begin( g_ctx.m_local, g_ctx.m_cmd );
						g_prediction.end( g_ctx.m_local );

						if ( v == 1 ) {
							if ( g_ctx.m_local->get_flags( ) & 1 && !( backflags & 1 ) ) {
								if ( !jb )
									hitgroudtick = i;
								jb = true;
							}
						}

						if ( !( g_ctx.m_local->get_flags( ) & 1 ) && v != 3 && v != 4 ) {
							cmd->m_buttons &= ~in_duck;
							cmd->m_buttons &= ~in_jump;
						}
						int preedflags = g_ctx.m_local->get_flags( );

						if ( preedflags & 1 )
							max_tick_on_ground += 1;
						if ( max_tick_on_ground > 1 )
							break;

						c_vector PredictedLocalPlayerOrigin   = g_ctx.m_local->get_origin( );
						c_vector PredictedLocalPlayerVelocity = g_ctx.m_local->get_velocity( );
						iCalcilate                            = 0.f;
						if ( !( v == 2 ) ) {
							if ( !iCalcilate && once == 1 && !( backflags & fl_ducking ) && !( preedflags & fl_ducking ) && BackupVelo2.m_z > 0.f &&
							     PredictedLocalPlayerVelocity.m_z > 0.f ) {
								if ( wait > 2 ) {
									Calculate  = Origin2;
									iCalcilate = Origin2.m_z - PredictedLocalPlayerOrigin.m_z;
									once += 1;
								} else
									wait += 1;
							}
						} else {
							if ( !iCalcilate && once == 1 && BackupVelo2.m_z > 0.f && PredictedLocalPlayerVelocity.m_z > 0.f ) {
								if ( wait > 2 ) {
									Calculate  = Origin2;
									iCalcilate = Origin2.m_z - PredictedLocalPlayerOrigin.m_z;
									once += 1;
								} else
									wait += 1;
							}
						}
						NewCmd[ i ].forwardmove = cmd->m_forward_move;
						NewCmd[ i ].sideMove    = cmd->m_side_move;
						NewCmd[ i ].viewangle   = cmd->m_view_point;
						save_cur                = angViewPointBackup;
						save_delta              = DeltaAngle;
						if ( iCalcilate ) {
							float z = 0;
							for ( int k = 0; k < 512; k++ ) {
								float o = z * ( z + 1 ) / 2;
								float p = iCalcilate + ( 0.1953125 * z );
								if ( p > 0 ) {
									if ( m_points_check.at( index ).jump ) {
										if ( v == 0 ) {
											if ( check( Calculate.m_z, Surf.m_z ) ) {
												if ( TicksOut < GetTickCount64( ) ) {
													TicksOut = GetTickCount64( ) + 500;
													if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_render, bool ) )
														myWindowDetect.AddWindow(
															5, std::string( "jump to surf " )
																   .append( std::to_string( Surf.m_z ) )
																   .append( " with " )
																   .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
																   .append( " (stand)" ) );
												}
												HitJump = true;
												ticks   = cmd->m_tick_count + i - 3;
												break;
											}
										}
									}

									if ( m_points_check.at( index ).c_jump ) {
										if ( v == 0 ) {
											if ( check( Calculate.m_z + 9.f, Surf.m_z ) ) {
												if ( TicksOut < GetTickCount64( ) ) {
													TicksOut = GetTickCount64( ) + 500;
													if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_render, bool ) )
														myWindowDetect.AddWindow(
															5, std::string( "jump to surf " )
																   .append( std::to_string( Surf.m_z ) )
																   .append( " with " )
																   .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
																   .append( " (crouch)" ) );
												}
												HitJump = true;
												ticks   = cmd->m_tick_count + i - 3;
												break;
											}
										}
									}

									if ( m_points_check.at( index ).minijump ) {
										if ( v == 1 ) {
											if ( check( Calculate.m_z, Surf.m_z ) ) {
												if ( TicksOut < GetTickCount64( ) ) {
													TicksOut = GetTickCount64( ) + 500;
													if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_render, bool ) )
														myWindowDetect.AddWindow(
															5, std::string( "mj to surf " )
																   .append( std::to_string( Surf.m_z ) )
																   .append( " with " )
																   .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
																   .append( " (stand)" ) );
												}
												HitMiniJump = true;
												ticks       = cmd->m_tick_count + i - 3;
												break;
											}
										}
									}

									if ( m_points_check.at( index ).c_minijump ) {
										if ( v == 1 ) {
											if ( check( Calculate.m_z + 9.f, Surf.m_z ) ) {
												if ( TicksOut < GetTickCount64( ) ) {
													TicksOut = GetTickCount64( ) + 500;
													if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_render, bool ) )
														myWindowDetect.AddWindow(
															5, std::string( "mj to surf " )
																   .append( std::to_string( Surf.m_z ) )
																   .append( " with " )
																   .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
																   .append( " (crouch)" ) );
												}
												HitMiniJump = true;
												ticks       = cmd->m_tick_count + i - 3;
												break;
											}
										}
									}

									if ( m_points_check.at( index ).longjump ) {
										if ( v == 1 ) {
											if ( check( Calculate.m_z + 8.9999704f, Surf.m_z ) ) {
												if ( TicksOut < GetTickCount64( ) ) {
													TicksOut = GetTickCount64( ) + 500;
													if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_render, bool ) )
														myWindowDetect.AddWindow(
															5, std::string( "lj to surf " )
																   .append( std::to_string( Surf.m_z ) )
																   .append( " with " )
																   .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
																   .append( " (stand)" ) );
												}
												HitLongJump = true;
												ticks       = cmd->m_tick_count + i - 3;
												break;
											}
										}
									}

									if ( m_points_check.at( index ).c_longjump ) {
										if ( v == 1 ) {
											if ( check( Calculate.m_z + 9.f + 8.9999704f, Surf.m_z ) ) {
												if ( TicksOut < GetTickCount64( ) ) {
													TicksOut = GetTickCount64( ) + 500;
													if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_render, bool ) )
														myWindowDetect.AddWindow(
															5, std::string( "lj to surf " )
																   .append( std::to_string( Surf.m_z ) )
																   .append( " with " )
																   .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
																   .append( " (crouch)" ) );
												}
												HitLongJump = true;
												ticks       = cmd->m_tick_count + i - 3;
												break;
											}
										}
									}

									if ( m_points_check.at( index ).crouch_hop ) {
										if ( v == 2 ) {
											if ( check( Calculate.m_z - 9.f, Surf.m_z ) ) {
												if ( TicksOut < GetTickCount64( ) ) {
													TicksOut = GetTickCount64( ) + 500;
													if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_render, bool ) )
														myWindowDetect.AddWindow(
															5, std::string( "crouch jump to surf " )
																   .append( std::to_string( Surf.m_z ) )
																   .append( " with " )
																   .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
																   .append( " (stand)" ) );
												}
												HitCHop = true;
												ticks   = cmd->m_tick_count + i - 3;
												break;
											}
										}
									}

									if ( m_points_check.at( index ).c_crouch_hop ) {
										if ( v == 2 ) {
											if ( check( Calculate.m_z, Surf.m_z ) ) {
												if ( TicksOut < GetTickCount64( ) ) {
													TicksOut = GetTickCount64( ) + 500;
													if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_render, bool ) )
														myWindowDetect.AddWindow(
															5, std::string( "crouch jump to surf " )
																   .append( std::to_string( Surf.m_z ) )
																   .append( " with " )
																   .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
																   .append( " (crouch)" ) );
												}
												HitCHop = true;
												ticks   = cmd->m_tick_count + i - 3;
												break;
											}
										}
									}

									if ( m_points_check.at( index ).jumpbug ) {
										if ( v == 3 ) {
											if ( check( Calculate.m_z, Surf.m_z ) ) {
												if ( TicksOut < GetTickCount64( ) ) {
													TicksOut = GetTickCount64( ) + 500;
													if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_render, bool ) )
														myWindowDetect.AddWindow(
															5, std::string( "jb to surf " )
																   .append( std::to_string( Surf.m_z ) )
																   .append( " with " )
																   .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
																   .append( " (stand)" ) );
												}
												HitJumpBug = true;
												ticks      = cmd->m_tick_count + i - 3;
												break;
											}
										}
									}

									if ( m_points_check.at( index ).c_jumpbug ) {
										if ( v == 3 ) {
											if ( check( Calculate.m_z + 9.f, Surf.m_z ) ) {
												if ( TicksOut < GetTickCount64( ) ) {
													TicksOut = GetTickCount64( ) + 500;
													if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_render, bool ) )
														myWindowDetect.AddWindow(
															5, std::string( "jb to surf " )
																   .append( std::to_string( Surf.m_z ) )
																   .append( " with " )
																   .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
																   .append( " (crouch)" ) );
												}
												HitJumpBug = true;
												ticks      = cmd->m_tick_count + i - 3;
												break;
											}
										}
									}

									if ( m_points_check.at( index ).c_mini_crouch_hop ) {
										if ( v == 2 ) {
											if ( check( Calculate.m_z + 9.f, Surf.m_z ) ) {
												if ( TicksOut < GetTickCount64( ) ) {
													TicksOut = GetTickCount64( ) + 500;
													if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_render, bool ) )
														myWindowDetect.AddWindow(
															5, std::string( "high crouch jump to surf " )
																   .append( std::to_string( Surf.m_z ) )
																   .append( " with " )
																   .append( std::to_string( g_prediction.backup_data.m_origin.m_z ) )
																   .append( " (crouch)" ) );
												}
												HitMiniChop = true;
												ticks       = cmd->m_tick_count + i + 10;
												break;
											}
										}
									}
								}

								Calculate.m_z = Calculate.m_z - p;
								z += 1;
							}
							break;
						}
					}
				} else {
					break;
				}
				cmd->m_buttons      = BackupButtons;
				cmd->m_forward_move = BackupForwardMove;
				cmd->m_side_move    = BackupSideMove;
			}

			cmd->m_buttons      = BackupButtons;
			cmd->m_forward_move = BackupForwardMove;
			cmd->m_side_move    = BackupSideMove;
			g_prediction.restore_entity_to_predicted_frame( BackupPredicted - 1 );

			if ( cmd->m_tick_count > ticks ) {
				ticks             = 0;
				HITGODA           = false;
				float_ground      = false;
				HitJump           = false;
				HitMiniJump       = false;
				HitLongJump       = false;
				HitCHop           = false;
				HitJumpBug        = false;
				HitMiniChop       = false;
				AlertJB           = false;
				cmd_i             = 0;
				do_fucking_strafe = false;
			}
			if ( cmd->m_tick_count < ticks ) {
				HITGODA = true;
				if ( g_prediction.backup_data.m_velocity.m_z < 0.f || g_ctx.m_local->get_velocity( ).m_z < 0.f ) {
					if ( cmd_i > 127 )
						cmd_i = 0;
					try {
						cmd->m_forward_move = NewCmd[ cmd_i ].forwardmove;
						cmd->m_side_move    = NewCmd[ cmd_i ].sideMove;
						if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_type, int ) == 1 ) {
							cmd->m_view_point = NewCmd[ cmd_i ].viewangle;
							start_movement_fix( cmd );
							cmd->m_view_point = angViewPointBackup;
							end_movement_fix( cmd );
						}
						cmd_i += 1;
					} catch ( ... ) {
						cmd->m_forward_move = 0.f;
						cmd->m_side_move    = 0.f;
					}

					do_fucking_strafe = true;
					cmd->m_buttons    = 0;
				} else {
					do_fucking_strafe = false;
				}

				if ( g_prediction.backup_data.m_velocity.m_z > 0.f ) {
					if ( HitJumpBug ) { }
					HitCHop    = false;
					HitJumpBug = false;
				}

				if ( HitCHop ) {
					cmd->m_buttons |= in_duck;
					if ( g_prediction.backup_data.m_flags & 1 )
						cmd->m_buttons |= in_jump;
				}
				if ( HitJump && g_prediction.backup_data.m_flags & 1 ) {
					cmd->m_buttons |= in_jump;
					HitJump = false;
				}
				if ( HitMiniJump && g_prediction.backup_data.m_flags & 1 ) {
					cmd->m_buttons |= in_jump;
					cmd->m_buttons |= in_duck;
					HitMiniJump = false;
				}
				if ( HitLongJump && g_prediction.backup_data.m_flags & 1 ) {
					cmd->m_buttons |= in_jump;
					cmd->m_buttons |= in_duck;
					ljticks     = g_interfaces.m_global_vars_base->m_tick_count + 3;
					HitLongJump = false;
				}
				if ( HitMiniChop ) {
					alert_mini_crouch_hop = true;
				}
				if ( ljticks > g_interfaces.m_global_vars_base->m_tick_count )
					cmd->m_buttons |= in_duck;
				else {
					if ( g_prediction.backup_data.m_velocity.m_z > 0 )
						ticks = 0;
				}
				AlertJB = HitJumpBug;
			} else {
				HITGODA = false;
			}
		}
	}
}

void n_movement::impl_t::on_frame_stage_notify( int stage )
{
	if ( stage != e_client_frame_stage::start )
		return;
	// if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_type, int ) == 2 ) {
	//	static c_angle aim_to = aim_1;
	//	if ( do_fucking_strafe ) {
	//		const float smoothingFactor = 0.1f;
	//		const float maxDeltaAngle   = 10.f; // Ìàêñèìàëüíîå èçìåíåíèå óãëà çà êàäð
	//
	//		// Ôóíêöèÿ äëÿ íîðìàëèçàöèè ðàçíèöû óãëîâ â äèàïàçîíå [-180, 180]
	//		auto normalizeDelta = []( float delta ) -> float {
	//			while ( delta > 180.f )
	//				delta -= 360.f;
	//			while ( delta < -180.f )
	//				delta += 360.f;
	//			return delta;
	//		};
	//
	//		// Èíòåðïîëÿöèÿ äëÿ pitch
	//		float deltaPitch = normalizeDelta( aim_1.m_x - aim_to.m_x );
	//		if ( fabs( deltaPitch ) > maxDeltaAngle )
	//			deltaPitch = ( deltaPitch > 0 ? maxDeltaAngle : -maxDeltaAngle );
	//		aim_to.m_x += deltaPitch * smoothingFactor;
	//
	//		// Èíòåðïîëÿöèÿ äëÿ yaw
	//		float deltaYaw = normalizeDelta( aim_1.m_y - aim_to.m_y );
	//		if ( fabs( deltaYaw ) > maxDeltaAngle )
	//			deltaYaw = ( deltaYaw > 0 ? maxDeltaAngle : -maxDeltaAngle );
	//		aim_to.m_y += deltaYaw * smoothingFactor;
	//
	//		// Èíòåðïîëÿöèÿ äëÿ roll (åñëè èñïîëüçóåòñÿ)
	//		float deltaRoll = normalizeDelta( aim_1.m_z - aim_to.m_z );
	//		if ( fabs( deltaRoll ) > maxDeltaAngle )
	//			deltaRoll = ( deltaRoll > 0 ? maxDeltaAngle : -maxDeltaAngle );
	//		aim_to.m_z += deltaRoll * smoothingFactor;
	//
	//		aim_to.normalize( );
	//		g_interfaces.m_engine_client->set_view_angles( aim_to );
	//	} else
	//		aim_to = back_ang;
	// }
}

void n_movement::impl_t::pixelsurf_assist_render( )
{
	if ( !GET_VARIABLE( g_variables.m_pixel_surf_assist, bool ) && !GET_VARIABLE( g_variables.m_bouncee_assist, bool ) )
		return;

	if ( !g_ctx.m_local )
		return;
	if ( !g_interfaces.m_engine_client->is_in_game( ) )
		return;
	if ( !g_ctx.m_local->is_alive( ) )
		return;

	static bool isBackspacePressed = false;

	// Обработка нажатия клавиши Backspace
	if ( GetAsyncKeyState( VK_BACK ) & 0x8000 ) {
		isBackspacePressed = true;
	} else {
		isBackspacePressed = false;
	}

	if ( m_points_check.size( ) > 0 || m_bounce_points_check.size( ) > 0 ) { }
}

float hitmarkerTimer = 0.0f;
bool showHitmarker   = false;
void RenderHitmarker( )
{
	if ( !GET_VARIABLE( g_variables.m_hit_marker, bool ) )
		return;

	// Настройки
	const float hitmarkerDuration = 0.5f;  // Общая длительность (секунды)
	const float extensionTime     = 0.1f;  // Время выдвижения линий (секунды)
	const float full_length       = 10.0f; // Длина линий (пиксели)
	const float gap               = 8.0f;  // Начальный отступ от центра (пиксели)
	const float move_distance     = 5.0f;  // Расстояние отхода линий (пиксели)
	const float angle             = 45.0f; // Угол наклона линий (градусы)
	const float thickness         = 1.0f;  // Толщина линий (пиксели)

	// Триггер hitmarker’а (например, нажатие пробела)

	// Если hitmarker активен
	if ( showHitmarker ) {
		ImDrawList* drawList = ImGui::GetForegroundDrawList( );

		// Вычисление времени и параметров анимации
		float elapsed_time   = hitmarkerDuration - hitmarkerTimer;
		float current_length = 0.0f;
		float alpha          = 0.0f;
		float offset         = 0.0f;

		if ( elapsed_time < extensionTime ) {
			// Фаза выдвижения
			float progress = elapsed_time / extensionTime;
			current_length = progress * full_length;
			alpha          = 255.0f;
		} else {
			// Фаза затухания с отходом линий
			current_length      = full_length;
			float fade_progress = ( elapsed_time - extensionTime ) / ( hitmarkerDuration - extensionTime );
			alpha               = 255.0f * ( 1.0f - fade_progress );
			offset              = fade_progress * move_distance;
		}

		// Цвет линий с учетом прозрачности
		ImU32 color = IM_COL32( 255, 255, 255, ( int )alpha );

		// Центр экрана
		float centerX = ImGui::GetIO( ).DisplaySize.x / 2.0f;
		float centerY = ImGui::GetIO( ).DisplaySize.y / 2.0f;

		// Угол в радианах
		float rad_angle = angle * 3.1415926535f / 180.0f;
		float dx        = cos( rad_angle );
		float dy        = sin( rad_angle );

		// Рисуем четыре линии под углом
		drawList->AddLine( ImVec2( centerX + offset * dx, centerY + offset * dy ),
		                   ImVec2( centerX + offset * dx + current_length * dx, centerY + offset * dy + current_length * dy ), color,
		                   thickness ); // Линия 1
		drawList->AddLine( ImVec2( centerX - offset * dx, centerY - offset * dy ),
		                   ImVec2( centerX - offset * dx - current_length * dx, centerY - offset * dy - current_length * dy ), color,
		                   thickness ); // Линия 2
		drawList->AddLine( ImVec2( centerX + offset * dy, centerY - offset * dx ),
		                   ImVec2( centerX + offset * dy + current_length * dy, centerY - offset * dx - current_length * dx ), color,
		                   thickness ); // Линия 3
		drawList->AddLine( ImVec2( centerX - offset * dy, centerY + offset * dx ),
		                   ImVec2( centerX - offset * dy - current_length * dy, centerY + offset * dx + current_length * dx ), color,
		                   thickness ); // Линия 4

		// Обновляем таймер
		hitmarkerTimer -= ImGui::GetIO( ).DeltaTime;
		if ( hitmarkerTimer <= 0.0f ) {
			showHitmarker = false;
		}
	}
}

void n_movement::impl_t::on_end_scene( )
{
	for ( size_t i = 0; i < m_bounce_points_check.size( ); ++i ) {
		points_check_t& point = m_bounce_points_check[ i ];
		if ( point.open_settings ) {
			ImGui::SetNextWindowPos( { g_ctx.m_width / 2, g_ctx.m_height / 2 } );
			ImGui::Begin( "bounce point settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar );
			const auto window                = ImGui::GetCurrentWindow( );
			constexpr auto background_height = 25.f;
			constexpr auto title_text        = "bounce point settings";
			const auto title_text_size       = g_render.m_fonts[ e_font_names::font_name_verdana_bd_11 ]->CalcTextSizeA(
                g_render.m_fonts[ e_font_names::font_name_verdana_bd_11 ]->FontSize, FLT_MAX, 0.f, title_text );
			const auto draw_list = window->DrawList;

			const auto size     = window->Size;
			const auto position = window->Pos;

			[ & ]( ) {
				/* render background */
				ImGui::PushClipRect( ImVec2( position.x, position.y ), ImVec2( position.x + size.x, position.y + background_height ), false );
				draw_list->AddRectFilled( ImVec2( position.x, position.y ), ImVec2( position.x + size.x, position.y + background_height ),
				                          ImColor( 25 / 255.f, 25 / 255.f, 25 / 255.f, 1.f ), ImGui::GetStyle( ).WindowRounding,
				                          ImDrawFlags_RoundCornersTop );
				ImGui::PopClipRect( );

				/* render gradient */
				RenderFadedGradientLine( draw_list, ImVec2( position.x, position.y + background_height - 1.f ), ImVec2( size.x, 1.f ),
				                         static_cast< ImColor >( ImGui::GetColorU32( ImGuiCol_::ImGuiCol_Accent ) ) );

				draw_list->AddText(
					g_render.m_fonts[ e_font_names::font_name_verdana_bd_11 ], g_render.m_fonts[ e_font_names::font_name_verdana_bd_11 ]->FontSize,
					ImVec2( position.x + ( ( size.x - title_text_size.x ) / 2.f ), position.y + ( ( background_height - title_text_size.y ) / 2.f ) ),
					ImColor( 1.f, 1.f, 1.f ), title_text );

				/* render outline */
				ImGui::PushClipRect( ImVec2( position.x + 1.f, position.y + 1.f ), ImVec2( position.x + size.x - 1.f, position.y + size.y - 1.f ),
				                     false );
				draw_list->AddRect( ImVec2( position.x + 1.f, position.y + 1.f ), ImVec2( position.x + size.x - 1.f, position.y + size.y - 1.f ),
				                    ImColor( 50 / 255.f, 50 / 255.f, 50 / 255.f ), ImGui::GetStyle( ).WindowRounding );
				ImGui::PopClipRect( );
			}( );

			ImGui::SetCursorPosY( 30.f );

			// Основные настройки поинта
			if ( ImGui::Button( "Add", ImVec2( 100.f, 25.f ) ) ) {
				point.active = !point.active;
			}

			ImGui::SliderFloat( "Radius", &point.radius, 50.f, 1000.f, "%.1f" );

			static bool delta_strafe_enabled = false;
			ImGui::Checkbox( "Delta Strafe", &delta_strafe_enabled );
			if ( delta_strafe_enabled ) {
				ImGui::SetNextItemWidth( 200.f );
				ImGui::SliderFloat( "##DeltaStrafe", &point.delta_strafe, 0.0f, 5.0f, "%.2f" );
			}

			ImGui::Separator( );

			// Движения
			ImGui::Checkbox( "jump (stand)", &point.jump );
			ImGui::SameLine( 130.f );
			ImGui::Checkbox( "jump (crouch)", &point.c_jump );

			ImGui::Checkbox( "minijump (stand)", &point.minijump );
			ImGui::SameLine( 130.f );
			ImGui::Checkbox( "minijump (crouch)", &point.c_minijump );

			ImGui::Checkbox( "longjump (stand)", &point.longjump );
			ImGui::SameLine( 130.f );
			ImGui::Checkbox( "longjump (crouch)", &point.c_longjump );

			ImGui::Checkbox( "jumpbug (stand)", &point.jumpbug );
			ImGui::SameLine( 130.f );
			ImGui::Checkbox( "jumpbug (crouch)", &point.c_jumpbug );

			ImGui::Checkbox( "crouch hop (stand)", &point.crouch_hop );
			ImGui::SameLine( 130.f );
			ImGui::Checkbox( "crouch hop (crouch)", &point.c_crouch_hop );

			if ( isEnterPressed( ) ) {
				point.open_settings = false;
				g_menu.m_opened     = false;
			}
			ImGui::End( );
		}
	}
	for ( size_t i = 0; i < m_points_check.size( ); ++i ) {
		points_check_t& point = m_points_check[ i ];
		if ( point.open_settings ) {
			ImGui::SetNextWindowPos( { g_ctx.m_width / 2, g_ctx.m_height / 2 } );
			ImGui::Begin( "pixelsurf point settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar );
			const auto window                = ImGui::GetCurrentWindow( );
			constexpr auto background_height = 25.f;
			constexpr auto title_text        = "pixelsurf point settings";
			const auto title_text_size       = g_render.m_fonts[ e_font_names::font_name_verdana_bd_11 ]->CalcTextSizeA(
                g_render.m_fonts[ e_font_names::font_name_verdana_bd_11 ]->FontSize, FLT_MAX, 0.f, title_text );
			const auto draw_list = window->DrawList;

			const auto size     = window->Size;
			const auto position = window->Pos;

			[ & ]( ) {
				/* render background */
				ImGui::PushClipRect( ImVec2( position.x, position.y ), ImVec2( position.x + size.x, position.y + background_height ), false );
				draw_list->AddRectFilled( ImVec2( position.x, position.y ), ImVec2( position.x + size.x, position.y + background_height ),
				                          ImColor( 25 / 255.f, 25 / 255.f, 25 / 255.f, 1.f ), ImGui::GetStyle( ).WindowRounding,
				                          ImDrawFlags_RoundCornersTop );
				ImGui::PopClipRect( );

				/* render gradient */
				RenderFadedGradientLine( draw_list, ImVec2( position.x, position.y + background_height - 1.f ), ImVec2( size.x, 1.f ),
				                         static_cast< ImColor >( ImGui::GetColorU32( ImGuiCol_::ImGuiCol_Accent ) ) );

				draw_list->AddText(
					g_render.m_fonts[ e_font_names::font_name_verdana_bd_11 ], g_render.m_fonts[ e_font_names::font_name_verdana_bd_11 ]->FontSize,
					ImVec2( position.x + ( ( size.x - title_text_size.x ) / 2.f ), position.y + ( ( background_height - title_text_size.y ) / 2.f ) ),
					ImColor( 1.f, 1.f, 1.f ), title_text );

				/* render outline */
				ImGui::PushClipRect( ImVec2( position.x + 1.f, position.y + 1.f ), ImVec2( position.x + size.x - 1.f, position.y + size.y - 1.f ),
				                     false );
				draw_list->AddRect( ImVec2( position.x + 1.f, position.y + 1.f ), ImVec2( position.x + size.x - 1.f, position.y + size.y - 1.f ),
				                    ImColor( 50 / 255.f, 50 / 255.f, 50 / 255.f ), ImGui::GetStyle( ).WindowRounding );
				ImGui::PopClipRect( );
			}( );

			ImGui::SetCursorPosY( 30.f );

			// Основные настройки поинта
			if ( ImGui::Button( "Add", ImVec2( 100.f, 25.f ) ) ) {
				point.active = !point.active;
			}

			ImGui::SliderFloat( "Radius", &point.radius, 50.f, 1000.f, "%.1f" );

			static bool delta_strafe_enabled = false;
			ImGui::Checkbox( "Delta Strafe", &delta_strafe_enabled );
			if ( delta_strafe_enabled ) {
				ImGui::SetNextItemWidth( 200.f );
				ImGui::SliderFloat( "##DeltaStrafe", &point.delta_strafe, 0.0f, 5.0f, "%.2f" );
			}

			ImGui::Separator( );

			// Движения
			ImGui::Checkbox( "jump (stand)", &point.jump );
			ImGui::SameLine( 130.f );
			ImGui::Checkbox( "jump (crouch)", &point.c_jump );

			ImGui::Checkbox( "minijump (stand)", &point.minijump );
			ImGui::SameLine( 130.f );
			ImGui::Checkbox( "minijump (crouch)", &point.c_minijump );

			ImGui::Checkbox( "longjump (stand)", &point.longjump );
			ImGui::SameLine( 130.f );
			ImGui::Checkbox( "longjump (crouch)", &point.c_longjump );

			ImGui::Checkbox( "jumpbug (stand)", &point.jumpbug );
			ImGui::SameLine( 130.f );
			ImGui::Checkbox( "jumpbug (crouch)", &point.c_jumpbug );

			ImGui::Checkbox( "crouch hop (stand)", &point.crouch_hop );
			ImGui::SameLine( 130.f );
			ImGui::Checkbox( "crouch hop (crouch)", &point.c_crouch_hop );

			ImGui::SetCursorPosX( 5.f );
			if ( isEnterPressed( ) ) {
				g_menu.m_opened     = false;
				point.open_settings = false;
			}
			ImGui::End( );
		}
	}

	RenderHitmarker( );
	if ( GET_VARIABLE( g_variables.m_pixel_surf_assist_render, bool ) || GET_VARIABLE( g_variables.m_bouncee_assist_render, bool ) ) {
		float deltaTime = ImGui::GetIO( ).DeltaTime; // Время между кадрами (в секундах)
		myWindowDetect.Update( deltaTime );
		ImDrawList* draw = ImGui::GetBackgroundDrawList( );
		myWindowDetect.Draw( draw, ImGui::GetIO( ).DisplaySize );
	}
}

bool WriteStringToFile( HANDLE hFile, const std::string& str )
{
	DWORD written;
	DWORD length = str.size( );
	if ( !WriteFile( hFile, &length, sizeof( length ), &written, nullptr ) ) {
		return false;
	}
	if ( !WriteFile( hFile, str.c_str( ), length, &written, nullptr ) ) {
		return false;
	}
	return true;
}

bool ReadStringFromFile( HANDLE hFile, std::string& str )
{
	DWORD read;
	DWORD length;
	if ( !ReadFile( hFile, &length, sizeof( length ), &read, nullptr ) ) {
		return false;
	}
	str.resize( length );
	if ( !ReadFile( hFile, &str[ 0 ], length, &read, nullptr ) ) {
		return false;
	}
	return true;
}

void saveConfig( const std::string& filename )
{
	HANDLE hFile = CreateFile( filename.c_str( ), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr );
	if ( hFile == INVALID_HANDLE_VALUE ) {
		return;
	}

	DWORD written;
	DWORD size = m_points_check.size( );
	WriteFile( hFile, &size, sizeof( size ), &written, nullptr );

	for ( const auto& point : m_points_check ) {
		WriteFile( hFile, &point.pos, sizeof( point.pos ), &written, nullptr );
		WriteStringToFile( hFile, point.map );
		WriteFile( hFile, &point.jump, sizeof( point.jump ), &written, nullptr );
		WriteFile( hFile, &point.minijump, sizeof( point.minijump ), &written, nullptr );
		WriteFile( hFile, &point.longjump, sizeof( point.longjump ), &written, nullptr );
		WriteFile( hFile, &point.jumpbug, sizeof( point.jumpbug ), &written, nullptr );
		WriteFile( hFile, &point.crouch_hop, sizeof( point.crouch_hop ), &written, nullptr );
		WriteFile( hFile, &point.c_jump, sizeof( point.c_jump ), &written, nullptr );
		WriteFile( hFile, &point.c_minijump, sizeof( point.c_minijump ), &written, nullptr );
		WriteFile( hFile, &point.c_longjump, sizeof( point.c_longjump ), &written, nullptr );
		WriteFile( hFile, &point.c_jumpbug, sizeof( point.c_jumpbug ), &written, nullptr );
		WriteFile( hFile, &point.c_crouch_hop, sizeof( point.c_crouch_hop ), &written, nullptr );
		WriteFile( hFile, &point.c_mini_crouch_hop, sizeof( point.c_mini_crouch_hop ), &written, nullptr );
		WriteFile( hFile, &point.active, sizeof( point.active ), &written, nullptr );
		WriteFile( hFile, &point.radius, sizeof( point.radius ), &written, nullptr );             // Предполагается, что уже есть
		WriteFile( hFile, &point.delta_strafe, sizeof( point.delta_strafe ), &written, nullptr ); // Добавлено
	}

	CloseHandle( hFile );
}

void loadConfig( const std::string& filename )
{
	HANDLE hFile = CreateFile( filename.c_str( ), GENERIC_READ, NULL, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
	if ( hFile == INVALID_HANDLE_VALUE ) {
		return;
	}

	DWORD read;
	DWORD size;
	ReadFile( hFile, &size, sizeof( size ), &read, nullptr );
	m_points_check.resize( size );

	for ( auto& point : m_points_check ) {
		ReadFile( hFile, &point.pos, sizeof( point.pos ), &read, nullptr );
		ReadStringFromFile( hFile, point.map );
		ReadFile( hFile, &point.jump, sizeof( point.jump ), &read, nullptr );
		ReadFile( hFile, &point.minijump, sizeof( point.minijump ), &read, nullptr );
		ReadFile( hFile, &point.longjump, sizeof( point.longjump ), &read, nullptr );
		ReadFile( hFile, &point.jumpbug, sizeof( point.jumpbug ), &read, nullptr );
		ReadFile( hFile, &point.crouch_hop, sizeof( point.crouch_hop ), &read, nullptr );
		ReadFile( hFile, &point.c_jump, sizeof( point.c_jump ), &read, nullptr );
		ReadFile( hFile, &point.c_minijump, sizeof( point.c_minijump ), &read, nullptr );
		ReadFile( hFile, &point.c_longjump, sizeof( point.c_longjump ), &read, nullptr );
		ReadFile( hFile, &point.c_jumpbug, sizeof( point.c_jumpbug ), &read, nullptr );
		ReadFile( hFile, &point.c_crouch_hop, sizeof( point.c_crouch_hop ), &read, nullptr );
		ReadFile( hFile, &point.c_mini_crouch_hop, sizeof( point.c_mini_crouch_hop ), &read, nullptr );
		ReadFile( hFile, &point.active, sizeof( point.active ), &read, nullptr );
		ReadFile( hFile, &point.radius, sizeof( point.radius ), &read, nullptr );             // Предполагается, что есть
		ReadFile( hFile, &point.delta_strafe, sizeof( point.delta_strafe ), &read, nullptr ); // Добавлено
	}

	CloseHandle( hFile );
}
void saveConfig2( const std::string& filename )
{
	HANDLE hFile = CreateFile( filename.c_str( ), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr );
	if ( hFile == INVALID_HANDLE_VALUE ) {
		return;
	}

	DWORD written;
	DWORD size = m_bounce_points_check.size( );
	WriteFile( hFile, &size, sizeof( size ), &written, nullptr );

	for ( const auto& point : m_bounce_points_check ) {
		WriteFile( hFile, &point.pos, sizeof( point.pos ), &written, nullptr );
		WriteStringToFile( hFile, point.map );
		WriteFile( hFile, &point.jump, sizeof( point.jump ), &written, nullptr );
		WriteFile( hFile, &point.minijump, sizeof( point.minijump ), &written, nullptr );
		WriteFile( hFile, &point.longjump, sizeof( point.longjump ), &written, nullptr );
		WriteFile( hFile, &point.jumpbug, sizeof( point.jumpbug ), &written, nullptr );
		WriteFile( hFile, &point.crouch_hop, sizeof( point.crouch_hop ), &written, nullptr );
		WriteFile( hFile, &point.c_jump, sizeof( point.c_jump ), &written, nullptr );
		WriteFile( hFile, &point.c_minijump, sizeof( point.c_minijump ), &written, nullptr );
		WriteFile( hFile, &point.c_longjump, sizeof( point.c_longjump ), &written, nullptr );
		WriteFile( hFile, &point.c_jumpbug, sizeof( point.c_jumpbug ), &written, nullptr );
		WriteFile( hFile, &point.c_crouch_hop, sizeof( point.c_crouch_hop ), &written, nullptr );
		WriteFile( hFile, &point.c_mini_crouch_hop, sizeof( point.c_mini_crouch_hop ), &written, nullptr );
		WriteFile( hFile, &point.active, sizeof( point.active ), &written, nullptr );
		WriteFile( hFile, &point.radius, sizeof( point.radius ), &written, nullptr );
		WriteFile( hFile, &point.delta_strafe, sizeof( point.delta_strafe ), &written, nullptr );
	}

	CloseHandle( hFile );
}
void loadConfig2( const std::string& filename )
{
	HANDLE hFile = CreateFile( filename.c_str( ), GENERIC_READ, NULL, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
	if ( hFile == INVALID_HANDLE_VALUE ) {
		return;
	}

	DWORD read;
	DWORD size;
	ReadFile( hFile, &size, sizeof( size ), &read, nullptr );
	m_bounce_points_check.resize( size );

	for ( auto& point : m_bounce_points_check ) {
		ReadFile( hFile, &point.pos, sizeof( point.pos ), &read, nullptr );
		ReadStringFromFile( hFile, point.map );
		ReadFile( hFile, &point.jump, sizeof( point.jump ), &read, nullptr );
		ReadFile( hFile, &point.minijump, sizeof( point.minijump ), &read, nullptr );
		ReadFile( hFile, &point.longjump, sizeof( point.longjump ), &read, nullptr );
		ReadFile( hFile, &point.jumpbug, sizeof( point.jumpbug ), &read, nullptr );
		ReadFile( hFile, &point.crouch_hop, sizeof( point.crouch_hop ), &read, nullptr );
		ReadFile( hFile, &point.c_jump, sizeof( point.c_jump ), &read, nullptr );
		ReadFile( hFile, &point.c_minijump, sizeof( point.c_minijump ), &read, nullptr );
		ReadFile( hFile, &point.c_longjump, sizeof( point.c_longjump ), &read, nullptr );
		ReadFile( hFile, &point.c_jumpbug, sizeof( point.c_jumpbug ), &read, nullptr );
		ReadFile( hFile, &point.c_crouch_hop, sizeof( point.c_crouch_hop ), &read, nullptr );
		ReadFile( hFile, &point.c_mini_crouch_hop, sizeof( point.c_mini_crouch_hop ), &read, nullptr );
		ReadFile( hFile, &point.active, sizeof( point.active ), &read, nullptr );
		ReadFile( hFile, &point.radius, sizeof( point.radius ), &read, nullptr );
		ReadFile( hFile, &point.delta_strafe, sizeof( point.delta_strafe ), &read, nullptr );
	}

	CloseHandle( hFile );
}