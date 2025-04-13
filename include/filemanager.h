#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <vector>
#include <string>

// Stores audio data from file
struct AudioFileData {
    std::vector<float> samples;
    int nChannels, sampleRate;
    size_t size;
    int frames;
    AudioFileData(std::vector<float> samplesVec = {}, int numChannels = 1, int sRate = 44100);
};

namespace FileManager {
    inline bool fileLoaded = false;
    inline bool loading = false;
    inline std::string currentFileName;

    // Extract audio data from file and store it in an AudioFileData struct
    AudioFileData LoadAudioFile(std::string filename);
}



#endif // FILEMANAGER_H