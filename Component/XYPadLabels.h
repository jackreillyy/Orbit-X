#pragma once
#include <JuceHeader.h>
#include "DistortionComboBox.h"
#include "graphics.h"


class XYPadLabels : public juce::Component
{
public:
    XYPadLabels()
    {
        setInterceptsMouseClicks(false, true);
        setOpaque(false);

        // Create combo boxes for each axis.
        rightComboBox  = std::make_unique<DistortionComboBox>();
        topComboBox    = std::make_unique<DistortionComboBox>();
        leftComboBox   = std::make_unique<DistortionComboBox>();
        bottomComboBox = std::make_unique<DistortionComboBox>();

        // Add the combo boxes as child components.
        addAndMakeVisible(rightComboBox.get());
        addAndMakeVisible(topComboBox.get());
        addAndMakeVisible(leftComboBox.get());
        addAndMakeVisible(bottomComboBox.get());
        
    }
    
    ~XYPadLabels() {
        rightComboBox->setLookAndFeel(nullptr);
        topComboBox->setLookAndFeel(nullptr);
        leftComboBox->setLookAndFeel(nullptr);
        bottomComboBox->setLookAndFeel(nullptr);
        
    }

    void resized() override
    {
        auto bounds = getLocalBounds().toFloat();
        
        // Individual margins for the "circle" area.
        float leftMargin   = 30.0f;
        float rightMargin  = 30.0f;
        float topMargin    = 30.0f;
        float bottomMargin = 30.0f;
        
        // Compute the area available for drawing the circle.
        juce::Rectangle<float> circleArea(
            bounds.getX() + leftMargin,
            bounds.getY() + topMargin,
            bounds.getWidth() - leftMargin - rightMargin,
            bounds.getHeight() - topMargin - bottomMargin
        );
        
        // Define the circle's diameter as the smaller of the available width and height.
        float diameter = std::min(circleArea.getWidth(), circleArea.getHeight());
        
        // Create a square for the circle and center it within the full bounds.
        juce::Rectangle<float> circleBounds;
        circleBounds.setSize(diameter, diameter);
        circleBounds.setCentre(bounds.getCentre());
        
        // Determine the center point and radius.
        auto center = circleBounds.getCentre();
        float radius = diameter * 0.5f;
        
        // Fixed combo box dimensions and a small offset.
        int comboWidth = 100;
        int comboHeight = 20;
        int offset = 5;
        
        // Position the right combo box to the right of the circle.
        // It will be placed just outside the circle's right boundary.
        rightComboBox->setBounds(static_cast<int>(center.x + radius + offset),
                                  static_cast<int>(center.y - comboHeight / 2),
                                  comboWidth, comboHeight);
        rightComboBox->toFront(true);

        
        // Position the top combo box above the circle.
        topComboBox->setBounds(static_cast<int>(center.x - comboWidth * 0.5f),
                              static_cast<int>(circleBounds.getY() - comboHeight - offset),
                              comboWidth, comboHeight);
        topComboBox->toFront(true);

        
        // Position the left combo box to the left of the circle.
        leftComboBox->setBounds(static_cast<int>(circleBounds.getX() - comboWidth - offset),
                                static_cast<int>(center.y - comboHeight / 2),
                                comboWidth, comboHeight);
        leftComboBox->toFront(true);

        
        // Position the bottom combo box below the circle.
        bottomComboBox->setBounds(static_cast<int>(center.x - comboWidth * 0.5f),
                                  static_cast<int>(circleBounds.getBottom() + offset),
                                  comboWidth, comboHeight);
        bottomComboBox->toFront(true);

    }
    
    DistortionComboBox* getRightCombo()  { return rightComboBox.get(); }
    DistortionComboBox* getTopCombo()    { return topComboBox.get(); }
    DistortionComboBox* getLeftCombo()   { return leftComboBox.get(); }
    DistortionComboBox* getBottomCombo() { return bottomComboBox.get(); }
    
private:
    std::unique_ptr<DistortionComboBox> rightComboBox;
    std::unique_ptr<DistortionComboBox> topComboBox;
    std::unique_ptr<DistortionComboBox> leftComboBox;
    std::unique_ptr<DistortionComboBox> bottomComboBox;
    
};


