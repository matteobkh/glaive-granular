#ifndef AUDIO_H
#define AUDIO_H

#include <atomic>
#include <vector>
#include <random>

#include "portaudio.h"

#include "filemanager.h"

#define MAX_GRAINS (20)

// Stereo grain with dynamic envelope
class Grain {
private:
    std::vector<float>* data;
    int index, start, size; // size could be a percentage of Hs
    float pan, envelope;
public:
    bool isPlaying;

    Grain(std::vector<float>* audioSamples = nullptr) ;

    void output(float& l, float& r);

    void trigger(int s, int l, float p);

    // Returns current index relative to the inputted audio samples
    inline int getCurrentRelIndex() { return index + start; };

    inline float getEnvelope() {
        return isPlaying ? envelope : 0;
    }
};
    
class GranularEngine {
private:
    std::mt19937 gen; // mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> distrib;
    int audioSize;
public:
    std::vector<Grain> grains;
    int index, Hs, Ha, density;
    // size ∈ [0,1), jitterAmount ∈ [0,1], randomPanAmt ∈ [0,1], 
    float size, stretch, jitterAmount, randomPanAmt, spread;

    GranularEngine(std::vector<float>& audioSamples);

    void playback(float& l, float& r);

    // update parameters, to leave parameters the same input any number <=0
    void updateParameters(float newSize = 0, float newStretch = 0, int newDensity = 0, int newHa = 0);
};

// Struct that contains all audio objects used in the program for easy access
struct AudioEngine {
    int sampleRate;
    AudioFileData audioData;

    GranularEngine granEng;
    std::atomic<bool> granularPlaying{false};
    bool loop = false;
    
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