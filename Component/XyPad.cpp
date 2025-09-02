#include "XyPad.h"
#include <JuceHeader.h>
#include <limits>
#include <cmath>
#include <juce_gui_basics/juce_gui_basics.h>


namespace
{
    inline double safeJmap(double value,
                           double sourceMin,
                           double sourceMax,
                           double targetMin,
                           double targetMax)
    {
        const auto range = sourceMax - sourceMin;
        if (std::abs(range) < std::numeric_limits<double>::epsilon())
            return targetMin;
        return targetMin + ((value - sourceMin) / range) * (targetMax - targetMin);
    }
}

using namespace juce;

// Custom Circular Bounds Constrainer to lock a component's center inside a circle
class CircularBoundsConstrainer : public juce::ComponentBoundsConstrainer
{
public:
    CircularBoundsConstrainer(juce::Component* parentComp)
        : juce::ComponentBoundsConstrainer(), parentComponent(parentComp)
    {}

    void checkBoundsForComponent(juce::Component* /*component*/,
                                 juce::Rectangle<int>& proposedBounds,
                                 bool /*isStretching*/) const 
    {
        if (parentComponent != nullptr)
        {
            // Use the parent's bounds to define the circle
            juce::Rectangle<int> parentBounds = parentComponent->getLocalBounds();
            juce::Point<int> center = parentBounds.getCentre();
            int radius = parentBounds.getWidth() / 2; // Assuming square bounds

            // Get the thumb's proposed center
            juce::Point<int> proposedCenter = proposedBounds.getCentre();
            int dx = proposedCenter.x - center.x;
            int dy = proposedCenter.y - center.y;
            float distance = std::sqrt(dx * dx + dy * dy);

            // If the thumb is outside the circle, clamp it to the circle's edge.
            if (distance > radius)
            {
                float scale = static_cast<float>(radius) / distance;
                int clampedDx = static_cast<int>(dx * scale);
                int clampedDy = static_cast<int>(dy * scale);
                juce::Point<int> newCenter(center.x + clampedDx, center.y + clampedDy);
                proposedBounds.setCentre(newCenter);
            }
        }
    }
private:
    juce::Component* parentComponent = nullptr;
};

namespace Gui
{

//==============================================================================
// XyPad::Thumb Implementation
//==============================================================================
XyPad::Thumb::Thumb() : constrainer()
{
    //constrainer = CircularBoundsConstrainer(parent);
    constrainer.setMinimumOnscreenAmounts(thumbSize, thumbSize, thumbSize, thumbSize);

}

void XyPad::Thumb::paint(Graphics &g)
{
    g.setColour(Colours::white);
    g.fillEllipse(getLocalBounds().toFloat());
}

void XyPad::Thumb::mouseDown(const MouseEvent& event)
{
    if (auto* parent = dynamic_cast<XyPad*>(getParentComponent()))
        parent->userIsDragging = true;
    dragger.startDraggingComponent(this, event);
}

void XyPad::Thumb::mouseDrag(const MouseEvent& event)
{
    dragger.dragComponent(this, event, &constrainer);
    if (moveCallback)
    {
        auto thumbCenter = getBounds().getCentre().toDouble();
        moveCallback(thumbCenter);
    }
}

void XyPad::Thumb::mouseUp(const MouseEvent& event)
{
    if (auto* parent = dynamic_cast<XyPad*>(getParentComponent()))
        parent->userIsDragging = false;
}

void XyPad::Thumb::thumbDrag(const MouseEvent& event)
{
    if (auto* parent = getParentComponent())
    {
        auto parentBounds = parent->getLocalBounds().toFloat();
        auto parentPos = parent->getLocalPoint(nullptr, localPointToGlobal(event.getPosition()));

        float xNorm = juce::jlimit(0.0f, 1.0f, parentPos.x / parentBounds.getWidth());
        float yNorm = juce::jlimit(0.0f, 1.0f, parentPos.y / parentBounds.getHeight());

        if (auto* xypad = dynamic_cast<XyPad*>(parent))
        {
            if (xypad->flipX) xNorm = 1.0f - xNorm;
            if (xypad->flipY) yNorm = 1.0f - yNorm;

            if (xypad->onXYChange)
                xypad->onXYChange(xNorm, yNorm);
        }

        parent->repaint();
    }
}

//==============================================================================
// XyPad::setThumbPositionNormalized
//==============================================================================
void XyPad::setThumbPositionNormalized(float normX, float normY)
{
    if (userIsDragging)
        return;

    auto bounds = getLocalBounds().toFloat();
    normX = juce::jlimit(0.0f, 1.0f, normX);
    normY = juce::jlimit(0.0f, 1.0f, normY);

    if (flipX)
        normX = 1.0f - normX;
    if (flipY)
        normY = 1.0f - normY;

    float centerX = normX * bounds.getWidth();
    float centerY = normY * bounds.getHeight();

    centerX = juce::jlimit(thumbSize / 2.0f, bounds.getWidth() - thumbSize / 2.0f, centerX);
    centerY = juce::jlimit(thumbSize / 2.0f, bounds.getHeight() - thumbSize / 2.0f, centerY);

    thumb.setCentrePosition(static_cast<int>(centerX), static_cast<int>(centerY));
    repaint();
}

//==============================================================================
// XyPad Implementation
//==============================================================================
XyPad::XyPad() 
{
    addAndMakeVisible(&thumb);
    startTimerHz(240);

    thumb.moveCallback = [this](Point<double> /*unused*/)
    {
        auto bounds = getLocalBounds().toDouble();
        auto center = thumb.getBounds().getCentre().toDouble();

        float xNorm = juce::jlimit(0.0, 1.0, center.getX() / bounds.getWidth());
        float yNorm = juce::jlimit(0.0, 1.0, center.getY() / bounds.getHeight());

        if (flipX)
            xNorm = 1.0f - xNorm;
        if (flipY)
            yNorm = 1.0f - yNorm;

        if (onXYChange)
            onXYChange(xNorm, yNorm);

        repaint();
    };
}

XyPad::~XyPad()
{
    for (auto* slider : X)
        if (slider != nullptr)
            slider->removeListener(this);
    for (auto* slider : Y)
        if (slider != nullptr)
            slider->removeListener(this);
    X.clear();
    Y.clear();
}

void XyPad::timerCallback()
{
    repaint();
}

void XyPad::resized()
{
    auto bounds = getLocalBounds();

    if (bounds.getWidth() < thumbSize || bounds.getHeight() < thumbSize)
        return;

    thumb.setBounds(bounds.withSizeKeepingCentre(thumbSize, thumbSize));

    if (!X.empty() && X[0] != nullptr)
        sliderValueChanged(X[0]);
    if (!Y.empty() && Y[0] != nullptr)
        sliderValueChanged(Y[0]);
}

void XyPad::paint(Graphics &g)
{
    g.setColour(Colours::darkgrey);
    // Draw a circle that fits within the component's bounds.
    auto bounds = getLocalBounds().toFloat();
    float diameter = std::min(bounds.getWidth(), bounds.getHeight());
    float x = (bounds.getWidth() - diameter) * 0.5f;
    float y = (bounds.getHeight() - diameter) * 0.5f;
    g.fillEllipse(x, y, diameter, diameter);
    
    // Optionally, draw radial guides or markers (e.g., crosshairs for 0°, 90°, 180°, 270°).
    g.setColour(Colours::lightgrey);
    g.drawEllipse(x, y, diameter, diameter, 2.0f);
    // Draw lines through the center:
    g.drawLine(bounds.getCentreX(), y, bounds.getCentreX(), y + diameter, 1.0f);
    g.drawLine(x, bounds.getCentreY(), x + diameter, bounds.getCentreY(), 1.0f);
}

/*
void XyPad::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);

    auto bounds = getLocalBounds().toFloat();
    auto centerX = bounds.getCentreX();
    auto centerY = bounds.getCentreY();

    g.setColour(juce::Colours::white);
    g.drawLine(0, centerY, bounds.getWidth(), centerY, 2.0f);
    g.drawLine(centerX, 0, centerX, bounds.getHeight(), 2.0f);
}
 */

void XyPad::registerSlider(Slider* slider, Axis axis)
{
    if (slider == nullptr)
        return;

    if (std::find(X.begin(), X.end(), slider) != X.end() ||
        std::find(Y.begin(), Y.end(), slider) != Y.end())
        return;

    slider->addListener(this);

    if (axis == Axis::X)
        X.push_back(slider);
    else if (axis == Axis::Y)
        Y.push_back(slider);
}

void XyPad::deRegisterSlider(Slider* slider)
{
    if (slider == nullptr)
        return;

    slider->removeListener(this);
    X.erase(std::remove(X.begin(), X.end(), slider), X.end());
    Y.erase(std::remove(Y.begin(), Y.end(), slider), Y.end());
}

void XyPad::mouseDown(const juce::MouseEvent& event)
{
    userIsDragging = true;
    auto bounds = getLocalBounds().toFloat();

    float clampedX = juce::jlimit(thumbSize / 2.0f, bounds.getWidth() - thumbSize / 2.0f, event.position.x);
    float clampedY = juce::jlimit(thumbSize / 2.0f, bounds.getHeight() - thumbSize / 2.0f, event.position.y);

    thumb.setCentrePosition(static_cast<int>(clampedX), static_cast<int>(clampedY));

    float normX = clampedX / bounds.getWidth();
    float normY = clampedY / bounds.getHeight();

    if (flipX)
        normX = 1.0f - normX;
    if (flipY)
        normY = 1.0f - normY;

    if (onXYChange)
        onXYChange(normX, normY);

    repaint();
}

void XyPad::mouseUp(const juce::MouseEvent& event)
{
    userIsDragging = false;
}

void XyPad::sliderValueChanged(Slider* slider)
{
    if (slider == nullptr || thumb.isMouseOverOrDragging(false))
        return;

    if (slider->getMaximum() == slider->getMinimum())
        return;

    const bool isXaxisSlider = (std::find(X.begin(), X.end(), slider) != X.end());
    const auto bounds = getLocalBounds().toDouble();
    const auto w = static_cast<double>(thumbSize);

    if (bounds.getWidth() - w <= 0 || bounds.getHeight() - w <= 0)
        return;

    auto currentCenter = thumb.getBounds().getCentre();

    if (isXaxisSlider)
    {
        double norm = safeJmap(slider->getValue(),
                               slider->getMinimum(),
                               slider->getMaximum(),
                               0.0, 1.0);
        if (flipX)
            norm = 1.0 - norm;

        double centerX = norm * (bounds.getWidth() - w) + w / 2.0;
        thumb.setCentrePosition(static_cast<int>(centerX), currentCenter.getY());
    }
    else
    {
        double norm = safeJmap(slider->getValue(),
                               slider->getMinimum(),
                               slider->getMaximum(),
                               0.0, 1.0);
        if (flipY)
            norm = 1.0 - norm;

        double centerY = norm * (bounds.getHeight() - w) + w / 2.0;
        thumb.setCentrePosition(currentCenter.getX(), static_cast<int>(centerY));
    }

    repaint();
}

} // namespace Gui

