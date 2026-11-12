#pragma once
#include "Equalizer.h"
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>
namespace cris
{
constexpr int fftSize=4096, displayBins=256;
struct AudioPair { float input=0,output=0; };
class AudioTap
{
public:
    void push(const float* input,const float* output,int count);
    int pop(AudioPair* dest,int capacity);
    void discard();
    std::uint64_t lost() const { return dropped.load(); }
private:
    static constexpr std::uint32_t capacity=32768;
    std::array<AudioPair,capacity> storage {};
    alignas(64) std::atomic<std::uint32_t> write {0};
    alignas(64) std::atomic<std::uint32_t> read {0};
    std::atomic<std::uint64_t> dropped {0};
};
struct SpectrumFrame
{
    SpectrumFrame() { input.fill(-120);output.fill(-120);regionDb.fill(-120); }
    std::array<float,displayBins> input {},output {};
    std::array<double,7> regionDb {};
    double rmsDb=-120,peak=0;
};
class Spectrum
{
public:
    Spectrum();
    SpectrumFrame calculate(const float* input,const float* output,double sampleRate);
private:
    std::array<std::complex<float>,fftSize> buffer {};
    std::array<std::complex<float>,fftSize/2> twiddles {};
    std::array<float,fftSize> window {};
    std::array<float,fftSize/2+1> power {};
    float windowSum=0;
    void transform(const float* input);
    void display(std::array<float,displayBins>& result,double sampleRate) const;
};
struct Suggestion
{
    std::string title,description;
    double frequency=280,gain=-2,q=1.2;
};
std::vector<Suggestion> recommend(const std::array<double,7>& regionDb,int context,double rmsDb);
}
