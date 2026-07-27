#include "Presets.h"

// Kunci parameter: BaseFreq, Pinch, Amount, SweepDepth (oktaf, + = turun),
// SweepTime (ms), Curve, Retrigger (0 Reset / 1 Legato / 2 Lock),
// Threshold, Sensitivity, Hold, SCHighPass, SCLowPass,
// StereoMode (0 Link / 1 Offset / 2 Mid-Side), Spread, TimeOffset, SideDepth,
// DryWet, Drive, OutputGain.
const std::vector<FactoryPreset>& getFactoryPresets()
{
    static const std::vector<FactoryPreset> presets =
    {
        // --- Lazers & Zaps -------------------------------------------------
        { "Lazers & Zaps", "Classic Kick Lazer", {{ "BaseFreq", 500.0f }, { "SweepDepth", 4.0f }, { "SweepTime", 150.0f }, { "Amount", 16.0f }, { "Pinch", 5.0f }, { "SCLowPass", 200.0f }}},
        { "Lazers & Zaps", "Short Zap",          {{ "BaseFreq", 900.0f }, { "SweepDepth", 3.5f }, { "SweepTime", 60.0f },  { "Amount", 12.0f }, { "Pinch", 6.0f }}},
        { "Lazers & Zaps", "Deep Zap",           {{ "BaseFreq", 150.0f }, { "SweepDepth", 5.0f }, { "SweepTime", 220.0f }, { "Amount", 20.0f }, { "Pinch", 4.0f }, { "SCLowPass", 300.0f }}},
        { "Lazers & Zaps", "Hi Zap",             {{ "BaseFreq", 2500.0f }, { "SweepDepth", 2.5f }, { "SweepTime", 45.0f }, { "Amount", 10.0f }, { "Pinch", 8.0f }}},
        { "Lazers & Zaps", "Pew Pew",            {{ "BaseFreq", 1200.0f }, { "SweepDepth", 4.0f }, { "SweepTime", 80.0f }, { "Amount", 8.0f },  { "Pinch", 10.0f }, { "Hold", 60.0f }}},
        { "Lazers & Zaps", "Laser Cannon",       {{ "BaseFreq", 300.0f }, { "SweepDepth", 5.5f }, { "SweepTime", 400.0f }, { "Amount", 24.0f }, { "Pinch", 5.0f }, { "Curve", 0.6f }}},
        { "Lazers & Zaps", "Retro Blaster",      {{ "BaseFreq", 700.0f }, { "SweepDepth", 3.0f }, { "SweepTime", 100.0f }, { "Amount", 6.0f },  { "Pinch", 12.0f }, { "Curve", 2.0f }}},
        { "Lazers & Zaps", "Snappy Laser",       {{ "BaseFreq", 600.0f }, { "SweepDepth", 4.0f }, { "SweepTime", 90.0f },  { "Amount", 14.0f }, { "Pinch", 6.0f }, { "Curve", 0.5f }}},
        { "Lazers & Zaps", "Tail Whip",          {{ "BaseFreq", 400.0f }, { "SweepDepth", 4.5f }, { "SweepTime", 300.0f }, { "Amount", 18.0f }, { "Curve", 2.5f }}},
        { "Lazers & Zaps", "Double Tap",         {{ "BaseFreq", 800.0f }, { "SweepDepth", 3.5f }, { "SweepTime", 70.0f },  { "Amount", 12.0f }, { "Pinch", 7.0f }, { "Hold", 35.0f }}},
        { "Lazers & Zaps", "Photon Snap",        {{ "BaseFreq", 1500.0f }, { "SweepDepth", 3.0f }, { "SweepTime", 55.0f }, { "Amount", 9.0f },  { "Pinch", 9.0f }, { "Curve", 0.7f }}},
        { "Lazers & Zaps", "Heavy Beam",         {{ "BaseFreq", 250.0f }, { "SweepDepth", 5.0f }, { "SweepTime", 500.0f }, { "Amount", 28.0f }, { "Pinch", 3.5f }, { "Curve", 1.4f }}},

        // --- Bass & Color --------------------------------------------------
        { "Bass & Color", "Color Bass Smear", {{ "BaseFreq", 180.0f }, { "SweepDepth", 2.5f }, { "SweepTime", 140.0f }, { "Amount", 22.0f }, { "Pinch", 3.0f }, { "SCLowPass", 250.0f }}},
        { "Bass & Color", "Liquid Low",       {{ "BaseFreq", 120.0f }, { "SweepDepth", 2.0f }, { "SweepTime", 200.0f }, { "Amount", 26.0f }, { "Pinch", 2.5f }, { "Curve", 1.8f }}},
        { "Bass & Color", "Growl Sweep",      {{ "BaseFreq", 350.0f }, { "SweepDepth", 3.0f }, { "SweepTime", 110.0f }, { "Amount", 18.0f }, { "Pinch", 5.0f }, { "Sensitivity", 9.0f }}},
        { "Bass & Color", "Rubber Bass",      {{ "BaseFreq", 220.0f }, { "SweepDepth", 1.8f }, { "SweepTime", 90.0f },  { "Amount", 20.0f }, { "Pinch", 6.0f }, { "Curve", 0.8f }}},
        { "Bass & Color", "Neuro Wob",        {{ "BaseFreq", 500.0f }, { "SweepDepth", 2.2f }, { "SweepTime", 60.0f },  { "Amount", 16.0f }, { "Pinch", 8.0f }, { "Hold", 40.0f }, { "Retrigger", 1.0f }}},
        { "Bass & Color", "Sub Bender",       {{ "BaseFreq", 80.0f },  { "SweepDepth", 2.0f }, { "SweepTime", 260.0f }, { "Amount", 24.0f }, { "Pinch", 2.0f }, { "SCLowPass", 150.0f }}},
        { "Bass & Color", "Formant Flip",     {{ "BaseFreq", 900.0f }, { "SweepDepth", -2.0f }, { "SweepTime", 120.0f }, { "Amount", 14.0f }, { "Pinch", 7.0f }}},
        { "Bass & Color", "Thick Glue",       {{ "BaseFreq", 300.0f }, { "SweepDepth", 1.5f }, { "SweepTime", 180.0f }, { "Amount", 30.0f }, { "Pinch", 2.2f }, { "DryWet", 70.0f }}},
        { "Bass & Color", "Talky Smear",      {{ "BaseFreq", 1100.0f }, { "SweepDepth", 1.8f }, { "SweepTime", 100.0f }, { "Amount", 12.0f }, { "Pinch", 9.0f }, { "Curve", 1.6f }}},
        { "Bass & Color", "Dark Slide",       {{ "BaseFreq", 160.0f }, { "SweepDepth", 3.2f }, { "SweepTime", 240.0f }, { "Amount", 20.0f }, { "Pinch", 4.0f }, { "Curve", 2.2f }}},

        // --- Drums ---------------------------------------------------------
        { "Drums", "Kick Snap",       {{ "BaseFreq", 600.0f }, { "SweepDepth", 3.0f }, { "SweepTime", 45.0f },  { "Amount", 8.0f },  { "Pinch", 5.0f }, { "SCLowPass", 180.0f }}},
        { "Drums", "Kick Boom Tail",  {{ "BaseFreq", 90.0f },  { "SweepDepth", 2.5f }, { "SweepTime", 160.0f }, { "Amount", 16.0f }, { "SCLowPass", 150.0f }, { "Curve", 2.0f }}},
        { "Drums", "Snare Zip",       {{ "BaseFreq", 1800.0f }, { "SweepDepth", 2.5f }, { "SweepTime", 65.0f }, { "Amount", 10.0f }, { "SCHighPass", 400.0f }, { "Pinch", 6.0f }}},
        { "Drums", "Snare Fat",       {{ "BaseFreq", 250.0f }, { "SweepDepth", 2.0f }, { "SweepTime", 85.0f },  { "Amount", 14.0f }, { "SCHighPass", 200.0f }, { "SCLowPass", 4000.0f }}},
        { "Drums", "Hat Shimmer",     {{ "BaseFreq", 6000.0f }, { "SweepDepth", 1.5f }, { "SweepTime", 35.0f }, { "Amount", 8.0f },  { "SCHighPass", 1900.0f }, { "Pinch", 7.0f }, { "DryWet", 60.0f }}},
        { "Drums", "Perc Bounce",     {{ "BaseFreq", 900.0f }, { "SweepDepth", 2.8f }, { "SweepTime", 75.0f },  { "Amount", 12.0f }, { "Hold", 30.0f }}},
        { "Drums", "Tom Bender",      {{ "BaseFreq", 200.0f }, { "SweepDepth", 2.2f }, { "SweepTime", 130.0f }, { "Amount", 15.0f }, { "SCLowPass", 500.0f }}},
        { "Drums", "Clap Smear",      {{ "BaseFreq", 1200.0f }, { "SweepDepth", 1.8f }, { "SweepTime", 95.0f }, { "Amount", 18.0f }, { "SCHighPass", 500.0f }, { "Curve", 1.5f }}},
        { "Drums", "Drum Bus Motion", {{ "BaseFreq", 700.0f }, { "SweepDepth", 1.2f }, { "SweepTime", 70.0f },  { "Amount", 10.0f }, { "DryWet", 45.0f }, { "Threshold", -20.0f }}},
        { "Drums", "Riddim Machine",  {{ "BaseFreq", 450.0f }, { "SweepDepth", 3.5f }, { "SweepTime", 95.0f },  { "Amount", 16.0f }, { "Pinch", 6.0f }, { "Hold", 25.0f }}},

        // --- Risers & FX (Depth negatif = sapuan naik) ----------------------
        { "Risers & FX", "Up Riser Short", {{ "BaseFreq", 3000.0f }, { "SweepDepth", -3.5f }, { "SweepTime", 300.0f },  { "Amount", 16.0f }, { "Retrigger", 2.0f }}},
        { "Risers & FX", "Up Riser Long",  {{ "BaseFreq", 6000.0f }, { "SweepDepth", -5.0f }, { "SweepTime", 1200.0f }, { "Amount", 24.0f }, { "Retrigger", 2.0f }, { "Curve", 1.8f }}},
        { "Risers & FX", "Inverse Zap",    {{ "BaseFreq", 4000.0f }, { "SweepDepth", -4.0f }, { "SweepTime", 120.0f },  { "Amount", 14.0f }, { "Pinch", 6.0f }}},
        { "Risers & FX", "Alarm Whoop",    {{ "BaseFreq", 1500.0f }, { "SweepDepth", -2.5f }, { "SweepTime", 160.0f },  { "Amount", 12.0f }, { "Pinch", 10.0f }, { "Hold", 80.0f }}},
        { "Risers & FX", "Doppler Pass",   {{ "BaseFreq", 2000.0f }, { "SweepDepth", -2.0f }, { "SweepTime", 600.0f },  { "Amount", 20.0f }, { "Curve", 1.3f }, { "Retrigger", 2.0f }}},
        { "Risers & FX", "Cyber Sirene",   {{ "BaseFreq", 1000.0f }, { "SweepDepth", -3.0f }, { "SweepTime", 250.0f },  { "Amount", 18.0f }, { "Pinch", 12.0f }, { "Retrigger", 1.0f }}},
        { "Risers & FX", "Impact Drop",    {{ "BaseFreq", 100.0f },  { "SweepDepth", 6.0f },  { "SweepTime", 700.0f },  { "Amount", 30.0f }, { "Curve", 0.5f }, { "Retrigger", 2.0f }}},
        { "Risers & FX", "Sci-Fi Door",    {{ "BaseFreq", 800.0f },  { "SweepDepth", -4.5f }, { "SweepTime", 90.0f },   { "Amount", 10.0f }, { "Pinch", 14.0f }}},
        { "Risers & FX", "Warp Engage",    {{ "BaseFreq", 350.0f },  { "SweepDepth", -5.5f }, { "SweepTime", 900.0f },  { "Amount", 26.0f }, { "Curve", 2.5f }, { "Retrigger", 2.0f }}},
        { "Risers & FX", "Falling Star",   {{ "BaseFreq", 5000.0f }, { "SweepDepth", 5.0f },  { "SweepTime", 1500.0f }, { "Amount", 22.0f }, { "Curve", 1.7f }, { "Retrigger", 2.0f }}},

        // --- Stereo & Space ------------------------------------------------
        { "Stereo & Space", "Wide Zap",              {{ "StereoMode", 1.0f }, { "Spread", 7.0f },  { "SweepDepth", 3.5f }, { "SweepTime", 100.0f }, { "Amount", 14.0f }}},
        { "Stereo & Space", "Ping Pong Phase",       {{ "StereoMode", 1.0f }, { "Spread", 12.0f }, { "TimeOffset", 15.0f }, { "SweepDepth", 3.0f }, { "SweepTime", 140.0f }}},
        { "Stereo & Space", "Side Sweep Only",       {{ "StereoMode", 2.0f }, { "SideDepth", 200.0f }, { "SweepDepth", 1.5f }, { "SweepTime", 150.0f }, { "Amount", 16.0f }}},
        { "Stereo & Space", "Center Punch Wide Tail",{{ "StereoMode", 2.0f }, { "SideDepth", 180.0f }, { "SweepDepth", 2.0f }, { "SweepTime", 200.0f }, { "Amount", 18.0f }}},
        { "Stereo & Space", "Rotor Twist",           {{ "StereoMode", 1.0f }, { "Spread", -9.0f }, { "TimeOffset", -12.0f }, { "SweepDepth", 3.0f }, { "Amount", 16.0f }}},
        { "Stereo & Space", "Offset Laser",          {{ "StereoMode", 1.0f }, { "Spread", 2.0f },  { "TimeOffset", 20.0f }, { "SweepDepth", 4.0f }, { "SweepTime", 120.0f }}},
        { "Stereo & Space", "Stereo Drift",          {{ "StereoMode", 1.0f }, { "Spread", 5.0f },  { "SweepDepth", 1.5f }, { "SweepTime", 400.0f }, { "Amount", 20.0f }, { "Curve", 1.6f }}},
        { "Stereo & Space", "MS Air",                {{ "StereoMode", 2.0f }, { "SideDepth", 150.0f }, { "BaseFreq", 3000.0f }, { "SweepDepth", 2.0f }, { "SCHighPass", 1000.0f }, { "DryWet", 70.0f }}},

        // --- Utility & Static ----------------------------------------------
        { "Utility & Static", "Static Spread",     {{ "SweepDepth", 0.0f }, { "Amount", 20.0f }, { "Pinch", 5.0f }}},
        { "Utility & Static", "Static Low Align",  {{ "SweepDepth", 0.0f }, { "BaseFreq", 120.0f }, { "Amount", 8.0f },  { "Pinch", 2.0f }}},
        { "Utility & Static", "Static Hi Shine",   {{ "SweepDepth", 0.0f }, { "BaseFreq", 5000.0f }, { "Amount", 12.0f }, { "Pinch", 6.0f }}},
        { "Utility & Static", "Gentle Motion",     {{ "SweepDepth", 0.8f }, { "SweepTime", 250.0f }, { "Amount", 10.0f }, { "DryWet", 60.0f }}},
        { "Utility & Static", "Subtle Glue",       {{ "SweepDepth", 0.5f }, { "SweepTime", 150.0f }, { "Amount", 6.0f },  { "DryWet", 40.0f }, { "Threshold", -30.0f }}},
        { "Utility & Static", "Phase Wash",        {{ "SweepDepth", 0.0f }, { "Amount", 32.0f }, { "Pinch", 8.0f }, { "DryWet", 85.0f }}},
        { "Utility & Static", "Vinyl Wow",         {{ "SweepDepth", 0.3f }, { "SweepTime", 800.0f }, { "Amount", 14.0f }, { "Curve", 1.2f }, { "Threshold", -35.0f }, { "Sensitivity", 4.0f }}},
        { "Utility & Static", "Transient Tickle",  {{ "SweepDepth", 1.0f }, { "SweepTime", 40.0f },  { "Amount", 6.0f },  { "DryWet", 50.0f }}},
        { "Utility & Static", "Mono Safe Sweep",   {{ "SweepDepth", 3.0f }, { "SweepTime", 120.0f }, { "Amount", 12.0f }}},
        { "Utility & Static", "Init",              {}},
    };

    return presets;
}

//==============================================================================

void PresetManager::refresh()
{
    entries.clear();

    const auto root = getUserPresetFolder();
    if (! root.isDirectory())
        return;

    // Subfolder terlebih dahulu sebagai kategori, lalu berkas di akar sebagai
    // preset buatan pengguna.
    auto dirs = root.findChildFiles (juce::File::findDirectories, false);
    dirs.sort();

    for (const auto& dir : dirs)
    {
        auto files = dir.findChildFiles (juce::File::findFiles, false, "*.lzp");
        files.sort();
        for (const auto& f : files)
            entries.push_back ({ dir.getFileName(), f.getFileNameWithoutExtension(), f });
    }

    auto rootFiles = root.findChildFiles (juce::File::findFiles, false, "*.lzp");
    rootFiles.sort();
    for (const auto& f : rootFiles)
        entries.push_back ({ "User", f.getFileNameWithoutExtension(), f });

    // Pertahankan sorotan pada preset yang sedang aktif setelah pemindaian
    currentIndex = -1;
    const auto name = getCurrentName();
    for (int i = 0; i < (int) entries.size(); ++i)
        if (entries[(size_t) i].name == name)
        {
            currentIndex = i;
            break;
        }
}

void PresetManager::load (int index)
{
    if (index < 0 || index >= (int) entries.size())
        return;

    loadUserPreset (entries[(size_t) index].file);
    currentIndex = index;
}

void PresetManager::step (int delta)
{
    if (entries.empty())
        refresh();
    if (entries.empty())
        return;

    const int n = (int) entries.size();
    load (((currentIndex + delta) % n + n) % n);
}

void PresetManager::saveUserPreset (const juce::File& file)
{
    setName (file.getFileNameWithoutExtension());
    if (auto xml = apvts.copyState().createXml())
        xml->writeTo (file);
    refresh();
}

void PresetManager::loadUserPreset (const juce::File& file)
{
    if (auto xml = juce::parseXML (file))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            setName (file.getFileNameWithoutExtension());
        }
    }
}

void PresetManager::exportFactoryPresets()
{
    const auto root = getUserPresetFolder();
    if (! root.createDirectory())
        return;

    // Hanya menyemai bila folder benar-benar belum berisi preset. Tanpa syarat
    // ini, preset yang sengaja dihapus pengguna akan muncul kembali setiap kali
    // plugin dibuka — dan folder tidak akan benar-benar menjadi sumber kebenaran.
    if (! root.findChildFiles (juce::File::findFiles, true, "*.lzp").isEmpty())
        return;

    // Kerangka diambil dari state APVTS yang hidup, bukan disusun manual, agar
    // nama simpul dan properti selalu mengikuti format JUCE yang sebenarnya.
    const auto templateTree = apvts.copyState();
    if (templateTree.getNumChildren() == 0)
        return;

    for (const auto& fp : getFactoryPresets())
    {
        auto dir = root.getChildFile (juce::File::createLegalFileName (fp.category));
        auto file = dir.getChildFile (juce::File::createLegalFileName (fp.name) + ".lzp");
        if (file.existsAsFile())
            continue;                       // jangan timpa berkas yang sudah ada

        dir.createDirectory();

        auto tree = templateTree.createCopy();
        tree.setProperty ("presetName", fp.name, nullptr);

        for (int i = 0; i < tree.getNumChildren(); ++i)
        {
            auto child = tree.getChild (i);
            if (! child.hasProperty ("id"))
                continue;

            const auto id = child.getProperty ("id").toString();
            auto* rp = apvts.getParameter (id);
            if (rp == nullptr)
                continue;

            // Mulai dari nilai bawaan, lalu terapkan penyimpangan preset
            float value = rp->convertFrom0to1 (rp->getDefaultValue());
            for (const auto& ov : fp.overrides)
                if (id == ov.first)
                    value = ov.second;

            child.setProperty ("value", value, nullptr);
        }

        if (auto xml = tree.createXml())
            xml->writeTo (file);
    }
}

void PresetManager::toggleAB()
{
    auto current = apvts.copyState().createXml();

    if (! onB)
    {
        slotA = std::move (current);
        if (slotB != nullptr)
            apvts.replaceState (juce::ValueTree::fromXml (*slotB));
        onB = true;
    }
    else
    {
        slotB = std::move (current);
        if (slotA != nullptr)
            apvts.replaceState (juce::ValueTree::fromXml (*slotA));
        onB = false;
    }
}

juce::File PresetManager::getUserPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
               .getChildFile ("Lazerator").getChildFile ("Presset");
}
