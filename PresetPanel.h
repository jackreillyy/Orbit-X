#pragma once

#include <JuceHeader.h>
#include "graphics.h"

namespace Gui
{

class PresetPanel : public juce::Component,
                    private juce::Button::Listener,
                    private juce::ComboBox::Listener
{
public:
    PresetPanel(Service::PresetManager& pm) : presetManager(pm)
    {
        presetList.setLookAndFeel(&comboBoxLookAndFeel);

        configureButton(saveButton, "Save");
        configureButton(deleteButton, "Delete");
        configureButton(previousPresetButton, "<");
        configureButton(nextPresetButton, ">");

        presetList.setTextWhenNothingSelected("- No Preset -");
        addAndMakeVisible(presetList);
        presetList.addListener(this);

        loadPresetList();
    }
    
    ~PresetPanel() override
    {
        saveButton.removeListener(this);
        deleteButton.removeListener(this);
        previousPresetButton.removeListener(this);
        nextPresetButton.removeListener(this);
        presetList.removeListener(this);

        presetList.setLookAndFeel(nullptr);
    }
    
    void resetLookAndFeel()
    {
        presetList.setLookAndFeel(nullptr);
    }
    

    void resized() override
    {
        auto container = getLocalBounds().reduced(4);
        auto bounds = container;
        
        saveButton.setBounds(bounds.removeFromLeft(container.proportionOfWidth(0.2f)).reduced(4));
        previousPresetButton.setBounds(bounds.removeFromLeft(container.proportionOfWidth(0.1f)).reduced(4));
        presetList.setBounds(bounds.removeFromLeft(container.proportionOfWidth(0.4f)).reduced(4));
        nextPresetButton.setBounds(bounds.removeFromLeft(container.proportionOfWidth(0.1f)).reduced(4));
        deleteButton.setBounds(bounds.reduced(4));
    }

private:
    void buttonClicked(juce::Button* button) override
    {
        if (button == &saveButton)
        {
            fileChooser = std::make_unique<juce::FileChooser>(
                "Please enter the name of the preset to save",
                Service::PresetManager::defaultDirectory,
                "*." + Service::PresetManager::extension
            );
            
            fileChooser->launchAsync(juce::FileBrowserComponent::saveMode,
                [this](const juce::FileChooser& chooser)
                {
                    auto resultFile = chooser.getResult();
                    presetManager.savePreset(resultFile.getFileNameWithoutExtension());
                    loadPresetList();
                });
        }
        else if (button == &previousPresetButton)
        {
            int index = presetManager.loadPreviousPreset();
            presetList.setSelectedItemIndex(index, juce::dontSendNotification);
        }
        else if (button == &nextPresetButton)
        {
            int index = presetManager.loadNextPreset();
            presetList.setSelectedItemIndex(index, juce::dontSendNotification);
        }
        else if (button == &deleteButton)
        {
            presetManager.deletePreset(presetManager.getCurrentPreset());
            loadPresetList();
        }
    }
    
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override
    {
        if (comboBoxThatHasChanged == &presetList)
        {
            presetManager.loadPreset(presetList.getItemText(presetList.getSelectedItemIndex()));
            
            if (auto* parent = getParentComponent())
            {
                parent->repaint(); // repaint full UI
                for (auto* child : parent->getChildren())
                {
                    if (auto* lfoContainer = dynamic_cast<LFOContainer*>(child))
                        lfoContainer->syncBypassState();
                }
            }
        }
    }
    
    void configureButton(juce::Button& button, const juce::String& buttonText)
    {
        button.setButtonText(buttonText);

        button.setColour(juce::TextButton::buttonColourId, juce::Colours::grey);
        button.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        button.setColour(juce::TextButton::textColourOffId, juce::Colours::white);

        addAndMakeVisible(button);
        button.addListener(this);
    }
    
    void loadPresetList()
    {
        presetList.clear(juce::dontSendNotification);
        auto allPresets = presetManager.getAllPresets();
        auto currentPreset = presetManager.getCurrentPreset();
        presetList.addItemList(allPresets, 1);
        presetList.setSelectedItemIndex(allPresets.indexOf(currentPreset), juce::dontSendNotification);
    }
    
    Service::PresetManager& presetManager;
    juce::TextButton saveButton, deleteButton, previousPresetButton, nextPresetButton;
    juce::ComboBox presetList;
    
    std::unique_ptr<juce::FileChooser> fileChooser;
    
    CustomComboBoxLookAndFeel comboBoxLookAndFeel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetPanel)
};

} // namespace Gui


