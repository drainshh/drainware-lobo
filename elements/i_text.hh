#pragma once
#include "../instalisation fonts/fonts2.h"

bool input_text(const char* name, const char* value, char* buf, size_t buf_size, float width, float height, ImGuiInputTextFlags flags) {
    return ImGui::InputTextEx(name, value, buf, buf_size, ImVec2(width, height), flags, 0, 0);
}
