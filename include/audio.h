#ifndef AUDIO_H
#define AUDIO_H

#include <atomic>
#include <vector>
#include <random>

#include "portaudio.h"

#include "filemanager.h"
#include "granular.h"

// Struct that contains all audio objects used in the program for easy access
struct AudioEngine {
    int sampleRate;
    AudioFileData audioData;

    GranularEngine granEng;
    std::atomic<bool> granularPlaying{false};
    bool loop = false;
    float start = 0.0f; //defines lower playback bound
    float end = 1.0f; //defines upper playback bound
    
    // room for more objects

    std::atomic<float> masterVolume; // Master volume

    // Constructor, please specify sample rate
    AudioEngine(const int sr, AudioFileData aData, float vol = 1.0f);
    
    void processAudio(float& l, float& r);
};

// Handles PortAudio initialization
class ScopedPaHandler
{
public:
    ScopedPaHandler()
        : _result(Pa_Initialize())
    {
    }
    ~ScopedPaHandler()
    {
        if (_result == paNoError)
        {
            Pa_Terminate();
        }
    }

    PaError result() const { return _result; }

private:
    PaError _result;
};

// Control audio stream
bool openAudio(PaDeviceIndex index, AudioEngine& audioEngine);
bool closeAudio();
bool startAudio();
bool stopAudio();

#endif // AUDIO_H