#include "AudioUtilityProcessor.h"
#include <cmath>

static float softClip(float x)
{

    constexpr float threshold =
        0.8912509f;

    const float ax =
        std::abs(x);

    if (ax <= threshold)
        return x;

    const float sign =
        x >= 0.0f
            ? 1.0f
            : -1.0f;

    const float over =
        ax - threshold;

    const float compressed =
        threshold +
        (1.0f - threshold)
        * std::tanh(
            over /
            (1.0f - threshold));

    return sign *
        juce::jmin(
            compressed,
            0.999f);
}

AudioUtilityProcessor::AudioUtilityProcessor(Mode modeToUse)
    : AudioProcessor(
        BusesProperties()
            .withInput("Input",
                       juce::AudioChannelSet::stereo(),
                       true)
            .withOutput("Output",
                        juce::AudioChannelSet::stereo(),
                        true)),
      mode(modeToUse)
{
}

const juce::String AudioUtilityProcessor::getName() const
{
    return mode == Mode::Input
        ? "Input Utility"
        : "Output Utility";
}

void AudioUtilityProcessor::prepareToPlay(
    double sampleRate,
    int)
{
    currentSampleRate = sampleRate;

    gainLinear.reset(
        sampleRate,
        0.02);

    gainLinear.setCurrentAndTargetValue(
        juce::Decibels::decibelsToGain(
            gainDb.load()));

    muteGain.reset(
        sampleRate,
        0.01);

    muteGain.setCurrentAndTargetValue(
        muted.load()
            ? 0.0f
            : 1.0f);
}

void AudioUtilityProcessor::releaseResources()
{
}

bool AudioUtilityProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const
{
    const auto& input =
        layouts.getMainInputChannelSet();

    const auto& output =
        layouts.getMainOutputChannelSet();

    if (input != output)
        return false;

    return output == juce::AudioChannelSet::mono()
        || output == juce::AudioChannelSet::stereo();
}

void AudioUtilityProcessor::setGainDb(float newGain)
{
    newGain = juce::jlimit(
        -60.0f,
        18.0f,
        newGain);

    gainDb.store(newGain);

    gainLinear.setTargetValue(
        juce::Decibels::decibelsToGain(
            newGain));
}

float AudioUtilityProcessor::getGainDb() const
{
    return gainDb.load();
}

void AudioUtilityProcessor::setLimiterEnabled(bool enabled)
{
    limiterEnabled.store(enabled);
}

bool AudioUtilityProcessor::getLimiterEnabled() const
{
    return limiterEnabled.load();
}

void AudioUtilityProcessor::setMuted(bool shouldMute)
{
    muted.store(shouldMute);

    muteGain.setTargetValue(
        shouldMute
            ? 0.0f
            : 1.0f);
}

bool AudioUtilityProcessor::getMuted() const
{
    return muted.load();
}

float AudioUtilityProcessor::getPeakLinear() const
{
    return peakLinear.load();
}

float AudioUtilityProcessor::getPeakDb() const
{
    const float peak =
        peakLinear.load();

    if (peak <= 0.000001f)
        return -100.0f;

    return juce::Decibels::gainToDecibels(
        peak);
}

void AudioUtilityProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int channels =
        buffer.getNumChannels();

    const int samples =
        buffer.getNumSamples();

    float blockPeak = 0.0f;

    for (int sample = 0;
         sample < samples;
         ++sample)
    {
        const float finalGain =
            gainLinear.getNextValue()
            * muteGain.getNextValue();

        for (int channel = 0;
             channel < channels;
             ++channel)
        {
            float value =
                buffer.getSample(
                    channel,
                    sample);

            value *= finalGain;

            if (mode == Mode::Output &&
                limiterEnabled.load())
            {
                value = softClip(value);
            }

            buffer.setSample(
                channel,
                sample,
                value);

            blockPeak =
                juce::jmax(
                    blockPeak,
                    std::abs(value));
        }
    }

    float oldPeak =
        peakLinear.load();

    const float fallingPeak =
        oldPeak * 0.93f;

    peakLinear.store(
        juce::jmax(
            blockPeak,
            fallingPeak));
}
