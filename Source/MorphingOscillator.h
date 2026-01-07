/*
  ==============================================================================

    MorphingOscillator.h
    Created:    24 Oct 2025 3:58:25pm
    Modified:   2 Jan 2026 12:17:00pm

  ==============================================================================
*/

#pragma once
#include <cmath>

struct MorphingWaveformSound final : public SynthesiserSound
{
    bool appliesToNote (int /*midiNoteNumber*/) override    { return true; }
    bool appliesToChannel (int /*midiChannel*/) override    { return true; }
};

/// MorphingWaveformVoice is a simple 'morphing oscillator'
/// that outputs a linear combination of two user-chosen waveforms. 
/// This combination is controlled by the 'wavePosition' variable,
/// allowing the user to dynamically 'fade' between the waveforms.
struct MorphingWaveformVoice final : public SynthesiserVoice
{
    void startNote (int midiNoteNumber, float velocity,
                    SynthesiserSound*, int /*currentPitchWheelPosition*/) override
    {
        phaseIndex = 0.0;
        auto currentFrequency = MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        auto cyclesPerSample = currentFrequency / getSampleRate();
        phaseIncrement = cyclesPerSample * MathConstants<double>::twoPi;
    }

    void renderNextBlock (AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        if (! approximatelyEqual (phaseIncrement, 0.0))
        {
            while (--numSamples >= 0)
            {
                auto wave_a_value = getWaveSample(wave_a, phaseIndex);
                auto wave_b_value = getWaveSample(wave_b, phaseIndex);
                auto interpolatedValue = (wave_a_value * (1.0 - wavePosition)) + (wave_b_value * wavePosition);
                auto levelAdjustedSample = interpolatedValue * level;
                auto outputLen = outputBuffer.getNumChannels();
                for (auto channel = 0; channel < outputLen; ++channel)
                    outputBuffer.addSample(channel, startSample, levelAdjustedSample);
                phaseIndex += phaseIncrement;
                startSample++;
            }
        }
    }

    double getWaveSample(int waveform, double input) {
        return waveform == 0 ? sineValue(input) :
               waveform == 1 ? squareValue(input) :
               waveform == 2 ? triangleValue(input) :
               sawValue(input);
    }

    void updateMorphFunctions(double position) {
        // Updates wavePosition, which determines our waveforms and their respective scalars
        auto positionFloor = std::floor(position);
        wave_a = static_cast<int>(positionFloor);
        wave_b = std::clamp(wave_a + 1, 0, 3);
        wavePosition = position - positionFloor;
    }

    bool canPlaySound (SynthesiserSound* sound) override { return dynamic_cast<MorphingWaveformSound*> (sound) != nullptr; }
    void stopNote (float /*velocity*/, bool /*allowTailOff*/) override { phaseIncrement = 0.0; }
    void pitchWheelMoved (int /*newValue*/) override                              {}
    void controllerMoved (int /*controllerNumber*/, int /*newValue*/) override    {}
    
    double sineValue(double currentAngle) { 
        return std::sin(currentAngle); 
    }
    double squareValue(double currentAngle) { 
        return 1.0 - 2.0 * static_cast<double>(std::sin(currentAngle) < 0.0); 
    }
    double triangleValue(double currentAngle) { 
        return (2.0 / MathConstants<double>::pi) * std::asin(std::sin(currentAngle)); 
    }
    double sawValue(double currentAngle) {
        return (2.0 / MathConstants<double>::pi) * (std::fmod(currentAngle, MathConstants<double>::twoPi) - MathConstants<double>::pi);
    }

    using SynthesiserVoice::renderNextBlock;

    double phaseIndex = 0.0, phaseIncrement = 0.0, level = 1.0, wavePosition = 0.0;
    int wave_a = 0, wave_b = 0;
};
