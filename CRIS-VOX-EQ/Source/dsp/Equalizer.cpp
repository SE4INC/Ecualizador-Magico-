#include "Equalizer.h"
namespace cris
{
Coefficients Coefficients::design(Band band, double sampleRate)
{
    if (!band.enabled) return {};
    const double rate = std::clamp(sampleRate, 8000.0, 384000.0);
    const double f = std::clamp(band.frequency, 20.0, std::min(20000.0, rate * 0.45));
    const double q = std::clamp(band.q, 0.15, 12.0);
    const double gain = std::clamp(band.gain, -12.0, 12.0);
    if (std::abs(gain) < 0.00001 && (band.type == FilterType::bell || band.type == FilterType::lowShelf || band.type == FilterType::highShelf)) return {};
    const double w = 2 * pi * f / rate, c = std::cos(w), s = std::sin(w);
    const double a = std::pow(10.0, gain / 40.0), alpha = s / (2 * q);
    double b0 = 1, b1 = 0, b2 = 0, a0 = 1, a1 = 0, a2 = 0;
    switch (band.type)
    {
        case FilterType::bell:
            b0=1+alpha*a; b1=-2*c; b2=1-alpha*a; a0=1+alpha/a; a1=-2*c; a2=1-alpha/a; break;
        case FilterType::highPass:
            b0=(1+c)/2; b1=-(1+c); b2=b0; a0=1+alpha; a1=-2*c; a2=1-alpha; break;
        case FilterType::lowPass:
            b0=(1-c)/2; b1=1-c; b2=b0; a0=1+alpha; a1=-2*c; a2=1-alpha; break;
        case FilterType::notch:
            b0=1; b1=-2*c; b2=1; a0=1+alpha; a1=-2*c; a2=1-alpha; break;
        case FilterType::lowShelf:
        {
            // Shelves use fixed slope S=1; Q belongs to bell/cut/notch filters.
            const double t = s * std::sqrt(2*a);
            b0=a*((a+1)-(a-1)*c+t); b1=2*a*((a-1)-(a+1)*c); b2=a*((a+1)-(a-1)*c-t);
            a0=(a+1)+(a-1)*c+t; a1=-2*((a-1)+(a+1)*c); a2=(a+1)+(a-1)*c-t; break;
        }
        case FilterType::highShelf:
        {
            const double t = s * std::sqrt(2*a);
            b0=a*((a+1)+(a-1)*c+t); b1=-2*a*((a-1)+(a+1)*c); b2=a*((a+1)+(a-1)*c-t);
            a0=(a+1)-(a-1)*c+t; a1=2*((a-1)-(a+1)*c); a2=(a+1)-(a-1)*c-t; break;
        }
    }
    return {b0/a0,b1/a0,b2/a0,a1/a0,a2/a0};
}
double Coefficients::magnitude(double frequency, double sampleRate) const
{
    const auto z = std::polar(1.0, -2*pi*frequency/sampleRate);
    return std::abs((b0+b1*z+b2*z*z)/(1.0+a1*z+a2*z*z));
}
double responseDb(const Settings& s, double f, double rate)
{
    double result = 0.0;
    for (auto band : s.bands) result += 20.0*std::log10(std::max(1.0e-12, Coefficients::design(band,rate).magnitude(f,rate)));
    return result;
}
void Equalizer::Filter::set(Coefficients c, bool immediate)
{
    if (c.b0==target.b0 && c.b1==target.b1 && c.b2==target.b2 && c.a1==target.a1 && c.a2==target.a2 && !immediate) return;
    target=c;
    if (immediate) { current=target; remaining=0; return; }
    constexpr double ramp=128.0;
    step={(target.b0-current.b0)/ramp,(target.b1-current.b1)/ramp,(target.b2-current.b2)/ramp,
          (target.a1-current.a1)/ramp,(target.a2-current.a2)/ramp};
    remaining=128;
}
void Equalizer::Filter::advance()
{
    if (remaining<=0) return;
    current.b0+=step.b0; current.b1+=step.b1; current.b2+=step.b2; current.a1+=step.a1; current.a2+=step.a2;
    if (--remaining==0) current=target;
}
double Equalizer::Filter::process(double x, int ch)
{
    const auto i=static_cast<std::size_t>(ch);
    const double y=current.b0*x+z1[i];
    z1[i]=current.b1*x-current.a1*y+z2[i]; z2[i]=current.b2*x-current.a2*y;
    if (std::abs(z1[i])<1.0e-24) z1[i]=0;
    if (std::abs(z2[i])<1.0e-24) z2[i]=0;
    return y;
}
void Equalizer::prepare(double sampleRate)
{
    rate=std::max(8000.0,sampleRate); smoothing=1-std::exp(-1/(rate*0.01));
    matchSmoothing=1-std::exp(-1/(rate*0.5));
    energyPole=std::exp(-1/(rate*0.75)); first=true; reset();
}
void Equalizer::reset()
{
    for (auto& f:filters) { f.z1.fill(0); f.z2.fill(0); }
    energyIn=energyWet=0; matchGain=matchTarget=1; wet=1; output=1; controlCounter=0;
}
void Equalizer::setSettings(const Settings& s)
{
    settings=s;
    for (std::size_t b=0;b<8;++b) filters[b].set(Coefficients::design(s.bands[b],rate),first);
    if (first) { wet=s.bypass?0.0:1.0; output=std::pow(10.0,s.outputDb/20.0); first=false; }
}
void Equalizer::process(float* const* data, int channels, int count)
{
    const double outTarget=std::pow(10.0,std::clamp(settings.outputDb,-24.0,12.0)/20.0);
    for (int i=0;i<count;++i)
    {
        for (auto& f:filters) f.advance();
        output+=smoothing*(outTarget-output); wet+=smoothing*((settings.bypass?0.0:1.0)-wet);
        matchGain+=matchSmoothing*(matchTarget-matchGain);
        double inPower=0, wetPower=0;
        for (int ch=0;ch<channels;++ch)
        {
            const double x=std::isfinite(data[ch][i])?data[ch][i]:0.0;
            double y=x; for (auto& filter:filters) y=filter.process(y,ch);
            inPower+=x*x; wetPower+=y*y;
            const double result=(x+wet*(y*matchGain-x))*output;
            data[ch][i]=std::isfinite(result)?static_cast<float>(result):0.0f;
        }
        energyIn=energyPole*energyIn+(1-energyPole)*inPower/channels;
        energyWet=energyPole*energyWet+(1-energyPole)*wetPower/channels;
        if (++controlCounter>=64)
        {
            controlCounter=0;
            matchTarget=settings.match && energyIn>1.0e-9 && energyWet>1.0e-12
                ? std::clamp(std::sqrt(energyIn/energyWet),0.501187,1.995262) : 1.0;
        }
    }
}
}
