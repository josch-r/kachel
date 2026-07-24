#include "oklch.h"

#include <cmath>

static float srgb_encode(float c)
{
    if (c < 0.0f)
        c = 0.0f;
    if (c > 1.0f)
        c = 1.0f;
    return c <= 0.0031308f ? 12.92f * c : 1.055f * powf(c, 1.0f / 2.4f) - 0.055f;
}

rgbf oklch_to_srgb(const oklch &c)
{
    float h = c.H * (float)M_PI / 180.0f;
    float a = c.C * cosf(h);
    float b = c.C * sinf(h);

    float l_ = c.L + 0.3963377774f * a + 0.2158037573f * b;
    float m_ = c.L - 0.1055613458f * a - 0.0638541728f * b;
    float s_ = c.L - 0.0894841775f * a - 1.2914855480f * b;
    float l = l_ * l_ * l_, m = m_ * m_ * m_, s = s_ * s_ * s_;

    return {
        srgb_encode(4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s),
        srgb_encode(-1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s),
        srgb_encode(-0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s)};
}

oklch oklab_lerp(const oklch &a, const oklch &b, float t)
{
    float ha = a.H * (float)M_PI / 180.0f, hb = b.H * (float)M_PI / 180.0f;
    float aa = a.C * cosf(ha), ab = a.C * sinf(ha);
    float ba = b.C * cosf(hb), bb = b.C * sinf(hb);
    float L = a.L + (b.L - a.L) * t;
    float la = aa + (ba - aa) * t;
    float lb = ab + (bb - ab) * t;
    float C = sqrtf(la * la + lb * lb);
    float H = atan2f(lb, la) * 180.0f / (float)M_PI;
    if (H < 0)
        H += 360.0f;
    return {L, C, H};
}

oklch oklch_lerp(const oklch &a, const oklch &b, float t)
{
    // shortest-arc hue interpolation
    float dh = b.H - a.H;
    if (dh > 180.0f)
        dh -= 360.0f;
    if (dh < -180.0f)
        dh += 360.0f;
    float h = a.H + dh * t;
    if (h < 0.0f)
        h += 360.0f;
    if (h >= 360.0f)
        h -= 360.0f;
    return {a.L + (b.L - a.L) * t, a.C + (b.C - a.C) * t, h};
}
