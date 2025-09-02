#include "LFOdsp.h"

LFOdsp::LFOdsp() : lfoBuffer(bufferSize, 0.0f), writeIndex(0)
{
    updatePhaseIncrement();
}

void LFOdsp::setSampleRate(double newSampleRate)
{
    sampleRate = newSampleRate;
    updatePhaseIncrement();
}

void LFOdsp::setFrequency(float newFrequency, bool resetPhase)
{
    if (newFrequency > 0)
    {
        frequency = newFrequency;
        updatePhaseIncrement();
        if (resetPhase)
            phase = 0.0;
    }
}

void LFOdsp::setSyncMode(bool shouldSync)
{
    syncMode = shouldSync;
    updatePhaseIncrement();
}

void LFOdsp::resetPhase()
{
    phase = 0.0;
}

void LFOdsp::setSyncNoteDivision(float newNoteDivision, double bpm)
{
    if (syncMode)
    {
        noteDivision = newNoteDivision;
        hostBPM = bpm;
        float newFrequency = (bpm / 60.0f) / noteDivision;
        setFrequency(newFrequency, false);
        updatePhaseIncrement();
    }
}

void LFOdsp::setDepth(float newDepth)
{
    depth = juce::jlimit(0.0f, 1.0f, newDepth);
}

void LFOdsp::setWaveform(int newWaveformType)
{
    waveformType = juce::jlimit(0, 4, newWaveformType);
}

void LFOdsp::updatePhaseIncrement()
{
    phaseIncrement = (frequency / sampleRate) * juce::MathConstants<double>::twoPi;
}

float LFOdsp::processModulation()
{
    //double previousPhase = phase;
    phase += phaseIncrement;
    
    bool wrapped = false;
    if (phase > juce::MathConstants<double>::twoPi)
    {
        phase -= juce::MathConstants<double>::twoPi;
        wrapped = true;
    }
    
    float lfoValue = 0.0f;
    switch (waveformType)
    {
        case 0: // Sine
            lfoValue = std::sin(phase);
            break;
        case 1: // Triangle
            lfoValue = (2.0f / juce::MathConstants<float>::pi) * std::asin(std::sin(phase));
            break;
        case 2: // Square
            lfoValue = (phase < juce::MathConstants<float>::pi) ? 1.0f : -1.0f;
            break;
        case 3: // Saw
            lfoValue = (2.0f * phase / juce::MathConstants<float>::twoPi) - 1.0f;
            break;
        case 4: // Random sample & hold
            if (wrapped)
                lastRandomValue = randomGenerator.nextFloat() * 2.0f - 1.0f;  // New random value in [-1, 1]
            lfoValue = lastRandomValue;
            break;
        default:
            lfoValue = 1.0f;
    }
    
    lfoValue *= (depth * 0.5f);
    currentModulation = lfoValue;
    
    int index = writeIndex.load(std::memory_order_relaxed);
    lfoBuffer[index] = lfoValue;
    writeIndex.store((index + 1) % bufferSize, std::memory_order_relaxed);
    
    return lfoValue;
}

std::vector<float> LFOdsp::getLFOBuffer()
{
    int localWriteIndex = writeIndex.load(std::memory_order_relaxed);
    std::vector<float> orderedBuffer(bufferSize);
    for (int i = 0; i < bufferSize; i++)
    {
        orderedBuffer[i] = lfoBuffer[(localWriteIndex + i) % bufferSize];
    }
    return orderedBuffer;
}

float LFOdsp::getCurrentModulation() const
{
    return currentModulation;
}

void LFOdsp::syncPhaseWith(const LFOdsp& other)
{
    this->phase = other.phase;
}

