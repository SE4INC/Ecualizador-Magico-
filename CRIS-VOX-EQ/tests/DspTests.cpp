#include "dsp/Equalizer.h"
#include "dsp/Spectrum.h"
#include "Presets.h"
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <new>
#include <memory>
#include <random>
#include <stdexcept>
#if defined(_MSC_VER)
#include <malloc.h>
#endif
namespace { std::atomic<bool> measuring {false};std::atomic<int> allocations {0}; }
void* operator new(std::size_t n) { if(measuring.load())allocations.fetch_add(1);if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc(); }
void* operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
void operator delete[](void* p,std::size_t) noexcept {std::free(p);}
void* operator new(std::size_t n,std::align_val_t alignment)
{
    if(measuring.load())allocations.fetch_add(1);const auto a=static_cast<std::size_t>(alignment);
#if defined(_MSC_VER)
    void* p=_aligned_malloc(n?n:1,a);
#else
    void* p=std::aligned_alloc(a,((std::max<std::size_t>(1,n)+a-1)/a)*a);
#endif
    if(p)return p;throw std::bad_alloc();
}
void* operator new[](std::size_t n,std::align_val_t a){return ::operator new(n,a);}
void operator delete(void* p,std::align_val_t) noexcept
{
#if defined(_MSC_VER)
    _aligned_free(p);
#else
    std::free(p);
#endif
}
void operator delete[](void* p,std::align_val_t a) noexcept {::operator delete(p,a);}
void operator delete(void* p,std::size_t,std::align_val_t a) noexcept {::operator delete(p,a);}
void operator delete[](void* p,std::size_t,std::align_val_t a) noexcept {::operator delete(p,a);}
namespace
{
void require(bool condition,const char* text){if(!condition)throw std::runtime_error(text);}
double sineGain(const cris::Settings& s,double frequency,double rate)
{
    cris::Equalizer eq;eq.prepare(rate);eq.setSettings(s);std::array<float,256> data {};float* ptr[]{data.data()};
    double in=0,out=0;const int blocks=static_cast<int>(rate*1.5/256);
    for(int b=0;b<blocks;++b)
    {
        for(int i=0;i<256;++i)data[i]=static_cast<float>(0.1*std::sin(2*cris::pi*frequency*(b*256+i)/rate));
        if(b>blocks/2)for(float x:data)in+=x*x;
        eq.process(ptr,1,256);if(b>blocks/2)for(float x:data)out+=x*x;
    }
    return 10*std::log10(out/in);
}
}
int main()
{
    try
    {
        for(double rate:{44100.0,48000.0,96000.0})
        {
            cris::Settings s;s.match=false;
            require(std::abs(sineGain(s,1000,rate))<0.00001,"Neutral EQ not transparent");
            s.bands[2]={1000,6,1,cris::FilterType::bell,true};
            require(std::abs(sineGain(s,1000,rate)-6)<0.03,"Bell gain incorrect");
            s.bands[2].gain=-6;require(std::abs(sineGain(s,1000,rate)+6)<0.03,"Bell cut incorrect");
            s=cris::Settings{};s.match=false;s.bands[0].enabled=true;s.bands[0].frequency=100;
            require(sineGain(s,25,rate)<-23,"High pass slope incorrect");
            require(std::abs(sineGain(s,100,rate)+3.0103)<0.05,"High pass cutoff incorrect");
            for(auto type:{cris::FilterType::bell,cris::FilterType::lowShelf,cris::FilterType::highShelf,cris::FilterType::highPass,cris::FilterType::lowPass,cris::FilterType::notch})
                for(double freq:{20.0,280.0,10000.0,20000.0})
                {
                    const auto c=cris::Coefficients::design({freq,12,12,type,true},rate);
                    require(std::isfinite(c.magnitude(1000,rate)),"Invalid coefficient response");
                    require(std::abs(c.a2)<1 && 1+c.a1+c.a2>0 && 1-c.a1+c.a2>0,"Unstable denominator");
                }
        }
        {
            cris::Settings s;s.match=true;s.bands[2]={1000,6,1,cris::FilterType::bell,true};
            require(std::abs(sineGain(s,1000,48000))<1.5,"Level matching not converging");
            s.bypass=true;require(std::abs(sineGain(s,1000,48000))<0.001,"Bypass changes level");
        }
        auto tap=std::make_unique<cris::AudioTap>();
        std::array<float,2048> input {},output {};
        for(int i=0;i<2048;++i){input[i]=static_cast<float>(i);output[i]=-input[i];}
        tap->push(input.data(),output.data(),2048);std::array<cris::AudioPair,2048> pairs {};
        require(tap->pop(pairs.data(),2048)==2048,"FIFO size incorrect");
        for(int i=0;i<2048;++i)require(pairs[i].input==input[i] && pairs[i].output==output[i],"FIFO corrupt");
        cris::Spectrum fft;std::array<float,cris::fftSize> tone {};
        for(int i=0;i<cris::fftSize;++i)tone[i]=static_cast<float>(0.2*std::sin(2*cris::pi*1000*i/48000));
        const auto spectrum=fft.calculate(tone.data(),tone.data(),48000);
        require(spectrum.regionDb[3]>spectrum.regionDb[0]+20,"FFT spectral region incorrect");
        require(cris::recommend({-30,-15,-30,-30,-40,-50,-60},0,-20).front().frequency==280,"Mud rule incorrect");
        require(cris::recommend({-30,-15,-30,-30,-40,-50,-60},0,-80).empty(),"Silence gets suggestions");
        require(cris::presets().size()==7,"Missing presets");
        require(cris::presets()[1].settings.bands[2].gain==-2,"Preset does not modify real band");
        for(double rate:{44100.0,48000.0,96000.0})for(int blockSize:{64,128,2048})
        {
            cris::Equalizer eq;eq.prepare(rate);auto settings=cris::presets()[1].settings;
            std::array<std::array<float,2048>,2> buffers {};float* channels[]{buffers[0].data(),buffers[1].data()};
            for(int i=0;i<2048;++i)buffers[0][i]=buffers[1][i]=static_cast<float>(0.1*std::sin(2*cris::pi*200*i/rate));
            allocations.store(0);measuring.store(true);eq.setSettings(settings);eq.process(channels,2,blockSize);measuring.store(false);
            require(allocations.load()==0,"Allocation on prepared EQ callback");
            const int blocks=static_cast<int>(rate/blockSize);
            const auto start=std::chrono::steady_clock::now();
            allocations.store(0);measuring.store(true);
            for(int block=0;block<blocks;++block)
            {
                settings.bands[5].gain=block%2?2.0:1.5;eq.setSettings(settings);
                for(int i=0;i<blockSize;++i)buffers[0][i]=buffers[1][i]=static_cast<float>(0.1*std::sin(2*cris::pi*200*(block*blockSize+i)/rate));
                eq.process(channels,2,blockSize);
            }
            measuring.store(false);const auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
            require(allocations.load()==0,"Allocation during automated EQ processing");
            require(elapsed<1.0,"EQ slower than real time");
            std::cout<<"EQ stereo "<<rate<<" Hz / "<<blockSize<<" samples: "<<elapsed*1000<<" ms per ~1s incl. test sine generation; allocations="<<allocations.load()<<"\n";
        }
        {
            cris::Equalizer eq;eq.prepare(96000);cris::Settings s;s.match=false;
            std::mt19937 rng(7);std::uniform_real_distribution<double> frequency(30,18000),gain(-8,8),q(0.3,4);
            std::array<float,128> samples {};float* p[]{samples.data()};
            for(int iteration=0;iteration<500;++iteration)
            {
                for(auto& band:s.bands)band={frequency(rng),gain(rng),q(rng),cris::FilterType::bell,true};eq.setSettings(s);
                for(int i=0;i<128;++i)samples[i]=0.02f*static_cast<float>(std::sin((iteration*128+i)*0.2));
                eq.process(p,1,128);for(float x:samples)require(std::isfinite(x) && std::abs(x)<100,"Automation instability");
            }
        }
        std::cout<<"PASS: filter response, stability, neutral, bypass, matching, FFT, rules, FIFO, presets and allocation checks.\n";return 0;
    }
    catch(const std::exception& e){measuring.store(false);std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
