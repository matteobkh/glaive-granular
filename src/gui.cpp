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

static int currentItem = -1;

/* const char** items = FileManager::files.data();
static const char* current_item = NULL; */

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

    std::vector<const char*> files_cstrs;
    for (const auto& item : FileManager::files)
        files_cstrs.push_back(item.c_str());

    if(ImGui::Combo("Select audio file", &currentItem, files_cstrs.data(), static_cast<int>(files_cstrs.size()))) {
        if (audioEngine.granularPlaying.load()) 
            audioEngine.granularPlaying.store(false);
        audioEngine.audioData = FileManager::LoadAudioFile(FileManager::files[currentItem]);
        std::cout << "Audio file loaded!" << std::endl 
            << "\tFile name: " << FileManager::files[currentItem] << std::endl
            << "\tSample rate: " << audioEngine.audioData.sampleRate << std::endl
            << "\tChannels: " << audioEngine.audioData.nChannels << std::endl 
            << "\tSize (in samples): " << audioEngine.audioData.size << std::endl;
        audioEngine.granEng = GranularEngine(audioEngine.audioData);
        FileManager::fileSelected = true;
    }

    if (FileManager::fileSelected){
        ImVec2 scopeSize = ImVec2(ImGui::GetContentRegionAvail().x,100);

        // scope
        ImGui::Text("Waveform");
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
        int knobsPerRow = 5;
        float windowWidth = ImGui::GetContentRegionAvail().x;
        float padding = ImGui::GetStyle().WindowPadding.x;
        float knobWidth = ImGui::GetTextLineHeight() * 4.0f;  // Default knob size
        float spacing = (windowWidth - (knobsPerRow * knobWidth)) / knobsPerRow; // Even spacing

        // Knob for pitch
        if (ImGuiKnobs::Knob("Pitch", &audioEngine.granEng.pitch, 0.1, 4, knobSpeed, "%.3f", ImGuiKnobVariant_Tick)) {
            // Recalculate the granular engine parameters whenever the grain size is updated
            audioEngine.granEng.updateParameters(audioEngine.granEng.size);
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) { //double click to reset default
            audioEngine.granEng.pitch = 1.0f;
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX(padding + spacing + knobWidth); // Position knob
        // Knob for Grain Size (values between 1 and 2000)
        if (ImGuiKnobs::Knob("Grain Size", &audioEngine.granEng.size, 0.1f, 0.999f, knobSpeed, "%.3f", ImGuiKnobVariant_Tick)) {
            // Recalculate the granular engine parameters whenever the grain size is updated
            audioEngine.granEng.updateParameters(audioEngine.granEng.size);
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) { //double click to reset default
            audioEngine.granEng.updateParameters(0.6f);
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX(padding + 2 * (knobWidth + spacing)); // Position knob
        // Knob for Stretch Factor (values between 0.1 and 10.0)
        if (ImGuiKnobs::Knob("Stretch Factor", &audioEngine.granEng.stretch, 0.1f, 10.0f, knobSpeed, "%.3f", ImGuiKnobVariant_Tick)) {
            // Recalculate the granular engine parameters whenever the stretch factor is updated
            audioEngine.granEng.updateParameters(0, audioEngine.granEng.stretch);
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) { //double click to reset default
            audioEngine.granEng.updateParameters(0, 2.0f);
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX(padding + 3 * (knobWidth + spacing)); // Position knob
        // Knob for Grain Density (values between 1 and 100)
        if (ImGuiKnobs::KnobInt("Grain Density", &audioEngine.granEng.density, 1, MAX_GRAINS, 0.0f, "%d", ImGuiKnobVariant_Stepped, 0, 0, MAX_GRAINS)) {
            // Recalculate the granular engine parameters whenever the grain density is updated
            audioEngine.granEng.updateParameters(0,0,audioEngine.granEng.density);
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) { //double click to reset default
            audioEngine.granEng.updateParameters(0,0,2);
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX(padding + 4 * (knobWidth + spacing)); // Position knob
        // Knob for analysis hopsize (automatically updates synthesis hopsize)
        if (ImGuiKnobs::KnobInt("Hopsize", &audioEngine.granEng.Ha, 100, 8000, knobSpeed, "%d", ImGuiKnobVariant_Tick)) {
            // Recalculate the granular engine parameters whenever the grain size is updated
            audioEngine.granEng.updateParameters(0,0,0,audioEngine.granEng.Ha);
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) { //double click to reset default
            audioEngine.granEng.updateParameters(0,0,0,3000);
        }

        ImGui::Text("Randomization parameters");
        // Jitter knob
        ImGuiKnobs::Knob("Jitter", &audioEngine.granEng.jitterAmount, 0.0f, 1.0f, knobSpeed, "%.3f", ImGuiKnobVariant_Tick);
        if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) { //double click to reset
            audioEngine.granEng.jitterAmount = 0.0f;
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX(padding + spacing + knobWidth); // Position knob
        // Random pan knob
        ImGuiKnobs::Knob("Random pan", &audioEngine.granEng.randomPanAmt, 0.0f, 0.5f, knobSpeed, "%.3f", ImGuiKnobVariant_Tick);
        if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) { //double click to reset
            audioEngine.granEng.randomPanAmt = 0.0f;
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX(padding + 2 * (spacing + knobWidth)); // Position knob
        // Spread knob
        ImGuiKnobs::Knob("Spread", &audioEngine.granEng.spread, 0.0f, 0.5f, knobSpeed, "%.3f", ImGuiKnobVariant_Tick);
        if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) { //double click to reset
            audioEngine.granEng.spread = 0.0f;
        }

        // Display the current values for debugging
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

        // Volume knob
        float mVol = audioEngine.masterVolume.load();
        if (ImGuiKnobs::Knob("Master Volume", &mVol, 0.0f, 1.0f, knobSpeed, "%.2f", ImGuiKnobVariant_Tick)) {
            audioEngine.masterVolume.store(mVol);
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) { //double click to reset
            mVol = 1;
            audioEngine.masterVolume.store(mVol);
        }
    }

    ImGui::End();
}