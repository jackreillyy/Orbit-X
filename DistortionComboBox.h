#pragma once

#include <JuceHeader.h>
#include "graphics.h"

class DistortionComboBox : public juce::ComboBox
{
public:
    DistortionComboBox();
    ~DistortionComboBox() override;

    void paint(juce::Graphics& g) override;
    
    std::function<void(int)> onSelectionChanged;

private:
    CustomComboBoxLookAndFeel customLNF;
};


