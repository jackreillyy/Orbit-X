#include "DistortionComboBox.h"
//#pragma once
#include <JuceHeader.h>
#include "graphics.h"


DistortionComboBox::DistortionComboBox()
{
    setLookAndFeel(&customLNF);

    addItem("Analog Clip", 1);
    addItem("Hard Clip", 2);
    addItem("Sinusoidal", 3);
    addItem("Waveshaped", 4);
    addItem("Arctan", 5);
    addItem("Asym", 6);
    addItem("Cascade", 7);
    addItem("Poly", 8);
    addItem("Rectify", 9);
    addItem("Log", 10);
    addItem("Bitcrusher", 11);
    addItem("Cubic", 12);
    addItem("Diode", 13);
    addItem("Tube", 14);
    addItem("Chebyshev", 15);
    addItem("Lofi", 16);
    addItem("Wavefolder", 17);
    
    setJustificationType(juce::Justification::centred);
    setSelectedId(1, juce::dontSendNotification);
    
    onChange = [this]() {
        if (onSelectionChanged)
            onSelectionChanged(getSelectedId());
    };
}

void DistortionComboBox::paint(juce::Graphics& g)
{
    juce::ComboBox::paint(g);
}

DistortionComboBox::~DistortionComboBox()
{
    setLookAndFeel(nullptr);
}

