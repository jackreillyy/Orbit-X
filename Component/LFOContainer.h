#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "LFOdsp.h"
#include <juce_core/juce_core.h>
#include <vector>
#include "graphics.h"


//==============================================================================

class LFOVisualizer : public juce::Component, private juce::Timer
{
public:
    LFOVisualizer(LFOdsp& lFOdspRef) : lFOdsp(lFOdspRef)
    {
        startTimerHz(1000);
    }
    void paint(juce::Graphics& g) override
        {
            g.fillAll(juce::Colours::black);
            g.setColour(juce::Colours::white);

            juce::Path lfoPath;
            drawLFO(lfoPath, getLocalBounds().toFloat());
            g.strokePath(lfoPath, juce::PathStrokeType(1.0f));
            
            
        }
    
private:
    LFOdsp& lFOdsp;

    void drawLFO(juce::Path& path, juce::Rectangle<float> bounds)
    {
        auto buffer = lFOdsp.getLFOBuffer();
        int numSamples = buffer.size();

        if (numSamples == 0){
            return;
        }
        
        //path.startNewSubPath(0, bounds.getCentreY());
        float x0 = 0.0f;
        float y0 = juce::jmap<float>(buffer[0], -1.0f, 1.0f, bounds.getBottom(), bounds.getY());
        path.startNewSubPath(x0, y0);


        for (int i = 0; i < numSamples; ++i)
        {
            float x = juce::jmap<float>(i, 0, numSamples - 1, 0, bounds.getWidth());
            float y = juce::jmap<float>(buffer[i], -1.0f, 1.0f, bounds.getBottom(), bounds.getY());

            path.lineTo(x, y);
        }
    }

    void timerCallback() override
    {
        repaint();
    }
};

//==============================================================================

class LFOContainer : public juce::Component
{
public:
    LFOContainer(OrbitXAudioProcessor& p, const juce::String& displayName, const juce::String& parameterPrefix, LFOdsp& lFOdspRef);
    
    ~LFOContainer();

    void paint(juce::Graphics& g) override;
    void resized() override;
    void setComponentEnabled(bool shouldBeEnabled);
    bool getBypassState() const { return bypassState;}
    
    static double getSyncFrequency(double bpm, int noteIndex);
    
    
    void syncBypassState();


private:
    OrbitXAudioProcessor& processor;
    LFOdsp& lFOdsp;
    
    void updateLfoFrequency();
    
    juce::String parameterPrefix;

    juce::GroupComponent groupComponent { "LFOContainer", "LFO" };

    juce::ToggleButton bypassButton;
    bool bypassState = false;
    

    juce::Slider depthSlider;
    juce::Label depthLabel;

    juce::Slider speedSlider;
    juce::Label speedLabel;
    
    float lastHzValue = 1.0f;
    float lastNoteValue = 5.0f;

    juce::ToggleButton rateSwitch;

    LFOVisualizer lfoVisualizer;

    juce::ComboBox shapeSelector;
    CustomComboBoxLookAndFeel comboBoxLNF;

    
    juce::String title;
    
    //bool bypassState = false;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> depthAttachment, speedAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAttachment;
    
    std::unique_ptr<juce::Component> bypassOverlayComponent;

};


