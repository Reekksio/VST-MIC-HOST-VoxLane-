#pragma once

#include <JuceHeader.h>
#include <atomic>

class AudioUtilityProcessor final : public juce::AudioProcessor
{
public:
    enum class Mode
    {
        Input,
        Output
    };

    explicit AudioUtilityProcessor(Mode modeToUse);

    const juce::String getName() const override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(
        juce::AudioBuffer<float>&,
        juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    double getTailLengthSeconds() const override { return 0.0; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    void setGainDb(float newGain);
    float getGainDb() const;

    void setLimiterEnabled(bool enabled);
    bool getLimiterEnabled() const;

    void setMuted(bool muted);
    bool getMuted() const;

    float getPeakDb() const;
    float getPeakLinear() const;

private:
    Mode mode;

    std::atomic<float> gainDb { 0.0f };
    std::atomic<float> peakLinear { 0.0f };
    std::atomic<bool> limiterEnabled { true };
    std::atomic<bool> muted { false };

    juce::SmoothedValue<float> gainLinear;
    juce::SmoothedValue<float> muteGain;

    double currentSampleRate = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioUtilityProcessor)
};
