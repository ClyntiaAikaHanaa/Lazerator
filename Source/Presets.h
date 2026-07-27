#pragma once
#include <JuceHeader.h>
#include <vector>
#include <utility>

// Preset pabrik (§3.1: minimal 60). Tabel ini hanya BENIH: ia dipakai sekali
// untuk menuliskan berkas .lzp ke folder pengguna bila berkasnya belum ada.
// Sumber kebenaran saat berjalan adalah isi folder, bukan tabel ini.
struct FactoryPreset
{
    const char* category;
    const char* name;
    std::vector<std::pair<const char*, float>> overrides;
};

const std::vector<FactoryPreset>& getFactoryPresets();

// Satu preset sebagaimana ditemukan di folder.
struct PresetEntry
{
    juce::String category, name;
    juce::File file;
};

// Pengelola preset: memindai Documents/Lazerator/Presset, memuat dan menyimpan
// berkas .lzp, serta A/B compare (§3.1). Semua dipanggil dari thread pesan.
class PresetManager
{
public:
    explicit PresetManager (juce::AudioProcessorValueTreeState& state) : apvts (state) {}

    // Menulis preset pabrik yang belum ada sebagai berkas .lzp, tersusun dalam
    // subfolder per kategori. Berkas yang sudah ada tidak pernah ditimpa.
    void exportFactoryPresets();

    // Memindai ulang folder. Subfolder menjadi kategori, berkas di akar
    // dikelompokkan sebagai "User".
    void refresh();

    const std::vector<PresetEntry>& getEntries() const { return entries; }
    int getCurrentIndex() const { return currentIndex; }

    void load (int index);
    void step (int delta);                       // maju/mundur melingkar

    juce::String getCurrentName() const
    {
        return apvts.state.getProperty ("presetName", "Init").toString();
    }

    void saveUserPreset (const juce::File& file);
    void loadUserPreset (const juce::File& file);

    void toggleAB();
    bool isSlotB() const { return onB; }

    static juce::File getUserPresetFolder();

private:
    void setName (const juce::String& name)
    {
        apvts.state.setProperty ("presetName", name, nullptr);
    }

    juce::AudioProcessorValueTreeState& apvts;
    std::vector<PresetEntry> entries;
    int currentIndex = -1;
    std::unique_ptr<juce::XmlElement> slotA, slotB;
    bool onB = false;
};
