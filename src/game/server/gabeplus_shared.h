#include "cbase.h"
#include "util.h"

////////////////////////
//  Display a message //
////////////////////////
inline void HudText(CBasePlayer* pPlayer,
                    const char* pszText,
                    int r = 255, int g = 255, int b = 255,
                    float x = -1.0f, float y = 0.25f,
                    float fadeIn = 0.1f, float fadeOut = 0.5f,
                    float hold = 2.0f, int channel = 1,
                    int effect = 0)
{
    if (!pPlayer || !pszText)
        return;

    hudtextparms_t params;
    params.channel = channel;
    params.x = x;
    params.y = y;
    params.effect = effect;
    params.r1 = r;
    params.g1 = g;
    params.b1 = b;
    params.a1 = 255;
    params.r2 = 255;
    params.g2 = 255;
    params.b2 = 255;
    params.a2 = 255;
    params.fadeinTime = fadeIn;
    params.fadeoutTime = fadeOut;
    params.holdTime = hold;
    params.fxTime = 0.5f;

    UTIL_HudMessage(pPlayer, params, pszText);
}