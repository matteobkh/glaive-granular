// Modified ImGui widgets
#ifndef WIDGETS_H
#define WIDGETS_H

#include "imgui.h"
#include "imgui_internal.h"

namespace Widgets {
    extern void Playhead(float fraction, const ImVec2& size_arg, float alpha = 1.0f);
    extern void PlotLines(const char* label, const float* values, int values_count, int values_offset = 0, const char* overlay_text = NULL, float scale_min = FLT_MAX, float scale_max = FLT_MAX, ImVec2 graph_size = ImVec2(0, 0), int stride = sizeof(float));
    extern void PlotLines(const char* label, float(*values_getter)(void* data, int idx), void* data, int values_count, int values_offset = 0, const char* overlay_text = NULL, float scale_min = FLT_MAX, float scale_max = FLT_MAX, ImVec2 graph_size = ImVec2(0, 0));
    bool Checkbox(const char* label, bool* v);
}

#endif // WIDGETS_H