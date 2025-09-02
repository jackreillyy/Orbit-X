#pragma once

#include <JuceHeader.h>

namespace JackDistortion {

// Base class for all distortion types
class DistortionBase {
public:
    virtual ~DistortionBase() = default;

    // Pure virtual functions for setting parameters and processing the buffer
    virtual void setParameters(float drive, float output) = 0;
    virtual void processBuffer(juce::AudioBuffer<float>& buffer, int channelNum) = 0;

    /** Compensation factor for this distortion's internal gain */
    virtual float getCompensation() const { return 1.0f; }
};

//------------------------------------------------------------------------------------------------------------//
// Analog Clip Distortion (Ableton saturator copy attempt)
class softClip : public DistortionBase {
public:
    void setParameters(float drive, float output) override {
        Drive = drive;
        Output = output;
    }

    float getCompensation() const override { return 2.0f; }

    void processBuffer(juce::AudioBuffer<float>& buff, int channelNum) override {
        float* channelData = buff.getWritePointer(channelNum);
        for (int i = 0; i < buff.getNumSamples(); ++i)
            channelData[i] = processSample(channelData[i]);
    }

    float processSample(float sample) {
        const float driveGain   = juce::Decibels::decibelsToGain(Drive + 12.0f);
        const float outputGain  = juce::Decibels::decibelsToGain(Output) / getCompensation();

        float x = driveGain * sample;
        x += 0.1f * std::copysign(1.0f, x); // Analog offset

        float clipped = x / (1.0f + std::abs(x));  // “fast tanh” style
        clipped = std::tanh(3.0f * clipped);

        return clipped * outputGain;
    }

private:
    float Drive = 1.0f, Output = 5.0f;
};

//------------------------------------------------------------------------------------------------------------//
// Hard Clip Distortion
class hardClip : public DistortionBase {
public:
    void setParameters(float drive, float output) override {
        Drive = drive;
        Output = output;
    }

    float getCompensation() const override { return 0.15f; }

    void processBuffer(juce::AudioBuffer<float>& buff, int channelNum) override {
        float* channelData = buff.getWritePointer(channelNum);
        for (int i = 0; i < buff.getNumSamples(); ++i)
            channelData[i] = processSample(channelData[i]);
    }

    float processSample(float sample) {
        const float driveGain   = juce::Decibels::decibelsToGain(Drive);
        const float outputGain  = juce::Decibels::decibelsToGain(Output) / getCompensation();
        const float drivenSample = driveGain * sample;

        if (drivenSample > threshold)
            return threshold * outputGain;
        else if (drivenSample < -threshold)
            return -threshold * outputGain;
        else
            return drivenSample * outputGain;
    }

private:
    float Drive = 22.0f, Output = 10.0f;
    float threshold = 0.1f;
};

//------------------------------------------------------------------------------------------------------------//
// Sinusoidal Fold Distortion
class sinusoidalFold : public DistortionBase {
public:
    void setParameters(float drive, float output) override {
        Drive = drive;
        Output = output;
    }

    void processBuffer(juce::AudioBuffer<float>& buff, int channelNum) override {
        float* channelData = buff.getWritePointer(channelNum);
        for (int i = 0; i < buff.getNumSamples(); ++i)
            channelData[i] = processSample(channelData[i]);
    }

    float processSample(float sample) {
        const float driveGain  = juce::Decibels::decibelsToGain(Drive);
        const float outputGain = juce::Decibels::decibelsToGain(Output); // no compensation
        float driven = driveGain * sample;
        float result = std::sin(juce::MathConstants<float>::pi * driven);
        return result * outputGain * 1.5f;
    }

private:
    float Drive = 3.0f, Output = 0.0f;
};

//------------------------------------------------------------------------------------------------------------//
// Wave Shaped Distortion
class waveShaped : public DistortionBase {
public:
    void setParameters(float drive, float output) override {
        Drive = drive;
        Output = output;
    }

    void setShape(float shape) { Shape = shape; }

    float getCompensation() const override { return 1.0f; }

    void processBuffer(juce::AudioBuffer<float>& buff, int channelNum) override {
        float* channelData = buff.getWritePointer(channelNum);
        for (int i = 0; i < buff.getNumSamples(); ++i)
            channelData[i] = processSample(channelData[i]);
    }

    float processSample(float sample) {
        const float driveGain  = juce::Decibels::decibelsToGain(Drive);
        const float outputGain = juce::Decibels::decibelsToGain(Output) / getCompensation();
        float x = driveGain * sample;
        float result = x - Shape * std::pow(x, 3) * 1.2f;
        return result * outputGain;
    }

private:
    float Drive = 7.0f, Output = 0.0f, Shape = 0.9f;
};

//------------------------------------------------------------------------------------------------------------//
// ArcTan Distortion
class arctan : public DistortionBase {
public:
    void setParameters(float drive, float output) override {
        Drive = drive;
        Output = output;
    }

    void processBuffer(juce::AudioBuffer<float>& buff, int channelNum) override {
        float* channelData = buff.getWritePointer(channelNum);
        for (int i = 0; i < buff.getNumSamples(); ++i)
            channelData[i] = processSample(channelData[i]);
    }

    float processSample(float sample) {
        const float driveGain  = juce::Decibels::decibelsToGain(Drive);
        const float outputGain = juce::Decibels::decibelsToGain(Output); // no compensation
        float x = driveGain * sample;
        float result = (2.0f / juce::MathConstants<float>::pi) * std::atan(k * x);
        return result * outputGain * 1.3f;
    }

private:
    float Drive = 10.0f, Output = 0.0f;
    float k = 20.0f;
};

//------------------------------------------------------------------------------------------------------------//
// Asymmetrical ArcTan Distortion
class asym : public DistortionBase {
public:
    void setParameters(float drive, float output) override {
        Drive = drive;
        Output = output;
    }

    void processBuffer(juce::AudioBuffer<float>& buff, int channelNum) override {
        float* channelData = buff.getWritePointer(channelNum);
        for (int i = 0; i < buff.getNumSamples(); ++i)
            channelData[i] = processSample(channelData[i]);
    }

    float processSample(float sample) {
        const float driveGain  = juce::Decibels::decibelsToGain(Drive);
        const float outputGain = juce::Decibels::decibelsToGain(Output); // no compensation
        float x = driveGain * sample;
        float result = (x >= 0)
            ? (2.0f / juce::MathConstants<float>::pi) * std::atan(k1 * x)
            : (2.0f / juce::MathConstants<float>::pi) * std::atan(k2 * x);
        return result * outputGain * 1.3f;
    }

private:
    float Drive = 10.0f, Output = 0.0f;
    float k1 = 8.0f, k2 = 20.0f;
};

//------------------------------------------------------------------------------------------------------------//
// Cascaded Nonlinear Distortion
class cascade : public DistortionBase {
public:
    void setParameters(float drive, float output) override {
        Drive = drive;
        Output = output;
    }

    float getCompensation() const override { return 0.6f; }

    void processBuffer(juce::AudioBuffer<float>& buff, int channelNum) override {
        float* channelData = buff.getWritePointer(channelNum);
        for (int i = 0; i < buff.getNumSamples(); ++i)
            channelData[i] = processSample(channelData[i]);
    }

    float processSample(float sample) {
        const float driveGain  = juce::Decibels::decibelsToGain(Drive);
        const float outputGain = juce::Decibels::decibelsToGain(Output) / getCompensation();
        float stage1 = std::tanh(driveGain * sample);
        float stage2 = std::atan(stage1);
        return stage2 * outputGain;
    }

private:
    float Drive = 20.0f, Output = 10.0f;
};

//------------------------------------------------------------------------------------------------------------//
// Polynomial Distortion
class poly : public DistortionBase {
public:
    void setParameters(float drive, float output) override {
        Drive = drive;
        Output = output;
    }

    void setShapeParameters(float A, float B) {
        paramA = A;
        paramB = B;
    }

    float getCompensation() const override { return 1.0f; }

    void processBuffer(juce::AudioBuffer<float>& buff, int channelNum) override {
        float* channelData = buff.getWritePointer(channelNum);
        for (int i = 0; i < buff.getNumSamples(); ++i)
            channelData[i] = processSample(channelData[i]);
    }

    float processSample(float sample) {
        const float driveGain  = juce::Decibels::decibelsToGain(Drive);
        const float outputGain = juce::Decibels::decibelsToGain(Output) / getCompensation();
        float x = driveGain * sample;
        //float result = x - paramA * std::pow(x, 2) - paramB * std::pow(x, 3);
        float result = x - (paramA * 1.3f) * std::pow(x,2) - (paramB * 1.3f) * std::pow(x,3);
        return result * outputGain;
    }

private:
    float Drive = 5.0f, Output = 0.0f;
    float paramA = 0.25f, paramB = 0.75f;
};

//------------------------------------------------------------------------------------------------------------//
// Full Wave Rectify Distortion
class rectify : public DistortionBase {
public:
    void setParameters(float drive, float output) override {
        Drive = drive;
        Output = output;
    }

    float getCompensation() const override { return 1.0f; }

    void processBuffer(juce::AudioBuffer<float>& buff, int channelNum) override {
        float* channelData = buff.getWritePointer(channelNum);
        for (int i = 0; i < buff.getNumSamples(); ++i)
            channelData[i] = processSample(channelData[i]);
    }

    float processSample(float sample) {
        const float driveGain  = juce::Decibels::decibelsToGain(Drive);
        const float outputGain = juce::Decibels::decibelsToGain(Output) / getCompensation();
        float x = driveGain * sample;
        return std::abs(x) * outputGain;
    }

private:
    float Drive = 10.0f, Output = 2.0f;
};

//------------------------------------------------------------------------------------------------------------//
// Logarithmic Distortion
class logarithmic : public DistortionBase {
public:
    void setParameters(float drive, float output) override {
        Drive = drive;
        Output = output;
    }

    float getCompensation() const override { return 0.6f; }

    void processBuffer(juce::AudioBuffer<float>& buff, int channelNum) override {
        float* channelData = buff.getWritePointer(channelNum);
        for (int i = 0; i < buff.getNumSamples(); ++i)
            channelData[i] = processSample(channelData[i]);
    }

    float processSample(float sample) {
        const float driveGain  = juce::Decibels::decibelsToGain(Drive);
        const float outputGain = juce::Decibels::decibelsToGain(Output) / getCompensation();
        float x = driveGain * sample;
        float a = 8.0f;
        float result = std::copysign(std::log(1.0f + a * std::abs(x)) / std::log(1.0f + a), x);
        return result * outputGain;
    }

private:
    float Drive = 20.0f, Output = 5.0f;
};

//------------------------------------------------------------------------------------------------------------//
// Bitcrushed Distortion
class bitcrusher : public DistortionBase {
public:
    void setParameters(float drive, float output) override {
        Drive = drive;
        Output = output;
    }

    void setBitDepth(float bits) { bitDepth = bits; }

    float getCompensation() const override { return 1.0f; }

    void processBuffer(juce::AudioBuffer<float>& buff, int channelNum) override {
        float* channelData = buff.getWritePointer(channelNum);
        for (int i = 0; i < buff.getNumSamples(); ++i)
            channelData[i] = processSample(channelData[i]);
    }

    float processSample(float sample) {
        const float driveGain  = juce::Decibels::decibelsToGain(Drive);
        const float outputGain = juce::Decibels::decibelsToGain(Output) / getCompensation();
        float x = driveGain * sample;
        float step = 1.0f / std::pow(2.0f, bitDepth);
        float result = std::round(x / step) * step;
        return result * outputGain;
    }

private:
    float Drive = 3.0f, Output = 0.0f;
    float bitDepth = 5.0f;
};

//------------------------------------------------------------------------------------------------------------//
// Cubic Distortion
class cubic : public DistortionBase {
public:
    void setParameters(float drive, float output) override {
        Drive = drive;
        Output = output;
    }

    float getCompensation() const override { return 0.8f; }

    void processBuffer(juce::AudioBuffer<float>& buff, int channelNum) override {
        float* channelData = buff.getWritePointer(channelNum);
        for (int i = 0; i < buff.getNumSamples(); ++i)
            channelData[i] = processSample(channelData[i]);
    }

    float processSample(float sample) {
        const float driveGain  = juce::Decibels::decibelsToGain(Drive);
        const float outputGain = juce::Decibels::decibelsToGain(Output) / getCompensation();
        float x = driveGain * sample;
        float result = x - (1.0f / 3.0f) * std::pow(x, 3.0f) + (1.0f / 5.0f) * std::pow(x, 5.0f);;
        return result * outputGain;
    }

private:
    float Drive = 12.0f, Output = 0.0f;
};

//------------------------------------------------------------------------------------------------------------//
// Diode Distortion
class diode : public DistortionBase {
public:
    void setParameters(float drive, float output) override {
        Drive = drive;
        Output = output;
    }

    float getCompensation() const override { return 0.4f; }

    void processBuffer(juce::AudioBuffer<float>& buff, int channelNum) override {
        float* channelData = buff.getWritePointer(channelNum);
        for (int i = 0; i < buff.getNumSamples(); ++i)
            channelData[i] = processSample(channelData[i]);
    }

    float processSample(float sample) {
        const float driveGain  = juce::Decibels::decibelsToGain(Drive);
        const float outputGain = juce::Decibels::decibelsToGain(Output) / getCompensation();
        float x = driveGain * sample;
        float result = 0.5f * (std::tanh(x - bias) + std::tanh(x + bias));
        return result * outputGain;
    }

private:
    float Drive = 20.0f, Output = -2.0f;
    float bias = 0.5f;
};

//------------------------------------------------------------------------------------------------------------//
// Tube Distortion
class tube : public DistortionBase {
public:
    void setParameters(float drive, float output) override {
        Drive = drive;
        Output = output;
    }

    void processBuffer(juce::AudioBuffer<float>& buff, int channelNum) override {
        float* channelData = buff.getWritePointer(channelNum);
        for (int i = 0; i < buff.getNumSamples(); ++i)
            channelData[i] = processSample(channelData[i]);
    }

    float processSample(float sample) {
        const float driveGain  = juce::Decibels::decibelsToGain(Drive);
        const float outputGain = juce::Decibels::decibelsToGain(Output); // no compensation
        float x = driveGain * sample;
        float result = 1.5f * x - 0.5f * std::pow(x, 3.0f);
        return result * outputGain;
    }

private:
    float Drive = 8.0f, Output = 0.0f;
};

//------------------------------------------------------------------------------------------------------------//
// Chebyshev Distortion
class chebyshev : public DistortionBase {
public:
    void setParameters(float drive, float output) override {
        Drive = drive;
        Output = output;
    }

    float getCompensation() const override { return 1.0f; }

    void processBuffer(juce::AudioBuffer<float>& buff, int channelNum) override {
        float* channelData = buff.getWritePointer(channelNum);
        for (int i = 0; i < buff.getNumSamples(); ++i)
            channelData[i] = processSample(channelData[i]);
    }

    float processSample(float sample) {
        const float inputGain   = juce::Decibels::decibelsToGain(Drive);
        const float outputGain  = juce::Decibels::decibelsToGain(Output) / getCompensation();
        float x = inputGain * sample;

        if (std::abs(x) < 1.0e-2f)
            return 0.0f;

        x = juce::jlimit(-1.0f, 1.0f, x);
        float T2 = 2.0f * x * x - 1.0f;
        float T3 = 4.0f * x * x * x - 3.0f * x;
        float mixed = 0.5f * T2 + 0.3f * T3;

        return mixed * outputGain;
    }

private:
    float Drive = 2.0f, Output = 7.0f;
};

//------------------------------------------------------------------------------------------------------------//
// Lofi Distortion (sample rate reduction)
class lofi : public DistortionBase {
public:
    void setParameters(float drive, float output) override {
        Drive = drive;
        Output = output;
        counter = 0;
    }

    float getCompensation() const override { return 1.0f; }

    void processBuffer(juce::AudioBuffer<float>& buff, int channelNum) override {
        float* channelData = buff.getWritePointer(channelNum);
        for (int i = 0; i < buff.getNumSamples(); ++i)
            channelData[i] = processSample(channelData[i]);
    }

    float processSample(float sample) {
        const float driveGain  = juce::Decibels::decibelsToGain(Drive);
        const float outputGain = juce::Decibels::decibelsToGain(Output) / getCompensation();
        if (++counter >= rateDivider) {
            lastSample = sample * driveGain;
            counter = 0;
        }
        return lastSample * outputGain;
    }

private:
    float Drive = 0.0f, Output = 0.0f;
    float lastSample = 0.0f;
    int counter = 0;
    const int rateDivider = 8;
};

//------------------------------------------------------------------------------------------------------------//
// Wavefolder Distortion
class wavefolder : public DistortionBase {
public:
    void setParameters(float drive, float output) override {
        Drive = drive;
        Output = output;
    }

    void processBuffer(juce::AudioBuffer<float>& buff, int channelNum) override {
        float* channelData = buff.getWritePointer(channelNum);
        for (int i = 0; i < buff.getNumSamples(); ++i)
            channelData[i] = processSample(channelData[i]);
    }

    float processSample(float sample) {
        const float driveGain  = juce::Decibels::decibelsToGain(Drive);
        const float outputGain = juce::Decibels::decibelsToGain(Output); // no compensation
        float x = driveGain * sample;
        const float foldThreshold = 1.0f;
        while (x > foldThreshold)  x =  2 * foldThreshold - x;
        while (x < -foldThreshold) x = -2 * foldThreshold - x;
        return x * outputGain;
    }

private:
    float Drive = 15.0f, Output = 7.0f;
};

} // namespace JackDistortion

//------------------------------------------------------------------------------------------------------------//
/*
// XYZ  Distortion
class XYZ : public DistortionBase {
public:
    void setParameters(float drive, float output) override {
        Drive = drive;
        Output = output;
    }

    void processBuffer(juce::AudioBuffer<float>& buff, int channelNum) override {
        float* channelData = buff.getWritePointer(channelNum);
        for (int i = 0; i < buff.getNumSamples(); i++) {
            channelData[i] = processSample(channelData[i]);
        }
    }

    float processSample(float sample) {
        float driveGain = juce::Decibels::decibelsToGain(Drive);
        float outputGain = juce::Decibels::decibelsToGain(Output);

        float drivenSample = driveGain * sample;
        //float result = XYZequationforthisprocessing;
        //return result * outputGain;
    }
    
private:
    float Drive = 0.0f, Output = 0.0f;
};
*/





