// Krang-texturer, genererade i kod (fas 2, första utkast).
// Alla texturer skapas en gång med fasta frön, i dubbel upplösning (2x) så att de håller vid 160 % skala,
// och ritas som upprepade mönster. Philip kan ersätta valfri textur med en genererad bild:
// lägg PNG-filen i Resources/ och returnera den från motsvarande funktion nedan – resten av koden ändras inte.
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>

namespace krang::ui::textures
{

namespace detail
{
    inline juce::uint8 clamp8 (float v) { return (juce::uint8) juce::jlimit (0, 255, juce::roundToInt (v)); }

    // Brus med jämn fördelning, deterministiskt.
    inline float hash (int x, int y, int seed)
    {
        auto h = (juce::uint32) x * 374761393u + (juce::uint32) y * 668265263u + (juce::uint32) seed * 2147483647u;
        h = (h ^ (h >> 13)) * 1274126177u;
        return (float) ((h ^ (h >> 16)) & 0xffff) / 65535.0f;
    }

    // Mjukt brus (värdebrus, bilinjärt), upprepas med perioderna periodX och periodY (i celler).
    inline float smoothNoise (float x, float y, int period, int seed, int periodY = -1)
    {
        if (periodY < 0) periodY = period;
        const int x0 = (int) std::floor (x), y0 = (int) std::floor (y);
        const float fx = x - (float) x0, fy = y - (float) y0;
        auto at = [&] (int xi, int yi) { return hash (((xi % period) + period) % period, ((yi % periodY) + periodY) % periodY, seed); };
        const float a = at (x0, y0), b = at (x0 + 1, y0), c = at (x0, y0 + 1), d = at (x0 + 1, y0 + 1);
        const float sx = fx * fx * (3 - 2 * fx), sy = fy * fy * (3 - 2 * fy);
        return a + (b - a) * sx + (c - a) * sy + (a - b - c + d) * sx * sy;
    }

    // Fraktalt brus: flera oktaver smoothNoise, sömlöst för en ruta med sidan size.
    inline float fbm (int x, int y, int size, int seed, int octaves = 4, int baseCells = 4)
    {
        float sum = 0.0f, amp = 0.5f;
        int cells = baseCells;
        for (int o = 0; o < octaves; ++o)
        {
            sum += amp * smoothNoise ((float) x * (float) cells / (float) size, (float) y * (float) cells / (float) size, cells, seed + o);
            amp *= 0.5f;
            cells *= 2;
        }
        return sum;
    }

    template <typename Fn>
    juce::Image generate (int w, int h, Fn&& pixel)
    {
        juce::Image img (juce::Image::ARGB, w, h, false);
        juce::Image::BitmapData data (img, juce::Image::BitmapData::writeOnly);
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                data.setPixelColour (x, y, pixel (x, y));
        return img;
    }
}

constexpr int tileSize = 512;      // pixlar i texturen
constexpr float density = 2.0f;   // texturpixlar per designpixel

// Mörk borstad chassimetall med fina repor och fläckar. Sömlös i båda riktningarna.
inline const juce::Image& chassisMetal()
{
    static const juce::Image img = detail::generate (tileSize, tileSize, [] (int x, int y)
    {
        using namespace detail;
        // Borstning: brus som sträcks ut horisontellt.
        // En brusrad per pixelrad, utsträckt 64 resp. 16 pixlar i x: ger borstade streck.
        const float brush = smoothNoise ((float) x / 64.0f, (float) y, tileSize / 64, 11, tileSize) * 0.6f
                          + smoothNoise ((float) x / 16.0f, (float) y * 0.5f, tileSize / 16, 12, tileSize / 2) * 0.4f;
        const float cloud = fbm (x, y, tileSize, 20, 4, 2);    // ojämn ton över ytan
        const float grain = hash (x, y, 31);
        float v = 28.0f + 24.0f * (brush - 0.5f) + 16.0f * (cloud - 0.5f) + 6.0f * (grain - 0.5f) + 8.0f;
        // Glesa ljusa repor.
        if (hash (x / 4, y, 41) > 0.998f) v += 26.0f;
        return juce::Colour (clamp8 (v), clamp8 (v * 0.97f), clamp8 (v * 0.93f));
    });
    return img;
}

// Modulplåt: lite mörkare, finare struktur och nötta partier.
inline const juce::Image& plate()
{
    static const juce::Image img = detail::generate (tileSize, tileSize, [] (int x, int y)
    {
        using namespace detail;
        const float cloud = fbm (x, y, tileSize, 50, 5, 4);
        const float grain = hash (x, y, 51);
        float v = 34.0f + 16.0f * (cloud - 0.5f) + 6.0f * (grain - 0.5f);
        const float wear = fbm (x, y, tileSize, 60, 3, 2);
        if (wear > 0.62f) v += (wear - 0.62f) * 60.0f;          // blanknött där händerna tar
        return juce::Colour (clamp8 (v), clamp8 (v * 0.96f), clamp8 (v * 0.9f));
    });
    return img;
}

// Rost och smuts: genomskinlig textur som läggs ovanpå metall.
inline const juce::Image& rust()
{
    static const juce::Image img = detail::generate (tileSize, tileSize, [] (int x, int y)
    {
        using namespace detail;
        const float n = fbm (x, y, tileSize, 70, 5, 3);
        const float speck = hash (x, y, 71);
        float a = juce::jlimit (0.0f, 1.0f, (n - 0.53f) * 3.5f);
        if (speck > 0.996f) a = juce::jmax (a, 0.5f);
        const float hue = fbm (x, y, tileSize, 72, 3, 6);
        const auto c = juce::Colour (0xff7a3a1a).interpolatedWith (juce::Colour (0xffa8602a), hue);
        return c.withAlpha (a * 0.7f);
    });
    return img;
}

// Åldrat papper för VU-mätaren: krämvitt med fläckar och en kafferingsfläck.
inline const juce::Image& meterPaper()
{
    static const juce::Image img = detail::generate (tileSize, tileSize, [] (int x, int y)
    {
        using namespace detail;
        const float n = fbm (x, y, tileSize, 80, 5, 4);
        const float fibre = hash (x, y / 2, 81);
        auto c = juce::Colour (0xfff1e6c8).darker (0.18f * (n - 0.3f) + 0.03f * fibre);
        const float dx = (float) x - 360.0f, dy = (float) y - 150.0f;
        const float ring = std::abs (std::sqrt (dx * dx + dy * dy) - 70.0f);
        if (ring < 3.0f) c = c.interpolatedWith (juce::Colour (0xffb08a5a), 0.25f * (1.0f - ring / 3.0f));
        return c;
    });
    return img;
}

// Fyller r med en upprepad textur i rätt täthet, och med valfri opacitet.
inline void fill (juce::Graphics& g, juce::Rectangle<float> r, const juce::Image& tex, float alpha = 1.0f, float cornerRadius = 0.0f)
{
    g.saveState();
    juce::FillType ft (tex, juce::AffineTransform::scale (1.0f / density));
    ft.setOpacity (alpha);
    g.setFillType (ft);
    if (cornerRadius > 0.0f) g.fillRoundedRectangle (r, cornerRadius);
    else                     g.fillRect (r);
    g.restoreState();
}

// Chassi: metall, lätt vinjettering mot kanterna och lite rost.
inline void fillChassis (juce::Graphics& g, juce::Rectangle<float> r, float rustAmount = 0.35f)
{
    fill (g, r, chassisMetal());
    fill (g, r, rust(), rustAmount);
    juce::ColourGradient vignette (juce::Colours::transparentBlack, r.getCentreX(), r.getCentreY(),
                                   juce::Colours::black.withAlpha (0.35f), r.getX(), r.getY(), true);
    g.setGradientFill (vignette);
    g.fillRect (r);
}

// Cache för statiska bakgrunder (chassi, ramar, rör, graverad text): ritas en gång i 2x
// när storleken ändras, och sedan bara som en bild. Gör omritningar billiga trots texturerna.
class BackgroundCache
{
public:
    template <typename PaintFn>
    void draw (juce::Graphics& g, juce::Rectangle<int> bounds, PaintFn&& paintBackground)
    {
        if (image.isNull() || bounds.getWidth() != cachedW || bounds.getHeight() != cachedH)
        {
            cachedW = bounds.getWidth();
            cachedH = bounds.getHeight();
            image = juce::Image (juce::Image::ARGB, juce::jmax (1, cachedW * 2), juce::jmax (1, cachedH * 2), true);
            juce::Graphics ig (image);
            ig.addTransform (juce::AffineTransform::scale (2.0f));
            paintBackground (ig);
        }
        g.drawImage (image, bounds.toFloat());
    }
    void invalidate() { image = {}; }

private:
    juce::Image image;
    int cachedW = 0, cachedH = 0;
};

} // namespace krang::ui::textures
