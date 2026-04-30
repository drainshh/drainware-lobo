#pragma once

#include <string>
#include <Windows.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../dependencies/imgui/imgui.h"
#include "../dependencies/imgui/imgui_freetype.h"
#include "../dependencies/imgui/imgui_internal.h"

namespace fonts_for_gui {
	inline ImFont* regular = nullptr;
	inline ImFont* bolder = nullptr;
	inline ImFont* regular_11 = nullptr;
	inline ImFont* bolder_11  = nullptr;
	inline ImFont* title = nullptr;
	inline ImFont* icons = nullptr;
	inline ImFont* logo  = nullptr;
	void instalisation( ImGuiIO& io );
} // namespace fonts_for_gui