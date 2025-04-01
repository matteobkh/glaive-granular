/* filemanager.cpp
Handles extracting audio data from files and holding list of audio files in
working directory */

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

// Include all required dr_libs
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"
#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

#include "filemanager.h"

// -- global variables definitions --
bool FileManager::fileSelected = false;
std::vector<std::string> files;

// AudioFileData struct def
AudioFileData::AudioFileData(std::vector<float> samplesVec, int numChannels, int sRate) 
    : samples(samplesVec), nChannels(numChannels), sampleRate(sRate), size(samples.size()), frames(size/nChannels) {}

AudioFileData FileManager::LoadAudioFile(std::string filename) {
    std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower); // Normalize extension

    if (filename.ends_with(".wav")) {
        // Read WAV
        unsigned int channels;
        unsigned int sampleRate;
        drflac_uint64 totalPCMFrameCount;
        float* pSampleData = drwav_open_file_and_read_pcm_frames_f32(filename.c_str(), &channels, &sampleRate, &totalPCMFrameCount, NULL);
        if (pSampleData == NULL) {
            std::cerr << "Failed to open and decode WAV file: " << filename << std::endl;
            return {};
        }
        std::vector<float> samples {pSampleData, pSampleData + channels * totalPCMFrameCount};
        AudioFileData data(samples, channels, sampleRate);
        drwav_free(pSampleData, NULL);
        return data;
    }
    else if (filename.ends_with(".flac")) {
        // Read FLAC
        unsigned int channels;
        unsigned int sampleRate;
        drflac_uint64 totalPCMFrameCount;
        float* pSampleData = drflac_open_file_and_read_pcm_frames_f32(filename.c_str(), &channels, &sampleRate, &totalPCMFrameCount, NULL);
        if (pSampleData == NULL) {
            std::cerr << "Failed to open and decode FLAC file: " << filename << std::endl;
            return {};
        }
        std::vector<float> samples {pSampleData, pSampleData + channels * totalPCMFrameCount};
        AudioFileData data(samples, channels, sampleRate);
        drflac_free(pSampleData, NULL);
        return data;
    }
    else if (filename.ends_with(".mp3")) {
        // Read MP3
        drmp3_config conf;
        drmp3_uint64 totalPCMFrameCount;
        float* pSampleData = drmp3_open_file_and_read_pcm_frames_f32(filename.c_str(), &conf, &totalPCMFrameCount, NULL);
        if (pSampleData == NULL) {
            std::cerr << "Failed to open and decode MP3 file: " << filename << std::endl;
            return {};
        }
        std::vector<float> samples {pSampleData, pSampleData + conf.channels * totalPCMFrameCount};
        AudioFileData data(samples, conf.channels, conf.sampleRate);
        drmp3_free(pSampleData, NULL);
        return data;
    }

    // Unsupported format
    std::cerr << "Unsupported audio format: " << filename << std::endl;
    return {};
}