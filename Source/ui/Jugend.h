// Jugend: KrangBox egna motiv (lurar, lutor, klockor, pumpar, tubor, kvarnen, bordun, spolar, speldosor).
// Grunden – palett, plakett, gloria, piskrapp, bårder – ligger i krang-libs (krang/stil/krang/Jugend.h).
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <krang/stil/krang/Jugend.h>

namespace krang::ui::jugend
{

// --- Lurstället: näverlur, bockhorn, skalmeja eller ångvissla -----------------------------

inline void lur (juce::Graphics& g, juce::Rectangle<float> area, int type, float rasp)
{
    using namespace juce;
    const auto c = Point<float> (area.getCentreX(), area.getY() + area.getHeight() * 0.34f);
    const auto a = plaque (g, area, c, juce::jmin (area.getWidth() * 0.4f, area.getHeight() * 0.26f));
    const float cx = a.getCentreX();
    Path body;
    if (type == 0) // näverlur: lång, lätt lutad, lindad i näver
    {
        const auto top = Point<float> (cx + a.getWidth() * 0.16f, a.getY() + 2.0f);
        const auto bot = Point<float> (cx - a.getWidth() * 0.12f, a.getBottom() - 2.0f);
        const auto d = bot - top;
        const float len = d.getDistanceFromOrigin();
        const Point<float> n (-d.y / len, d.x / len);
        body.startNewSubPath (top + n * 2.5f);
        body.lineTo (bot + n * 15.0f);
        body.quadraticTo (bot + d * 0.03f, bot - n * 15.0f);
        body.lineTo (top - n * 2.5f);
        body.closeSubPath();
        // Ljud ur luren: piskrapp ut ur klockan.
        whiplash (g, bot + n * 4.0f, bot + Point<float> (a.getWidth() * 0.4f, -a.getHeight() * 0.25f), 14.0f, 5.0f, pal::rose.withAlpha (0.8f));
        whiplash (g, bot - n * 4.0f, bot + Point<float> (a.getWidth() * 0.45f, -a.getHeight() * 0.05f), -10.0f, 4.0f, pal::teal.withAlpha (0.7f));
        fillInk (g, body, pal::birch);
        // Näverns porer och lindningarna.
        Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (body);
        Random rnd (7);
        for (int i = 0; i < 70; ++i)
        {
            const float t = rnd.nextFloat();
            const auto p = top + d * t + n * ((rnd.nextFloat() - 0.5f) * (5.0f + 26.0f * t));
            g.setColour (pal::ink.withAlpha (0.55f));
            g.fillRect (Rectangle<float> (2.0f + 4.0f * rnd.nextFloat(), 1.2f).withCentre (p));
        }
        for (int i = 1; i < 6; ++i)
        {
            const float t = (float) i / 6.0f;
            const auto p = top + d * t;
            const float w = 3.0f + 12.0f * t;
            g.setColour (pal::woodDark);
            g.drawLine (Line<float> (p + n * w, p - n * w), 3.0f);
            g.setColour (pal::rose);
            g.drawLine (Line<float> (p + n * w, p - n * w), 1.2f);
        }
    }
    else if (type == 1) // bockhorn: krökt horn med fingerhål och en läderrem
    {
        const float x0 = a.getX() + a.getWidth() * 0.18f, y0 = a.getY() + a.getHeight() * 0.12f;
        body.startNewSubPath (x0, y0);
        body.cubicTo (a.getRight(), a.getY() + a.getHeight() * 0.25f, a.getRight() - 4.0f, a.getBottom() - a.getHeight() * 0.2f, cx - 8.0f, a.getBottom() - 6.0f);
        body.lineTo (cx - 26.0f, a.getBottom() - 20.0f);
        body.cubicTo (a.getRight() - 30.0f, a.getBottom() - a.getHeight() * 0.3f, a.getRight() - 30.0f, a.getY() + a.getHeight() * 0.3f, x0 - 4.0f, y0 + 7.0f);
        body.closeSubPath();
        whiplash (g, { cx - 18.0f, a.getBottom() - 12.0f }, { a.getX() + 4.0f, a.getY() + a.getHeight() * 0.55f }, -12.0f, 5.0f, pal::rose.withAlpha (0.8f));
        fillInk (g, body, Colour (0xffe2d3a8));
        // Hornets ringar.
        g.setColour (pal::woodDark.withAlpha (0.6f));
        for (int i = 1; i < 9; ++i)
        {
            const float t = (float) i / 9.0f;
            const auto p = body.getPointAlongPath (body.getLength() * 0.5f * t);
            g.fillEllipse (Rectangle<float> (3.0f, 3.0f).withCentre (p.translated (-6.0f * t, 4.0f + 6.0f * t)));
        }
        // Läderremmen som en böljande slinga.
        Path strap;
        strap.startNewSubPath (x0 + 6.0f, y0 + 4.0f);
        strap.cubicTo (a.getX(), a.getY() + a.getHeight() * 0.6f, cx, a.getY() + a.getHeight() * 0.5f, cx - 20.0f, a.getBottom() - 18.0f);
        inkLine (g, strap, 3.0f, pal::woodDark);
    }
    else if (type == 2) // skalmeja: trärör med vulst, fingerhål och utsvängd klocka
    {
        const float w = 9.0f, bellW = 26.0f;
        const float yTop = a.getY() + 4.0f, yBell = a.getBottom() - a.getHeight() * 0.22f, yBot = a.getBottom() - 2.0f;
        body.startNewSubPath (cx - w * 0.6f, yTop + 12.0f);
        body.lineTo (cx + w * 0.6f, yTop + 12.0f);
        body.lineTo (cx + w, yBell);
        body.quadraticTo (cx + w + 4.0f, yBot - 6.0f, cx + bellW, yBot);
        body.lineTo (cx - bellW, yBot);
        body.quadraticTo (cx - w - 4.0f, yBot - 6.0f, cx - w, yBell);
        body.closeSubPath();
        whiplash (g, { cx + bellW - 4.0f, yBot - 4.0f }, { a.getRight() - 4.0f, a.getY() + a.getHeight() * 0.45f }, 12.0f, 5.0f, pal::rose.withAlpha (0.8f));
        whiplash (g, { cx - bellW + 4.0f, yBot - 4.0f }, { a.getX() + 4.0f, a.getY() + a.getHeight() * 0.5f }, -12.0f, 4.0f, pal::teal.withAlpha (0.7f));
        fillInk (g, body, pal::wood);
        Path reed;
        reed.addRoundedRectangle (cx - 2.5f, yTop, 5.0f, 13.0f, 2.0f);
        fillInk (g, reed, pal::goldLight, 1.0f);
        for (int i = 0; i < 7; ++i)
        {
            const float y = yTop + 24.0f + (float) i * (yBell - yTop - 34.0f) / 6.0f;
            g.setColour (pal::ink);
            g.fillEllipse (Rectangle<float> (4.5f, 4.5f).withCentre ({ cx, y }));
        }
        g.setColour (pal::goldLight);
        g.drawLine (cx - w * 0.7f, yTop + 20.0f, cx + w * 0.7f, yTop + 20.0f, 2.0f);
        g.drawLine (cx - w, yBell, cx + w, yBell, 2.0f);
    }
    else // ångvissla: mässingsklocka med kupol och munspringa, på ett rör med ventil – och ånga som Muchas hår
    {
        const float bw = jmin (a.getWidth() * 0.36f, 40.0f), top = a.getY() + a.getHeight() * 0.2f;
        const float bellBottom = top + a.getHeight() * 0.34f, mouthY = bellBottom + 6.0f;
        for (int i = 0; i < 4; ++i)
        {
            const float x = cx - bw * 0.6f + (float) i * bw * 0.4f;
            whiplash (g, { x, mouthY - 2.0f }, { x + (float) (i - 1.5f) * 22.0f, a.getY() + 2.0f }, (i % 2 == 0 ? 12.0f : -12.0f), 5.0f,
                      (i % 2 == 0 ? Colours::white : pal::sageLight).withAlpha (0.9f));
        }
        Path pipe;
        pipe.addRectangle (cx - 5.0f, mouthY + 8.0f, 10.0f, a.getBottom() - mouthY - 8.0f);
        fillInk (g, pipe, pal::iron);
        Path base;   // skålen under munspringan
        base.startNewSubPath (cx - bw * 0.5f, mouthY);
        base.quadraticTo (cx, mouthY + 18.0f, cx + bw * 0.5f, mouthY);
        base.closeSubPath();
        fillInk (g, base, pal::brass);
        Path bell;
        bell.startNewSubPath (cx - bw * 0.5f, bellBottom);
        bell.lineTo (cx - bw * 0.5f, top + bw * 0.3f);
        bell.quadraticTo (cx - bw * 0.5f, top, cx, top);
        bell.quadraticTo (cx + bw * 0.5f, top, cx + bw * 0.5f, top + bw * 0.3f);
        bell.lineTo (cx + bw * 0.5f, bellBottom);
        bell.closeSubPath();
        fillInk (g, bell, pal::brass);
        Path finial;
        finial.addEllipse (cx - 4.0f, top - 9.0f, 8.0f, 10.0f);
        fillInk (g, finial, pal::goldLight, 1.0f);
        g.setColour (pal::goldDark);   // ringar runt klockan
        for (int i = 1; i < 4; ++i)
            g.drawLine (cx - bw * 0.5f, top + (bellBottom - top) * (0.25f + 0.22f * (float) i), cx + bw * 0.5f, top + (bellBottom - top) * (0.25f + 0.22f * (float) i), 1.0f);
        g.setColour (pal::goldLight.withAlpha (0.8f));
        g.drawLine (cx - bw * 0.3f, top + bw * 0.25f, cx - bw * 0.3f, bellBottom - 4.0f, 2.0f);
        g.setColour (pal::ink);  // munspringan
        g.fillRect (Rectangle<float> (bw * 0.9f, 4.0f).withCentre ({ cx, mouthY }));
        Path lever;
        lever.startNewSubPath (cx + 4.0f, mouthY + 34.0f);
        lever.lineTo (cx + bw * 1.0f, mouthY + 14.0f);
        inkLine (g, lever, 3.0f, pal::ink);
        g.setColour (pal::rose);
        g.fillEllipse (Rectangle<float> (9.0f, 9.0f).withCentre ({ cx + bw * 1.0f, mouthY + 14.0f }));
        body = bell;
    }
    rustSpots (g, body, rasp, 11 + type);
}

// --- Lutverket: kantele, luta, cittra eller rostharpa, med strängarna och var man knäpper -----

inline void lutInstrument (juce::Graphics& g, juce::Rectangle<float> area, int type, float rust, float pluck)
{
    using namespace juce;
    const auto c = Point<float> (area.getCentreX(), area.getY() + area.getHeight() * 0.36f);
    const auto a = plaque (g, area, c, jmin (area.getWidth() * 0.42f, area.getHeight() * 0.28f));
    const float cx = a.getCentreX(), top = a.getY() + 4.0f, bottom = a.getBottom() - 4.0f, h = bottom - top;
    Path shape, soundhole;
    int strings = 5;
    float sTop = top + 10.0f, sBottom = bottom - 10.0f, sWidthTop = 20.0f, sWidthBottom = 50.0f;
    Colour wood = pal::wood;
    if (type == 0) // kantele: vinge i den smala änden, strängarna solfjäderformade
    {
        const float wt = jmin (a.getWidth() * 0.22f, 24.0f), wb = jmin (a.getWidth() * 0.46f, 52.0f);
        shape.startNewSubPath (cx - wt, top + 10.0f);
        shape.quadraticTo (cx - wt - 4.0f, top - 2.0f, cx + wt + 10.0f, top);       // vingen
        shape.lineTo (cx + wb, bottom);
        shape.quadraticTo (cx, bottom + 6.0f, cx - wb, bottom);
        shape.closeSubPath();
        soundhole.addStar ({ cx, top + h * 0.62f }, 6, 4.0f, 9.0f);
        strings = 5; sWidthTop = wt * 1.4f; sWidthBottom = wb * 1.4f; sTop = top + 12.0f; sBottom = bottom - 8.0f;
    }
    else if (type == 1) // luta: päronkropp, hals och bakåtböjd skruvlåda
    {
        const float bw = jmin (a.getWidth() * 0.46f, 52.0f);
        const float bodyTop = top + h * 0.40f;
        shape.startNewSubPath (cx - 6.0f, top + 14.0f);
        shape.lineTo (cx + 6.0f, top + 14.0f);
        shape.lineTo (cx + 7.0f, bodyTop);
        shape.cubicTo (cx + bw, bodyTop + h * 0.05f, cx + bw * 1.05f, bottom - h * 0.12f, cx, bottom);
        shape.cubicTo (cx - bw * 1.05f, bottom - h * 0.12f, cx - bw, bodyTop + h * 0.05f, cx - 7.0f, bodyTop);
        shape.closeSubPath();
        Path pegbox;
        pegbox.addRoundedRectangle (cx - 5.0f, top, 14.0f, 16.0f, 3.0f);
        pegbox.applyTransform (AffineTransform::rotation (0.5f, cx, top + 14.0f));
        fillInk (g, pegbox, pal::woodDark, 1.2f);
        // Rosetten som en jugendblomma.
        const Point<float> rc (cx, bodyTop + (bottom - bodyTop) * 0.38f);
        for (int k = 0; k < 8; ++k)
        {
            Path petal;
            petal.addEllipse (-3.0f, -10.0f, 6.0f, 10.0f);
            petal.applyTransform (AffineTransform::rotation (pi * 0.25f * (float) k).translated (rc));
            soundhole.addPath (petal);
        }
        wood = Colour (0xffb98a4c);
        strings = 6; sWidthTop = 9.0f; sWidthBottom = 16.0f; sTop = top + 16.0f; sBottom = bottom - h * 0.1f;
    }
    else if (type == 2) // cittra: platt låda med rund rosett och många strängar
    {
        const float w = jmin (a.getWidth() * 0.86f, 104.0f);
        shape.startNewSubPath (cx - w * 0.5f, top + 14.0f);
        shape.lineTo (cx + w * 0.3f, top + 14.0f);
        shape.quadraticTo (cx + w * 0.5f, top + 14.0f, cx + w * 0.5f, top + 40.0f);
        shape.lineTo (cx + w * 0.5f, bottom - 10.0f);
        shape.lineTo (cx - w * 0.5f, bottom - 10.0f);
        shape.closeSubPath();
        soundhole.addEllipse (cx - 13.0f, top + h * 0.45f, 26.0f, 26.0f);
        wood = Colour (0xff7c5232);
        strings = 9; sWidthTop = w * 0.8f; sWidthBottom = w * 0.8f; sTop = top + 22.0f; sBottom = bottom - 16.0f;
    }
    else // rostharpa: en lyra av järn med jugendvolyter
    {
        const float w = jmin (a.getWidth() * 0.42f, 48.0f);
        shape.startNewSubPath (cx - w * 0.5f, bottom - 6.0f);
        shape.cubicTo (cx - w * 1.4f, bottom - h * 0.45f, cx - w * 0.2f, top + h * 0.25f, cx - w * 0.9f, top + 8.0f);
        shape.quadraticTo (cx - w * 1.2f, top - 4.0f, cx - w * 0.6f, top + 4.0f);
        shape.lineTo (cx + w * 0.6f, top + 4.0f);
        shape.quadraticTo (cx + w * 1.2f, top - 4.0f, cx + w * 0.9f, top + 8.0f);
        shape.cubicTo (cx + w * 0.2f, top + h * 0.25f, cx + w * 1.4f, bottom - h * 0.45f, cx + w * 0.5f, bottom - 6.0f);
        shape.closeSubPath();
        Path hole;
        hole.addRoundedRectangle (cx - w * 0.45f, top + 14.0f, w * 0.9f, h - 34.0f, 6.0f);
        shape.addPath (hole);
        shape.setUsingNonZeroWinding (false);
        wood = pal::iron;
        strings = 6; sWidthTop = w * 0.7f; sWidthBottom = w * 0.7f; sTop = top + 14.0f; sBottom = bottom - 20.0f;
    }
    // Klangen: två piskrapp ur instrumentet.
    whiplash (g, { cx + sWidthBottom * 0.4f, sBottom - 10.0f }, { a.getRight() - 2.0f, a.getY() + h * 0.2f }, 12.0f, 5.0f, pal::rose.withAlpha (0.75f));
    whiplash (g, { cx - sWidthBottom * 0.4f, sBottom - 10.0f }, { a.getX() + 2.0f, a.getY() + h * 0.3f }, -12.0f, 4.0f, pal::teal.withAlpha (0.65f));
    fillInk (g, shape, wood);
    if (! soundhole.isEmpty())
    {
        g.setColour (pal::ink.withAlpha (0.85f));
        g.fillPath (soundhole);
    }
    // Strängarna, rostiga med Surr, och knäppunkten som en glödande linje.
    const auto metal = (type == 1 ? pal::birch : pal::steel).interpolatedWith (pal::rust, rust);
    for (int i = 0; i < strings; ++i)
    {
        const float f = ((float) i + 0.5f) / (float) strings - 0.5f;
        g.setColour (metal);
        g.drawLine (cx + f * sWidthTop, sTop, cx + f * sWidthBottom, sBottom, type == 1 ? 0.8f : 1.1f);
    }
    const float t = 0.05f + 0.9f * pluck * 0.5f;
    const float py = sBottom - (sBottom - sTop) * t;
    const float pw = (sWidthBottom + (sWidthTop - sWidthBottom) * t) * 0.5f + 5.0f;
    g.setColour (Colour (0xffff8a2a).withAlpha (0.35f));
    g.drawLine (cx - pw, py, cx + pw, py, 5.0f);
    g.setColour (Colour (0xffff8a2a));
    g.drawLine (cx - pw, py, cx + pw, py, 1.6f);
    rustSpots (g, shape, rust, 21 + type);
}

// --- Klockstapeln: kyrkklocka, skälla, gong, räls eller skålklocka ---------------------------

inline void bell (juce::Graphics& g, juce::Rectangle<float> area, int type, float crack)
{
    using namespace juce;
    const auto hc = Point<float> (area.getCentreX(), area.getY() + area.getHeight() * 0.42f);
    const auto a = plaque (g, area, hc, jmin (area.getWidth() * 0.36f, area.getHeight() * 0.34f));
    const float cx = a.getCentreX();
    // Bjälken med volyter i ändarna.
    const float beamY = a.getY() + a.getHeight() * 0.12f;
    Path beam;
    beam.addRoundedRectangle (a.getX() + 18.0f, beamY - 5.0f, a.getWidth() - 36.0f, 10.0f, 3.0f);
    fillInk (g, beam, pal::woodDark, 1.4f);
    for (float side : { -1.0f, 1.0f })
    {
        Path scroll;
        const float x = side < 0 ? a.getX() + 18.0f : a.getRight() - 18.0f;
        scroll.addCentredArc (x - side * 2.0f, beamY + 8.0f, 8.0f, 8.0f, 0.0f, side < 0 ? 0.0f : 0.0f, side * 1.6f * pi, true);
        inkLine (g, scroll, 2.0f, pal::woodDark);
    }
    const float top = beamY + 5.0f, h = a.getBottom() - top - 6.0f;
    const float w = jmin (a.getWidth() * 0.78f, h * 1.15f);
    Path shape;
    switch (type)
    {
        case 0: // kyrkklocka med slagring
            shape.startNewSubPath (cx - w * 0.18f, top + 10.0f);
            shape.quadraticTo (cx, top + 2.0f, cx + w * 0.18f, top + 10.0f);
            shape.cubicTo (cx + w * 0.22f, top + h * 0.45f, cx + w * 0.30f, top + h * 0.72f, cx + w * 0.48f, top + h * 0.86f);
            shape.quadraticTo (cx + w * 0.5f, top + h * 0.93f, cx + w * 0.42f, top + h * 0.93f);
            shape.lineTo (cx - w * 0.42f, top + h * 0.93f);
            shape.quadraticTo (cx - w * 0.5f, top + h * 0.93f, cx - w * 0.48f, top + h * 0.86f);
            shape.cubicTo (cx - w * 0.30f, top + h * 0.72f, cx - w * 0.22f, top + h * 0.45f, cx - w * 0.18f, top + 10.0f);
            shape.closeSubPath();
            break;
        case 1: // skälla: fyrkantig koskälla
            shape.startNewSubPath (cx - w * 0.16f, top + 14.0f);
            shape.lineTo (cx + w * 0.16f, top + 14.0f);
            shape.quadraticTo (cx + w * 0.2f, top + h * 0.4f, cx + w * 0.27f, top + h * 0.72f);
            shape.lineTo (cx - w * 0.27f, top + h * 0.72f);
            shape.quadraticTo (cx - w * 0.2f, top + h * 0.4f, cx - w * 0.16f, top + 14.0f);
            shape.closeSubPath();
            break;
        case 2: // gong
        {
            const float d = jmin (w * 0.8f, h * 0.82f);
            shape.addEllipse (cx - d * 0.5f, top + 12.0f, d, d);
            break;
        }
        case 3: // räls
            shape.addRoundedRectangle (cx - w * 0.45f, top + h * 0.48f, w * 0.9f, 14.0f, 2.0f);
            break;
        default: // skålklocka på en kudde
            shape.startNewSubPath (cx - w * 0.36f, top + h * 0.48f);
            shape.cubicTo (cx - w * 0.34f, top + h * 0.86f, cx + w * 0.34f, top + h * 0.86f, cx + w * 0.36f, top + h * 0.48f);
            shape.closeSubPath();
            break;
    }
    const auto b = shape.getBounds();
    // Upphängning.
    if (type == 2 || type == 3)
    {
        for (float f : { 0.22f, 0.78f })
        {
            Path rope;
            rope.startNewSubPath (b.getX() + b.getWidth() * f, top);
            rope.lineTo (b.getX() + b.getWidth() * f, b.getY() + (type == 2 ? b.getHeight() * 0.15f : 3.0f));
            inkLine (g, rope, 1.6f, pal::woodDark);
        }
    }
    else if (type == 4)
    {
        Path cushion;
        cushion.addRoundedRectangle (b.getX() - 6.0f, b.getBottom() - 4.0f, b.getWidth() + 12.0f, 14.0f, 7.0f);
        fillInk (g, cushion, pal::rose, 1.2f);
    }
    else
    {
        Path yoke;
        yoke.addRoundedRectangle (cx - 6.0f, top, 12.0f, 14.0f, 3.0f);
        fillInk (g, yoke, pal::iron, 1.2f);
    }
    // Klangen: piskrapp ut från klockans sidor.
    whiplash (g, { b.getRight() - 4.0f, b.getBottom() - 8.0f }, { a.getRight() - 4.0f, top + h * 0.2f }, 14.0f, 6.0f, pal::rose.withAlpha (0.7f));
    whiplash (g, { b.getX() + 4.0f, b.getBottom() - 8.0f }, { a.getX() + 4.0f, top + h * 0.25f }, -14.0f, 5.0f, pal::teal.withAlpha (0.6f));
    fillInk (g, shape, type == 3 ? pal::iron : pal::brass, 1.8f);
    // Jugendornament på klockan: ett band med bågar.
    if (type == 0 || type == 1)
    {
        Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (shape);
        const float y = b.getY() + b.getHeight() * (type == 0 ? 0.3f : 0.25f);
        g.setColour (pal::goldDark);
        g.drawLine (b.getX(), y, b.getRight(), y, 1.0f);
        g.drawLine (b.getX(), y + 9.0f, b.getRight(), y + 9.0f, 1.0f);
        for (float x = b.getX(); x < b.getRight(); x += 9.0f)
        {
            Path arc;
            arc.addCentredArc (x + 4.5f, y + 9.0f, 4.0f, 6.0f, 0.0f, -pi * 0.5f, pi * 0.5f, true);
            g.strokePath (arc, PathStrokeType (0.9f));
        }
        g.setColour (pal::goldLight.withAlpha (0.7f));
        g.drawLine (b.getX() + b.getWidth() * 0.3f, b.getY() + 14.0f, b.getX() + b.getWidth() * 0.22f, b.getBottom() - 14.0f, 2.0f);
    }
    else if (type == 2)
    {
        g.setColour (pal::goldDark);
        g.drawEllipse (b.reduced (b.getWidth() * 0.16f), 1.2f);
        g.drawEllipse (b.reduced (b.getWidth() * 0.3f), 0.8f);
        Path boss;
        boss.addEllipse (b.withSizeKeepingCentre (b.getWidth() * 0.18f, b.getHeight() * 0.18f));
        fillInk (g, boss, pal::goldLight, 1.0f);
    }
    rustSpots (g, shape, 0.15f + 0.5f * crack, 31 + type);
    if (crack > 0.02f) // sprickan
    {
        Path cp;
        float x = b.getX() + b.getWidth() * 0.62f, y = b.getBottom() - 3.0f;
        cp.startNewSubPath (x, y);
        const int steps = 2 + (int) (crack * 8.0f);
        for (int i = 0; i < steps; ++i)
        {
            x += (i % 2 == 0 ? -1.0f : 1.0f) * b.getWidth() * 0.05f;
            y -= b.getHeight() * 0.07f;
            cp.lineTo (x, y);
        }
        inkLine (g, cp, 1.6f);
    }
}

// --- Pumphuset: vattenfon, såg, plåt eller cymbal, och stråken ------------------------------

inline void pump (juce::Graphics& g, juce::Rectangle<float> area, int type, float water, float rust, float pressure)
{
    using namespace juce;
    const auto hc = Point<float> (area.getCentreX(), area.getY() + area.getHeight() * 0.38f);
    const auto a = plaque (g, area, hc, jmin (area.getWidth() * 0.4f, area.getHeight() * 0.3f));
    const float cx = a.getCentreX(), cy = a.getCentreY();
    const auto metal = pal::steel.interpolatedWith (pal::rust, rust * 0.7f);
    Path shape;
    Point<float> bowAt;
    if (type == 0) // vattenfon: skål med stavar och vatten
    {
        const float rx = jmin (a.getWidth(), a.getHeight()) * 0.34f;
        const float bowlY = cy + a.getHeight() * 0.12f;
        for (int i = 0; i < 11; ++i)
        {
            const float f = (float) i / 10.0f - 0.5f;
            const float len = rx * (0.9f + 0.45f * (float) ((i * 7) % 5) / 4.0f);
            Path rod;
            rod.startNewSubPath (cx + f * rx * 1.7f, bowlY);
            rod.lineTo (cx + f * rx * 2.3f, bowlY - len);
            inkLine (g, rod, 3.2f, pal::ink);
            inkLine (g, rod, 1.6f, metal.brighter (0.2f));
        }
        Path tank;
        tank.startNewSubPath (cx - rx, bowlY);
        tank.cubicTo (cx - rx, bowlY + rx * 0.8f, cx + rx, bowlY + rx * 0.8f, cx + rx, bowlY);
        tank.closeSubPath();
        fillInk (g, tank, metal);
        Path rim;
        rim.addEllipse (cx - rx, bowlY - 5.0f, rx * 2.0f, 10.0f);
        fillInk (g, rim, pal::teal.withMultipliedBrightness (0.9f + 0.2f * water), 1.2f);
        // Vattnet som böljande hår ur skålen.
        for (int k = 0; k < 2 + (int) (water * 3.0f); ++k)
            whiplash (g, { cx - rx * 0.4f + (float) k * rx * 0.3f, bowlY + rx * 0.5f },
                      { cx - rx * 1.2f + (float) k * rx * 0.7f, a.getBottom() - 2.0f }, (k % 2 == 0 ? 8.0f : -8.0f), 4.0f,
                      pal::teal.withAlpha (0.7f));
        shape = tank;
        bowAt = { cx + rx * 0.95f, bowlY - rx * 0.9f };
    }
    else if (type == 1) // såg: S-böjt blad med handtag
    {
        const float w = a.getWidth() * 0.38f;
        shape.startNewSubPath (cx - w, cy + 26.0f);
        shape.cubicTo (cx - w * 0.3f, cy - 40.0f, cx + w * 0.3f, cy + 30.0f, cx + w, cy - 24.0f);
        shape.lineTo (cx + w, cy - 6.0f);
        shape.cubicTo (cx + w * 0.3f, cy + 50.0f, cx - w * 0.3f, cy - 16.0f, cx - w, cy + 46.0f);
        shape.closeSubPath();
        fillInk (g, shape, metal);
        // Tänderna.
        g.setColour (pal::ink);
        for (int i = 0; i < 18; ++i)
        {
            const auto p = shape.getPointAlongPath (shape.getLength() * (0.52f + 0.4f * (float) i / 18.0f));
            g.fillEllipse (Rectangle<float> (2.0f, 2.0f).withCentre (p));
        }
        Path handle;
        handle.addRoundedRectangle (cx - w - 22.0f, cy + 26.0f, 26.0f, 24.0f, 7.0f);
        fillInk (g, handle, pal::wood);
        g.setColour (pal::ink);
        g.fillEllipse (cx - w - 15.0f, cy + 33.0f, 10.0f, 10.0f);
        bowAt = { cx + w * 0.9f, cy - 20.0f };
    }
    else if (type == 2) // plåt i två kedjor
    {
        auto plate = Rectangle<float> (cx - a.getWidth() * 0.32f, a.getY() + a.getHeight() * 0.2f, a.getWidth() * 0.64f, a.getHeight() * 0.62f);
        for (float f : { 0.18f, 0.82f })
        {
            const float x = plate.getX() + plate.getWidth() * f;
            for (float y = a.getY(); y < plate.getY(); y += 7.0f)
            {
                g.setColour (pal::ink);
                g.drawEllipse (x - 2.5f, y, 5.0f, 7.0f, 1.2f);
            }
        }
        shape.startNewSubPath (plate.getTopLeft());
        shape.lineTo (plate.getTopRight());
        shape.quadraticTo (plate.getRight() + 8.0f, plate.getCentreY(), plate.getRight(), plate.getBottom());
        shape.lineTo (plate.getX(), plate.getBottom());
        shape.quadraticTo (plate.getX() + 8.0f, plate.getCentreY(), plate.getX(), plate.getY());
        shape.closeSubPath();
        fillInk (g, shape, metal);
        g.setColour (pal::goldDark.withAlpha (0.5f));
        for (int i = 1; i < 4; ++i)
            g.drawLine (plate.getX() + 6.0f, plate.getY() + plate.getHeight() * (float) i / 4.0f,
                        plate.getRight() - 6.0f, plate.getY() + plate.getHeight() * (float) i / 4.0f + 6.0f, 0.8f);
        bowAt = { plate.getRight() - 4.0f, plate.getY() + 8.0f };
    }
    else // cymbal på stativ
    {
        const float w = a.getWidth() * 0.4f;
        Path stand;
        stand.startNewSubPath (cx, cy);
        stand.lineTo (cx, a.getBottom() - 6.0f);
        stand.startNewSubPath (cx, a.getBottom() - 20.0f);
        stand.lineTo (cx - 22.0f, a.getBottom() - 2.0f);
        stand.startNewSubPath (cx, a.getBottom() - 20.0f);
        stand.lineTo (cx + 22.0f, a.getBottom() - 2.0f);
        inkLine (g, stand, 2.4f, pal::iron);
        shape.addEllipse (cx - w, cy - 12.0f, w * 2.0f, 24.0f);
        fillInk (g, shape, pal::brass.interpolatedWith (pal::rust, rust * 0.6f));
        g.setColour (pal::goldDark);
        g.drawEllipse (cx - w * 0.6f, cy - 7.0f, w * 1.2f, 14.0f, 0.8f);
        Path bossC;
        bossC.addEllipse (cx - 9.0f, cy - 6.0f, 18.0f, 9.0f);
        fillInk (g, bossC, pal::goldLight, 1.0f);
        bowAt = { cx + w - 4.0f, cy - 2.0f };
    }
    rustSpots (g, shape, rust, 41 + type);
    // Stråken: trä och tagel; mer tryck böjer taglet mot metallen.
    const auto bowFrom = bowAt + Point<float> (-30.0f, -46.0f), bowTo = bowAt + Point<float> (24.0f, 30.0f);
    Path stick;
    stick.startNewSubPath (bowFrom);
    stick.quadraticTo ((bowFrom + bowTo) * 0.5f + Point<float> (8.0f, -8.0f), bowTo);
    inkLine (g, stick, 3.4f, pal::ink);
    inkLine (g, stick, 1.8f, pal::woodDark);
    Path hair;
    hair.startNewSubPath (bowFrom + Point<float> (3.0f, 3.0f));
    hair.quadraticTo ((bowFrom + bowTo) * 0.5f + Point<float> (-4.0f - 8.0f * pressure, 6.0f + 6.0f * pressure), bowTo + Point<float> (-3.0f, 0.0f));
    inkLine (g, hair, 1.2f, pal::birch);
    g.setColour (pal::rose);
    g.fillEllipse (Rectangle<float> (7.0f, 7.0f).withCentre (bowTo));
}

// --- Krigstuban: helikon, lyssnartrattar eller megafoner ----------------------------------

inline void tuba (juce::Graphics& g, juce::Rectangle<float> area, int type, int horns, float pressure)
{
    using namespace juce;
    const auto hc = Point<float> (area.getCentreX() + area.getWidth() * 0.08f, area.getY() + area.getHeight() * 0.4f);
    const auto a = plaque (g, area, hc, jmin (area.getWidth() * 0.38f, area.getHeight() * 0.32f));
    Graphics::ScopedSaveState clip (g);
    g.reduceClipRegion (archPath (area));
    for (int k = horns - 1; k >= 0; --k)
    {
        const float depth = horns > 1 ? (float) k / (float) (horns - 1) : 0.0f;
        const float s = jmin (a.getHeight(), a.getWidth() * 0.95f) * ((horns > 1 ? 0.8f : 0.92f) - 0.38f * depth);
        const float ox = a.getX() + a.getWidth() * (horns > 1 ? 0.6f - 0.4f * depth : 0.55f);
        const float oy = a.getY() + a.getHeight() * (0.58f - 0.22f * depth);
        const auto shade = [depth] (Colour c) { return c.interpolatedWith (pal::parchmentDk, depth * 0.45f); };
        if (type == 0) // helikon: ett varv rör runt kroppen och en stor klocka uppåt framåt
        {
            Path coil;
            coil.addEllipse (ox - s * 0.42f, oy - s * 0.18f, s * 0.6f, s * 0.5f);
            inkLine (g, coil, s * 0.075f + 2.0f, pal::ink);
            inkLine (g, coil, s * 0.075f, shade (pal::brass));
            Path valves;
            for (int v = 0; v < 3; ++v)
                valves.addRoundedRectangle (ox - s * 0.2f + (float) v * s * 0.07f, oy - s * 0.02f, s * 0.05f, s * 0.16f, 2.0f);
            fillInk (g, valves, shade (pal::goldLight), 1.0f);
            Path bellP;
            const Point<float> m (ox + s * 0.2f, oy - s * 0.25f);
            bellP.startNewSubPath (ox + s * 0.02f, oy - s * 0.05f);
            bellP.quadraticTo (m.x - s * 0.02f, m.y + s * 0.1f, m.x + s * 0.02f, m.y - s * 0.22f);
            bellP.lineTo (m.x + s * 0.26f, m.y + s * 0.1f);
            bellP.quadraticTo (m.x + s * 0.02f, m.y + s * 0.12f, ox + s * 0.06f, oy + s * 0.04f);
            bellP.closeSubPath();
            fillInk (g, bellP, shade (pal::brass));
            Path mouth;
            mouth.addEllipse (Rectangle<float> (s * 0.34f, s * 0.13f).withCentre ({ m.x + s * 0.14f, m.y - s * 0.06f }));
            mouth.applyTransform (AffineTransform::rotation (0.95f, m.x + s * 0.14f, m.y - s * 0.06f));
            fillInk (g, mouth, pal::ink.brighter (0.15f), 1.6f);
            if (k == 0)
                for (int w = 0; w < 1 + (int) (pressure * 3.0f); ++w)
                    whiplash (g, { m.x + s * 0.2f, m.y - s * 0.15f }, { a.getRight() - 2.0f - (float) w * 10.0f, a.getY() + (float) w * 14.0f },
                              (w % 2 == 0 ? 10.0f : -10.0f), 5.0f, (w % 2 == 0 ? pal::rose : pal::teal).withAlpha (0.75f));
        }
        else // lyssnartrattar eller megafoner: koniska trattar på stativ
        {
            const bool locator = type == 1;
            const float len = s * (locator ? 0.75f : 0.55f), mouthH = s * (locator ? 0.42f : 0.3f);
            const Point<float> tip (ox - len * 0.5f, oy + s * 0.08f), mouthC (ox + len * 0.5f, oy - s * 0.06f);
            Path legs;
            legs.startNewSubPath (ox, oy + s * 0.02f);
            legs.lineTo (ox - s * 0.14f, a.getBottom());
            legs.startNewSubPath (ox, oy + s * 0.02f);
            legs.lineTo (ox + s * 0.14f, a.getBottom());
            inkLine (g, legs, 2.2f, shade (pal::woodDark));
            Path cone;
            cone.startNewSubPath (tip.translated (0.0f, -3.0f));
            cone.quadraticTo (ox, oy - s * 0.05f, mouthC.x, mouthC.y - mouthH * 0.5f);
            cone.lineTo (mouthC.x, mouthC.y + mouthH * 0.5f);
            cone.quadraticTo (ox, oy + s * 0.08f, tip.x, tip.y + 3.0f);
            cone.closeSubPath();
            fillInk (g, cone, shade (locator ? pal::sage : pal::iron));
            if (locator) // nitade band
            {
                g.setColour (pal::ink.withAlpha (0.6f));
                for (int b = 1; b < 4; ++b)
                {
                    const float t = (float) b / 4.0f;
                    const float x = tip.x + (mouthC.x - tip.x) * t;
                    const float hh = 3.0f + (mouthH * 0.5f - 3.0f) * t * t;
                    g.drawLine (x, mouthC.y + (tip.y - mouthC.y) * (1.0f - t) - hh, x, mouthC.y + (tip.y - mouthC.y) * (1.0f - t) + hh, 1.0f);
                }
            }
            Path mouth;
            mouth.addEllipse (Rectangle<float> (mouthH * 0.32f, mouthH).withCentre (mouthC));
            fillInk (g, mouth, pal::ink.brighter (0.2f), 1.6f);
            g.setColour (shade (pal::goldLight));
            g.drawEllipse (Rectangle<float> (mouthH * 0.32f, mouthH).withCentre (mouthC), 2.0f);
            if (! locator)
            {
                g.setColour (pal::iron.withAlpha (0.8f));
                for (int r = 1; r < 3; ++r)
                    g.drawEllipse (Rectangle<float> (mouthH * 0.32f, mouthH).withCentre (mouthC).reduced (mouthH * 0.05f * (float) r, mouthH * 0.14f * (float) r), 0.8f);
            }
            if (k == 0)
                for (int w = 0; w < 1 + (int) (pressure * 3.0f); ++w)
                    whiplash (g, mouthC.translated (2.0f, 0.0f), { a.getRight() - 2.0f, mouthC.y - mouthH * (0.6f - 0.4f * (float) w) },
                              (w % 2 == 0 ? 9.0f : -9.0f), 5.0f, (w % 2 == 0 ? pal::rose : pal::teal).withAlpha (0.75f));
        }
    }
}


// --- Basmålen: kvarnen som mal basen – tratt, kugghjul, kvarnstenar och mjölet som rinner ------------


inline void mill (juce::Graphics& g, juce::Rectangle<float> area, float grind)
{
    using namespace juce;
    const auto hc = Point<float> (area.getCentreX(), area.getY() + area.getHeight() * 0.42f);
    const auto a = plaque (g, area, hc, jmin (area.getWidth() * 0.42f, area.getHeight() * 0.26f));
    const float cx = a.getCentreX(), w = jmin (a.getWidth() * 0.9f, 120.0f);
    // Mjölet: strängar som rinner ur stenarna, fler ju hårdare den mal.
    const float stoneY = a.getY() + a.getHeight() * 0.62f;
    for (int i = 0; i < 2 + (int) (grind * 4.0f); ++i)
        whiplash (g, { cx + ((float) i - 2.0f) * 6.0f, stoneY + 10.0f }, { cx + ((float) i - 2.5f) * w * 0.22f, a.getBottom() - 2.0f },
                  (i % 2 == 0 ? 7.0f : -7.0f), 4.0f, (i % 2 == 0 ? pal::birch : pal::roseLight).withAlpha (0.9f));
    // Kugghjulen bakom.
    cog (g, { cx - w * 0.2f, a.getY() + a.getHeight() * 0.4f }, w * 0.28f, 12, 0.1f, pal::brass);
    cog (g, { cx + w * 0.26f, a.getY() + a.getHeight() * 0.3f }, w * 0.17f, 9, 0.3f, pal::goldLight);
    // Tratten överst.
    Path hopper;
    const float hy = a.getY() + a.getHeight() * 0.1f;
    hopper.startNewSubPath (cx - w * 0.34f, hy);
    hopper.lineTo (cx + w * 0.34f, hy);
    hopper.lineTo (cx + w * 0.07f, hy + a.getHeight() * 0.2f);
    hopper.lineTo (cx - w * 0.07f, hy + a.getHeight() * 0.2f);
    hopper.closeSubPath();
    fillInk (g, hopper, pal::wood);
    g.setColour (pal::woodDark);
    for (int i = 1; i < 4; ++i)
        g.drawLine (cx - w * 0.34f + w * 0.17f * (float) i, hy + 2.0f, cx - w * 0.07f + w * 0.035f * (float) i, hy + a.getHeight() * 0.2f - 2.0f, 0.8f);
    // Kvarnstenarna, sedda från sidan: två tjocka skivor med räfflor.
    for (int k = 0; k < 2; ++k)
    {
        const float y = stoneY - 16.0f + (float) k * 14.0f;
        Path stone;
        stone.addRoundedRectangle (cx - w * 0.44f, y, w * 0.88f, 13.0f, 6.0f);
        fillInk (g, stone, k == 0 ? pal::steel : pal::iron);
        g.setColour (pal::ink.withAlpha (0.5f));
        for (float x = cx - w * 0.4f; x < cx + w * 0.4f; x += 7.0f)
            g.drawLine (x, y + 3.0f, x + 4.0f, y + 10.0f, 0.8f);
    }
    // Axeln.
    Path shaft;
    shaft.addRectangle (cx - 3.0f, stoneY - 34.0f, 6.0f, 20.0f);
    fillInk (g, shaft, pal::iron, 1.0f);
}

// --- Bordunorgeln: instrumentet som klangfärgen härmar ----------------------------------------
// typ: 0 vallflöjt, 1 flöjt/pipa, 2 säckpipa, 3 pipa och harpa, 4 nyckelharpa.

inline void bordunInstrument (juce::Graphics& g, juce::Rectangle<float> area, int type, float drone)
{
    using namespace juce;
    const auto hc = Point<float> (area.getCentreX(), area.getY() + area.getHeight() * 0.38f);
    const auto a = plaque (g, area, hc, jmin (area.getWidth() * 0.42f, area.getHeight() * 0.27f));
    const float cx = a.getCentreX(), top = a.getY() + 2.0f, bottom = a.getBottom() - 2.0f, h = bottom - top;
    // Bordunen: en lång, lugn sträng av ljud längst ned, starkare med Bordun.
    whiplash (g, { a.getX() + 4.0f, bottom - h * 0.12f }, { a.getRight() - 4.0f, bottom - h * 0.16f }, 6.0f, 3.0f + 5.0f * drone,
              pal::teal.withAlpha (0.4f + 0.5f * drone));
    auto pipe = [&] (Point<float> from, Point<float> to, float w0, float w1, Colour col, int holes)
    {
        const auto d = to - from;
        const float len = jmax (1.0f, d.getDistanceFromOrigin());
        const Point<float> n (-d.y / len, d.x / len);
        Path p;
        p.startNewSubPath (from + n * w0);
        p.lineTo (to + n * w1);
        p.lineTo (to - n * w1);
        p.lineTo (from - n * w0);
        p.closeSubPath();
        fillInk (g, p, col);
        for (int i = 0; i < holes; ++i)
        {
            g.setColour (pal::ink);
            g.fillEllipse (Rectangle<float> (3.6f, 3.6f).withCentre (from + d * (0.35f + 0.5f * (float) i / (float) jmax (1, holes))));
        }
        g.setColour (pal::goldLight);
        for (float t : { 0.08f, 0.92f })
        {
            const auto q = from + d * t;
            const float ww = w0 + (w1 - w0) * t;
            g.drawLine (Line<float> (q + n * ww, q - n * ww), 2.0f);
        }
        return p;
    };
    if (type == 0) // vallflöjt: lång och slät, utan hål
    {
        pipe ({ cx + 22.0f, top + 4.0f }, { cx - 18.0f, bottom - h * 0.06f }, 3.0f, 6.0f, pal::wood, 0);
        whiplash (g, { cx + 20.0f, top + 10.0f }, { a.getRight() - 4.0f, top + h * 0.5f }, 10.0f, 4.0f, pal::rose.withAlpha (0.8f));
    }
    else if (type == 1) // flöjt eller pipa: kortare, med fingerhål
    {
        pipe ({ cx + 14.0f, top + h * 0.12f }, { cx - 12.0f, bottom - h * 0.12f }, 4.0f, 5.0f, Colour (0xffb98a4c), 6);
        whiplash (g, { cx - 12.0f, bottom - h * 0.12f }, { a.getRight() - 4.0f, top + h * 0.4f }, -10.0f, 4.0f, pal::rose.withAlpha (0.8f));
    }
    else if (type == 2) // säckpipa: säcken i mönstrat tyg, chanter nedåt och bordunpipa med band över axeln
    {
        pipe ({ cx + 6.0f, top + h * 0.38f }, { cx + 30.0f, top + 4.0f }, 3.5f, 6.5f, pal::wood, 0);   // bordunpipan
        const Point<float> bc (cx - 4.0f, top + h * 0.5f);
        Path bag;
        bag.addEllipse (Rectangle<float> (jmin (a.getWidth() * 0.62f, 66.0f), h * 0.32f).withCentre (bc));
        bag.applyTransform (AffineTransform::rotation (-0.35f, bc.x, bc.y));
        fillInk (g, bag, pal::rose);
        {
            Graphics::ScopedSaveState s (g);
            g.reduceClipRegion (bag);
            g.setColour (pal::roseLight.withAlpha (0.8f));
            for (int i = 0; i < 6; ++i)
                for (int j = 0; j < 4; ++j)
                    g.fillEllipse (Rectangle<float> (4.0f, 4.0f).withCentre ({ bc.x - 30.0f + (float) i * 12.0f + (float) (j % 2) * 6.0f, bc.y - 20.0f + (float) j * 12.0f }));
        }
        pipe ({ cx - 10.0f, bc.y + h * 0.1f }, { cx - 18.0f, bottom - h * 0.08f }, 3.5f, 6.0f, pal::woodDark, 5);  // chantern
        pipe ({ cx - 26.0f, bc.y - h * 0.06f }, { cx - 40.0f, bc.y - h * 0.24f }, 2.0f, 2.5f, pal::woodDark, 0);   // blåsröret
        whiplash (g, { cx + 30.0f, top + 4.0f }, { a.getRight() - 4.0f, top + h * 0.45f }, 10.0f, 4.0f, pal::sageDark.withAlpha (0.8f));   // banden
        whiplash (g, { cx + 28.0f, top + 8.0f }, { a.getRight() - 10.0f, top + h * 0.6f }, -8.0f, 3.0f, pal::rose.withAlpha (0.8f));
    }
    else if (type == 3) // pipa och harpa: en liten bågharpa med en pipa framför
    {
        Path harp;
        harp.startNewSubPath (cx - 22.0f, bottom - h * 0.08f);
        harp.quadraticTo (cx - 36.0f, top + h * 0.35f, cx - 6.0f, top + h * 0.14f);
        harp.quadraticTo (cx + 22.0f, top + h * 0.16f, cx + 26.0f, top + h * 0.26f);
        harp.lineTo (cx - 18.0f, bottom - h * 0.1f);
        inkLine (g, harp, 6.0f, pal::ink);
        inkLine (g, harp, 4.0f, pal::wood);
        g.setColour (pal::steel);
        for (int i = 0; i < 7; ++i)
        {
            const float t = (float) i / 7.0f;
            g.drawLine (cx - 22.0f + (float) i * 6.0f, top + h * (0.22f + 0.02f * (float) i), cx - 22.0f + (float) i * 1.5f, bottom - h * (0.12f + 0.06f * t), 0.9f);
        }
        pipe ({ cx + 20.0f, top + h * 0.3f }, { cx + 10.0f, bottom - h * 0.06f }, 3.0f, 4.0f, Colour (0xffb98a4c), 5);
    }
    else // nyckelharpa: lång kropp, nyckelrad, stråke
    {
        const float w = jmin (a.getWidth() * 0.34f, 34.0f);
        Path body;
        body.startNewSubPath (cx - 4.0f, top + 4.0f);
        body.lineTo (cx + 4.0f, top + 4.0f);
        body.lineTo (cx + 6.0f, top + h * 0.3f);
        body.cubicTo (cx + w, top + h * 0.32f, cx + w * 0.7f, top + h * 0.55f, cx + w * 0.6f, top + h * 0.6f);
        body.cubicTo (cx + w, top + h * 0.7f, cx + w, bottom - 6.0f, cx, bottom - 2.0f);
        body.cubicTo (cx - w, bottom - 6.0f, cx - w, top + h * 0.7f, cx - w * 0.6f, top + h * 0.6f);
        body.cubicTo (cx - w * 0.7f, top + h * 0.55f, cx - w, top + h * 0.32f, cx - 6.0f, top + h * 0.3f);
        body.closeSubPath();
        fillInk (g, body, pal::wood);
        // Nyckelraden och strängarna.
        Path keybox;
        keybox.addRoundedRectangle (cx - 7.0f, top + h * 0.12f, 14.0f, h * 0.5f, 2.0f);
        fillInk (g, keybox, pal::woodDark, 1.0f);
        g.setColour (pal::birch);
        for (int i = 0; i < 9; ++i)
            g.fillRect (cx + 6.0f, top + h * 0.15f + (float) i * h * 0.05f, 6.0f, 2.0f);
        g.setColour (pal::steel);
        for (int i = -1; i <= 1; ++i)
            g.drawLine (cx + (float) i * 3.0f, top + 6.0f, cx + (float) i * 4.0f, bottom - 10.0f, 0.8f);
        g.setColour (pal::ink);
        for (float side : { -1.0f, 1.0f })
            g.fillEllipse (Rectangle<float> (5.0f, 10.0f).withCentre ({ cx + side * w * 0.45f, top + h * 0.78f }));
        // Stråken.
        Path bow;
        bow.startNewSubPath (cx - w * 1.3f, top + h * 0.62f);
        bow.quadraticTo (cx, top + h * 0.58f, cx + w * 1.3f, top + h * 0.78f);
        inkLine (g, bow, 2.4f, pal::woodDark);
    }
}

// --- Spolslingan: induktionsspolen – en mässingsspole lindad med koppartråd, och gnistor som slingor ---

inline void coil (juce::Graphics& g, juce::Rectangle<float> area, bool phaseDistortion, float activity)
{
    using namespace juce;
    const auto hc = Point<float> (area.getCentreX(), area.getY() + area.getHeight() * 0.32f);
    const auto a = plaque (g, area, hc, jmin (area.getWidth() * 0.44f, area.getHeight() * 0.24f));
    const float cx = a.getCentreX(), top = a.getY(), h = a.getHeight();
    const float w = jmin (a.getWidth() * 0.62f, 60.0f);
    const float coilTop = top + h * 0.3f, coilBottom = top + h * 0.82f;
    // Gnistor från kulan överst: slingor som piskrapp, fler när slingan går.
    const Point<float> ball (cx, top + h * 0.16f);
    for (int i = 0; i < 2 + (int) (activity * 3.0f); ++i)
    {
        const float ang = -pi * 0.4f + pi * 0.8f * (float) i / (float) (1 + (int) (activity * 3.0f) + 1);
        whiplash (g, ball, { ball.x + std::sin (ang) * a.getWidth() * 0.5f, ball.y - std::cos (ang) * h * 0.16f + 10.0f },
                  (i % 2 == 0 ? 6.0f : -6.0f), 3.0f, (i % 2 == 0 ? pal::goldLight : pal::roseLight).withAlpha (0.95f));
    }
    Path rod;
    rod.addRectangle (cx - 2.5f, ball.y, 5.0f, coilTop - ball.y);
    fillInk (g, rod, pal::iron, 1.0f);
    Path sphere;
    sphere.addEllipse (Rectangle<float> (16.0f, 16.0f).withCentre (ball));
    fillInk (g, sphere, pal::brass);
    // Lindningen: kopparvarv mellan två mässingsskivor.
    Path winding;
    winding.addRoundedRectangle (cx - w * 0.42f, coilTop, w * 0.84f, coilBottom - coilTop, 4.0f);
    fillInk (g, winding, Colour (0xffb5653a));
    g.setColour (pal::ink.withAlpha (0.55f));
    for (float y = coilTop + 4.0f; y < coilBottom - 2.0f; y += 3.5f)
        g.drawLine (cx - w * 0.42f, y, cx + w * 0.42f, y + 1.5f, 0.8f);
    g.setColour (Colour (0xffe8a070).withAlpha (0.6f));
    g.drawLine (cx - w * 0.25f, coilTop + 4.0f, cx - w * 0.25f, coilBottom - 4.0f, 2.0f);
    for (float y : { coilTop, coilBottom })
    {
        Path flange;
        flange.addRoundedRectangle (cx - w * 0.55f, y - 5.0f, w * 1.1f, 10.0f, 4.0f);
        fillInk (g, flange, pal::brass);
    }
    // Foten, och Fasvridaren som ett kugghjul vid sidan.
    Path foot;
    foot.addRoundedRectangle (cx - w * 0.7f, coilBottom + 6.0f, w * 1.4f, 10.0f, 3.0f);
    fillInk (g, foot, pal::woodDark);
    if (phaseDistortion)
        cog (g, { cx + w * 0.62f, coilBottom - 14.0f }, w * 0.24f, 10, 0.2f, pal::goldLight);
}


// --- Dosan: en öppen speldosa snett framifrån – lock med intarsia, valsen med stift, kammen och nyckeln ---
// typ: 0 speldosa, 1 kyrkspeldosa, 2 glasdosa. rust och spring 0..1.

inline void musicBox (juce::Graphics& g, juce::Rectangle<float> area, int type, float rust, float spring)
{
    using namespace juce;
    const auto hc = Point<float> (area.getCentreX(), area.getY() + area.getHeight() * 0.36f);
    const auto a = plaque (g, area, hc, jmin (area.getWidth() * 0.3f, area.getHeight() * 0.3f));

    const Colour woodC = type == 1 ? Colour (0xff6e3f22) : type == 2 ? Colour (0xffb98f5e) : Colour (0xff8a5530);
    // Lådans mått och en sned parallellprojektion: u längs bredden, v inåt (djup), z uppåt.
    const float W = jmin (a.getWidth() * 0.7f, (a.getHeight() - 14.0f) / 0.88f);
    const float H = W * 0.24f;                         // lådans höjd framtill
    const float dx = W * 0.12f, dy = W * 0.32f;        // djupets riktning på skärmen: vi ser in i lådan uppifrån
    const float x0 = a.getCentreX() - (W + dx) * 0.5f;
    const float yf = a.getBottom() - H - 8.0f;         // framkantens ovansida
    auto P = [&] (float u, float v, float z) { return Point<float> (x0 + u * W + v * dx, yf - v * dy - z); };
    auto quad = [] (Point<float> p0, Point<float> p1, Point<float> p2, Point<float> p3)
    {
        Path q; q.startNewSubPath (p0); q.lineTo (p1); q.lineTo (p2); q.lineTo (p3); q.closeSubPath(); return q;
    };

    // Musiken: slingor och små noter som stiger ur dosan.
    for (int i = 0; i < 3; ++i)
        whiplash (g, P (0.3f + 0.2f * (float) i, 0.5f, 0.0f), { a.getX() + a.getWidth() * (0.15f + 0.35f * (float) i), a.getY() + 4.0f },
                  (i % 2 == 0 ? 12.0f : -12.0f), 4.0f, (i == 1 ? pal::teal : pal::rose).withAlpha (0.7f));

    // Locket: uppfällt bakåt från bakkanten, vi ser dess insida med intarsia.
    const float lidH = W * 0.3f, lidBack = W * 0.04f;
    const auto l0 = P (0.0f, 1.0f, 0.0f), l1 = P (1.0f, 1.0f, 0.0f);
    const auto l2 = l1 + Point<float> (lidBack, -lidH), l3 = l0 + Point<float> (lidBack, -lidH);
    const auto lid = quad (l0, l1, l2, l3);
    fillInk (g, lid, type == 2 ? pal::sageLight.withAlpha (0.75f) : woodC.darker (0.1f));
    {
        // Intarsian: en oval kartusch med en jugendblomma, i en tunn ram.
        const auto lc = (l0 + l1 + l2 + l3) * 0.25f;
        const float lw = (l1.x - l0.x) * 0.7f, lh = lidH * 0.62f;
        Path frame;
        frame.addRoundedRectangle (Rectangle<float> ((l1.x - l0.x) * 0.86f, lidH * 0.82f).withCentre (lc), 6.0f);
        g.setColour (pal::goldLight.withAlpha (0.8f));
        g.strokePath (frame, PathStrokeType (1.2f));
        Path oval;
        oval.addEllipse (Rectangle<float> (lw, lh).withCentre (lc));
        fillInk (g, oval, type == 2 ? pal::birch.withAlpha (0.6f) : pal::parchment.withAlpha (0.9f), 1.2f);
        for (int k = 0; k < 6; ++k)
        {
            Path petal;
            petal.addEllipse (-lh * 0.06f, -lh * 0.3f, lh * 0.12f, lh * 0.28f);
            petal.applyTransform (AffineTransform::rotation (pi / 3.0f * (float) k).translated (lc));
            g.setColour (pal::rose);
            g.fillPath (petal);
        }
        g.setColour (pal::gold);
        g.fillEllipse (Rectangle<float> (lh * 0.14f, lh * 0.14f).withCentre (lc));
        // Rankor ut från blomman.
        for (float side : { -1.0f, 1.0f })
        {
            Path vine;
            vine.startNewSubPath (lc);
            vine.cubicTo (lc.x + side * lw * 0.2f, lc.y + lh * 0.3f, lc.x + side * lw * 0.35f, lc.y - lh * 0.2f, lc.x + side * lw * 0.42f, lc.y + lh * 0.05f);
            g.setColour (pal::sageDark);
            g.strokePath (vine, PathStrokeType (1.2f));
        }
        if (type == 1) // kyrkspeldosan: ett spetsbågsfönster i locket
        {
            Path arch;
            arch.startNewSubPath (lc.x - lw * 0.18f, lc.y + lh * 0.45f);
            arch.lineTo (lc.x - lw * 0.18f, lc.y - lh * 0.2f);
            arch.quadraticTo (lc.x - lw * 0.18f, lc.y - lh * 0.45f, lc.x, lc.y - lh * 0.55f);
            arch.quadraticTo (lc.x + lw * 0.18f, lc.y - lh * 0.45f, lc.x + lw * 0.18f, lc.y - lh * 0.2f);
            arch.lineTo (lc.x + lw * 0.18f, lc.y + lh * 0.45f);
            g.setColour (pal::goldDark);
            g.strokePath (arch, PathStrokeType (1.4f));
        }
    }

    // Lådans insida: mörka innerväggar och en botten med sammet.
    const auto rim = quad (P (0, 0, 0), P (1, 0, 0), P (1, 1, 0), P (0, 1, 0));
    fillInk (g, rim, woodC.darker (0.6f), 1.4f);
    const float in = 0.06f;
    const auto floor = quad (P (in, in, -H * 0.3f), P (1 - in, in, -H * 0.3f), P (1 - in, 1 - in, -H * 0.3f), P (in, 1 - in, -H * 0.3f));
    {
        Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (rim);
        fillInk (g, floor.createPathWithRoundedCorners (2.0f), type == 1 ? pal::sageDark : Colour (0xff7a2e2e), 0.0f);   // sammet

        // Kammens platta (mässing) framtill och stavarna som pekar inåt mot valsen.
        const float zm = H * 0.04f;
        const auto plate = quad (P (0.18f, 0.12f, zm), P (0.86f, 0.12f, zm), P (0.86f, 0.3f, zm), P (0.18f, 0.3f, zm));
        fillInk (g, plate, pal::brass, 1.0f);
        const int teeth = type == 1 ? 30 : 22;
        const Colour steel = type == 2 ? Colour (0xffcfeaf2) : pal::steel;
        for (int i = 0; i < teeth; ++i)
        {
            const float u = 0.2f + 0.64f * ((float) i + 0.5f) / (float) teeth;
            const float len = 0.22f + 0.16f * (1.0f - (float) i / (float) (teeth - 1));   // basstavarna (vänster) är längst
            const bool broken = rust > 0.6f && (i * 5) % 9 == 0;
            const bool rusty = rust > 0.05f && ((i * 11) % 7) < (int) (rust * 7.0f);
            Path tooth;
            tooth.startNewSubPath (P (u, 0.3f, zm));
            tooth.lineTo (P (u, 0.3f + (broken ? len * 0.4f : len), zm));
            g.setColour (pal::ink);
            g.strokePath (tooth, PathStrokeType (2.6f));
            g.setColour (rusty ? steel.interpolatedWith (pal::rust, 0.65f) : steel);
            g.strokePath (tooth, PathStrokeType (1.4f));
        }

        // Valsen: en mässingscylinder med stift, liggande bakom kammen, med kugghjul och regulator till höger.
        const float zc = H * 0.1f, rC = H * 0.3f;
        const auto c0 = P (0.16f, 0.8f, zc), c1 = P (0.82f, 0.8f, zc);
        Path cyl;
        cyl.startNewSubPath (c0.translated (0, -rC));
        cyl.lineTo (c1.translated (0, -rC));
        cyl.addCentredArc (c1.x, c1.y, rC * 0.4f, rC, 0.0f, 0.0f, pi, false);
        cyl.lineTo (c0.translated (0, rC));
        cyl.addCentredArc (c0.x, c0.y, rC * 0.4f, rC, 0.0f, pi, 2.0f * pi, false);
        cyl.closeSubPath();
        fillInk (g, cyl, pal::brass.interpolatedWith (pal::rust, rust * 0.4f));
        g.setColour (pal::goldLight.withAlpha (0.8f));
        g.drawLine (c0.x + 4.0f, c0.y - rC * 0.55f, c1.x - 4.0f, c1.y - rC * 0.55f, 1.6f);
        g.setColour (pal::ink.withAlpha (0.85f));
        Random rnd (3);
        for (int i = 0; i < 90; ++i)
        {
            const float t = rnd.nextFloat(), ph = rnd.nextFloat() * 1.8f - 0.9f;
            const auto p = c0 + (c1 - c0) * t + Point<float> (0.0f, ph * rC);
            g.fillEllipse (Rectangle<float> (1.8f, 1.8f).withCentre (p));
        }
        Path capL;
        capL.addEllipse (Rectangle<float> (rC * 0.8f, rC * 2.0f).withCentre (c0));
        fillInk (g, capL, pal::goldLight, 1.0f);
        cog (g, P (0.9f, 0.78f, zc + rC * 0.2f), rC * 0.9f, 12, 0.2f, pal::goldLight);
        rustSpots (g, cyl, rust, 51);
    }
    // Lådans framsida och högra sida, med intarsia och ett mässingsnyckelhål.
    const auto front = quad (P (0, 0, 0), P (1, 0, 0), P (1, 0, -H), P (0, 0, -H));
    const auto side = quad (P (1, 0, 0), P (1, 1, 0), P (1, 1, -H), P (1, 0, -H));
    fillInk (g, side, woodC.darker (0.35f));
    fillInk (g, front, woodC);
    {
        const auto f0 = P (0, 0, 0);
        const float fw = W, fh = H;
        g.setColour (pal::goldLight.withAlpha (0.75f));
        g.drawRect (Rectangle<float> (f0.x + 5.0f, f0.y + 4.0f, fw - 10.0f, fh - 8.0f), 1.0f);
        // Rankor i intarsian, och en liten skylt för fjädern.
        for (float side2 : { -1.0f, 1.0f })
        {
            const Point<float> c (f0.x + fw * 0.5f, f0.y + fh * 0.5f);
            Path vine;
            vine.startNewSubPath (c.translated (side2 * fw * 0.14f, 0.0f));
            vine.cubicTo (c.x + side2 * fw * 0.24f, c.y - fh * 0.35f, c.x + side2 * fw * 0.34f, c.y + fh * 0.35f, c.x + side2 * fw * 0.44f, c.y);
            g.setColour (pal::goldLight.withAlpha (0.7f));
            g.strokePath (vine, PathStrokeType (1.0f));
            g.setColour (pal::rose);
            g.fillEllipse (Rectangle<float> (4.0f, 4.0f).withCentre ({ c.x + side2 * fw * 0.44f, c.y }));
        }
        const auto plateR = Rectangle<float> (fw * 0.4f, fh * 0.46f).withCentre ({ f0.x + fw * 0.5f, f0.y + fh * 0.5f });
        Path plateP;
        plateP.addRoundedRectangle (plateR, 3.0f);
        fillInk (g, plateP, pal::brass, 1.0f);
        g.setColour (pal::ink);
        g.setFont (juce::Font (juce::FontOptions (jlimit (7.0f, 10.0f, plateR.getHeight() * 0.55f), juce::Font::bold)));
        g.drawText (spring > 0.6f ? juce::String::fromUTF8 ("FJÄDERN TAR SLUT") : juce::String ("UPPDRAGEN"), plateR, Justification::centred);
    }
    // Fötter.
    for (float u : { 0.06f, 0.94f })
    {
        Path foot;
        foot.addEllipse (Rectangle<float> (12.0f, 7.0f).withCentre (P (u, 0.0f, -H).translated (0.0f, 2.0f)));
        fillInk (g, foot, pal::goldDark, 1.0f);
    }
    // Nyckeln på sidan: en fjärilsnyckel som står snett mindre ju mer fjädern har tagit slut.
    {
        const auto kc = P (1.0f, 0.5f, -H * 0.5f).translated (dx * 0.2f, 0.0f);
        Path shaft;
        shaft.addRectangle (kc.x, kc.y - 2.0f, 12.0f, 4.0f);
        fillInk (g, shaft, pal::goldDark, 1.0f);
        const float ang = pi * (0.1f + 0.5f * (1.0f - spring));
        Path key;
        key.addEllipse (-7.0f, -16.0f, 14.0f, 14.0f);
        key.addEllipse (-7.0f, 2.0f, 14.0f, 14.0f);
        key.addRectangle (-2.0f, -4.0f, 4.0f, 8.0f);
        key.applyTransform (AffineTransform::scale (0.6f, 1.0f).rotated (ang).translated (kc.translated (16.0f, 0.0f)));
        fillInk (g, key, pal::brass, 1.2f);
    }
}

} // namespace krang::ui::jugend
