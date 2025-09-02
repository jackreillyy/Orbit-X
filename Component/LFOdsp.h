
#pragma once

#include <JuceHeader.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <vector>
#include <mutex>

class LFOdsp
{
public:
    
    LFOdsp();
    
    void setSampleRate(double newSampleRate);
    void setFrequency(float newFrequency, bool resetPhase = false);
    void setWaveform(int newWaveformType);
    
    void setSyncMode(bool shouldSync);
    void setSyncNoteDivision(float noteDivision, double bpm);
    void setDepth(float newDepth);
    
    void resetPhase();
    void syncPhaseWith(const LFOdsp& other);
    
    float processModulation();
    float getCurrentModulation() const;

    bool getSyncMode() const { return syncMode; }
    
    std::vector<float> getLFOBuffer();
    
private:
    
    double sampleRate = 48000.0;
    
    float frequency = 1.0f;
    float depth = 1.0f;
    
    bool syncMode = false;
    float noteDivision = 5.0f;
    double hostBPM = 120.0;
    
    int waveformType = 0;
    
    double phase = 0.0;
    double phaseIncrement = 0.0;
    
    void updatePhaseIncrement();
    
    static constexpr int bufferSize = 8192;
    std::vector<float> lfoBuffer;
    
//    int writeIndex = 0;
//    std::mutex bufferMutex;
    std::atomic<int> writeIndex { 0 };
    
    float currentModulation = 0.0f;
    
    juce::Random randomGenerator;
    float lastRandomValue = 0.0f;
};

