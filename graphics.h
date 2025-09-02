#pragma once
#include <JuceHeader.h>
namespace JackGraphics {

using namespace juce;

class SimpleLnF : public juce::LookAndFeel_V4
{
public:
    
    SimpleLnF()  
    {
        SliderColour = Colours::white;
    }
    void setColour(Colour c)
    {
        SliderColour = c;
    }
    
    void setColour(Colour c, Colour b1){
        SliderColour = c;
        BackgroundColourOne = b1;
        BackgroundColourTwo = SliderColour;
        
    }
    
    void setColour(Colour c, Colour b1, Colour b2){
        SliderColour = c;
        BackgroundColourOne = b1;
        BackgroundColourTwo = b2;
    }
    
    void drawRotarySlider(Graphics &, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, Slider &) override;

    void drawLinearSlider (Graphics& g, int x, int y, int width, int height,
                                           float sliderPos,
                                           float minSliderPos,
                                           float maxSliderPos,
                                      const Slider::SliderStyle style, Slider& slider) override;
    
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box) override;

    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                           bool isSeparator, bool isActive, bool isHighlighted,
                           bool isTicked, bool hasSubMenu, const juce::String& text,
                           const juce::String& shortcutKeyText, const juce::Drawable* icon,
                           const juce::Colour* textColour) override;

private:
    Colour SliderColour;
    Colour BackgroundColourOne, BackgroundColourTwo;
};

}
using CustomComboBoxLookAndFeel = JackGraphics::SimpleLnF;


