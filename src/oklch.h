#pragma once

#include <cstdint>

// OKLCH color math for the generative face (SPEC §5.16, §6).
// Reference OKLab transform, sRGB-encoded, clamped.

struct oklch
{
    float L; // 0..1
    float C;
    float H; // degrees
};

struct rgbf
{
    float r, g, b; // linear-encoded sRGB 0..1 (gamma applied)
};

rgbf oklch_to_srgb(const oklch &c);
oklch oklch_lerp(const oklch &a, const oklch &b, float t);

// OKLab-space lerp (design rule: all blends pass near-gray, never rotate
// hue in LCH — low chroma everywhere makes this safe and band-free)
oklch oklab_lerp(const oklch &a, const oklch &b, float t);
