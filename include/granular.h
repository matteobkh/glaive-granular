#ifndef GRANULAR_H
#define GRANULAR_H

#include <vector>
#include <random>

#include "filemanager.h"

#define MAX_GRAINS (20)

// Stereo grain with dynamic envelope
class Grain {
private:
    AudioFileData* data;
    int start, size; // size could be a percentage of Hs
    float index, interval, pan, envelope;
public:
    bool isPlaying;

    Grain(AudioFileData* audioSamples = nullptr) ;

    void outputStereo(float& l, float& r);

    void trigger(int grainStart, int grainLength, float grainPan, float pitch, bool reverse);

    // Return values from audio data around point n necessary for interpolation
    std::vector<float> get4Points(int n);

    // Returns current index relative to the inputted audio samples
    inline int getCurrentRelIndex() { return index + start; };

    inline float getEnvelope() {
        return isPlaying ? envelope : 0;
    }
};
    
class GranularEngine {
private:
    int jitOffset = 0;
    std::mt19937 gen; // mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> distrib;
    int audioSize;
public:
    std::vector<Grain> grains;
    int index, Hs, Ha, density, semitones, cents, revprob;
    // size ∈ [0,1), jitterAmount ∈ [0,1], randomPanAmt ∈ [0,1], 
    float size, stretch, jitterAmount, randomPanAmt, spread, pitch;

    GranularEngine(AudioFileData& audioSamples);

    void playback(float& l, float& r);

    // update parameters, to leave parameters the same input any number <=0
    void updateParameters(float newSize = 0, float newStretch = 0, int newDensity = 0, int newHa = 0, int newSemitones = 25, int newCents = 101);
};

#endif //GRANULAR_H