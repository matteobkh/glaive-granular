/* gui.cpp
Contains GUI window to be rendered in main loop */

#include <iostream>

#include "imgui-knobs.h"

#include "gui.h"
#include "audio.h"
#include "filemanager.h"
#include "widgets.h"

#ifndef M_PI
#define M_PI (3.14159265)
#endif

// knob speeds for coarse and fine tuning
#define FAST (0.0f)
#define SLOW (0.0003f)

// Oscillator bank window
void renderGUI(AudioEngine& audioEngine) {
    // Toggle fine tuning knobs
    float knobSpeed = FAST;
    if(ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl)) {
        knobSpeed = SLOW;
    } else {
        knobSpeed = FAST;
    }

    ImGui::Begin("Glaive Granular", NULL, ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoTitleBar);

    if (FileManager::fileLoaded){
        ImVec2 scopeSize = ImVec2(ImGui::GetContentRegionAvail().x,100);

        // scope
        ImGui::SeparatorText(FileManager::currentFileName.c_str());
        // 
        ImGui::PushID(0);
        ImVec2 plotPos = ImGui::GetCursorScreenPos();
        ImGui::SetNextItemAllowOverlap();
        Widgets::PlotLines(
            "##audio", 
            audioEngine.audioData.samples.data(), 
            audioEngine.audioData.samples.size(), 
            0, nullptr, -1.0f, 1.0f, scopeSize
        );
        for (int i = 0; i < audioEngine.granEng.density; i++) {
            Grain& g = audioEngine.granEng.grains[i];
            ImGui::SetCursorScreenPos(plotPos);
            Widgets::Playhead(1.0f * g.getCurrentRelIndex() / audioEngine.audioData.frames, scopeSize, g.getEnvelope());
        }
        ImGui::PopID();

        // Button or space bar to play/pause 
        if(ImGui::Button("Play/Pause") || ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
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
        Widgets::Checkbox("Loop", &audioEngine.loop);

        // even knob spacing
        int knobsPerRow = 4;
        float windowWidth = ImGui::GetContentRegionAvail().x;
        float padding = ImGui::GetStyle().WindowPadding.x;
        float knobWidth = ImGui::GetTextLineHeight() * 4.0f;  // Default knob size
        float spacing = (windowWidth - (knobsPerRow * knobWidth)) / knobsPerRow; // Even spacing

        ImGui::SeparatorText("Granular parameters");
        // Knob for Grain Size (values between 1 and 2000)
        if (ImGuiKnobs::Knob("Grain Size", &audioEngine.granEng.size, 0.1f, 0.999f, knobSpeed, "%.3f", ImGuiKnobVariant_Tick)) {
            // Recalculate the granular engine parameters whenever the grain size is updated
            audioEngine.granEng.updateParameters(audioEngine.granEng.size);
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) { //double click to reset default
            audioEngine.granEng.updateParameters(0.6f);
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX(padding + spacing + knobWidth); // Position knob
        // Knob for Stretch Factor (values between 0.1 and 10.0)
        if (ImGuiKnobs::Knob("Stretch", &audioEngine.granEng.stretch, 0.1f, 10.0f, knobSpeed, "%.3f", ImGuiKnobVariant_Tick)) {
            // Recalculate the granular engine parameters whenever the stretch factor is updated
            audioEngine.granEng.updateParameters(0, audioEngine.granEng.stretch);
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) { //double click to reset default
            audioEngine.granEng.updateParameters(0, 2.0f);
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX(padding + 2 * (knobWidth + spacing)); // Position knob
        // Knob for Grain Density (values between 1 and 100)
        if (ImGuiKnobs::KnobInt("Density", &audioEngine.granEng.density, 1, MAX_GRAINS, 0.0f, "%d", ImGuiKnobVariant_Stepped, 0.0f, 0, MAX_GRAINS)) {
            // Recalculate the granular engine parameters whenever the grain density is updated
            audioEngine.granEng.updateParameters(0,0,audioEngine.granEng.density);
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) { //double click to reset default
            audioEngine.granEng.updateParameters(0,0,2);
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX(padding + 3 * (knobWidth + spacing)); // Position knob
        // Knob for analysis hopsize (automatically updates synthesis hopsize)
        if (ImGuiKnobs::KnobInt("Hopsize", &audioEngine.granEng.Ha, 100, 8000, 0.0f, "%d", ImGuiKnobVariant_Tick)) {
            // Recalculate the granular engine parameters whenever the grain size is updated
            audioEngine.granEng.updateParameters(0,0,0,audioEngine.granEng.Ha);
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) { //double click to reset default
            audioEngine.granEng.updateParameters(0,0,0,3000);
        }

        ImGui::SeparatorText("Randomization parameters");
        // Jitter knob
        ImGuiKnobs::Knob("Jitter", &audioEngine.granEng.jitterAmount, 0.0f, 1.0f, knobSpeed, "%.3f", ImGuiKnobVariant_Tick);
        if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) { //double click to reset
            audioEngine.granEng.jitterAmount = 0.0f;
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX(padding + spacing + knobWidth); // Position knob
        // Random pan knob
        ImGuiKnobs::Knob("Pan", &audioEngine.granEng.randomPanAmt, 0.0f, 1.0f, knobSpeed, "%.3f", ImGuiKnobVariant_Tick);
        if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) { //double click to reset
            audioEngine.granEng.randomPanAmt = 0.0f;
        }
        ImGui::SameLine();
        /* ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine(); */
        ImGui::SetCursorPosX(padding + 2 * (spacing + knobWidth)); // Position knob
        // Spread knob
        ImGuiKnobs::Knob("Spread", &audioEngine.granEng.spread, 0.0f, 0.5f, knobSpeed, "%.3f", ImGuiKnobVariant_Tick);
        if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) { //double click to reset
            audioEngine.granEng.spread = 0.0f;
        }

        ImGui::BeginChild(
            "c1", 
            ImVec2(
                ImGui::GetWindowWidth()/2-padding, 
                ImGui::CalcTextSize("Gp", ImGui::FindRenderedTextEnd("Gp"), false).y+3.0f
            ),
            ImGuiChildFlags_None,
            ImGuiWindowFlags_NoScrollbar
        );
        ImGui::SeparatorText("Grain pitch");
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild(
            "c2", 
            ImVec2(
                ImGui::GetWindowWidth()/2-padding*2, 
                ImGui::CalcTextSize("Gp", ImGui::FindRenderedTextEnd("Gp"), false).y+3.0f
            ),
            ImGuiChildFlags_None,
            ImGuiWindowFlags_NoScrollbar
        );
        ImGui::SeparatorText("Master");
        ImGui::EndChild();
       // ImGui::SameLine();
        //ImGui::SetCursorPosX(padding + 2 * (spacing + knobWidth));
        //ImGui::SeparatorText("Master");
        // Knob for pitch in semitones
        if (ImGuiKnobs::KnobInt("Semitones", &audioEngine.granEng.semitones, -24, 24, 0.0f, "%d", ImGuiKnobVariant_Stepped, 0.0f, 0, 13)) {
            // Recalculates pitch when semitones changes
            audioEngine.granEng.updateParameters(0,0,0,0,audioEngine.granEng.semitones);
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) { //double click to reset default
            audioEngine.granEng.updateParameters(0,0,0,0,0);
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX(padding + spacing + knobWidth); // Position knob
        // Knob for cents (fine tuning)
        if (ImGuiKnobs::KnobInt("Cents", &audioEngine.granEng.cents, -100, 100, 0.0f, "%d", ImGuiKnobVariant_Tick)) {
            // Recalculates pitch when cents changes
            audioEngine.granEng.updateParameters(0,0,0,0,25,audioEngine.granEng.cents);
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) { //double click to reset default
            audioEngine.granEng.updateParameters(0,0,0,0,25,0);
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth()/2 + padding/2);
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();
        // Volume knob
        ImGui::SetCursorPosX(ImGui::GetWindowWidth()/2 + padding * 2);
        float mVol = audioEngine.masterVolume.load();
        if (ImGuiKnobs::Knob("Volume", &mVol, 0.0f, 1.0f, knobSpeed, "%.2f", ImGuiKnobVariant_Tick)) {
            audioEngine.masterVolume.store(mVol);
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) { //double click to reset
            mVol = 1;
            audioEngine.masterVolume.store(mVol);
        }

        // Display the current values for debugging
        /* ImGui::Text("Current s. index: %d", audioEngine.granEng.index);
        ImGui::Text(
            "Current a. index: %d", 
            static_cast<int>(1.0f * audioEngine.granEng.index / audioEngine.granEng.stretch)
        );
        ImGui::Text(
            "Current grain size: %d", 
            static_cast<int>(1.0f * audioEngine.granEng.Hs * audioEngine.granEng.size)
        );
        ImGui::Text("Current Ha: %d", audioEngine.granEng.Ha);
        ImGui::Text("Current Hs: %d", audioEngine.granEng.Hs);
        ImGui::Text("Current pitch: %.3f", audioEngine.granEng.pitch); */
    } else {
        const char* text;
        if (FileManager::loading)
            text = "Loading file...";
        else
            text = "Please drag and drop an audio file\nin one of the following formats:\n.wav, .flac or .mp3";
        ImVec2 textSize = ImGui::CalcTextSize(text, ImGui::FindRenderedTextEnd(text));
        ImGui::SetCursorPos(ImVec2(
            ImGui::GetWindowWidth() / 2 - textSize.x / 2,
            ImGui::GetWindowHeight() / 2 - textSize.y / 2
        ));
        ImGui::Text("%s", text);
    }

    ImGui::End();
}