/* granular.cpp
Objects and functions relating to grains */

#include <iostream>

#include "granular.h"

// Hermite polynomial interpolation from https://stackoverflow.com/questions/1125666/how-do-you-do-bicubic-or-other-non-linear-interpolation-of-re-sampled-audio-da
static float Interpolate4(float x0, float x1, float x2, float x3, float t) {
	float c0 = x1;
	float c1 = .5F * (x2 - x0);
	float c2 = x0 - (2.5F * x1) + (2 * x2) - (.5F * x3);
	float c3 = (.5F * (x3 - x0)) + (1.5F * (x1 - x2));
	return (((((c3 * t) + c2) * t) + c1) * t) + c0;
}

// -- Grain class defs --
Grain::Grain(AudioFileData* audioData) 
    : data(audioData), start(0), size(0), index(0.0f), interval(0.0f),
      pan(0.5f), envelope(0.0f), isPlaying(false) {}

void Grain::outputStereo(float& l, float& r) {
    if (!isPlaying || !data || index / abs(interval) >= size || (start + size) * 2 >= data->size || index < 0) {
        isPlaying = false;
        return;
    }
    // hann window
    envelope = cosf(2.0f * M_PI * (index / abs(interval) / size) + M_PI) / 2.0f + 0.5f;
    int n = static_cast<int>(start + index) * 2;
    float t = index - floor(index);
    std::vector<float> p = get4Points(n);
    l += Interpolate4(p[0], p[1], p[2], p[3], t)
        * envelope
        * (1.0f - pan);
    p = get4Points(n+1);
    r += Interpolate4(p[0], p[1], p[2], p[3], t)
        * envelope
        * pan;
    index += interval;
}

void Grain::trigger(int grainStart, int grainLength, float grainPan, float pitch, bool reverse) {
    size = grainLength;
    pan = grainPan;
    if (reverse) {
        start = grainStart + grainLength - 1;
        interval = -pitch;
        index = grainLength - 1;
    } else {
        start = grainStart;
        interval = pitch;
        index = 0;
    }
    isPlaying = true;
    // for debug:
    //std::cout << "Grain " << i << " triggered at " << s << std::endl;
}

std::vector<float> Grain::get4Points(int n) {
    int s = data->nChannels; // get number of sample points per frame
    int indexes[] = {n-s, n, n+s, n+(s*2)};
    std::vector<float> points(4);
    for (size_t i = 0; i < 4; i++) {
        if (indexes[i] < 0 || indexes[i] > data->size)
            points[i] = 0;
        else 
            points[i] = data->samples[indexes[i]];
    }
    return points;
}

// -- Granular engine class defs --
GranularEngine::GranularEngine(AudioFileData& audiodata) 
    :   audioSize(audiodata.size), grains(MAX_GRAINS, &audiodata), 
        index(0), Hs(6000), Ha(3000), density(2), semitones(0), cents(0), 
        revprob(0), size(0.6f), stretch(2.0f), jitterAmount(0.0f), 
        randomPanAmt(0.0f), spread(0.0f), pitch(1.0f)
{
    std::random_device rd;
    gen = std::mt19937(rd());
    distrib = std::uniform_int_distribution<>(1,100);
    std::cout << "Granular engine created" << std::endl;
}

void GranularEngine::playback(float& l, float& r) {
    for (int i = 0; i < density; i++) {
        // ensure jitter doesn't cause the index to be < 0
        if (index % Hs == std::min(Hs-1, std::max(0, i * Hs / density + jitOffset))) {
            if (jitterAmount > 0) {
                // can shift trigger time up to Hs/2 frames early or late 
                jitOffset = (Hs / -2.0f + distrib(gen) / 100.0f * Hs) * jitterAmount;
            } else {
                jitOffset = 0;
            }
            float pan = 0.5f;
            if (randomPanAmt > 0)
                pan += (distrib(gen) / 100.0f - 0.5f) * randomPanAmt;
            int spreadOffset = 0;
            if (spread >= 0.0004f)
                spreadOffset = spread * (distrib(gen) * audioSize / 100.0f);
            if (!grains[i].isPlaying) {
                grains[i].trigger(
                    index / Hs * Ha + 1.0f * Ha / density * i + spreadOffset, 
                    1.0f * size * Hs - jitOffset, 
                    pan,
                    pitch,
                    distrib(gen) < revprob
                );
            }
            // debug
            /* std::cout << "Grain " << i << " triggered at " 
                << std::max(0, i * Hs / density + jitOffset) << std::endl; */
        }
        if (grains[i].isPlaying)
            grains[i].outputStereo(l, r);
    }
    index++;
}

void GranularEngine::updateParameters(float newSize, float newStretch, int newDensity, int newHa, int newSemitones, int newCents) {
    if (newSize > 0)
        size = newSize;
    if (newStretch > 0) {
        stretch = newStretch;
        //Hs = BASE_HS * stretch;
        //Ha = Hs / stretch;
        Hs = Ha * stretch;
    }
    if (newDensity > 0) {
        for (int i = std::min(density, newDensity); i < std::max(density, newDensity); i++) {
            grains[i].isPlaying = false;
        }
        density = newDensity;
        /* -- for debug: --
        int n = 0;
        for (auto g : grains) {
            std::cout << "Grain " << n;
            g.isPlaying ? std::cout << " isPlaying = true" << std::endl
                : std::cout << " isPlaying = false" << std::endl;
            n++;
        } */
    }
    if (newHa > 0) {
        Ha = newHa;
        Hs = Ha * stretch;
    }
    if (newSemitones >= -24 && newSemitones <= 24) {
        semitones = newSemitones;
        pitch = 1.0f / pow(2.0f, -semitones / 12.0f) * pow(2.0f, cents / 1200.0f);
    }
    if (newCents >= -100 && newCents <= 100) {
        cents = newCents;
        pitch = 1.0f / pow(2.0f, -semitones / 12.0f) * pow(2.0f, cents / 1200.0f);
    }
}