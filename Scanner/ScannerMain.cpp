#include <JuceHeader.h>

static int scanPlugin(
    const juce::String& pluginPath,
    const juce::File& outputFile)
{
    juce::AudioPluginFormatManager formatManager;
    formatManager.addDefaultFormats();

    juce::KnownPluginList scannedPlugins;

    bool vst3Found = false;

    for (auto* format : formatManager.getFormats())
    {
        if (format == nullptr)
            continue;

        if (!format->getName().containsIgnoreCase("VST3"))
            continue;

        vst3Found = true;

        juce::OwnedArray<juce::PluginDescription> foundTypes;

        scannedPlugins.scanAndAddFile(
            pluginPath,
            false,
            foundTypes,
            *format);

        break;
    }

    if (!vst3Found)
        return 10;

    auto xml = scannedPlugins.createXml();

    if (xml == nullptr)
        return 11;

    outputFile.deleteFile();

    if (!xml->writeTo(outputFile))
        return 12;

    auto logFile =
        outputFile.getSiblingFile(
            outputFile.getFileNameWithoutExtension()
            + ".log");

    juce::String log;

    log << "PLUGIN: "
        << pluginPath
        << "\n";

    log << "FOUND: "
        << scannedPlugins.getNumTypes()
        << "\n";

    for (const auto& type :
         scannedPlugins.getTypes())
    {
        log << type.name
            << " | "
            << type.manufacturerName
            << "\n";
    }

    logFile.replaceWithText(log);

    return scannedPlugins.getNumTypes() > 0
        ? 0
        : 13;
}

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    if (argc < 3)
        return 2;

    const juce::String pluginPath =
        juce::String::fromUTF8(argv[1]);

    const juce::File outputFile(
        juce::String::fromUTF8(argv[2]));

    if (pluginPath.isEmpty())
        return 3;

    try
    {
        return scanPlugin(
            pluginPath,
            outputFile);
    }
    catch (...)
    {
        return 100;
    }
}
