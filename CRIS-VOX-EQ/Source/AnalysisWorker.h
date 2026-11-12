#pragma once
#include "dsp/Spectrum.h"
#include <juce_core/juce_core.h>
#include <memory>
#include <mutex>

struct AnalysisResult
{
    cris::SpectrumFrame spectrum;
    std::vector<cris::Suggestion> suggestions;
    juce::String message {"Reproduce tu voz y pulsa ANALIZAR."};
    std::uint64_t resultId=0,settingsRevision=0;
    bool complete=false;
};
class AnalysisWorker final : private juce::Thread
{
public:
    AnalysisWorker();
    ~AnalysisWorker() override;
    void prepare(double sampleRate);
    void setVisible(bool value) { visible.store(value); }
    bool needsAudio() const { return visible.load() || analysing.load(); }
    void push(const float* input,const float* output,int count) { tap.push(input,output,count); }
    void request(int context,std::uint64_t revision);
    bool busy() const { return analysing.load(); }
    std::shared_ptr<const AnalysisResult> snapshot() const;
private:
    void run() override;
    cris::AudioTap tap;
    cris::Spectrum fft;
    std::atomic<bool> visible {false},analysing {false};
    std::atomic<double> sampleRate {48000};
    std::atomic<std::uint64_t> rateRevision {0},requestId {0},settingsRevision {0};
    std::atomic<int> requestedContext {0};
    mutable std::mutex mutex;
    std::shared_ptr<const AnalysisResult> published;
};
