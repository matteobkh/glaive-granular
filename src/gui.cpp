#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "imgui.h"
//#ifndef IMGUI_DISABLE
#include "imgui_internal.h"
//#endif

#include "imgui-knobs.h"
#include "gui.h"
#include "audio.h"
#include "filemanager.h"

#include <mutex>
#include <cmath>

#ifndef M_PI
#define M_PI (3.14159265)
#endif

// knob speeds for coarse and fine tuning
#define FAST (0.0f)
#define SLOW (0.0003f)

// size_arg (for each axis) < 0.0f: align to end, 0.0f: auto, > 0.0f: specified size
static void ImPlayhead(float fraction, const ImVec2& size_arg, float alpha = 1.0f)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = ImGui::CalcItemSize(size_arg, ImGui::CalcItemWidth(), g.FontSize + style.FramePadding.y * 2.0f);
    ImRect bb(pos, pos + size);
    ImGui::ItemSize(size, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, 0))
        return;

    // Out of courtesy we accept a NaN fraction without crashing
    float fill_n0 = 0.0f;
    float fill_n1 = (fraction == fraction) ? fraction : 0.0f;

    const float fill_width_n = 0.003f;
    fill_n0 = ImFmod(fraction, 1.0f) * (1.0f + fill_width_n) - fill_width_n;
    fill_n1 = ImSaturate(fill_n0 + fill_width_n);
    fill_n0 = ImSaturate(fill_n0);

    // Render
    ImGui::RenderFrame(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg, 0), true, style.FrameRounding);
    bb.Expand(ImVec2(-style.FrameBorderSize, -style.FrameBorderSize));
    ImGui::RenderRectFilledRangeH(window->DrawList, bb, ImGui::GetColorU32(ImGuiCol_PlotHistogram, alpha), fill_n0, fill_n1, style.FrameRounding);
}

static bool fileSelected = false;

// Oscillator bank window
void renderGUI(AudioEngine& audioEngine) {
    // Toggle fine tuning knobs
    float knobSpeed = FAST;
    if(ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl)) {
        knobSpeed = SLOW;
    } else {
        knobSpeed = FAST;
    }

    ImGui::Begin("Glaive Granular");

    // choose audio file
    int item_current = -1;

    if (ImGui::Combo("Choose file", &item_current, files.data(), files.size())) {
        if (audioEngine.granularPlaying.load()) 
            audioEngine.granularPlaying.store(false);
        audioEngine.audioData = LoadWavFile(files[item_current]);
        audioEngine.granEng = GranularEngine(audioEngine.audioData.samples);
        fileSelected = true;
    }

    if (fileSelected){
        ImVec2 scopeSize = ImVec2(500,100);

        // scope
        ImGui::Text("Waveform");
        // 
        ImGui::PushID(0);
        ImVec2 plotPos = ImGui::GetCursorScreenPos();
        ImGui::SetNextItemAllowOverlap();
        ImGui::PlotLines(
            "##audio", 
            audioEngine.audioData.samples.data(), 
            audioEngine.audioData.samples.size(), 
            0, nullptr, -1.0f, 1.0f, scopeSize
        );
        //ImVec2 button2_pos = ImVec2(plotPos.x + 5.0f, plotPos.y + 5.0f);
        //ImGui::SetCursorScreenPos(plotPos);
        for (int i = 0; i < audioEngine.granEng.density; i++) {
            Grain& g = audioEngine.granEng.grains[i];
            ImGui::SetCursorScreenPos(plotPos);
            ImPlayhead(1.0f * g.getCurrentRelIndex() / audioEngine.audioData.frames, scopeSize, g.getEnvelope());
        }
        ImGui::PopID();

        if(ImGui::Button("Play/Pause")) {
            audioEngine.granularPlaying.load() 
                ? audioEngine.granularPlaying.store(false) 
                : audioEngine.granularPlaying.store(true);
        }
        ImGui::SameLine();
        if(ImGui::Button("Stop")) {
            audioEngine.granularPlaying.store(false);
            audioEngine.granEng.index = 0;
        }
        ImGui::SameLine();
        ImGui::Checkbox("Loop", &audioEngine.loop);


        // Slider for Grain Size (values between 1 and 2000)
        if (ImGuiKnobs::Knob("Grain Size", &audioEngine.granEng.size, 0.1, 0.99, knobSpeed, "%.3f", ImGuiKnobVariant_WiperDot)) {
            // Recalculate the granular engine parameters whenever the grain size is updated
            audioEngine.granEng.updateParameters(audioEngine.granEng.size);
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) { //double click to reset
            audioEngine.granEng.updateParameters(0.6f);
        }
        ImGui::SameLine();
        // Slider for Stretch Factor (values between 0.1 and 10.0)
        if (ImGuiKnobs::Knob("Stretch Factor", &audioEngine.granEng.stretch, 0.1f, 10.0f, knobSpeed, "%.3f", ImGuiKnobVariant_WiperDot)) {
            // Recalculate the granular engine parameters whenever the stretch factor is updated
            audioEngine.granEng.updateParameters(0, audioEngine.granEng.stretch);
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) { //double click to reset
            audioEngine.granEng.updateParameters(0, 2.0f);
        }
        ImGui::SameLine();
        // Slider for Grain Density (values between 1 and 100)
        if (ImGuiKnobs::KnobInt("Grain Density", &audioEngine.granEng.density, 1, MAX_GRAINS, 0.0f, "%d", ImGuiKnobVariant_Stepped, 0, 0, MAX_GRAINS)) {
            // Recalculate the granular engine parameters whenever the grain density is updated
            audioEngine.granEng.updateParameters(0,0,audioEngine.granEng.density);
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) { //double click to reset
            audioEngine.granEng.updateParameters(0,0,2);
        }

        // Slider for synthesis hopsize
        if (ImGui::SliderInt("Hs", &audioEngine.granEng.Hs, 100, 12000)) {
            audioEngine.granEng.updateParameters(0,0,0,audioEngine.granEng.Hs);
        }

        // Slider for analysis hopsize
        if (ImGui::SliderInt("Ha", &audioEngine.granEng.Ha, 100, 6000)) {
            audioEngine.granEng.updateParameters(0,0,0,0,audioEngine.granEng.Ha);
        }

        ImGui::Text("Randomize");
        //ImGui::SliderFloat("Time jitter amount", &audioEngine.granEng.jitterAmount, 0.0f, 1.0f);
        // Jitter knob
        ImGuiKnobs::Knob("Jitter", &audioEngine.granEng.jitterAmount, 0.0f, 1.0f, knobSpeed, "%.3f", ImGuiKnobVariant_WiperDot);
        if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) { //double click to reset
            audioEngine.granEng.jitterAmount = 0.0f;
        }
        ImGui::SameLine();
        // Random pan knob
        ImGuiKnobs::Knob("Random pan", &audioEngine.granEng.randomPanAmt, 0.0f, 1.0f, knobSpeed, "%.3f", ImGuiKnobVariant_WiperDot);
        if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) { //double click to reset
            audioEngine.granEng.randomPanAmt = 0.0f;
        }
        ImGui::SameLine();
        // Spread knob
        ImGuiKnobs::Knob("Spread", &audioEngine.granEng.spread, 0.0f, 1.0f, knobSpeed, "%.3f", ImGuiKnobVariant_WiperDot);
        if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) { //double click to reset
            audioEngine.granEng.spread = 0.0f;
        }

        /* ImGui::SliderFloat("Random pan amount", &audioEngine.granEng.randomPanAmt, 0.0f, 1.0f);
        ImGui::SliderFloat("Spread amount", &audioEngine.granEng.spread, 0.0f, 1.0f); */

        // Display the current values for clarity
        ImGui::Text("Current s. index: %d", audioEngine.granEng.index);
        ImGui::Text("Current a. index: %d", static_cast<int>(1.0f * audioEngine.granEng.index / audioEngine.granEng.stretch));
        ImGui::Text("Current grain size: %d", static_cast<int>(1.0f * audioEngine.granEng.Hs * audioEngine.granEng.size));
        ImGui::Text("Current Ha: %d", audioEngine.granEng.Ha);
        ImGui::Text("Current Hs: %d", audioEngine.granEng.Hs);

        // Volume knob
        float mVol = audioEngine.masterVolume.load();
        if (ImGuiKnobs::Knob("Master Volume", &mVol, 0.0f, 1.0f, knobSpeed, "%.2f", ImGuiKnobVariant_WiperDot)) {
            audioEngine.masterVolume.store(mVol);
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) { //double click to reset
            mVol = 1;
            audioEngine.masterVolume.store(mVol);
        }
}

    ImGui::End();
}