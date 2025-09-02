#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "distortion.h"
#include "LFOdsp.h"
#include "LFOContainer.h"

//==============================================================================
OrbitXAudioProcessor::OrbitXAudioProcessor() :
     AudioProcessor (BusesProperties()
            .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
            .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
                apvts(*this, nullptr, "Parameters", createParameterLayout()),
                presetManager(std::make_unique<Service::PresetManager>(apvts))
{
//    depthParam = apvts.getRawParameterValue("LFO_Depth");
//    rateParam = apvts.getRawParameterValue("LFO_Rate");
//    syncParam = apvts.getRawParameterValue("LFO_Sync");
//    noteDivisionParam = apvts.getRawParameterValue("LFO_NoteDivision");

    xyXParam = apvts.getRawParameterValue("XY_X");
    xyYParam = apvts.getRawParameterValue("XY_Y");

    postXYDriveParam = apvts.getRawParameterValue("PostXYDrive");
    outputMixParam = apvts.getRawParameterValue("OutputMix");

    bypassParamX = apvts.getRawParameterValue("LFO_X_Bypass");
    bypassParamY = apvts.getRawParameterValue("LFO_Y_Bypass");
    
    apvts.state.setProperty(Service::PresetManager::presetNameProperty, "", nullptr);
    apvts.state.setProperty("version", ProjectInfo::versionString, nullptr);
}

OrbitXAudioProcessor::~OrbitXAudioProcessor()
{
    channelDistortions.clear();
}

double OrbitXAudioProcessor::getBPM() const
{
    return (currentPositionInfo.bpm > 0) ? currentPositionInfo.bpm : 120.0;
}

juce::AudioProcessorValueTreeState::ParameterLayout OrbitXAudioProcessor::createParameterLayout()
{
    using namespace juce;
    
    APVTS::ParameterLayout layout;
    auto checkParam = [](const std::string& id)
    {
        jassert(id.find(" ") == std::string::npos);
    };

    // XY Pad parameters
    layout.add(std::make_unique<AudioParameterFloat>(
        ParameterID("XY_X", 1), "XY X",
        NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    layout.add(std::make_unique<AudioParameterFloat>(
        ParameterID("XY_Y", 2), "XY Y",
        NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    checkParam("PostXYDrive");
    layout.add(std::make_unique<AudioParameterFloat>(
        ParameterID("PostXYDrive",3), "Drive",
        NormalisableRange<float>(1.0f, 10.0f, 0.1f), 5.0f));
    checkParam("OutputMix");
    layout.add(std::make_unique<AudioParameterFloat>(
        ParameterID("OutputMix",4), "Output Mix",
        NormalisableRange<float>(0.0f, 100.0f, 1.0f), 100.0f));
    // LFO parameters
    checkParam("LFO_X_Depth");
    layout.add(std::make_unique<AudioParameterFloat>(
        ParameterID("LFO_X_Depth", 5), "LFO X Depth",
        NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));
    checkParam("LFO_X_Rate");
    layout.add(std::make_unique<AudioParameterFloat>(
        ParameterID("LFO_X_Rate", 6), "LFO X Rate",
        NormalisableRange<float>(0.1f, 20.0f, 0.01f), 0.1f));
    checkParam("LFO_X_Sync");
    layout.add(std::make_unique<AudioParameterBool>(
        ParameterID("LFO_X_Sync", 7), "LFO X Sync", false));
    checkParam("LFO_X_NoteDivision");
    layout.add(std::make_unique<AudioParameterChoice>(
        ParameterID("LFO_X_NoteDivision", 8), "LFO X Note Division",
        StringArray{"1/32", "1/16", "1/16T", "1/8", "1/8T", "1/4", "1/4T",
                    "1/2", "1/2T", "1", "2", "4", "8", "16"}, 5));
    checkParam("LFO_X_Shape");
    layout.add(std::make_unique<AudioParameterChoice>(
        ParameterID("LFO_X_Shape", 9), "LFO X Shape",
        StringArray{"Sine", "Triangle", "Square", "Saw", "Random"}, 0));

    checkParam("LFO_Y_Depth");
    layout.add(std::make_unique<AudioParameterFloat>(
        ParameterID("LFO_Y_Depth", 10), "LFO Y Depth",
        NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));
    checkParam("LFO_Y_Rate");
    layout.add(std::make_unique<AudioParameterFloat>(
        ParameterID("LFO_Y_Rate", 11), "LFO Y Rate",
        NormalisableRange<float>(0.1f, 20.0f, 0.01f), 0.1f));
    checkParam("LFO_Y_Sync");
    layout.add(std::make_unique<AudioParameterBool>(
        ParameterID("LFO_Y_Sync", 12), "LFO Y Sync", false));
    checkParam("LFO_Y_NoteDivision");
    layout.add(std::make_unique<AudioParameterChoice>(
        ParameterID("LFO_Y_NoteDivision", 13), "LFO Y Note Division",
        StringArray{"1/32", "1/16", "1/16T", "1/8", "1/8T", "1/4", "1/4T",
                    "1/2", "1/2T", "1", "2", "4", "8", "16"}, 5));
    checkParam("LFO_Y_Shape");
    layout.add(std::make_unique<AudioParameterChoice>(
        ParameterID("LFO_Y_Shape", 14), "LFO Y Shape",
        StringArray{"Sine", "Triangle", "Square", "Saw", "Random"}, 0));

    checkParam("LFO_X_Bypass");
    layout.add(std::make_unique<AudioParameterBool>(
        ParameterID("LFO_X_Bypass", 15), "LFO X Bypass", false));
    checkParam("LFO_Y_Bypass");
    layout.add(std::make_unique<AudioParameterBool>(
        ParameterID("LFO_Y_Bypass", 16), "LFO Y Bypass", false));

    // Distortion parameters – use a default of 0 (Soft Clip)
    checkParam("Distortion_Right");
    layout.add(std::make_unique<AudioParameterChoice>(
        ParameterID("Distortion_Right", 17), "Distortion Right",
        StringArray{"Soft Clip", "Hard Clip", "Sinusoidal Fold", "Wave Shaped",
                    "Arctan", "Asymmetrical Arctan", "Cascade", "Polynomial",
                    "Rectify", "Logarithmic", "Bitcrusher", "Cubic", "Diode", "Tube", "Chebyshev", "Lofi", "Wavefolder"}, 0));
    checkParam("Distortion_Top");
    layout.add(std::make_unique<AudioParameterChoice>(
        ParameterID("Distortion_Top", 18), "Distortion Top",
        StringArray{"Soft Clip", "Hard Clip", "Sinusoidal Fold", "Wave Shaped",
                    "Arctan", "Asymmetrical Arctan", "Cascade", "Polynomial",
                    "Rectify", "Logarithmic", "Bitcrusher", "Cubic", "Diode", "Tube", "Chebyshev", "Lofi", "Wavefolder"}, 0));
    checkParam("Distortion_Left");
    layout.add(std::make_unique<AudioParameterChoice>(
        ParameterID("Distortion_Left", 19), "Distortion Left",
        StringArray{"Soft Clip", "Hard Clip", "Sinusoidal Fold", "Wave Shaped",
                    "Arctan", "Asymmetrical Arctan", "Cascade", "Polynomial",
                    "Rectify", "Logarithmic", "Bitcrusher", "Cubic", "Diode", "Tube", "Chebyshev", "Lofi", "Wavefolder"}, 0));
    checkParam("Distortion_Bottom");
    layout.add(std::make_unique<AudioParameterChoice>(
        ParameterID("Distortion_Bottom", 20), "Distortion Bottom",
        StringArray{"Soft Clip", "Hard Clip", "Sinusoidal Fold", "Wave Shaped",
                    "Arctan", "Asymmetrical Arctan", "Cascade", "Polynomial",
                    "Rectify", "Logarithmic", "Bitcrusher", "Cubic", "Diode", "Tube", "Chebyshev", "Lofi", "Wavefolder"}, 0));

    return layout;
}

void OrbitXAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void OrbitXAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid()){
        apvts.replaceState(tree);
    }
}

//==============================================================================
const juce::String OrbitXAudioProcessor::getName() const { return JucePlugin_Name; }

bool OrbitXAudioProcessor::acceptsMidi() const { return false; }
bool OrbitXAudioProcessor::producesMidi() const { return false; }
bool OrbitXAudioProcessor::isMidiEffect() const { return false; }
double OrbitXAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int OrbitXAudioProcessor::getNumPrograms() { return 1; }
int OrbitXAudioProcessor::getCurrentProgram() { return 0; }
void OrbitXAudioProcessor::setCurrentProgram (int index) {}
const juce::String OrbitXAudioProcessor::getProgramName (int index) { return {}; }
void OrbitXAudioProcessor::changeProgramName (int index, const juce::String& newName) {}

//==============================================================================
void OrbitXAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Ensure all parameter pointers are assigned after APVTS is fully initialized
//    depthParam = apvts.getRawParameterValue("LFO_Depth");
//    rateParam = apvts.getRawParameterValue("LFO_Rate");
//    syncParam = apvts.getRawParameterValue("LFO_Sync");
//    noteDivisionParam = apvts.getRawParameterValue("LFO_NoteDivision");

    xyXParam = apvts.getRawParameterValue("XY_X");
    xyYParam = apvts.getRawParameterValue("XY_Y");

    postXYDriveParam = apvts.getRawParameterValue("PostXYDrive");
    outputMixParam = apvts.getRawParameterValue("OutputMix");

    lfoX.setSampleRate(sampleRate);
    lfoY.setSampleRate(sampleRate);

    bypassParamX = apvts.getRawParameterValue("LFO_X_Bypass");
    bypassParamY = apvts.getRawParameterValue("LFO_Y_Bypass");

    // Initialize per-channel distortion objects.
    const int numChannels = getTotalNumInputChannels();
    channelDistortions.clear();
    channelDistortions.resize(numChannels);
    
    for (int ch = 0; ch < numChannels; ++ch)
    {
        channelDistortions[ch].softClip       = std::make_unique<JackDistortion::softClip>();
        channelDistortions[ch].hardClip       = std::make_unique<JackDistortion::hardClip>();
        channelDistortions[ch].sinusoidalFold = std::make_unique<JackDistortion::sinusoidalFold>();
        channelDistortions[ch].waveShaper     = std::make_unique<JackDistortion::waveShaped>();
        channelDistortions[ch].arctan         = std::make_unique<JackDistortion::arctan>();
        channelDistortions[ch].asym           = std::make_unique<JackDistortion::asym>();
        channelDistortions[ch].cascade        = std::make_unique<JackDistortion::cascade>();
        channelDistortions[ch].poly           = std::make_unique<JackDistortion::poly>();
        channelDistortions[ch].rectify        = std::make_unique<JackDistortion::rectify>();
        channelDistortions[ch].logarithmic    = std::make_unique<JackDistortion::logarithmic>();
        channelDistortions[ch].bitcrusher     = std::make_unique<JackDistortion::bitcrusher>();
        channelDistortions[ch].cubic          = std::make_unique<JackDistortion::cubic>();
        channelDistortions[ch].diode          = std::make_unique<JackDistortion::diode>();
        channelDistortions[ch].tube           = std::make_unique<JackDistortion::tube>();
        channelDistortions[ch].chebyshev      = std::make_unique<JackDistortion::chebyshev>();
        channelDistortions[ch].lofi           = std::make_unique<JackDistortion::lofi>();
        channelDistortions[ch].wavefolder     = std::make_unique<JackDistortion::wavefolder>();
    }
    smoothedX.reset(sampleRate, 0.3);
    smoothedY.reset(sampleRate, 0.3);
    smoothedX.setCurrentAndTargetValue(*xyXParam);
    smoothedY.setCurrentAndTargetValue(*xyYParam);
    
    smoothedWeightRight.reset(sampleRate, 0.3);
    smoothedWeightTop.reset(sampleRate, 0.3);
    smoothedWeightLeft.reset(sampleRate, 0.3);
    smoothedWeightBottom.reset(sampleRate, 0.3);
    
    // Initialize additional smoothing for LFO modulation.
    modulationSmoothX = 10.0f;
    modulationSmoothY = 10.0f;
    
    smoothedGain.reset(sampleRate, 0.5);     // 0.5 sec smoothing time
    smoothedGain.setCurrentAndTargetValue(1.0f);
    smoothedRMS.reset(sampleRate, 0.5);        // 0.5 sec smoothing for RMS
    smoothedRMS.setCurrentAndTargetValue(0.0f);
    
    smoothedMix.reset(sampleRate, 0.15);       // Smooth transition time for output mix
    smoothedMix.setCurrentAndTargetValue(1.0f);
}

void OrbitXAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool OrbitXAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
//    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
//        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
//        return false;
    
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    
    return true;
}
#endif

//==============================================================================
void OrbitXAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{

    
    //if (*outputMixParam < 0.001f)
    float rawMix  = *outputMixParam;
    float mixFrac = rawMix * 0.01f;
    smoothedMix.setTargetValue(mixFrac);
    if (mixFrac < 0.001f)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.copyFrom(ch, 0, buffer.getReadPointer(ch), buffer.getNumSamples());
        return;
    }


    distortionRightAlgorithm  = static_cast<int>(*apvts.getRawParameterValue("Distortion_Right")) + 1;
    distortionTopAlgorithm    = static_cast<int>(*apvts.getRawParameterValue("Distortion_Top")) + 1;
    distortionLeftAlgorithm   = static_cast<int>(*apvts.getRawParameterValue("Distortion_Left")) + 1;
    distortionBottomAlgorithm = static_cast<int>(*apvts.getRawParameterValue("Distortion_Bottom")) + 1;
    
    playHead = this->getPlayHead();
    if (playHead != nullptr)
    {
        // Update current position info from host
        playHead->getCurrentPosition(currentPositionInfo);
    }
    
    juce::ScopedNoDenormals noDenormals;
    auto numSamples = buffer.getNumSamples();
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
    
    // --- Tempo Sync (unchanged) ---
    if (auto* playHead = getPlayHead())
    {
        juce::AudioPlayHead::CurrentPositionInfo posInfo;
        if (playHead->getCurrentPosition(posInfo))
        {
            bool syncX = (*apvts.getRawParameterValue("LFO_X_Sync") > 0.5f);
            bool syncY = (*apvts.getRawParameterValue("LFO_Y_Sync") > 0.5f);
            
            // Sync LFO phases only when the sync state changes
            if (syncX != previousSyncX || syncY != previousSyncY)
            {
                if (syncX && syncY)
                {
                    lfoX.resetPhase();
                    lfoY.syncPhaseWith(lfoX);
                }
                else if (syncX)
                {
                    lfoX.resetPhase();
                }
                else if (syncY)
                {
                    lfoY.resetPhase();
                }
                previousSyncX = syncX;
                previousSyncY = syncY;
            }
            wasPlayingBefore = posInfo.isPlaying;
        }
    }
    
    double currentSampleRate = getSampleRate();
    lfoX.setSampleRate(currentSampleRate);
    lfoY.setSampleRate(currentSampleRate);
    
    // --- LFO Frequency Settings (unchanged) ---
    float rateX = *apvts.getRawParameterValue("LFO_X_Rate");
    bool syncX = *apvts.getRawParameterValue("LFO_X_Sync") > 0.5f;
    float noteDivisionX = *apvts.getRawParameterValue("LFO_X_NoteDivision");
    int rawShapeX = static_cast<int>(*apvts.getRawParameterValue("LFO_X_Shape"));
    
    if (syncX)
    {
        float syncFreqX = static_cast<float>(LFOContainer::getSyncFrequency(getBPM(), static_cast<int>(noteDivisionX)));
        lfoX.setFrequency(syncFreqX);
    }
    else
    {
        lfoX.setFrequency(rateX);
    }
    
    float rateY = *apvts.getRawParameterValue("LFO_Y_Rate");
    bool syncY = *apvts.getRawParameterValue("LFO_Y_Sync") > 0.5f;
    float noteDivisionY = *apvts.getRawParameterValue("LFO_Y_NoteDivision");
    int rawShapeY = static_cast<int>(*apvts.getRawParameterValue("LFO_Y_Shape"));
    
    if (syncY)
    {
        float syncFreqY = static_cast<float>(LFOContainer::getSyncFrequency(getBPM(), static_cast<int>(noteDivisionY)));
        lfoY.setFrequency(syncFreqY);
    }
    else
    {
        lfoY.setFrequency(rateY);
    }
    
    lfoX.setDepth(*apvts.getRawParameterValue("LFO_X_Depth"));
    lfoX.setWaveform(rawShapeX);
    lfoY.setWaveform(rawShapeY);
    lfoY.setDepth(*apvts.getRawParameterValue("LFO_Y_Depth"));
    
    smoothedX.setTargetValue(*xyXParam);
    smoothedY.setTargetValue(*xyYParam);
    
    // === FIX: Compute LFO modulation once per sample ===
    std::vector<float> lfoValuesX(numSamples, 0.0f);
    std::vector<float> lfoValuesY(numSamples, 0.0f);
    
    for (int i = 0; i < numSamples; ++i)
    {
        float rawModX = (*bypassParamX < 0.5f) ? lfoX.processModulation() : 0.0f;
        float rawModY = (*bypassParamY < 0.5f) ? lfoY.processModulation() : 0.0f;
        
        // Store computed modulation values to use later in per-channel processing.
        lfoValuesX[i] = rawModX;
        lfoValuesY[i] = rawModY;
        
        // Update modulation smoothing used for XY pad mapping.
        modulationSmoothX += (rawModX - modulationSmoothX) * modulationSmoothingCoeff;
        modulationSmoothY += (rawModY - modulationSmoothY) * modulationSmoothingCoeff;
        
        float smoothX = smoothedX.getNextValue();
        float smoothY = smoothedY.getNextValue();
        
        float effectiveX = juce::jlimit(0.0f, 1.0f, smoothX + modulationSmoothX);
        float effectiveY = juce::jlimit(0.0f, 1.0f, smoothY + modulationSmoothY);
        
        // Update debug variables (or further processing) as needed.
        modulatedX = effectiveX;
        modulatedY = effectiveY;
    }
    
    // --- Distortion Processing ---
    const float driveValue = *apvts.getRawParameterValue("PostXYDrive");
    //const float outputMix   = (*apvts.getRawParameterValue("OutputMix")) / 100.0f;
    //const float outputMix = *outputMixParam;  // now in 0–1
    const float outputMix = mixFrac;

    float baseX = *xyXParam;
    float baseY = *xyYParam;

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto& distortions = channelDistortions[channel];

        distortions.softClip->setParameters(driveValue, 0.0f);
        distortions.hardClip->setParameters(driveValue, 0.0f);
        distortions.sinusoidalFold->setParameters(driveValue, 0.0f);
        distortions.waveShaper->setParameters(driveValue, 0.0f);
        distortions.arctan->setParameters(driveValue, 0.0f);
        distortions.asym->setParameters(driveValue, 0.0f);
        distortions.cascade->setParameters(driveValue, 0.0f);
        distortions.poly->setParameters(driveValue, 0.0f);
        distortions.rectify->setParameters(driveValue, 0.0f);
        distortions.logarithmic->setParameters(driveValue, 0.0f);
        distortions.bitcrusher->setParameters(driveValue, 0.0f);
        distortions.cubic->setParameters(driveValue, 0.0f);
        distortions.diode->setParameters(driveValue, 0.0f);
        distortions.tube->setParameters(driveValue, 0.0f);
        distortions.chebyshev->setParameters(driveValue, 0.0f);
        distortions.lofi->setParameters(driveValue, 0.0f);
        distortions.wavefolder->setParameters(driveValue, 0.0f);

        auto* dry = buffer.getReadPointer(channel);
        auto* wet = buffer.getWritePointer(channel);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // === FIX: Use the precomputed LFO modulation values ===
            float currentLfoX = lfoValuesX[sample];
            float currentLfoY = lfoValuesY[sample];
            lfoXValue.store(currentLfoX, std::memory_order_relaxed);
            lfoYValue.store(currentLfoY, std::memory_order_relaxed);
            
            float effectiveX = juce::jlimit(0.0f, 1.0f, baseX + currentLfoX);
            float effectiveY = juce::jlimit(0.0f, 1.0f, baseY + currentLfoY);
            
            float centeredX = (effectiveX - 0.5f) * 2.0f;
            float centeredY = (effectiveY - 0.5f) * 2.0f;
            
            float radius = std::min(1.0f, std::sqrt(centeredX * centeredX + centeredY * centeredY));
            float angle = std::atan2(centeredY, centeredX);
            if (angle < 0)
                angle += juce::MathConstants<float>::twoPi;
            
            // Define ideal angles.
            float idealRight = 0.0f;
            float idealTop   = 3.0f * juce::MathConstants<float>::halfPi;
            float idealLeft  = juce::MathConstants<float>::pi;
            float idealBottom= juce::MathConstants<float>::halfPi;
            
            auto angleDiff = [](float a, float b) -> float {
                float diff = std::fmod(b - a + juce::MathConstants<float>::pi,
                                       2.0f * juce::MathConstants<float>::twoPi) - juce::MathConstants<float>::pi;
                return std::abs(diff);
            };
            
            float diffRight = angleDiff(idealRight, angle);
            float diffTop   = angleDiff(idealTop, angle);
            float diffLeft  = angleDiff(idealLeft, angle);
            float diffBottom= angleDiff(idealBottom, angle);
            
            float sharpness = 2.0f;
            float rawRight = std::exp(-sharpness * diffRight * diffRight);
            float rawTop   = std::exp(-sharpness * diffTop * diffTop);
            float rawLeft  = std::exp(-sharpness * diffLeft * diffLeft);
            float rawBottom= std::exp(-sharpness * diffBottom * diffBottom);
            
            float sumRaw = rawRight + rawTop + rawLeft + rawBottom;
            rawRight /= sumRaw;
            rawTop   /= sumRaw;
            rawLeft  /= sumRaw;
            rawBottom/= sumRaw;
            
            float centerWeight = 0.25f;
            float targetWeightRight = (1.0f - radius) * centerWeight + radius * rawRight;
            float targetWeightTop   = (1.0f - radius) * centerWeight + radius * rawTop;
            float targetWeightLeft  = (1.0f - radius) * centerWeight + radius * rawLeft;
            float targetWeightBottom= (1.0f - radius) * centerWeight + radius * rawBottom;
            
            float totalW = targetWeightRight + targetWeightTop + targetWeightLeft + targetWeightBottom;
            targetWeightRight /= totalW;
            targetWeightTop   /= totalW;
            targetWeightLeft  /= totalW;
            targetWeightBottom/= totalW;
            
            smoothedWeightRight.setTargetValue(targetWeightRight);
            smoothedWeightTop.setTargetValue(targetWeightTop);
            smoothedWeightLeft.setTargetValue(targetWeightLeft);
            smoothedWeightBottom.setTargetValue(targetWeightBottom);
            
            float weightRight = smoothedWeightRight.getNextValue();
            float weightTop   = smoothedWeightTop.getNextValue();
            float weightLeft  = smoothedWeightLeft.getNextValue();
            float weightBottom= smoothedWeightBottom.getNextValue();
            
            float inputSample = dry[sample];
            float outputRight = processDistortionSample(distortionRightAlgorithm, channel, inputSample);
            float outputTop   = processDistortionSample(distortionTopAlgorithm,   channel, inputSample);
            float outputLeft  = processDistortionSample(distortionLeftAlgorithm,  channel, inputSample);
            float outputBottom= processDistortionSample(distortionBottomAlgorithm,channel, inputSample);
            
            float blendedSample = (outputRight * weightRight +
                                   outputTop   * weightTop +
                                   outputLeft  * weightLeft +
                                   outputBottom* weightBottom);
            
            blendedSample *= 0.25f;
            float currentOutputMix = smoothedMix.getNextValue();
            wet[sample] = (blendedSample * currentOutputMix) + (inputSample * (1.0f - currentOutputMix));
        }
    }
    
    // --- Post-Distortion Gain Processing ---
    applyPostDistortionGain(buffer);
}

void OrbitXAudioProcessor::applyPostDistortionGain(juce::AudioBuffer<float>& buffer)
{
    if (( *outputMixParam * 0.01f ) < 0.01f)
        return;
    
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    
    double sumSquares = 0.0;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
            sumSquares += data[i] * data[i];
    }
    double meanSquare = sumSquares / (numChannels * numSamples);
    float measuredRMS = std::sqrt(static_cast<float>(meanSquare));
    measuredRMS = juce::jmax(measuredRMS, 0.001f);
    
    smoothedRMS.setTargetValue(measuredRMS);
    float smoothRMS = smoothedRMS.getNextValue();
    
    float targetRMS = 0.707f;  // ~ -3 dBFS for sine wave
    float desiredGain = targetRMS / smoothRMS;
        desiredGain = juce::jlimit(0.01f, 5.0f, desiredGain);
    //desiredGain = juce::jlimit(0.0f, 10.0f, desiredGain);
    
    float currentGain = smoothedGain.getCurrentValue();
    float sampleRateF = static_cast<float>(getSampleRate());
    float attackTime = 0.5f;
    float releaseTime = 1.5f;
    float attackCoeff = 1.0f - std::exp(-1.0f / (attackTime * sampleRateF));
    float releaseCoeff = 1.0f - std::exp(-1.0f / (releaseTime * sampleRateF));
    
    if (desiredGain > currentGain)
        currentGain += (desiredGain - currentGain) * attackCoeff;
    else
        currentGain += (desiredGain - currentGain) * releaseCoeff;
    
    smoothedGain.setCurrentAndTargetValue(currentGain);
    
    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
            data[i] *= currentGain;
    }
}

//==============================================================================
void OrbitXAudioProcessor::updateDSP(float drive, float mix)
{
    for (auto& channel : channelDistortions)
    {
        if (channel.softClip)
            channel.softClip->setParameters(drive, 0.0f);
        if (channel.hardClip)
            channel.hardClip->setParameters(drive, 0.0f);
        if (channel.sinusoidalFold)
            channel.sinusoidalFold->setParameters(drive, 0.0f);
        if (channel.waveShaper)
            channel.waveShaper->setParameters(drive, 0.0f);
        if (channel.arctan)
            channel.arctan->setParameters(drive, 0.0f);
        if (channel.asym)
            channel.asym->setParameters(drive, 0.0f);
        if (channel.cascade)
            channel.cascade->setParameters(drive, 0.0f);
        if (channel.poly)
            channel.poly->setParameters(drive, 0.0f);
        if (channel.rectify)
            channel.rectify->setParameters(drive, 0.0f);
        if (channel.logarithmic)
            channel.logarithmic->setParameters(drive, 0.0f);
        if (channel.bitcrusher)
            channel.bitcrusher->setParameters(drive, 0.0f);
        if (channel.cubic)
            channel.cubic->setParameters(drive, 0.0f);
        if (channel.diode)
            channel.diode->setParameters(drive, 0.0f);
        if (channel.tube)
            channel.tube->setParameters(drive, 0.0f);
        if (channel.chebyshev)
            channel.chebyshev->setParameters(drive, 0.0f);
        if (channel.lofi)
            channel.lofi->setParameters(drive, 0.0f);
        if (channel.wavefolder)
            channel.wavefolder->setParameters(drive, 0.0f);
    }
}

//==============================================================================
bool OrbitXAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* OrbitXAudioProcessor::createEditor()
{
    return new OrbitXAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OrbitXAudioProcessor();
}

float OrbitXAudioProcessor::processDistortionSample(int algID, int channel, float sample)
{
    auto& distortions = channelDistortions[channel];
    switch (algID)
    {
        case 1: // Soft Clip Distortion
            return distortions.softClip->processSample(sample);
        case 2: // Hard Clip Distortion
            return distortions.hardClip->processSample(sample);
        case 3: // Sinusoidal Fold Distortion
            return distortions.sinusoidalFold->processSample(sample);
        case 4: // Wave Shaped Distortion
            return distortions.waveShaper->processSample(sample);
        case 5: // Arctan Distortion
            return distortions.arctan->processSample(sample);
        case 6: // Asymmetrical Arctan Distortion
            return distortions.asym->processSample(sample);
        case 7: // Cascade Distortion
            return distortions.cascade->processSample(sample);
        case 8: // Polynomial Distortion
            return distortions.poly->processSample(sample);
        case 9: // Full Wave Rectify Distortion
            return distortions.rectify->processSample(sample);
        case 10: // Logarithmic Distortion
            return distortions.logarithmic->processSample(sample);
        case 11: // Bitcrusher Distortion
            return distortions.bitcrusher->processSample(sample);
        case 12: // cubic Distortion
            return distortions.cubic->processSample(sample);
        case 13: // Diode Distortion
            return distortions.diode->processSample(sample);
        case 14: // Tube Distortion
            return distortions.tube->processSample(sample);
        case 15: // Chebyshev Distortion
            return distortions.chebyshev->processSample(sample);
        case 16: // Lofi Distortion
            return distortions.lofi->processSample(sample);
        case 17: // Wavefolder Distortion
            return distortions.wavefolder->processSample(sample);
        default:
            return distortions.softClip->processSample(sample);
    }
}

void OrbitXAudioProcessor::setDistortionRightAlgorithm(int alg)
{
    distortionRightAlgorithm = alg;
}

void OrbitXAudioProcessor::setDistortionTopAlgorithm(int alg)
{
    distortionTopAlgorithm = alg;
}

void OrbitXAudioProcessor::setDistortionLeftAlgorithm(int alg)
{
    distortionLeftAlgorithm = alg;
}

void OrbitXAudioProcessor::setDistortionBottomAlgorithm(int alg)
{
    distortionBottomAlgorithm = alg;
}

void OrbitXAudioProcessor::setDistortionAParameter(float value)
{
    distortionAParam = value;
}

void OrbitXAudioProcessor::setDistortionBParameter(float value)
{
    distortionBParam = value;
}

void OrbitXAudioProcessor::setDistortionCParameter(float value)
{
    distortionCParam = value;
}

void OrbitXAudioProcessor::setDistortionDParameter(float value)
{
    distortionDParam = value;
}

void OrbitXAudioProcessor::syncLFOPhases()
{
    lfoX.resetPhase();
    lfoY.syncPhaseWith(lfoX);
}



