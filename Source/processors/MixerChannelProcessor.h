#pragma once

#include "JuceHeader.h"
#include "DefaultAudioProcessor.h"
#include "view/level_meter/LevelMeter.h"

class MixerChannelProcessor : public DefaultAudioProcessor {
public:
    explicit MixerChannelProcessor() :
            DefaultAudioProcessor(getPluginDescription()),
            balanceParameter(new AudioParameterFloat("balance", "Balance", NormalisableRange<float>(0.0f, 1.0f), balance.getTargetValue(), "",
                                                     AudioProcessorParameter::genericParameter, defaultStringFromValue, defaultValueFromString)),
            gainParameter(new AudioParameterFloat("gain", "Gain", NormalisableRange<float>(0.0f, 1.0f), gain.getTargetValue(), "dB",
                                                  AudioProcessorParameter::genericParameter, defaultStringFromDbValue, defaultValueFromDbString)) {

        inlineEditor = std::make_unique<InlineEditor>(*this);
        balanceParameter->addListener(this);
        gainParameter->addListener(this);

        addParameter(balanceParameter);
        addParameter(gainParameter);
    }

    ~MixerChannelProcessor() override {
        gainParameter->removeListener(this);
        balanceParameter->removeListener(this);
    }
    static const String name() { return "Mixer Channel"; }

    static PluginDescription getPluginDescription() {
        return DefaultAudioProcessor::getPluginDescription(name(), false, false);
    }

    Component* getInlineEditor() override {
        return inlineEditor.get();
    }

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override {
        balance.reset(getSampleRate(), 0.05);
        gain.reset(getSampleRate(), 0.05);
    }

    void parameterChanged(AudioProcessorParameter *parameter, float newValue) override {
        if (parameter == balanceParameter) {
            balance.setValue(newValue);
        } else if (parameter == gainParameter) {
            gain.setValue(newValue);
        }
    }

    void processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages) override {
        if (buffer.getNumChannels() == 2) {
            // 0db at center, linear stereo balance control
            // http://www.kvraudio.com/forum/viewtopic.php?t=148865
            for (int i = 0; i < buffer.getNumSamples(); i++) {
                const float balanceValue = balance.getNextValue();

                float leftChannelGain, rightChannelGain;
                if (balanceValue < 0.5) {
                    leftChannelGain = 1;
                    rightChannelGain = balanceValue * 2;
                } else {
                    leftChannelGain = (1 - balanceValue) * 2;
                    rightChannelGain = 1;
                }

                buffer.setSample(0, i, buffer.getSample(0, i) * leftChannelGain);
                buffer.setSample(1, i, buffer.getSample(1, i) * rightChannelGain);
            }
        }
        meterSource.measureBlock(buffer);
        gain.applyGain(buffer, buffer.getNumSamples());
    }

    LevelMeterSource* getMeterSource() {
        return &meterSource;
    }

private:
    class InlineEditor : public Component {
    public:
        explicit InlineEditor(MixerChannelProcessor &processor) : processor(processor) {
            meter = std::make_unique<LevelMeter>();
            meter->setMeterSource(processor.getMeterSource());
            addAndMakeVisible(meter.get());
        }

        void resized() override {
            meter->setBounds(getLocalBounds().removeFromLeft(getWidth() / 3).withX(getWidth() / 6));
        }
    private:
        MixerChannelProcessor &processor;
        std::unique_ptr<LevelMeter> meter;
    };

    std::unique_ptr<InlineEditor> inlineEditor;

    LinearSmoothedValue<float> balance { 0.5 };
    LinearSmoothedValue<float> gain { 0.5 };

    AudioParameterFloat *balanceParameter;
    AudioParameterFloat *gainParameter;
    LevelMeterSource meterSource;
};
