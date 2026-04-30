
#pragma once

#include <any>
#include <deque>
#include <functional>
#include <shared_mutex>
#include <string>

#pragma comment( lib, "d3d9.lib" )
#pragma comment( lib, "d3dx9.lib" )
#include <d3d9.h>
#include <d3dx9.h>
#include <d3dx9tex.h>

#include "../../dependencies/steam/isteamclient.h"
#include "../../game/sdk/classes/c_vector.h"

struct ImFont;
struct ImDrawList;
struct ImColor;

class c_vector_2d;
class c_vector;
namespace n_draw
{
	struct impl_t {
		void render_text( ImFont* fnt, float x, float y, std::string str, const unsigned int clr, bool center = false, bool dropshadow = false,
		                  bool outline = false, const unsigned int clr_outlined = 0);
		void rect_outlined( float x, float y, float w, float h, const unsigned int clr );
		void rect_filled( float x, float y, float w, float h, const unsigned int clr,
		                  const unsigned int clr_outlined =0, bool outline = false );
		void rect_gradient( float x, float y, float w, float h, const unsigned int top_left, const unsigned int top_right,
		                    const unsigned int bot_right, const unsigned int bot_left );
		void line( float x1, float y1, float x2, float y2, const unsigned int clr, float m_thickness = 1.f );
	};
} // namespace n_draw
inline n_draw::impl_t g_draw{ };