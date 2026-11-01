// Krang-texturer, genererade i kod (fas 2, första utkast).
// Alla texturer skapas en gång med fasta frön, i dubbel upplösning (2x) så att de håller vid 160 % skala,
// och ritas som upprepade mönster. Philip kan ersätta valfri textur med en genererad bild:
// lägg PNG-filen i Resources/ och returnera den från motsvarande funktion nedan – resten av koden ändras inte.
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
#include <vector>
#include <map>

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

// --- Åldrade ytor (fas 2, från stilprovet 1 nov) ---------------------------------------------
// I stället för en upprepad textur ritas varje plåt unikt för sin storlek, med:
//   ljus:      en höjdkarta (bucklor, repor) som belyses snett uppifrån vänster – ytan får relief
//   slitage:   kanter och hörn blanknötta, smuts i skarven innanför kanten (skuggad vrå)
//   repor:     enskilda repor med ljus och mörk kant, i borstningens riktning och tvärs över
//   rost:      rinner nedåt från skruvarna, i stället för att ligga slumpvis
// Påslaget som standard (beslut 1 nov). KrangSnapshot --stilprov-textur visar före och efter.

inline bool& useAgedLook() { static bool on = true; return on; } // false = de gamla, upprepade texturerna

namespace aged
{
    // Brusrutor som delas av alla plåtar: räknas fram en gång (512 × 512) och läses sedan med olika förskjutning
    // per plåt. Varje värde läses två gånger – en gång rakt och en gång med x och y bytta – så att rutornas
    // upprepning inte syns.
    struct Tiles
    {
        static constexpr int size = 512, mask = size - 1;
        std::vector<float> light, tone, wear, grime, pits, hue, width;

        static const Tiles& get()
        {
            static const Tiles t;
            return t;
        }

        static float at (const std::vector<float>& t, int x, int y) { return t[(size_t) ((y & mask) * size + (x & mask))]; }

    private:
        Tiles()
        {
            using namespace detail;
            const size_t n = (size_t) (size * size);
            std::vector<float> height (n);
            light.resize (n); tone.resize (n); wear.resize (n); grime.resize (n); pits.resize (n); hue.resize (n); width.resize (n);
            for (int y = 0; y < size; ++y)
                for (int x = 0; x < size; ++x)
                {
                    const size_t i = (size_t) (y * size + x);
                    // Höjd: bucklor plus fin borstning (sträckt i x).
                    height[i] = fbm (x, y, size, 901, 4, 3) * 1.2f
                              + smoothNoise ((float) x / 64.0f, (float) y, size / 64, 902, size) * 0.10f
                              + smoothNoise ((float) x / 16.0f, (float) y * 0.5f, size / 16, 903, size / 2) * 0.05f;
                    tone[i]  = fbm (x, y, size, 904, 3, 2);
                    wear[i]  = fbm (x, y, size, 905, 3, 8);
                    grime[i] = fbm (x, y, size, 906, 4, 6);
                    pits[i]  = fbm (x, y, size, 907, 5, 6);
                    hue[i]   = fbm (x, y, size, 908, 3, 10);
                    width[i] = fbm (x, y, size, 909, 2, 8);
                }
            // Ljuset: lutningen i höjdkartan mot ett ljus snett uppifrån vänster.
            for (int y = 0; y < size; ++y)
                for (int x = 0; x < size; ++x)
                {
                    const float dx = height[(size_t) (y * size + ((x + 1) & mask))] - height[(size_t) (y * size + ((x - 1) & mask))];
                    const float dy = height[(size_t) (((y + 1) & mask) * size + x)] - height[(size_t) (((y - 1) & mask) * size + x)];
                    light[(size_t) (y * size + x)] = juce::jlimit (-1.0f, 1.0f, (-dx - dy) * 9.0f);
                }
        }
    };

    // Ritar en unik, åldrad plåt i dubbel upplösning för en yta på w × h designpixlar.
    inline juce::Image generatePlate (int w, int h, int seed, float baseLevel, juce::Colour tint, float rustAmount)
    {
        using namespace detail;
        const auto& t = Tiles::get();
        const int W = juce::jmax (4, w * 2), H = juce::jmax (4, h * 2);
        juce::Random rnd (seed * 7919 + 13);
        const int ox1 = rnd.nextInt (512), oy1 = rnd.nextInt (512), ox2 = rnd.nextInt (512), oy2 = rnd.nextInt (512);
        auto mix = [&] (const std::vector<float>& tile, int x, int y)
        {
            // Två läsningar jämnar ut bruset; sträck tillbaka det kring medelvärdet så att kontrasten blir som förut.
            const float v = 0.62f * Tiles::at (tile, x + ox1, y + oy1) + 0.38f * Tiles::at (tile, y + oy2, x + ox2);
            return 0.47f + (v - 0.47f) * 1.35f;
        };
        const juce::Point<float> screws[4] = { { 12.0f, 12.0f }, { (float) W - 12.0f, 12.0f },
                                               { 12.0f, (float) H - 12.0f }, { (float) W - 12.0f, (float) H - 12.0f } };
        const float tr = tint.getFloatRed() * 1.15f, tg = tint.getFloatGreen() * 1.15f, tb = tint.getFloatBlue() * 1.15f;
        const juce::Colour rustDark (0xff6e3214), rustLight (0xffa65a24);

        juce::Image img (juce::Image::ARGB, W, H, false);
        {
            juce::Image::BitmapData data (img, juce::Image::BitmapData::writeOnly);
            for (int y = 0; y < H; ++y)
                for (int x = 0; x < W; ++x)
                {
                    // Ljuset läses utan att byta axlar: borstningen ska gå åt samma håll överallt.
                    const float light = 0.62f * Tiles::at (t.light, x + ox1, y + oy1) + 0.38f * Tiles::at (t.light, x + ox2, y + oy2);
                    const float edge = (float) juce::jmin (juce::jmin (x, W - 1 - x), juce::jmin (y, H - 1 - y));
                    const float worn = juce::jlimit (0.0f, 1.0f, 1.0f - edge / 10.0f) * (0.5f + 0.5f * mix (t.wear, x, y));
                    const float grime = juce::jlimit (0.0f, 1.0f, 1.0f - std::abs (edge - 22.0f) / 18.0f) * mix (t.grime, x, y);
                    const float falloff = 1.0f - 0.25f * ((float) x / (float) W + (float) y / (float) H);
                    const float grain = hash (x, y, seed + 8);
                    const float v = baseLevel * falloff * (0.8f + 0.4f * mix (t.tone, x, y)) + 22.0f * light + 5.0f * (grain - 0.5f)
                                  + 26.0f * worn - 14.0f * grime;
                    auto c = juce::Colour (clamp8 (v * tr), clamp8 (v * tg), clamp8 (v * tb));

                    // Rost: rinner nedåt från skruvarna, och samlas i gropar.
                    float rust = 0.0f;
                    for (auto& sc : screws)
                    {
                        const float below = (float) y - sc.y;
                        const float ddx = std::abs ((float) x - sc.x);
                        if (ddx < 24.0f && below > -16.0f)
                        {
                            const float width = 4.0f + 6.0f * mix (t.width, x, y) + juce::jmax (0.0f, below) * 0.04f;
                            const float streak = juce::jlimit (0.0f, 1.0f, 1.3f - ddx / width) * std::exp (-juce::jmax (0.0f, below) / ((float) H * 0.3f));
                            const float dist = std::sqrt (ddx * ddx + below * below);
                            rust = juce::jmax (rust, juce::jmax (streak, juce::jlimit (0.0f, 1.0f, 1.0f - dist / 16.0f)));
                        }
                    }
                    const float pits = juce::jlimit (0.0f, 1.0f, (mix (t.pits, x, y) - 0.6f) * 4.0f);
                    rust = juce::jlimit (0.0f, 1.0f, (rust * 0.9f + pits * 0.5f) * rustAmount);
                    if (rust > 0.0f)
                    {
                        const auto rc = rustDark.interpolatedWith (rustLight, mix (t.hue, x, y));
                        c = c.interpolatedWith (rc.withMultipliedBrightness (0.8f + 0.2f * (light + 1.0f)), juce::jmin (1.0f, rust * 1.1f));
                    }
                    data.setPixelColour (x, y, c);
                }
        }

        // Repor: tunna spår med mörk botten och ljus kant, de flesta längs borstningen, några tvärs över.
        juce::Graphics g (img);
        const int scratches = 6 + (W * H) / 30000;
        for (int k = 0; k < scratches; ++k)
        {
            float x = rnd.nextFloat() * (float) W, y = rnd.nextFloat() * (float) H;
            const bool across = rnd.nextFloat() < 0.3f;
            float angle = across ? rnd.nextFloat() * 3.14159f : (rnd.nextFloat() - 0.5f) * 0.25f;
            const int len = 40 + rnd.nextInt (across ? 120 : 360);
            const float depth = 0.25f + 0.5f * rnd.nextFloat();
            juce::Path p;
            p.startNewSubPath (x, y);
            for (int i = 0; i < len; i += 4)
            {
                angle += (rnd.nextFloat() - 0.5f) * 0.12f;
                x += 4.0f * std::cos (angle); y += 4.0f * std::sin (angle);
                p.lineTo (x, y);
            }
            g.setColour (juce::Colours::black.withAlpha (0.35f * depth));
            g.strokePath (p, juce::PathStrokeType (1.0f));
            g.setColour (juce::Colours::white.withAlpha (0.10f * depth));
            g.strokePath (p, juce::PathStrokeType (0.8f), juce::AffineTransform::translation (0.8f, 0.8f));
        }
        return img;
    }

    // Plåtarna sparas när de väl ritats: att byta kammare eller öppna fönstret igen kostar då ingenting.
    inline juce::Image makePlate (int w, int h, int seed, float baseLevel, juce::Colour tint, float rustAmount)
    {
        static std::map<juce::String, juce::Image> cache;
        const auto key = juce::String (w) + "x" + juce::String (h) + ":" + juce::String (seed) + ":" + juce::String (baseLevel)
                       + ":" + tint.toString() + ":" + juce::String (rustAmount);
        if (auto it = cache.find (key); it != cache.end()) return it->second;
        if (cache.size() > 96) cache.clear();
        return cache[key] = generatePlate (w, h, seed, baseLevel, tint, rustAmount);
    }

    // Skruv med spår, fas och ljus, i stället för en platt nit.
    inline void drawScrew (juce::Graphics& g, juce::Point<float> c, float r, float angle)
    {
        juce::ColourGradient body (juce::Colour (0xffd9c79a), c.x - r * 0.6f, c.y - r * 0.6f, juce::Colour (0xff4a3a20), c.x + r, c.y + r, true);
        g.setGradientFill (body);
        g.fillEllipse (juce::Rectangle<float> (r * 2, r * 2).withCentre (c));
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawEllipse (juce::Rectangle<float> (r * 2, r * 2).withCentre (c), 0.6f);
        const float sx = std::cos (angle) * r * 0.8f, sy = std::sin (angle) * r * 0.8f;
        g.setColour (juce::Colours::black.withAlpha (0.75f));
        g.drawLine (c.x - sx, c.y - sy, c.x + sx, c.y + sy, juce::jmax (0.8f, r * 0.35f));
        g.setColour (juce::Colours::white.withAlpha (0.25f));
        g.drawLine (c.x - sx + 0.5f, c.y - sy + 0.6f, c.x + sx + 0.5f, c.y + sy + 0.6f, 0.5f);
    }

    inline int seedFor (juce::Rectangle<float> r) { return (int) (r.getX() * 13.0f + r.getY() * 31.0f + r.getWidth() * 7.0f) & 0xffff; }
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
    if (useAgedLook())
    {
        const auto img = aged::makePlate ((int) r.getWidth(), (int) r.getHeight(), aged::seedFor (r) + 500, 30.0f,
                                          juce::Colour (0xff8a8478), rustAmount * 0.6f);
        g.drawImage (img, r);
        juce::ColourGradient vignette (juce::Colours::transparentBlack, r.getCentreX(), r.getCentreY(),
                                       juce::Colours::black.withAlpha (0.4f), r.getX(), r.getY(), true);
        g.setGradientFill (vignette);
        g.fillRect (r);
        return;
    }
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
