#include "VstPanel.h"

#include "MainComponent.h"

VstPanel::VstPanel(
    MainComponent& ownerToUse)
    : owner(ownerToUse)
{
    wireCallbacks();
}

void VstPanel::setScanningState(bool scanning)
{
    scanButton.setEnabled(!scanning);
    cancelScanButton.setEnabled(scanning);
}

void VstPanel::wireCallbacks()
{

    pluginSearch.setTextToShowWhenEmpty(
        "Search VST3...",
        juce::Colours::grey);

    pluginSearch.setSelectAllWhenFocused(true);

    pluginSearch.onTextChange =
        [this]
        {
            owner.refreshPluginBrowser();
        };

    addAndMakeVisible(pluginSearch);

    pluginBrowserList.setModel(
        &owner.pluginBrowserModel);

    pluginBrowserList.setRowHeight(48);

    pluginBrowserList.setColour(
        juce::ListBox::backgroundColourId,
        juce::Colour(0xff202020));

    addAndMakeVisible(pluginBrowserList);

    addAndMakeVisible(scanButton);
    addAndMakeVisible(cancelScanButton);
    addAndMakeVisible(addButton);
    addAndMakeVisible(removeButton);

    addAndMakeVisible(bypassButton);

    addAndMakeVisible(upButton);
    addAndMakeVisible(downButton);

    addAndMakeVisible(editorButton);

    addAndMakeVisible(globalBypassButton);
    addAndMakeVisible(muteButton);

    scanButton.onClick = [this]
    {
        owner.startPluginScan();
    };

    cancelScanButton.onClick = [this]
    {
        owner.scanCancelled.store(true);
    };

    globalBypassButton.onClick = [this]
    {
        owner.setGlobalBypass(
            globalBypassButton
                .getToggleState());
    };

    muteButton.onClick =
        [this]
        {
            owner.setMuted(
                muteButton
                    .getToggleState());
        };

    cancelScanButton.setEnabled(false);

    addButton.onClick =
        [this]
        {
            const int row =
                pluginBrowserList
                    .getSelectedRow();

            owner.addPluginFromBrowser(row);
        };

    removeButton.onClick = [this]
    {
        owner.removeSelectedPlugin();
    };

    bypassButton.onClick = [this]
    {
        owner.toggleSelectedBypass();
    };

    upButton.onClick = [this]
    {
        owner.moveSelectedPluginUp();
    };

    downButton.onClick = [this]
    {
        owner.moveSelectedPluginDown();
    };

    editorButton.onClick = [this]
    {
        owner.openSelectedPluginEditor();
    };

    chainTitle.setText(
        "VST3 Chain",
        juce::dontSendNotification);

    chainTitle.setFont(20.0f);

    addAndMakeVisible(chainTitle);

    pluginChainList.setModel(&owner.chainModel);

    pluginChainList.setRowHeight(38);

    addAndMakeVisible(pluginChainList);

    addAndMakeVisible(presetCombo);
    addAndMakeVisible(savePresetButton);
    addAndMakeVisible(deletePresetButton);
    addAndMakeVisible(defaultPresetButton);

    presetCombo.setTextWhenNoChoicesAvailable(
        "No presets");

    presetCombo.setTextWhenNothingSelected(
        "Select preset...");

    presetCombo.onChange =
        [this]
        {
            if (owner.isLoadingVstPreset.load())
                return;

            const int index =
                presetCombo.getSelectedId();

            if (index <= 0 ||
                index > owner.vstPresetNames.size())
            {
                owner.activeVstPresetId.clear();
                return;
            }

            const auto name =
                owner.vstPresetNames[index - 1];

            if (name.isNotEmpty())
                owner.loadPreset(name);
        };

    savePresetButton.onClick =
        [this]
        {
            owner.saveCurrentPreset();
        };

    deletePresetButton.onClick =
        [this]
        {
            owner.deleteSelectedPreset();
        };

    defaultPresetButton.onClick =
        [this]
        {
            owner.toggleDefaultPreset();
        };

    addAndMakeVisible(
        clearPluginCacheButton);

    clearPluginCacheButton.onClick =
        [this]
        {
            owner.clearPluginCache();
        };

    addAndMakeVisible(
        autoReconnectButton);

    addAndMakeVisible(
        autoStartButton);

    autoReconnectButton.onClick =
        [this]
        {
            owner.autoReconnectEnabled =
                autoReconnectButton
                    .getToggleState();

            owner.saveState();
        };

    autoStartButton.onClick =
        [this]
        {
            owner.setAutoStart(
                autoStartButton
                    .getToggleState());
        };
}

void VstPanel::resized()
{
    auto area =
        getLocalBounds()
            .reduced(4);

    auto searchRow = area.removeFromTop(34);
    pluginSearch.setBounds(searchRow);

    area.removeFromTop(5);

    auto browserArea =
        area.removeFromTop(
            juce::jmin(
                220,
                area.getHeight() / 3));

    pluginBrowserList.setBounds(
        browserArea);

    area.removeFromTop(6);

    auto scanRow =
        area.removeFromTop(38);

    addButton.setBounds(
        scanRow.removeFromLeft(100)
            .reduced(2));

    scanButton.setBounds(
        scanRow.removeFromLeft(110)
            .reduced(2));

    cancelScanButton.setBounds(
        scanRow.removeFromLeft(90)
            .reduced(2));

    clearPluginCacheButton.setBounds(
        scanRow.removeFromLeft(110)
            .reduced(2));

    autoStartButton.setBounds(
        scanRow.removeFromLeft(150)
            .reduced(2));

    area.removeFromTop(8);

    auto presetRow =
        area.removeFromTop(36);

    presetCombo.setBounds(
        presetRow
            .removeFromLeft(200)
            .reduced(2));

    defaultPresetButton.setBounds(
        presetRow
            .removeFromLeft(120)
            .reduced(2));

    savePresetButton.setBounds(
        presetRow
            .removeFromLeft(110)
            .reduced(2));

    deletePresetButton.setBounds(
        presetRow
            .removeFromLeft(90)
            .reduced(2));

    area.removeFromTop(6);

    auto chainHeader =
        area.removeFromTop(32);

    chainTitle.setBounds(
        chainHeader
            .removeFromLeft(130));

    globalBypassButton.setBounds(
        chainHeader
            .removeFromLeft(120)
            .reduced(2));

    muteButton.setBounds(
        chainHeader
            .removeFromLeft(90)
            .reduced(2));

    autoReconnectButton.setBounds(
        chainHeader
            .removeFromLeft(150)
            .reduced(2));

    auto bottomButtons =
        area.removeFromBottom(44);

    editorButton.setBounds(
        bottomButtons
            .removeFromLeft(90)
            .reduced(2));

    bypassButton.setBounds(
        bottomButtons
            .removeFromLeft(90)
            .reduced(2));

    upButton.setBounds(
        bottomButtons
            .removeFromLeft(55)
            .reduced(2));

    downButton.setBounds(
        bottomButtons
            .removeFromLeft(55)
            .reduced(2));

    removeButton.setBounds(
        bottomButtons
            .removeFromLeft(80)
            .reduced(2));

    pluginChainList.setBounds(
        area.reduced(2));
}
