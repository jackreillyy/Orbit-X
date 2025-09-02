#pragma once

#include <JuceHeader.h>

class SvgSliderComponent : public juce::Slider
{
public:

    SvgSliderComponent (const void* trackSvgData, int trackSvgDataSize,
                        const void* thumbSvgData, int thumbSvgDataSize);
    ~SvgSliderComponent() override;

    void paint (juce::Graphics& g) override;
    juce::Slider& getSlider() { return *this; }

private:
    // Drawable for the SVG track.
    std::unique_ptr<juce::Drawable> trackDrawable;
    std::unique_ptr<juce::Drawable> thumbDrawable;};

