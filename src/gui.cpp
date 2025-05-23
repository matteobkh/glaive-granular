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

static bool debug = false;

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

        // get window padding
        float padding = ImGui::GetStyle().WindowPadding.x;

        // scope
        ImGui::SeparatorText(FileManager::currentFileName.c_str());

        ImGui::PushID(0);
        ImVec2 plotPos = ImGui::GetCursorScreenPos();
        ImGui::SetNextItemAllowOverlap();
        // render audio file waveform
        Widgets::PlotLines(
            "##audio", 
            audioEngine.audioData.samples.data(), 
            audioEngine.audioData.samples.size(), 
            0, nullptr, -1.0f, 1.0f, scopeSize
        );
        // Render playheads for each grain currently playing
        for (int i = 0; i < audioEngine.granEng.density; i++) {
            Grain& g = audioEngine.granEng.grains[i];
            ImGui::SetCursorScreenPos(plotPos);
            Widgets::Playhead(1.0f * g.getCurrentRelIndex() / audioEngine.audioData.frames, scopeSize, g.getEnvelope());
        }
        // render start and end points
        ImGui::SetCursorScreenPos(ImVec2(plotPos.x+audioEngine.start*scopeSize.x, plotPos.y));
        ImGui::Button("##start", ImVec2(2.5f, scopeSize.y));
        if (ImGui::IsItemActive()) { // defines button behavior when clicked and dragged
            audioEngine.start = std::min(
                std::max(0.0f, (ImGui::GetIO().MousePos.x - padding) / scopeSize.x), 
                audioEngine.end - 0.01f
            );
            int scaledStartPoint = audioEngine.start * audioEngine.audioData.frames * audioEngine.granEng.stretch;
            if (audioEngine.granEng.index < scaledStartPoint) {
                audioEngine.granEng.index = scaledStartPoint+1;
            }
            ImGui::BeginTooltip();
            ImGui::Text("Start: %f", audioEngine.start);
            ImGui::EndTooltip();
        } else if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
            ImGui::BeginTooltip();
            ImGui::Text("Start: %f", audioEngine.start);
            ImGui::EndTooltip();
        }
        
        ImGui::SetCursorScreenPos(ImVec2(plotPos.x+audioEngine.end*scopeSize.x, plotPos.y));
        ImGui::Button("##end", ImVec2(2.5f, scopeSize.y));
        if (ImGui::IsItemActive()) { // defines button behavior when clicked and dragged
            audioEngine.end = std::min(
                std::max(
                    audioEngine.start + 0.01f, 
                    (ImGui::GetIO().MousePos.x - padding) / scopeSize.x
                ),
                1.0f
            );
            ImGui::BeginTooltip();
            ImGui::Text("End: %f", audioEngine.end);
            ImGui::EndTooltip();
        } else if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
            ImGui::BeginTooltip();
            ImGui::Text("End: %f", audioEngine.end);
            ImGui::EndTooltip();
        }
        ImGui::PopID(); //0

        // Button or space bar to play/pause 
        if(ImGui::Button("Play/Pause") || ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
            audioEngine.granularPlaying.store(!audioEngine.granularPlaying.load());
        }
        ImGui::SameLine();
        if(ImGui::Button("Stop")) {
            audioEngine.granularPlaying.store(false);
            audioEngine.granEng.index = audioEngine.start 
                * audioEngine.audioData.frames * audioEngine.granEng.stretch;
        }
        ImGui::SameLine();
        Widgets::Checkbox("Loop", &audioEngine.loop);

        // even knob spacing
        int knobsPerRow = 4;
        float windowWidth = ImGui::GetContentRegionAvail().x;
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
            int scaledStartPoint = audioEngine.start * audioEngine.audioData.frames * audioEngine.granEng.stretch;
            if (audioEngine.granEng.index < scaledStartPoint) {
                audioEngine.granEng.index = scaledStartPoint+1;
            }
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
        ImGui::SetCursorPosX(padding + 2 * (spacing + knobWidth)); // Position knob
        // Spread knob
        ImGuiKnobs::Knob("Spread", &audioEngine.granEng.spread, 0.0004f, 1.0f, knobSpeed, "%.3f", ImGuiKnobVariant_Tick, 0.0f, ImGuiKnobFlags_Logarithmic);
        if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) { //double click to reset
            audioEngine.granEng.spread = 0.0f;
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX(padding + 3 * (spacing + knobWidth)); // Position knob
        // Reverse grain probability knob
        ImGuiKnobs::KnobInt("Reverse Prob.", &audioEngine.granEng.revprob,0,100,0.0f,"%d%%",ImGuiKnobVariant_Tick);
        if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) { //double click to reset
            audioEngine.granEng.revprob = 0;
        }

        ImGui::BeginChild(
            "c1", 
            ImVec2(
                ImGui::GetWindowWidth()/2-padding, 
                ImGui::CalcTextSize("Gp", ImGui::FindRenderedTextEnd("Gp"), false).y+3.0f
            ),
            ImGuiChildFlags_None,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
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
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        );
        ImGui::SeparatorText("Master");
        ImGui::EndChild();

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

        // Display debug information
        if (ImGui::IsKeyPressed(ImGuiKey_D, false) && ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
            debug = !debug;

        if (debug) {
            ImGui::SeparatorText("Debug");
            ImGui::Text("Current s. index: %d", audioEngine.granEng.index);
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
            ImGui::Text("Current pitch: %.3f", audioEngine.granEng.pitch); 
            ImGui::Text("Playback start: %f, end: %f", audioEngine.start, audioEngine.end);
        }
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