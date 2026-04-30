#include <string>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../dependencies/imgui/imgui.h"
#include "../dependencies/imgui/imgui_internal.h"
#include "../dependencies/imgui/imgui_freetype.h"

namespace Utils {
    inline ImVec2 PortableSquare(ImVec2 position, const ImVec2& size) {
        ImVec2 rect_min = position;
        ImVec2 rect_max = position + size;

        bool is_hovered = ImGui::GetMousePos().x >= rect_min.x &&
            ImGui::GetMousePos().y >= rect_min.y &&
            ImGui::GetMousePos().x <= rect_max.x &&
            ImGui::GetMousePos().y <= rect_max.y;

        /*
        * also you can create a window and use it but which diffrence with default function?
        * bool is_hovered = ImGui::IsItemHovered();
        * bool is_hovered = ImGui::IsMouseHoveringRect(rect_min, rect_max);
        */

        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && is_hovered) {
            position += ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
        }

        position = ImClamp(position, ImVec2(0.0f, 0.0f), ImVec2(ImGui::GetWindowSize().x - size.x, ImGui::GetWindowSize().y - size.y));

        return position;
    }
}