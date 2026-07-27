#include "PresetBar.h"
#include "LazerLook.h"
#include "Presets.h"

PresetBar::PresetBar (LazeratorAudioProcessor& p)
    : processor (p)
{
    for (auto* b : { &prevButton, &nextButton, &nameButton, &saveButton, &abButton })
        addAndMakeVisible (*b);

    prevButton.onClick = [this] { processor.presets.step (-1); };
    nextButton.onClick = [this] { processor.presets.step (1); };
    nameButton.onClick = [this] { showMenu(); };
    saveButton.onClick = [this] { savePreset(); };
    abButton.onClick   = [this] { processor.presets.toggleAB(); };

    // Benih: tuliskan berkas preset pabrik yang belum ada, lalu pindai folder.
    // Kalau foldernya sempat terhapus, langkah ini memulihkannya sendiri.
    processor.presets.exportFactoryPresets();
    processor.presets.refresh();

    nameButton.setButtonText (processor.presets.getCurrentName());
    startTimerHz (4);
}

void PresetBar::resized()
{
    auto r = getLocalBounds();
    prevButton.setBounds (r.removeFromLeft (26));
    r.removeFromLeft (4);
    abButton.setBounds (r.removeFromRight (36));
    r.removeFromRight (4);
    saveButton.setBounds (r.removeFromRight (48));
    r.removeFromRight (4);
    nextButton.setBounds (r.removeFromRight (26));
    r.removeFromRight (4);
    nameButton.setBounds (r);
}

void PresetBar::timerCallback()
{
    nameButton.setButtonText (processor.presets.getCurrentName());
    abButton.setButtonText (processor.presets.isSlotB() ? "B" : "A");
}

void PresetBar::showMenu()
{
    // Sumber daftar adalah isi folder, bukan tabel di dalam binary
    auto& pm = processor.presets;
    pm.refresh();

    juce::PopupMenu menu;
    // PopupMenu tidak mewarisi LookAndFeel dari komponen target — harus diberikan
    // eksplisit agar menu ikut tema terang.
    menu.setLookAndFeel (&getLookAndFeel());

    const auto& entries = pm.getEntries();
    const int current = pm.getCurrentIndex();

    if (entries.empty())
    {
        menu.addItem (-1, "Folder preset kosong", false);
    }
    else
    {
        juce::String category;
        juce::PopupMenu sub;
        bool categoryHasCurrent = false;

        auto flush = [&]
        {
            if (category.isNotEmpty())
                menu.addSubMenu (category, sub, true, nullptr, categoryHasCurrent, 0);
        };

        for (int i = 0; i < (int) entries.size(); ++i)
        {
            if (category != entries[(size_t) i].category)
            {
                flush();
                sub = {};
                categoryHasCurrent = false;
                category = entries[(size_t) i].category;
            }
            const bool isCurrent = i == current;
            categoryHasCurrent = categoryHasCurrent || isCurrent;
            sub.addItem (i + 1, entries[(size_t) i].name, true, isCurrent);
        }
        flush();

        menu.addSeparator();
        menu.addItem (9000, "Buka folder preset...");
    }

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&nameButton),
        [this] (int result)
        {
            if (result == 9000)
                PresetManager::getUserPresetFolder().revealToUser();
            else if (result > 0)
                processor.presets.load (result - 1);
        });
}

void PresetBar::savePreset()
{
    auto folder = PresetManager::getUserPresetFolder();
    folder.createDirectory();

    chooser = std::make_unique<juce::FileChooser> (
        "Simpan preset", folder.getChildFile (processor.presets.getCurrentName() + ".lzp"), "*.lzp");

    chooser->launchAsync (juce::FileBrowserComponent::saveMode
                        | juce::FileBrowserComponent::canSelectFiles
                        | juce::FileBrowserComponent::warnAboutOverwriting,
        [this] (const juce::FileChooser& fc)
        {
            const auto file = fc.getResult();
            if (file.getFullPathName().isEmpty())
                return;
            processor.presets.saveUserPreset (file.withFileExtension ("lzp"));
        });
}
