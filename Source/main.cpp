#include <JuceHeader.h>
#include "MainComponent.h"

class VSTMicHostApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override
    {

        return "VoxLane 0.5";

    }

    const juce::String getApplicationVersion() override
    {
        return "0.5.0";
    }

    bool moreThanOneInstanceAllowed() override
    {
        return false;
    }

    void initialise(const juce::String& commandLine) override
    {
        const bool startHidden =
            commandLine.contains("--tray");

        mainWindow =
            std::make_unique<MainWindow>(
                startHidden);
    }

    void shutdown() override
    {
        mainWindow.reset();
    }

    void systemRequestedQuit() override
    {
        quit();
    }

private:
    class MainWindow final : public juce::DocumentWindow
    {
    public:
        class Tray final
            : public juce::SystemTrayIconComponent
        {
        public:
            explicit Tray(
                MainWindow& ownerToUse)
                : owner(ownerToUse)
            {
                juce::Image icon(juce::Image::ARGB, 32, 32, true);
                juce::Graphics g(icon);
                g.setColour(juce::Colour(0xff48b89b));
                g.fillRoundedRectangle(11.0f, 3.0f, 10.0f, 18.0f, 5.0f);
                g.drawRoundedRectangle(7.0f, 8.0f, 18.0f, 18.0f, 8.0f, 2.0f);
                g.fillRect(15, 25, 2, 5);
                g.fillRect(10, 29, 12, 2);
                setIconImage(icon, icon);
                setIconTooltip("VoxLane 0.5");
            }

            void mouseDoubleClick(
                const juce::MouseEvent&) override
            {
                owner.setVisible(true);
                owner.toFront(true);
            }

            void mouseDown(
                const juce::MouseEvent& event) override
            {
                if (!event.mods
                         .isRightButtonDown())
                    return;

                juce::PopupMenu menu;

                menu.addItem(
                    1,
                    "Open");

                menu.addSeparator();

                menu.addItem(
                    2,
                    "Quit");

                menu.showMenuAsync(
                    juce::PopupMenu::Options(),
                    [this](int result)
                    {
                        if (result == 1)
                        {
                            owner.setVisible(true);
                            owner.toFront(true);
                        }

                        if (result == 2)
                        {
                            juce::JUCEApplication::
                                getInstance()
                                ->systemRequestedQuit();
                        }
                    });
            }

        private:
            MainWindow& owner;
        };

        explicit MainWindow(
            bool startHidden)
            : DocumentWindow(

                "VoxLane 0.5",

                juce::Colour(0xff181818),
                DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);

            setContentOwned(new MainComponent(),true);

            setResizable(true, true);

            centreWithSize(1100,720);

            tray =
                std::make_unique<Tray>(
                    *this);

            setVisible(!startHidden);
        }

        void closeButtonPressed() override
        {

            setVisible(false);
        }

    private:
        std::unique_ptr<Tray> tray;
    };

    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(VSTMicHostApplication)
