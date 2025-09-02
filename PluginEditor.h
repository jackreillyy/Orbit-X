#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "XyPad.h"
#include "graphics.h"
#include "LFOContainer.h"
#include "XYPadLabels.h"
#include "PresetPanel.h"
#include "SvgSliderComponent.h"

class OrbitXAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   public juce::Timer
{
public:
    OrbitXAudioProcessorEditor(OrbitXAudioProcessor&);
    ~OrbitXAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    
    juce::Image cachedNoiseImage;
    int cachedNoiseWidth = 0, cachedNoiseHeight = 0;

private:
    OrbitXAudioProcessor& audioProcessor;
    juce::Image titleText;
    int titleWidth = 0;
    int titleHeight = 0;
    
    void drawTitle (juce::Graphics& g);

    Gui::XyPad xyPad1;
    std::unique_ptr<XYPadLabels> xyPadLabels;
    
    std::unique_ptr<Gui::PresetPanel> presetPanel;

    void xyPad1Moved(float x, float y);

    std::unique_ptr<SvgSliderComponent> driveSlider;
    juce::Label driveLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveATTACH;

    std::unique_ptr<SvgSliderComponent> mixSlider;
    juce::Label outputMixLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputATTACH;
    
    std::unique_ptr<LFOContainer> lfoXContainer;
    std::unique_ptr<LFOContainer> lfoYContainer;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rightDistortionAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> topDistortionAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> leftDistortionAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> bottomDistortionAttachment;

    // Move the global LookAndFeel pointer to the bottom
    std::unique_ptr<JackGraphics::SimpleLnF> lnf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrbitXAudioProcessorEditor)
};


