#include "CreditsView.h"
#include "LazerLook.h"

namespace
{
    constexpr float kCardW = 560.0f;
    constexpr float kCardH = 356.0f;

    // Teks berjarak huruf lebar, seperti pada kop kartu kredit.
    void drawTracked (juce::Graphics& g, const juce::String& text, juce::Rectangle<int> area,
                      float fontHeight, float tracking, juce::Justification just)
    {
        juce::String spaced;
        for (int i = 0; i < text.length(); ++i)
        {
            spaced << text[i];
            if (i < text.length() - 1)
                spaced << ' ';
        }
        auto font = lazer::uiFont (fontHeight, true);
        font.setExtraKerningFactor (tracking);
        g.setFont (font);
        g.drawText (spaced, area, just);
    }
}

CreditsView::CreditsView()
{
    logo = juce::ImageCache::getFromMemory (BinaryData::AnakBaek_DSP_png,
                                            BinaryData::AnakBaek_DSP_pngSize);
    setWantsKeyboardFocus (true);
}

void CreditsView::visibilityChanged()
{
    if (isVisible())
        grabKeyboardFocus();
}

juce::Rectangle<float> CreditsView::cardBounds() const
{
    return juce::Rectangle<float> (kCardW, kCardH).withCentre (getLocalBounds().toFloat().getCentre());
}

void CreditsView::paint (juce::Graphics& g)
{
    // Peredup latar, memakai tinta palet agar menyatu dengan tema terang
    g.fillAll (lazer::ink.withAlpha (0.55f));

    const auto card = cardBounds();

    juce::Path cardPath;
    cardPath.addRoundedRectangle (card, 14.0f);
    juce::DropShadow (juce::Colour (0x50000000), 26, { 0, 8 }).drawForPath (g, cardPath);
    g.setColour (lazer::panel);
    g.fillPath (cardPath);
    g.setColour (lazer::ink.withAlpha (0.20f));
    g.strokePath (cardPath, juce::PathStrokeType (1.2f));

    const int left  = (int) card.getX() + 42;
    const int right = (int) card.getRight() - 42;
    const int width = right - left;

    // Nama produk, aksen pink, berjarak huruf lebar
    g.setColour (lazer::pink);
    drawTracked (g, "LAZERATOR", { left, (int) card.getY() + 26, width, 14 },
                 11.0f, 0.34f, juce::Justification::centredLeft);

    // Nama perusahaan
    g.setColour (lazer::ink);
    g.setFont (lazer::uiFont (32.0f, true));
    g.drawText ("AnakBaek DSP", left, (int) card.getY() + 48, width, 40,
                juce::Justification::centredLeft);

    // Baris dukungan
    {
        const int y = (int) card.getY() + 92;
        auto italic = lazer::uiFont (17.0f);
        italic.setItalic (true);
        g.setFont (italic);
        g.setColour (lazer::inkSoft);
        const juce::String prefix = "Support by";
        g.drawText (prefix, left, y, width, 24, juce::Justification::centredLeft);

        const int offset = juce::GlyphArrangement::getStringWidthInt (italic, prefix) + 12;
        g.setColour (lazer::ink);
        g.setFont (lazer::uiFont (17.0f, true));
        g.drawText ("Creation Vault", left + offset, y, width - offset, 24,
                    juce::Justification::centredLeft);
    }

    // Garis pemisah
    g.setColour (lazer::ink.withAlpha (0.15f));
    g.fillRect (left, (int) card.getY() + 130, width, 1);

    // Judul kredit
    g.setColour (lazer::inkSoft);
    drawTracked (g, "CREDITS", { left, (int) card.getY() + 146, width, 14 },
                 10.5f, 0.34f, juce::Justification::centred);

    // Baris kredit: nama beraksen di kiri, peran di kolom tetap
    const int nameX = left + 24;
    const int roleX = left + 172;
    struct Row { const char* who; const char* role; };
    const Row rows[] = {
        { "anak baek", "DSP, Program, and Design" },
        { "WOY",       "Design and Marketing" }
    };

    for (int i = 0; i < 2; ++i)
    {
        const int y = (int) card.getY() + 186 + i * 34;
        g.setColour (lazer::pink);
        g.setFont (lazer::uiFont (14.0f, true));
        g.drawText (rows[i].who, nameX, y, roleX - nameX - 10, 20, juce::Justification::centredLeft);

        g.setColour (lazer::ink);
        g.drawText (rows[i].role, roleX, y, right - roleX, 20, juce::Justification::centredLeft);
    }

    // Versi
    g.setColour (lazer::inkSoft);
    g.setFont (lazer::uiFont (11.0f, true));
    g.drawText ("v" + juce::String (JucePlugin_VersionString),
                left, (int) card.getBottom() - 34, 120, 16, juce::Justification::centredLeft);

    // Logo pojok kanan bawah. Berkasnya putih solid, jadi diwarnai ulang lewat
    // kanal alpha agar terbaca di atas kartu terang.
    if (logo.isValid())
    {
        const float h = 68.0f;
        const float w = h * (float) logo.getWidth() / (float) juce::jmax (1, logo.getHeight());
        const auto dest = juce::Rectangle<float> (card.getRight() - 46.0f - w,
                                                  card.getBottom() - 30.0f - h, w, h);

        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (logo, juce::AffineTransform::scale (dest.getWidth()  / (float) logo.getWidth(),
                                                                dest.getHeight() / (float) logo.getHeight())
                                      .translated (dest.getX(), dest.getY()));
        g.fillAll (lazer::ink.withAlpha (0.85f));
    }
}

void CreditsView::mouseUp (const juce::MouseEvent& e)
{
    if (! cardBounds().contains (e.position))
        setVisible (false);
}

bool CreditsView::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        setVisible (false);
        return true;
    }
    return false;
}
