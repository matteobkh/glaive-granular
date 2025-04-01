/* audio.cpp
Handles audio stream and audio processing objects */

#include <iostream>
#include <algorithm>

#include "portaudio.h"
#include "audio.h"
#include "filemanager.h"

#define FRAMES_PER_BUFFER  (256)

static PaStream* stream;

// Following objects declared in audio.h
// -- Grain class defs --
Grain::Grain(std::vector<float>* audioSamples) 
    : data(audioSamples), index(0), start(0), size(0), pan(0.5), envelope(0), isPlaying(false) {}

void Grain::output(float& l, float& r) {
    if (!isPlaying || !data || index >= size || (start + size) * 2 >= (*data).size()) {
        isPlaying = false;
        return;
    }
    // hann window
    envelope = cosf(2.0f * M_PI * (1.0f * index / size) + M_PI) / 2.0f + 0.5f;
    l += (*data)[(start + index) * 2] * envelope * (1.0f - pan);
    r += (*data)[(start + index) * 2 + 1] * envelope * pan;
    index++;
}

void Grain::trigger(int s, int l, float p) {
    start = s;
    size = l;
    pan = p;
    index = 0;
    isPlaying = true;
    // for debug:
    //std::cout << "Grain " << i << " triggered at " << s << std::endl;
}

// -- Granular engine class defs --
GranularEngine::GranularEngine(std::vector<float>& audioSamples) 
    :   audioSize(audioSamples.size()), grains(MAX_GRAINS, &audioSamples), 
        index(0), Hs(6000), Ha(3000), density(2), size(0.6f), stretch(2.0f), 
        jitterAmount(0), randomPanAmt(0), spread(0)
{
    std::random_device rd;
    gen = std::mt19937(rd());
    distrib = std::uniform_int_distribution<>(1,100);
    std::cout << "Granular engine created" << std::endl;
}

void GranularEngine::playback(float& l, float& r) {
    for (int i = 0; i < density; i++) {
        int jitOffset = 0;
        if (jitterAmount > 0) {
            // can shift trigger time up to Hs/2 frames early or late 
            jitOffset = (Hs / -2.0f + distrib(gen) / 100.0f * Hs) * jitterAmount;
        }
        int spreadOffset = 0;
        if (spread > 0) {
            spreadOffset = spread * (distrib(gen) * audioSize / 100.0f);
        }
        // ensure jitter doesn't cause the index to be < 0
        if (index % Hs == std::max(0, i * Hs / density + jitOffset)) {
            float pan = 0.5f;
            if (randomPanAmt > 0)
                pan = (distrib(gen) / 100.0f - 0.5f) * randomPanAmt + 0.5f;
            grains[i].trigger(
                index / Hs * Ha + 1.0f * Ha / density * i + spreadOffset, 
                1.0f * size * Hs, 
                pan
            );
        }
        if (grains[i].isPlaying)
            grains[i].output(l, r);
    }
    index++;
}

void GranularEngine::updateParameters(float newSize, float newStretch, int newDensity, int newHa) {
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
}

// -- AudioEngine struct defs --
AudioEngine::AudioEngine(const int sr, AudioFileData aData, float vol)
    : sampleRate(sr), audioData(aData), granEng(audioData.samples), masterVolume(vol)
{
    std::cout << "AudioEngine created! Sample Rate = " << sampleRate << std::endl;
}
    
void AudioEngine::processAudio(float& l, float& r) {
    if (granularPlaying) {
        granEng.playback(l, r);
    }
    if (granEng.index >= audioData.frames * granEng.stretch) {
        if (!loop) granularPlaying.store(false);
        granEng.index = 0;
    }
    l *= masterVolume;
    r *= masterVolume;
}

// Where audio processing happens for each buffer
static int paCallback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData )
{
    AudioEngine* engine = static_cast<AudioEngine*>(userData);
    float *out = (float*)outputBuffer;
    unsigned long i;

    (void) timeInfo; /* Prevent unused variable warnings. */
    (void) statusFlags;
    (void) inputBuffer;

    for(i=0; i<framesPerBuffer; i++) {
        float left = 0.;
        float right = 0.;

        engine->processAudio(left, right);

        *out++ = left; /* left */
        *out++ = right;  /* right */
    }

    return paContinue;
}

static void StreamFinished( void* userData ) {
    printf("Stream Completed\n");
}

bool openAudio(PaDeviceIndex index, AudioEngine& audioEngine) {
    PaStreamParameters outputParameters;

    outputParameters.device = index;
    if (outputParameters.device == paNoDevice) {
        return false;
    }

    const PaDeviceInfo* pInfo = Pa_GetDeviceInfo(index);
    if (pInfo != 0)
    {
        printf("Output device name: '%s'\r", pInfo->name);
    }

    outputParameters.channelCount = 2;       /* stereo output */
    outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    PaError err = Pa_OpenStream(
        &stream,
        NULL, /* no input */
        &outputParameters,
        audioEngine.sampleRate,
        FRAMES_PER_BUFFER,
        paClipOff,      /* we won't output out of range samples so don't bother clipping them */
        paCallback,
        &audioEngine    // pointer to audio engine to access audio objects
        );

    if (err != paNoError)
    {
        /* Failed to open stream to device !!! */
        return false;
    }

    err = Pa_SetStreamFinishedCallback(stream, &StreamFinished);

    if (err != paNoError)
    {
        Pa_CloseStream( stream );
        stream = 0;

        return false;
    }

    return true;
}

bool closeAudio() {
    if (stream == 0)
        return false;

    PaError err = Pa_CloseStream( stream );
    stream = 0;

    return (err == paNoError);
}


bool startAudio() {
    if (stream == 0)
        return false;

    PaError err = Pa_StartStream( stream );

    return (err == paNoError);
}

bool stopAudio() {
    if (stream == 0)
        return false;

    PaError err = Pa_StopStream( stream );

    return (err == paNoError);
}