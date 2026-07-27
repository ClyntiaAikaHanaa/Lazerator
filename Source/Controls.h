#pragma once
#include <JuceHeader.h>
#include "LazerLook.h"
#include <cmath>

namespace lazer
{
    // Bantuan nada: temperamen sama, A4 = 440 Hz.
    inline int frequencyToMidiNote (double hz)
    {
        return juce::roundToInt (69.0 + 12.0 * std::log2 (juce::jmax (1.0, hz) / 440.0));
    }

    inline double midiNoteToFrequency (int note)
    {
        return 440.0 * std::pow (2.0, (note - 69) / 12.0);
    }

    inline juce::String noteName (int note)
    {
        static const char* const names[] = { "C", "C#", "D", "D#", "E", "F",
                                             "F#", "G", "G#", "A", "A#", "B" };
        return juce::String (names[((note % 12) + 12) % 12]) + juce::String (note / 12 - 1);
    }
}

// Pil tersegmen untuk parameter pilihan (pengganti combo box) — satu klik,
// semua opsi terlihat, tidak ada menu tersembunyi.
class SegmentedControl : public juce::Component
{
public:
    explicit SegmentedControl (juce::RangedAudioParameter& param)
        : attachment (param, [this] (float v)
                      {
                          current = (int) std::lround (v);
                          repaint();
                      })
    {
        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (&param))
            items = choice->choices;
        attachment.sendInitialUpdate();
    }

    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat().reduced (1.0f);
        const float rad = r.getHeight() * 0.5f;

        g.setColour (lazer::panel);
        g.fillRoundedRectangle (r, rad);
        g.setColour (lazer::ink.withAlpha (0.30f));
        g.drawRoundedRectangle (r, rad, 1.2f);

        if (items.isEmpty())
            return;

        const float segW = r.getWidth() / (float) items.size();
        g.setFont (lazer::uiFont (10.5f));

        for (int i = 0; i < items.size(); ++i)
        {
            auto seg = juce::Rectangle<float> (r.getX() + (float) i * segW, r.getY(),
                                               segW, r.getHeight());
            if (i == current)
            {
                g.setColour (lazer::ink);
                g.fillRoundedRectangle (seg.reduced (2.0f), rad - 2.0f);
                g.setColour (lazer::panel);
            }
            else
                g.setColour (lazer::inkSoft);

            g.drawText (items[i], seg.toNearestInt(), juce::Justification::centred);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (items.isEmpty() || getWidth() <= 0)
            return;
        const int idx = juce::jlimit (0, items.size() - 1,
                                      (int) (e.position.x / ((float) getWidth() / (float) items.size())));
        attachment.setValueAsCompleteGesture ((float) idx);
    }

private:
    juce::ParameterAttachment attachment;
    juce::StringArray items;
    int current = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SegmentedControl)
};

// Tombol bulat berikon headphone untuk SC Listen — ikon lebih cepat dibaca
// daripada teks pada kontrol sekecil ini.
class HeadphoneToggle : public juce::ToggleButton
{
public:
    HeadphoneToggle() { setTooltip ("Listen: monitor sinyal deteksi"); }

    void paintButton (juce::Graphics& g, bool highlighted, bool) override
    {
        const auto r = getLocalBounds().toFloat().reduced (1.0f);
        const bool on = getToggleState();

        g.setColour (on ? lazer::ink : lazer::panel);
        g.fillEllipse (r);
        if (! on)
        {
            g.setColour (lazer::ink.withAlpha (highlighted ? 0.55f : 0.30f));
            g.drawEllipse (r.reduced (0.6f), 1.2f);
        }

        // Ikon: headband setengah lingkaran, earcup menggantung di kedua ujungnya
        const auto icon = r.reduced (r.getWidth() * 0.24f);
        const float rad  = icon.getWidth() * 0.33f;
        const float cupW = rad * 0.56f;
        const float cupH = rad * 0.95f;
        const float bandW = juce::jmax (1.1f, rad * 0.17f);
        const float cx = icon.getCentreX();
        const float cy = icon.getCentreY() - cupH * 0.24f;

        g.setColour (on ? lazer::panel : lazer::inkSoft);

        juce::Path band;
        band.addCentredArc (cx, cy, rad, rad, 0.0f,
                            -juce::MathConstants<float>::halfPi,
                             juce::MathConstants<float>::halfPi, true);
        g.strokePath (band, juce::PathStrokeType (bandW, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));

        const float cupY = cy - bandW * 0.5f;
        g.fillRoundedRectangle (cx - rad - cupW * 0.5f, cupY, cupW, cupH, cupW * 0.42f);
        g.fillRoundedRectangle (cx + rad - cupW * 0.5f, cupY, cupW, cupH, cupW * 0.42f);
    }
};

// Kenop yang nilainya bisa diketik: klik ganda memunculkan kotak teks kecil.
class ValueSlider : public juce::Slider
{
public:
    ValueSlider() = default;

    // Menahan nilai pada nada temperamen sama saat ditarik, agar frekuensinya
    // tidak fals. Tahan Shift untuk menggeser bebas, dan mengetik nilai lewat
    // klik ganda tetap menerima angka apa pun.
    void setSnapToNotes (bool shouldSnap) { snapToNotes = shouldSnap; }

    double snapValue (double attemptedValue, DragMode dragMode) override
    {
        if (! snapToNotes
            || dragMode == notDragging
            || juce::ModifierKeys::getCurrentModifiers().isShiftDown())
            return attemptedValue;

        int note = lazer::frequencyToMidiNote (attemptedValue);
        double snapped = lazer::midiNoteToFrequency (note);

        // Nada terdekat bisa jatuh di luar rentang kenop (20 Hz menempel ke
        // D#0 = 19,45 Hz). Geser ke nada berikutnya yang masih muat, supaya
        // ujung rentang tetap mendarat tepat di sebuah nada.
        while (snapped < getMinimum() && note < 140)
            snapped = lazer::midiNoteToFrequency (++note);
        while (snapped > getMaximum() && note > 0)
            snapped = lazer::midiNoteToFrequency (--note);

        return snapped;
    }

    void mouseDoubleClick (const juce::MouseEvent&) override
    {
        if (editor != nullptr)
            return;

        editor = std::make_unique<juce::TextEditor>();
        auto& e = *editor;
        addAndMakeVisible (e);

        e.setBounds (getLocalBounds().withSizeKeepingCentre (
            juce::jmin (getWidth() - 6, 64), 18));
        e.setJustification (juce::Justification::centred);
        e.setFont (lazer::uiFont (12.0f));
        e.setColour (juce::TextEditor::backgroundColourId, lazer::panel);
        e.setColour (juce::TextEditor::textColourId, lazer::ink);
        e.setColour (juce::TextEditor::outlineColourId, lazer::pink);
        e.setColour (juce::TextEditor::focusedOutlineColourId, lazer::pink);
        e.setColour (juce::TextEditor::highlightColourId, lazer::pink.withAlpha (0.30f));
        e.setColour (juce::TextEditor::highlightedTextColourId, lazer::ink);
        e.setInputRestrictions (12, "0123456789.,-+");
        e.setSelectAllWhenFocused (true);

        // Angka mentah, bukan teks berformat: "1.0kHz" akan terbaca sebagai 1.0
        // kalau dikirim balik ke parser.
        e.setText (juce::String (getValue(), 3).trimCharactersAtEnd ("0")
                                               .trimCharactersAtEnd ("."), false);
        e.grabKeyboardFocus();

        e.onReturnKey = [this] { closeEditor (true); };
        e.onEscapeKey = [this] { closeEditor (false); };
        e.onFocusLost = [this] { closeEditor (true); };
    }

private:
    void closeEditor (bool apply)
    {
        if (editor == nullptr)
            return;

        // Dipindahkan lebih dulu supaya panggilan balik yang menyusul
        // (mis. onFocusLost sesudah onReturnKey) langsung berhenti di sini.
        auto ed = std::move (editor);

        if (apply)
        {
            const auto text = ed->getText().trim();
            if (text.isNotEmpty())
                setValue (getValueFromText (text), juce::sendNotificationSync);
        }

        // Kita sedang berada di dalam panggilan balik editor: lepas sekarang,
        // hapus setelah pesan ini selesai.
        removeChildComponent (ed.get());
        auto* raw = ed.release();
        juce::MessageManager::callAsync ([raw] { delete raw; });
    }

    std::unique_ptr<juce::TextEditor> editor;
    bool snapToNotes = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ValueSlider)
};

// Area transparan yang dapat diklik (dipakai judul untuk membuka jendela kredit).
class ClickableArea : public juce::Component
{
public:
    std::function<void()> onClick;

    ClickableArea() { setMouseCursor (juce::MouseCursor::PointingHandCursor); }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (onClick != nullptr && getLocalBounds().contains (e.getPosition()))
            onClick();
    }
};
