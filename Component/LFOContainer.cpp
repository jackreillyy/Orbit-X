#include "LFOContainer.h"

/// A helper class for drawing the bypass overlay.
/// It uses the container’s full bounds (reduced by your desired margins)
/// and draws the overlay when visible.
class BypassOverlay : public juce::Component
{
public:
    BypassOverlay() {}
    ~BypassOverlay() override {}

    void paint(juce::Graphics& g) override
    {
        // Use the container's full bounds with reductions.
        juce::Rectangle<int> overlayBounds = getLocalBounds().reduced(13).withTrimmedTop(5);
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.fillRoundedRectangle(overlayBounds.toFloat(), 4.0f);
        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.setFont(15.0f);
    }
};

LFOContainer::LFOContainer(OrbitXAudioProcessor& p, const juce::String& displayName,
                           const juce::String& parameterPrefix, LFOdsp& lfoDspRef)
    : processor(p), lFOdsp(lfoDspRef), lfoVisualizer(lfoDspRef),
      title(displayName), parameterPrefix(parameterPrefix),
      groupComponent("LFOContainer", displayName)
{
    groupComponent.setText(title);
    addAndMakeVisible(groupComponent);

    // LFO Shape ComboBox must be initialized BEFORE attachment
    shapeSelector.setLookAndFeel(&comboBoxLNF);
    addAndMakeVisible(shapeSelector);
    shapeSelector.addItem("Sine",     1);
    shapeSelector.addItem("Triangle", 2);
    shapeSelector.addItem("Square",   3);
    shapeSelector.addItem("Saw",      4);
    shapeSelector.addItem("Random",   5);

    // Now attach the shape selector to the parameter
    shapeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        p.apvts, parameterPrefix + "_Shape", shapeSelector);

    // Create remaining attachments
    depthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, parameterPrefix + "_Depth", depthSlider);
    speedAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, parameterPrefix + "_Rate", speedSlider);
    syncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.apvts, parameterPrefix + "_Sync", rateSwitch);

    // Bypass Button
    addAndMakeVisible(bypassButton);
    bypassButton.setButtonText("Bypass");
    bypassButton.setClickingTogglesState(true);
    bypassButton.onClick = [this, parameterPrefix]() {
        auto* bypassParam = processor.apvts.getParameter(parameterPrefix + "_Bypass");
        bool newState = bypassButton.getToggleState();
        bypassParam->setValueNotifyingHost(newState ? 1.0f : 0.0f);
        repaint();
        setComponentEnabled(!newState);
    };
    syncBypassState();

    // Depth Knob
    addAndMakeVisible(depthSlider);
    depthSlider.setSliderStyle(juce::Slider::Rotary);
    depthSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    depthLabel.setText("Depth", juce::dontSendNotification);
    depthLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(depthLabel);
    depthSlider.setRange(0.0f, 1.0f, 0.01);
    depthSlider.textFromValueFunction = [](double value) {
        return juce::String(static_cast<int>(value * 100)) + "%";
    };
    depthSlider.valueFromTextFunction = [](const juce::String& text) {
        return static_cast<double>(text.getIntValue()) / 100.0;
    };
    depthSlider.updateText();

    // Speed Knob
    speedSlider.setRange(0.1, 20.0);
    speedSlider.setSkewFactorFromMidPoint(6.0);
    addAndMakeVisible(speedSlider);
    speedSlider.setSliderStyle(juce::Slider::Rotary);
    speedSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    speedLabel.setText("Speed", juce::dontSendNotification);
    speedLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(speedLabel);
    speedSlider.textFromValueFunction = [this](double value) {
        if (rateSwitch.getToggleState()) {
            static const juce::StringArray noteValues = {
                "16", "8", "4", "2", "1", "1/2T", "1/2", "1/4T", "1/4",
                "1/8T", "1/8", "1/16T", "1/16", "1/32"
            };
            int index = juce::jlimit(0, noteValues.size() - 1, static_cast<int>(value));
            return noteValues[index];
        }
        return value < 1.0 ? juce::String(value, 2) + " Hz" : juce::String(static_cast<int>(value)) + " Hz";
    };
    speedSlider.valueFromTextFunction = [this](const juce::String& text) {
        if (rateSwitch.getToggleState()) {
            static const juce::StringArray noteValues = {
                "16", "8", "4", "2", "1", "1/2T", "1/2", "1/4T", "1/4",
                "1/8T", "1/8", "1/16T", "1/16", "1/32"
            };
            return static_cast<double>(noteValues.indexOf(text));
        }
        return static_cast<double>(text.getIntValue());
    };
    speedSlider.updateText();
    speedSlider.onValueChange = [this]() { updateLfoFrequency(); };
    speedSlider.onDragEnd = [this]() {
        if (rateSwitch.getToggleState()) {
            double value = speedSlider.getValue();
            speedSlider.setValue(std::round(value), juce::dontSendNotification);
            updateLfoFrequency();
        }
    };

    // Sync toggle
    addAndMakeVisible(rateSwitch);
    rateSwitch.setButtonText("Sync");
    rateSwitch.setClickingTogglesState(true);
    rateSwitch.onClick = [this]() {
        if (rateSwitch.getToggleState()) {
            lastHzValue = speedSlider.getValue();
            speedSlider.setRange(0, 13, 1);
            speedSlider.setSkewFactorFromMidPoint(7.0);
            speedSlider.setValue(lastNoteValue, juce::dontSendNotification);
        } else {
            lastNoteValue = speedSlider.getValue();
            speedSlider.setRange(0.1, 20.0);
            speedSlider.setSkewFactorFromMidPoint(1.0);
            speedSlider.setValue(lastHzValue, juce::dontSendNotification);
        }
        speedSlider.updateText();
        updateLfoFrequency();
    };

    // Visualizer
    addAndMakeVisible(lfoVisualizer);

    // Overlay
    bypassOverlayComponent = std::make_unique<BypassOverlay>();
    addAndMakeVisible(bypassOverlayComponent.get());
    bypassOverlayComponent->setAlwaysOnTop(false);
    bypassOverlayComponent->setInterceptsMouseClicks(false, false);
    bypassOverlayComponent->setVisible(bypassState);
}

LFOContainer::~LFOContainer() {
    shapeSelector.setLookAndFeel(nullptr);
}

//--------------------------------------------------------------------------------------------------

double LFOContainer::getSyncFrequency(double bpm, int noteIndex)
{
    const std::vector<double> multipliers = {
        0.015625, 0.03125, 0.0625, 0.125, 0.25, 0.75, 0.5,
        1.5, 1.0, 12.0, 8.0, 24.0, 16.0, 32.0
    };
    int idx = juce::jlimit(0, static_cast<int>(multipliers.size() - 1), noteIndex);
    return (bpm / 60.0) * multipliers[idx];
}

void LFOContainer::updateLfoFrequency()
{
    double newFrequency = 0.0;
    if (rateSwitch.getToggleState())
    {
        double bpm = processor.getBPM();
        int noteIndex = static_cast<int>(speedSlider.getValue());
        newFrequency = getSyncFrequency(bpm, noteIndex);
        juce::String noteDivisionParamID = parameterPrefix + "_NoteDivision";
        if (auto* param = processor.apvts.getParameter(noteDivisionParamID))
        {
            float normalizedValue = noteIndex / 13.0f;
            param->setValueNotifyingHost(normalizedValue);
        }
    }
    else
    {
        newFrequency = speedSlider.getValue();
    }
    lFOdsp.setFrequency(newFrequency);
    
    if (rateSwitch.getToggleState())
    {
        // Phase reset to restart cycle.
        processor.syncLFOPhases();
    }
}

//--------------------------------------------------------------------------------------------------

void LFOContainer::syncBypassState()
{
    bool paramState = processor.apvts.getRawParameterValue(parameterPrefix + "_Bypass")->load() > 0.5f;
    bypassState = paramState;
    bypassButton.setToggleState(bypassState, juce::dontSendNotification);
    setComponentEnabled(!bypassState);
    if (bypassOverlayComponent)
        bypassOverlayComponent->setVisible(bypassState);
}

void LFOContainer::setComponentEnabled(bool shouldBeEnabled)
{
    depthSlider.setEnabled(shouldBeEnabled);
    depthLabel.setEnabled(shouldBeEnabled);
    speedSlider.setEnabled(shouldBeEnabled);
    speedLabel.setEnabled(shouldBeEnabled);
    rateSwitch.setEnabled(shouldBeEnabled);
    shapeSelector.setEnabled(shouldBeEnabled);
}

void LFOContainer::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::transparentBlack);

    auto buffer = lFOdsp.getLFOBuffer();
    if (buffer.empty() || buffer.size() < 2)
        return;

    juce::Path lfoPath;
    auto bounds = lfoVisualizer.getLocalBounds().toFloat();
    float width = bounds.getWidth();
    float height = bounds.getHeight();
    float centerY = bounds.getCentreY();

    // Draw LFO waveform.
    lfoPath.startNewSubPath(0, centerY - (buffer[0] * height * 0.4f));
    for (size_t i = 1; i < buffer.size(); ++i)
    {
        float x = juce::jmap<float>(static_cast<float>(i), 0.0f, static_cast<float>(buffer.size() - 1), 0.0f, width);
        float y = centerY - (buffer[i] * height * 0.4f);
        lfoPath.lineTo(x, y);
    }

    g.setColour(juce::Colours::white);
    
    syncBypassState();
    
    if (bypassState)
    {
        juce::Rectangle<int> overlayBounds = getLocalBounds().reduced(13).withTrimmedTop(5);
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.fillRoundedRectangle(overlayBounds.toFloat(), 4.0f);
        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.setFont(15.0f);
        // Optionally, draw text:
        //g.drawFittedText("LFO BYPASSED", overlayBounds, juce::Justification::centred, 1);
    }
}

//--------------------------------------------------------------------------------------------------

void LFOContainer::resized()
{
    juce::Grid grid;
    groupComponent.setText(title);
    groupComponent.setBounds(getLocalBounds().reduced(10));
    lfoVisualizer.setBounds(getLocalBounds().reduced(20));

    grid.templateRows.clear();
    grid.templateColumns.clear();

    for (int i = 0; i < 9; ++i)
        grid.templateColumns.add(juce::Grid::TrackInfo(juce::Grid::Fr(1)));
    for (int i = 0; i < 9; ++i)
        grid.templateRows.add(juce::Grid::TrackInfo(juce::Grid::Fr(1)));

    juce::GridItem bypassButtonItem(bypassButton);
    bypassButtonItem.row.start = 2;
    bypassButtonItem.row.end = 2;
    bypassButtonItem.column.start = 2;
    bypassButtonItem.column.end = 5;

    juce::GridItem speedSliderItem(speedSlider);
    speedSliderItem.row.start = 6;
    speedSliderItem.row.end = 8;
    speedSliderItem.column.start = 2;
    speedSliderItem.column.end = 4;
    speedSliderItem.minWidth = 65;
    speedSliderItem.minHeight = 65;

    juce::GridItem speedLabelItem(speedLabel);
    speedLabelItem.row.start = 8;
    speedLabelItem.column.start = 2;
    speedLabelItem.column.end = 4;

    juce::GridItem rateSwitchItem(rateSwitch);
    rateSwitchItem.row.start = 8;
    rateSwitchItem.column.start = 4;
    rateSwitchItem.column.end = 6;

    juce::GridItem depthSliderItem(depthSlider);
    depthSliderItem.row.start = 3;
    depthSliderItem.row.end = 5;
    depthSliderItem.column.start = 2;
    depthSliderItem.column.end = 4;
    depthSliderItem.minWidth = 65;
    depthSliderItem.minHeight = 65;

    juce::GridItem depthLabelItem(depthLabel);
    depthLabelItem.row.start = 5;
    depthLabelItem.column.start = 2;
    depthLabelItem.column.end = 4;

    juce::GridItem lfoVisualizerItem(lfoVisualizer);
    lfoVisualizerItem.row.start = 3;
    lfoVisualizerItem.row.end = 7;
    lfoVisualizerItem.column.start = 5;
    lfoVisualizerItem.column.end = 9;

    juce::GridItem shapeSelectorItem(shapeSelector);
    shapeSelectorItem.row.start = 8;
    shapeSelectorItem.row.end = 8;
    shapeSelectorItem.column.start = 6;
    shapeSelectorItem.column.end = 9;

    grid.items = {
        bypassButtonItem,
        speedSliderItem,
        speedLabelItem,
        depthSliderItem,
        depthLabelItem,
        lfoVisualizerItem,
        shapeSelectorItem,
        rateSwitchItem
    };

    grid.performLayout(getLocalBounds().reduced(10));
    
    if (bypassOverlayComponent)
        bypassOverlayComponent->setBounds(getLocalBounds());
    
    syncBypassState();
}

