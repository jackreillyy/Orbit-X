#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "graphics.h"
#include "XYPadLabels.h"
#include "SvgSliderComponent.h"
#include <atomic>

OrbitXAudioProcessorEditor::OrbitXAudioProcessorEditor(OrbitXAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Create and set the custom global LookAndFeel.
    lnf = std::make_unique<JackGraphics::SimpleLnF>();
    juce::LookAndFeel::setDefaultLookAndFeel(lnf.get());

    presetPanel = std::make_unique<Gui::PresetPanel>(p.getPresetManager());
    addAndMakeVisible(*presetPanel);

    lfoXContainer = std::make_unique<LFOContainer>(audioProcessor, "LFO X", "LFO_X", audioProcessor.getLFOX());
    lfoYContainer = std::make_unique<LFOContainer>(audioProcessor, "LFO Y", "LFO_Y", audioProcessor.getLFOY());
    addAndMakeVisible(*lfoXContainer);
    addAndMakeVisible(*lfoYContainer);
    
    titleText = juce::ImageFileFormat::loadFrom (
                    BinaryData::titleShine_png,
                    BinaryData::titleShine_pngSize);

    if (titleText.isValid())
    {
        titleWidth  = titleText.getWidth();
        titleHeight = titleText.getHeight();
    }

    setSize(850, 850); // initial
    setResizeLimits(600, 600, 1600, 1600); // allow small but still usable
    setResizable(true, true); // allow dragging to resize
    getConstrainer()->setFixedAspectRatio(1.0); // keep it square


    addAndMakeVisible(&xyPad1);
    xyPad1.flipX = false;
    xyPad1.flipY = false;

    xyPadLabels = std::make_unique<XYPadLabels>();
    addAndMakeVisible(xyPadLabels.get());
    xyPad1.toFront(true);
    xyPadLabels->toFront(true);

    rightDistortionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, "Distortion_Right", *xyPadLabels->getRightCombo());
    topDistortionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, "Distortion_Top", *xyPadLabels->getTopCombo());
    leftDistortionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, "Distortion_Left", *xyPadLabels->getLeftCombo());
    bottomDistortionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, "Distortion_Bottom", *xyPadLabels->getBottomCombo());

    driveSlider = std::make_unique<SvgSliderComponent>(
        BinaryData::SLIDERSVG_svg, BinaryData::SLIDERSVG_svgSize,
        BinaryData::SLIDERTHUMBSVG_svg, BinaryData::SLIDERTHUMBSVG_svgSize);
    addAndMakeVisible(driveSlider.get());

    mixSlider = std::make_unique<SvgSliderComponent>(
        BinaryData::SLIDERSVG_svg, BinaryData::SLIDERSVG_svgSize,
        BinaryData::SLIDERTHUMBSVG_svg, BinaryData::SLIDERTHUMBSVG_svgSize);
    addAndMakeVisible(mixSlider.get());

    driveSlider->setRange(0.1, 2.0);
    driveSlider->setValue(0.1);
    driveSlider->setSliderStyle(juce::Slider::LinearVertical);
    driveSlider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    driveATTACH = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "PostXYDrive", *driveSlider);
    driveLabel.setText("Drive", juce::dontSendNotification);
    driveLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(driveLabel);

//    mixSlider->setRange(0.0, 1.0);
//    mixSlider->setValue(1.0);
    mixSlider->setRange(0.0, 100.0);
    mixSlider->setValue(100.0);
    mixSlider->setNumDecimalPlacesToDisplay(0);
    mixSlider->setTextValueSuffix(" %");
    mixSlider->setSliderStyle(juce::Slider::LinearVertical);
    mixSlider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    outputATTACH = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "OutputMix", *mixSlider);
    outputMixLabel.setText("Mix", juce::dontSendNotification);
    outputMixLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(outputMixLabel);

    repaint();
    resized();

    xyPad1.onXYChange = [this](float x, float y)
    {
        float normX = (x - 0.5f) * 2.0f;
        float normY = (y - 0.5f) * 2.0f;
        float angle = std::atan2(normY, normX);
        if (angle < 0)
            angle += juce::MathConstants<float>::twoPi;
        if (auto* paramX = audioProcessor.apvts.getParameter("XY_X"))
            paramX->setValueNotifyingHost(x);
        if (auto* paramY = audioProcessor.apvts.getParameter("XY_Y"))
            paramY->setValueNotifyingHost(y);
    };

    startTimer(50); //used to be 4
}

OrbitXAudioProcessorEditor::~OrbitXAudioProcessorEditor()
{
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);

    setLookAndFeel(nullptr);
    // 1. Reset attachments first.
    rightDistortionAttachment.reset();
    topDistortionAttachment.reset();
    leftDistortionAttachment.reset();
    bottomDistortionAttachment.reset();
    driveATTACH.reset();
    outputATTACH.reset();

    // 2. Clear LookAndFeel pointers for components that set their own LookAndFeel.
    if (xyPadLabels)
    {
        xyPadLabels->getRightCombo()->setLookAndFeel(nullptr);
        xyPadLabels->getTopCombo()->setLookAndFeel(nullptr);
        xyPadLabels->getLeftCombo()->setLookAndFeel(nullptr);
        xyPadLabels->getBottomCombo()->setLookAndFeel(nullptr);
    }
    if (presetPanel)
        presetPanel->resetLookAndFeel();

    // Clear LookAndFeel for SVG sliders.
    if (driveSlider)
        driveSlider->setLookAndFeel(nullptr);
    if (mixSlider)
        mixSlider->setLookAndFeel(nullptr);
    
    xyPad1.setLookAndFeel(nullptr);
    removeAllChildren();
    //LookAndFeel::setDefaultLookAndFeel(nullptr);
}

void OrbitXAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Calculate center and radius based on the XY pad bounds.
    auto xyBounds = xyPad1.getBounds().toFloat();
    float centerX = xyBounds.getCentreX();
    float centerY = xyBounds.getCentreY();
    float radius = std::max(xyBounds.getWidth(), xyBounds.getHeight()) * 1.1f; // Wider spread

    // Create a radial gradient for the background.
    juce::ColourGradient radialGradient(
        juce::Colour(50, 50, 65),  // Brighter center color
        centerX, centerY,
        juce::Colour(30, 30, 40),  // Less dark outer color
        centerX, centerY + radius,
        true);
    radialGradient.addColour(0.25, juce::Colour(70, 70, 90));  // Mid-point stop
    radialGradient.addColour(0.6, juce::Colour(40, 40, 55));   // Outer color stop
    g.setGradientFill(radialGradient);
    g.fillAll();

    // Create an ambient glow layer.
    juce::Colour centerGlowColour = juce::Colour(110, 110, 140).withAlpha(0.30f);
    juce::Colour outerTransparent = juce::Colour(0, 0, 0).withAlpha(0.0f);
    juce::ColourGradient glow(
        centerGlowColour,
        centerX, centerY,
        outerTransparent,
        centerX, centerY + radius * 2.0f,
        true);
    g.setGradientFill(glow);
    g.fillAll();

    // Draw the cached noise image overlay.
    g.setOpacity(0.05f);
    g.drawImageAt(cachedNoiseImage, 0, 0);
    g.setOpacity(1.0f); // Reset opacity
    
    drawTitle (g);
}

void OrbitXAudioProcessorEditor::drawTitle (juce::Graphics& g)
{
    if (! titleText.isValid())
        return;

    const int leftMargin      = 10;  // 10px in from left edge
    const int verticalSpacing = -40;   // px gap below LFO container

    const int drawW = titleWidth  / 10;
    const int drawH = titleHeight / 10;

    // find the bottom of the LFO Y container
    int lfoBottom = lfoYContainer
                      ? lfoYContainer->getBounds().getBottom()
                      : (getHeight() - drawH - verticalSpacing);

    int x = leftMargin;
    int y = lfoBottom + verticalSpacing;

    g.drawImage (titleText,
                 x, y,
                 drawW, drawH,
                 0, 0,
                 titleWidth, titleHeight);
}

void OrbitXAudioProcessorEditor::resized()
{
    auto allArea = getLocalBounds().reduced(20);
    int totalHeight = allArea.getHeight();

    int presetHeight = static_cast<int>(totalHeight * 0.05);
    if (presetPanel)
        presetPanel->setBounds(allArea.removeFromTop(presetHeight));

    int topSectionHeight = static_cast<int>(totalHeight * 0.55);
    auto topSectionArea = allArea.removeFromTop(topSectionHeight);

    const int sliderWidth = 50;
    const int sideMargin = 25;
    const int gapBetweenSliderAndPad = 20;

    juce::Rectangle<int> driveSliderArea(
        topSectionArea.getX() + sideMargin,
        topSectionArea.getY(),
        sliderWidth,
        topSectionHeight);

    juce::Rectangle<int> mixSliderArea(
        topSectionArea.getRight() - sideMargin - sliderWidth,
        topSectionArea.getY(),
        sliderWidth,
        topSectionHeight);

    int xyPadAreaX = driveSliderArea.getRight() + gapBetweenSliderAndPad;
    int xyPadAreaWidth = mixSliderArea.getX() - gapBetweenSliderAndPad - xyPadAreaX;
    juce::Rectangle<int> xyPadArea(xyPadAreaX, topSectionArea.getY(), xyPadAreaWidth, topSectionHeight);

    int availablePadSize = std::min(xyPadArea.getWidth(), xyPadArea.getHeight());
    int padSize = static_cast<int>(availablePadSize * 0.8f);

    auto padBounds = xyPadArea.withSizeKeepingCentre(padSize, padSize);
    xyPad1.setBounds(padBounds);
    xyPad1.toFront(true);

    if (xyPadLabels)
    {
        int leftMargin = 110;
        int rightMargin = 110;
        int topMargin = 35;
        int bottomMargin = 35;

        int newX = padBounds.getX() - leftMargin;
        int newY = padBounds.getY() - topMargin;
        int newWidth = padBounds.getWidth() + leftMargin + rightMargin;
        int newHeight = padBounds.getHeight() + topMargin + bottomMargin;

        juce::Rectangle<int> expandedLabelBounds(newX, newY, newWidth, newHeight);
        xyPadLabels->setBounds(expandedLabelBounds);
        xyPadLabels->toFront(true);
    }

    if (driveSlider)
        driveSlider->setBounds(driveSliderArea.reduced(5));
    if (mixSlider)
        mixSlider->setBounds(mixSliderArea.reduced(5));

    int labelHeight = 20;
    driveLabel.setBounds(
        driveSliderArea.getX(),
        driveSliderArea.getBottom(),
        driveSliderArea.getWidth(),
        labelHeight);
    outputMixLabel.setBounds(
        mixSliderArea.getX(),
        mixSliderArea.getBottom(),
        mixSliderArea.getWidth(),
        labelHeight);

    auto lfoSection = allArea;
    int halfWidth = lfoSection.getWidth() / 2;
    if (lfoXContainer)
        lfoXContainer->setBounds(lfoSection.removeFromLeft(halfWidth).reduced(20));
    if (lfoYContainer)
        lfoYContainer->setBounds(lfoSection.reduced(20));

    // --- Update the cached noise texture ---
    int w = getWidth();
    int h = getHeight();
    if (cachedNoiseImage.isNull() || cachedNoiseWidth != w || cachedNoiseHeight != h)
    {
        cachedNoiseImage = juce::Image(juce::Image::PixelFormat::ARGB, w, h, true);
        cachedNoiseWidth = w;
        cachedNoiseHeight = h;

        juce::Graphics noiseG(cachedNoiseImage);
        juce::Random rand;
        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                uint8 noiseAlpha = static_cast<uint8>(rand.nextInt(35)); // Adjust intensity if desired
                cachedNoiseImage.setPixelAt(x, y, juce::Colour::fromRGBA(255, 255, 255, noiseAlpha));
            }
        }
    }
}

void OrbitXAudioProcessorEditor::timerCallback()
{
    auto bounds = getLocalBounds();
    if (bounds.getWidth() < 700 || bounds.getHeight() < 600)
        setSize(750, 750);

    float baseX1 = *audioProcessor.apvts.getRawParameterValue("XY_X");
    float baseY1 = *audioProcessor.apvts.getRawParameterValue("XY_Y");

    if (*audioProcessor.bypassParamX < 0.5f)
        baseX1 = 0.5f;
    if (*audioProcessor.bypassParamY < 0.5f)
        baseY1 = 0.5f;

    float lfoModX = (*audioProcessor.bypassParamX < 0.5f)
                      ? audioProcessor.lfoXValue.load(std::memory_order_relaxed)
                      : 0.0f;
    float lfoModY = (*audioProcessor.bypassParamY < 0.5f)
                      ? audioProcessor.lfoYValue.load(std::memory_order_relaxed)
                      : 0.0f;


    float effectiveX1 = juce::jlimit(0.0f, 1.0f, baseX1 + lfoModX);
    float effectiveY1 = juce::jlimit(0.0f, 1.0f, baseY1 + lfoModY);

    if (!xyPad1.isBeingDragged())
        xyPad1.setThumbPositionNormalized(effectiveX1, effectiveY1);
}

void OrbitXAudioProcessorEditor::xyPad1Moved(float x, float y)
{
    if (auto* paramX = audioProcessor.apvts.getParameter("XY_X"))
        paramX->setValueNotifyingHost(x);
    if (auto* paramY = audioProcessor.apvts.getParameter("XY_Y"))
        paramY->setValueNotifyingHost(y);

    audioProcessor.setDistortionAParameter(x);
    audioProcessor.setDistortionBParameter(y);
}

