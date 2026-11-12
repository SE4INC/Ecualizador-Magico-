#pragma once
#include "dsp/Equalizer.h"
#include <array>
namespace cris
{
struct Preset { const char* name;Settings settings; };
inline const std::array<Preset,7>& presets()
{
    static const auto values=[]
    {
        std::array<Preset,7> v {{{"NEUTRO",{}},{"RAP GRAVE - CLARIDAD",{}},{"RAP - PRESENCIA",{}},
            {"VOZ OSCURA - ABRIR",{}},{"VOZ NASAL - SUAVIZAR",{}},{"VOZ BRILLANTE - CONTROL",{}},{"CANTO MELODICO - SUAVE",{}}}};
        v[1].settings.bands[0].enabled=true;v[1].settings.bands[0].frequency=65;
        v[1].settings.bands[2].gain=-2;v[1].settings.bands[5].gain=1.5;
        v[2].settings.bands[0].enabled=true;v[2].settings.bands[0].frequency=75;
        v[2].settings.bands[2].gain=-1.5;v[2].settings.bands[5].gain=2;
        v[3].settings.bands[5].gain=1.5;v[3].settings.bands[7].gain=2;
        v[4].settings.bands[4].frequency=1400;v[4].settings.bands[4].gain=-2;v[4].settings.bands[4].q=1.5;
        v[5].settings.bands[6].gain=-2;v[5].settings.bands[6].frequency=5500;
        v[5].settings.bands[7].gain=-1;
        v[6].settings.bands[0].enabled=true;v[6].settings.bands[0].frequency=60;
        v[6].settings.bands[2].gain=-1;v[6].settings.bands[7].gain=1;
        return v;
    }();
    return values;
}
}
