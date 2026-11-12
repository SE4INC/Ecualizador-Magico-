#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>

namespace cris
{
constexpr double pi = 3.14159265358979323846;
enum class FilterType { bell, lowShelf, highShelf, highPass, lowPass, notch };
struct Band
{
    double frequency = 1000.0, gain = 0.0, q = 1.0;
    FilterType type = FilterType::bell;
    bool enabled = true;
};
struct Settings
{
    std::array<Band, 8> bands {{
        {70,0,0.707,FilterType::highPass,false}, {130,0,0.707,FilterType::lowShelf,true},
        {280,0,1.2,FilterType::bell,true}, {800,0,1.2,FilterType::bell,true},
        {1500,0,1.2,FilterType::bell,true}, {3000,0,0.9,FilterType::bell,true},
        {5500,0,1.5,FilterType::bell,true}, {10000,0,0.707,FilterType::highShelf,true}}};
    double outputDb = 0.0;
    bool match = true, bypass = false;
};
struct Coefficients
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    static Coefficients design(Band band, double sampleRate);
    double magnitude(double frequency, double sampleRate) const;
};
double responseDb(const Settings& settings, double frequency, double sampleRate);
class Equalizer
{
public:
    void prepare(double sampleRate);
    void setSettings(const Settings& settings);
    void reset();
    void process(float* const* channels, int channelCount, int sampleCount);
    double matchGainDb() const { return 20.0 * std::log10(std::max(1.0e-9, matchGain)); }
private:
    struct Filter
    {
        Coefficients current, target, step;
        std::array<double, 2> z1 {}, z2 {};
        int remaining = 0;
        void set(Coefficients c, bool immediate);
        void advance();
        double process(double x, int channel);
    };
    std::array<Filter, 8> filters;
    Settings settings;
    double rate = 48000.0, output = 1.0, wet = 1.0, matchGain = 1.0, matchTarget = 1.0;
    double energyIn = 0.0, energyWet = 0.0, energyPole = 0.0, smoothing = 0.0, matchSmoothing = 0.0;
    int controlCounter = 0;
    bool first = true;
};
}
