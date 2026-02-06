/*
  ==============================================================================

   This file is part of the JUCE framework examples.
   Copyright (c) Raw Material Software Limited

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
   REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
   AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
   INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
   LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
   OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
   PERFORMANCE OF THIS SOFTWARE.

  ==============================================================================
*/

// ================================================================================================
// The following code is derived from the AudioSynthesiserDemo.h JUCE demo with few modifications.
// ================================================================================================

#pragma once
#include <cmath>
#include "MorphingOscillator.h"

struct SynthAudioSource final : public AudioSource
{
    SynthAudioSource (MidiKeyboardState& keyState)  : keyboardState (keyState)
    {
        synth.addVoice (new MorphingWaveformVoice());
        synth.clearSounds();
        synth.addSound (new MorphingWaveformSound());
    }

    void prepareToPlay (int /*samplesPerBlockExpected*/, double sampleRate) override
    {
        midiCollector.reset (sampleRate);
        synth.setCurrentPlaybackSampleRate (sampleRate);
    }

    void releaseResources() override {}

    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        bufferToFill.clearActiveBufferRegion();
        MidiBuffer incomingMidi;
        midiCollector.removeNextBlockOfMessages (incomingMidi, bufferToFill.numSamples);
        keyboardState.processNextMidiBuffer (incomingMidi, 0, bufferToFill.numSamples, true);
        synth.renderNextBlock (*bufferToFill.buffer, incomingMidi, 0, bufferToFill.numSamples);
    }

    MidiMessageCollector midiCollector;
    MidiKeyboardState& keyboardState;
    Synthesiser synth;
};

class Callback final : public AudioIODeviceCallback
{
public:
    Callback (AudioSourcePlayer& playerIn) : player (playerIn) {}

    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const AudioIODeviceCallbackContext& context) override
    {
        player.audioDeviceIOCallbackWithContext (inputChannelData,
                                                 numInputChannels,
                                                 outputChannelData,
                                                 numOutputChannels,
                                                 numSamples,
                                                 context);
    }

    void audioDeviceAboutToStart (AudioIODevice* device) override
    {
        player.audioDeviceAboutToStart (device);
    }

    void audioDeviceStopped() override
    {
        player.audioDeviceStopped();
    }

private:
    AudioSourcePlayer& player;
};

class AudioSynthesiserDemo final : public Component
{
public:
    AudioSynthesiserDemo()
    {
        addAndMakeVisible (keyboardComponent);

        audioSourcePlayer.setSource (&synthAudioSource);
        
        addAndMakeVisible(waveformBlend);
        waveformBlend.setRange (0.0, 2.0, 0.01);
        waveformBlend.setValue(0.0, dontSendNotification);
        waveformBlend.onValueChange = [this]() {
            auto& synth = synthAudioSource.synth;
            MorphingWaveformVoice* voice = static_cast<MorphingWaveformVoice*>(synth.getVoice(0));
            voice->updateMorphFunctions(waveformBlend.getValue());
        };
        addAndMakeVisible(waveformBlendLabel);
        waveformBlendLabel.setText("Waveform", juce::dontSendNotification);
        waveformBlendLabel.attachToComponent(&waveformBlend, true);

       #ifndef JUCE_DEMO_RUNNER
        audioDeviceManager.initialise (0, 2, nullptr, true, {}, nullptr);
       #endif

        audioDeviceManager.addAudioCallback (&callback);
        audioDeviceManager.addMidiInputDeviceCallback ({}, &(synthAudioSource.midiCollector));

        setOpaque (true);
        setSize (600, 200);
    }

    ~AudioSynthesiserDemo() override
    {
        audioSourcePlayer.setSource (nullptr);
        audioDeviceManager.removeMidiInputDeviceCallback ({}, &(synthAudioSource.midiCollector));
        audioDeviceManager.removeAudioCallback (&callback);
    }

    void paint (Graphics& g) override
    {
        g.fillAll (juce::Colours::grey);
    }

    void resized() override
    {
        auto width = getWidth();
        auto height = getHeight();
        keyboardComponent   .setBounds (0, height * 0.2, width, height * 0.8);
        waveformBlend       .setBounds (width * 0.25, 0, width * 0.5, height * 0.2);
    }

private:
   #ifndef JUCE_DEMO_RUNNER
    AudioDeviceManager audioDeviceManager;
   #else
    AudioDeviceManager& audioDeviceManager { getSharedAudioDeviceManager (0, 2) };
   #endif

    MidiKeyboardState keyboardState;
    AudioSourcePlayer audioSourcePlayer;
    SynthAudioSource synthAudioSource        { keyboardState };
    MidiKeyboardComponent keyboardComponent  { keyboardState, MidiKeyboardComponent::horizontalKeyboard};

    Label waveformBlendLabel;
    Slider waveformBlend;

    Callback callback { audioSourcePlayer };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioSynthesiserDemo)
};
