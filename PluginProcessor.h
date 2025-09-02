#pragma once

#include <JuceHeader.h>
#include "distortion.h"
#include "LFOdsp.h"
#include <juce_dsp/juce_dsp.h>
#include "PresetManager.h"

//==============================================================================
// Define a struct to hold per-channel distortion instances.
struct ChannelDistortions
{
    std::unique_ptr<JackDistortion::softClip>       softClip;
    std::unique_ptr<JackDistortion::hardClip>       hardClip;
    std::unique_ptr<JackDistortion::sinusoidalFold>   sinusoidalFold;
    std::unique_ptr<JackDistortion::waveShaped>       waveShaper;
    std::unique_ptr<JackDistortion::arctan>           arctan;
    std::unique_ptr<JackDistortion::asym>             asym;
    std::unique_ptr<JackDistortion::cascade>          cascade;
    std::unique_ptr<JackDistortion::poly>             poly;
    std::unique_ptr<JackDistortion::rectify>          rectify;
    std::unique_ptr<JackDistortion::logarithmic>      logarithmic;
    std::unique_ptr<JackDistortion::bitcrusher>       bitcrusher;
    std::unique_ptr<JackDistortion::cubic>            cubic;
    std::unique_ptr<JackDistortion::diode>            diode;
    std::unique_ptr<JackDistortion::tube>             tube;
    std::unique_ptr<JackDistortion::chebyshev>        chebyshev;
    std::unique_ptr<JackDistortion::lofi>             lofi;
    std::unique_ptr<JackDistortion::wavefolder>       wavefolder;
};

class OrbitXAudioProcessor  : public juce::AudioProcessor
{
public:
    OrbitXAudioProcessor();
    ~OrbitXAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    Service::PresetManager& getPresetManager(){
        return *presetManager;
    }
    
    // APVTS for parameter management.
    using APVTS = juce::AudioProcessorValueTreeState;
    static APVTS::ParameterLayout createParameterLayout();
    APVTS apvts {*this, nullptr, "Parameters", createParameterLayout()};
    
    void updateDSP(float drive, float mix);
    
    // Mapping variables for converting LFO output to XY pad values.
    float baseX = 0.5f;
    float baseY = 0.5f;
    float maxXOffset = 0.5f;
    float maxYOffset = 0.5f;
    float modulatedX = 0.5f;
    float modulatedY = 0.5f;
    float lfoModX = 0.0f;
    float lfoModY = 0.0f;
    
    // LFO parameter pointers
//    std::atomic<float>* depthParam = nullptr;
//    std::atomic<float>* rateParam = nullptr;
//    std::atomic<float>* syncParam = nullptr;
//    std::atomic<float>* noteDivisionParam = nullptr;
    
    std::atomic<float>        lfoXValue { 0.0f };
    std::atomic<float>        lfoYValue { 0.0f };
    
    // XY Pad parameter pointers
    std::atomic<float>* xyXParam = nullptr;
    std::atomic<float>* xyYParam = nullptr;

    // Drive and Output Mix parameter pointers
    std::atomic<float>* postXYDriveParam = nullptr;
    std::atomic<float>* outputMixParam = nullptr;

    double getBPM() const;
    
    // LFO DSP instances.
//    LFOdsp lfo; // Unused in modulation
    LFOdsp lfoX;
    LFOdsp lfoY;
//    LFOdsp lfoDsp; // Not updated
    
    void applyLFOtoModulation();
    
    std::atomic<float>* bypassParamX = nullptr;
    std::atomic<float>* bypassParamY = nullptr;
    
    void syncLFOPhases();
    
    LFOdsp& getLFOdsp(){
        return lfoX;
    }
    
    LFOdsp& getLFOX(){
        return lfoX;
    }
    LFOdsp& getLFOY(){
        return lfoY;
    }
    
    bool wasPlayingBefore = false;
    
    bool previousSyncX = false;
    bool previousSyncY = false;
    
    // Distortion algorithm identifiers.
    int distortionRightAlgorithm = 1;
    int distortionTopAlgorithm = 1;
    int distortionLeftAlgorithm = 1;
    int distortionBottomAlgorithm = 1;
    
    // Additional distortion parameters.
    float distortionAParam = 0.5f;
    float distortionBParam = 0.5f;
    float distortionCParam = 0.5f;
    float distortionDParam = 0.5f;
    
    juce::SmoothedValue<float> smoothedWeightRight;
    juce::SmoothedValue<float> smoothedWeightTop;
    juce::SmoothedValue<float> smoothedWeightLeft;
    juce::SmoothedValue<float> smoothedWeightBottom;
    
    juce::SmoothedValue<float> smoothedX;
    juce::SmoothedValue<float> smoothedY;
    float modulationSmoothX = 0.0f;
    float modulationSmoothY = 0.0f;
    const float modulationSmoothingCoeff = 0.05f;
    
    juce::SmoothedValue<float> smoothedMix;
    
    juce::SmoothedValue<float> smoothedGain;
    juce::SmoothedValue<float> smoothedRMS;
    
    float processDistortionSample(int algID, int channel, float sample);

    void setDistortionRightAlgorithm(int alg);
    void setDistortionTopAlgorithm(int alg);
    void setDistortionLeftAlgorithm(int alg);
    void setDistortionBottomAlgorithm(int alg);
    
    void setDistortionAParameter(float value);
    void setDistortionBParameter(float value);
    void setDistortionCParameter(float value);
    void setDistortionDParameter(float value);
    
    void applyPostDistortionGain(juce::AudioBuffer<float>& buffer);

private:
    std::vector<ChannelDistortions> channelDistortions;
    float getLfoFrequencyFromSync(float noteDivisionValue, double bpm);
    void updateHostBPM();
    juce::AudioPlayHead* playHead = nullptr;
    juce::AudioPlayHead::CurrentPositionInfo currentPositionInfo;
    
    std::unique_ptr<Service::PresetManager> presetManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrbitXAudioProcessor)
};



