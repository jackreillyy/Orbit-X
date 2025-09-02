#include "SvgSliderComponent.h"

SvgSliderComponent::SvgSliderComponent (const void* trackSvgData, int trackSvgDataSize,
                                        const void* thumbSvgData, int thumbSvgDataSize)
    : juce::Slider(juce::Slider::LinearVertical, juce::Slider::TextBoxBelow)
{
    {
        juce::MemoryInputStream trackStream (trackSvgData, trackSvgDataSize, false);
        trackDrawable = juce::Drawable::createFromImageDataStream (trackStream);
    }
    {
        juce::MemoryInputStream thumbStream (thumbSvgData, thumbSvgDataSize, false);
        thumbDrawable = juce::Drawable::createFromImageDataStream (thumbStream);
    }
    
    setSliderStyle (juce::Slider::LinearVertical);
    setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 20);
}

SvgSliderComponent::~SvgSliderComponent()
{
}

void SvgSliderComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    auto trackArea = bounds.reduced(0.0f, 25.0f);  //  vertical padding

    // track
    if (trackDrawable)
        trackDrawable->drawWithin(g, trackArea, juce::RectanglePlacement::stretchToFit, 1.0f);

    // Thumb positioning
    float minValue = (float)getMinimum();
    float maxValue = (float)getMaximum();
    float currentValue = (float)getValue();
    float proportion = (currentValue - minValue) / (maxValue - minValue);

    float thumbHeight = 20.0f;
    float thumbWidth = bounds.getWidth();
    float thumbY = juce::jmap(proportion, trackArea.getBottom(), trackArea.getY());

    juce::Rectangle<float> thumbRect(bounds.getX(), thumbY - (thumbHeight * 0.5f), thumbWidth, thumbHeight);

    if (thumbDrawable)
    {
        thumbDrawable->drawWithin(g, thumbRect, juce::RectanglePlacement::centred, 1.0f);
    }
    else
    {
        g.setColour(juce::Colours::white);
        g.fillRect(thumbRect);
        g.setColour(juce::Colours::black);
        g.drawRect(thumbRect, 1.0f);
    }
}



