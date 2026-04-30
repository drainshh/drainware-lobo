#include "draw.h"
#include "render.h"
#include "../../game/sdk/classes/c_color.h"
#include "../../dependencies/imgui/helpers/fonts.h"
#include "../../game/sdk/includes/includes.h"
#include "../includes/includes.h"




void n_draw::impl_t::render_text( ImFont* fnt, float x, float y, std::string str, const unsigned int clr, bool center, bool dropshadow, bool outline,
                                  const unsigned int clr_outlined )
{
	text_draw_object_t text = { };
	text.m_font             = fnt;
	text.m_position         = c_vector_2d( x, y );
	text.m_text             = str;
	text.m_color            = clr;
	text.m_outline_color    = clr_outlined;
	text.m_draw_flags       = e_text_flags::text_flag_none;
	if ( dropshadow )
		text.m_draw_flags = e_text_flags::text_flag_dropshadow;
	if ( outline )
		text.m_draw_flags = e_text_flags::text_flag_outline;
	if ( center )
		text.m_position.m_x = text.m_position.m_x - fnt->CalcTextSizeA(fnt->FontSize,FLT_MAX,NULL,str.c_str() ).x/2;
	g_render.m_draw_data.emplace_back( e_draw_type::draw_type_text, std::make_any< text_draw_object_t >( text ) );
}

void n_draw::impl_t::rect_outlined( float x, float y, float w, float h, const unsigned int clr )
{
	rect_draw_object_t rect_outlined{ };
	rect_outlined.m_min = c_vector_2d( x, y );
	rect_outlined.m_max = c_vector_2d( x + w, y + h );
	rect_outlined.m_color = clr;
	rect_outlined.m_outline_color = c_color( 0, 0, 0, 0 ).get_u32( );
	rect_outlined.m_filled = false;
	rect_outlined.m_rounding      = 0.f;
	rect_outlined.m_corner_rounding_flags = 0;
	rect_outlined.m_thickness             = 1.f;
	rect_outlined.m_outline_flags         = e_rect_flags::rect_flag_none;
	g_render.m_draw_data.emplace_back( e_draw_type::draw_type_rect, std::make_any< rect_draw_object_t >( rect_outlined ) );
}

void n_draw::impl_t::rect_filled( float x, float y, float w, float h, const unsigned int clr, const unsigned int clr_outlined, bool outline )
{
	rect_draw_object_t rect_filled{ };
	rect_filled.m_min                   = c_vector_2d( x, y );
	rect_filled.m_max                   = c_vector_2d( x + w, y + h );
	rect_filled.m_color                 = clr;
	rect_filled.m_outline_color         = clr_outlined;
	rect_filled.m_filled                = true;
	rect_filled.m_rounding              = 0.f;
	rect_filled.m_corner_rounding_flags = 0;
	rect_filled.m_thickness             = 1.f;
	rect_filled.m_outline_flags         = e_rect_flags::rect_flag_none;
	if ( outline ) {
		rect_filled.m_outline_flags |= e_rect_flags::rect_flag_inner_outline;
		rect_filled.m_outline_flags |= e_rect_flags::rect_flag_outer_outline;
	}

	g_render.m_draw_data.emplace_back( e_draw_type::draw_type_rect, std::make_any< rect_draw_object_t >( rect_filled ) );
}

void n_draw::impl_t::rect_gradient( float x, float y, float w, float h, const unsigned int top_left, const unsigned int top_right,
                                    const unsigned int bot_right, const unsigned int bot_left )
{
	gradient_rect_draw_object_t rect_gradient{ };
	rect_gradient.min = c_vector_2d( x, y );
	rect_gradient.max = c_vector_2d( x + w, y + h );
	rect_gradient.left_top_clr = top_left;
	rect_gradient.right_top_clr = top_right;
	rect_gradient.right_bottom_clr = bot_right;
	rect_gradient.left_bottom_clr  = bot_left;
	g_render.m_draw_data.emplace_back( e_draw_type::draw_type_gradient_rect, std::make_any< gradient_rect_draw_object_t >( rect_gradient ) );
}



void n_draw::impl_t::line( float x1, float y1, float x2, float y2, const unsigned int clr, float m_thickness ) { 
	line_draw_object_t line{ };
	line.m_start = c_vector_2d( x1, y1 );
	line.m_end   = c_vector_2d( x2, y2 );
	line.m_color = clr;
	line.m_thickness = m_thickness;
	g_render.m_draw_data.emplace_back( e_draw_type::draw_type_line, std::make_any< line_draw_object_t >( line ) );
}
