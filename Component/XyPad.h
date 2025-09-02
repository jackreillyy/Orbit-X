#pragma once

#include <JuceHeader.h>
#include <mutex>
#include <vector>
#include <algorithm>
#include <functional>

namespace Gui
{

class XyPad : public juce::Component,
public juce::Slider::Listener, private juce::Timer
    {
    public:
        enum class Axis { X, Y };

        XyPad();
        ~XyPad() override;

        void resized() override;
        void paint(juce::Graphics& g) override;

        void registerSlider(juce::Slider* slider, Axis axis);
        void deRegisterSlider(juce::Slider* slider);

        void sliderValueChanged(juce::Slider* slider) override;
        
        void timerCallback() override;

        static constexpr int thumbSize = 15;
        std::function<void(float, float)> onXYChange;
        
        void setThumbPositionNormalized(float normX, float normY);
        
        bool flipX = false;
        bool flipY = false;
        
        bool isBeingDragged() const { return userIsDragging; }


        class Thumb : public juce::Component
        {
        public:
            Thumb();
            void paint(juce::Graphics& g) override;
            void mouseDown(const juce::MouseEvent& event) override;
            void mouseDrag(const juce::MouseEvent& event) override;
            void mouseUp(const juce::MouseEvent& event) override;
            void thumbDrag(const juce::MouseEvent& event);
            
            std::function<void(juce::Point<double>)> moveCallback;
            juce::ComponentDragger dragger;
            // Use our custom circular bounds constrainer instead of the base class.
            ComponentBoundsConstrainer constrainer;
        };

    private:
        Thumb thumb;
        std::mutex vectorMutex;
        std::vector<juce::Slider*> X;
        std::vector<juce::Slider*> Y;
        
        bool userIsDragging = false;

        void mouseDown(const juce::MouseEvent& event) override;
        void mouseUp(const juce::MouseEvent& event) override;
        // The following constrainer is for the XyPad component itself.
        juce::ComponentBoundsConstrainer constrainer;
        //JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Thumb);
    };
}

