
#include <math.h>  // For sinf, cosf, and fmaxf

#include "pannel_collection.h"



void UI_loading_indicator_circle(const char* label, f32 indicator_radius, int circle_count, f32 speed, ImVec4* main_color, ImVec4* backdrop_color) {
    ImGuiWindow* window = igGetCurrentWindow();
    if (window->SkipItems)
        return;

    
    ImVec2 pos;
    igGetCursorPos(&pos);
    
    const f32 circle_radius = indicator_radius / 15.0f;
    const f32 updated_indicator_radius = indicator_radius - 4.0f * circle_radius;
    
    ImVec2 max_pos;
    max_pos.x = pos.x + indicator_radius * 2.0f;
    max_pos.y = pos.y + indicator_radius * 2.0f;
    
    ImRect bb;
    bb.Min = pos;
    bb.Max = max_pos;
    
    const ImGuiID id = igGetID_Str(label);
    igItemSize_Vec2(max_pos, 0.0f);
    if (!igItemAdd(bb, id, NULL, 0))
        return;
    
    const f32 t = (f32)igGetTime();
    const f32 degree_offset = 2.0f * M_PI / circle_count;
    
    ImDrawList* draw_list = igGetWindowDrawList();
    
    for (int i = 0; i < circle_count; ++i) {
        const f32 angle = degree_offset * i;
        const f32 x = updated_indicator_radius * sinf(angle);
        const f32 y = updated_indicator_radius * cosf(angle);
        const f32 growth = fmaxf(0.0f, sinf(t * speed - i * degree_offset));
        
        ImVec4 color;
        color.x = main_color->x * growth + backdrop_color->x * (1.0f - growth);
        color.y = main_color->y * growth + backdrop_color->y * (1.0f - growth);
        color.z = main_color->z * growth + backdrop_color->z * (1.0f - growth);
        color.w = 1.0f;
        
        ImVec2 circle_center;
        circle_center.x = pos.x + indicator_radius + x;
        circle_center.y = pos.y + indicator_radius - y;
        
        const ImU32 color_u32 = igGetColorU32_Vec4(color);
        ImDrawList_AddCircleFilled(draw_list, circle_center, circle_radius + growth * circle_radius, color_u32, 0);
    }
}