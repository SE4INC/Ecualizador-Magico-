#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "dsp/Equalizer.h"
#include "AnalysisWorker.h"
#include "Presets.h"

class CrisVoxEQProcessor final : public juce::AudioProcessor,private juce::AudioProcessorValueTreeState::Listener
{
public:
    CrisVoxEQProcessor();
    ~CrisVoxEQProcessor() override;
    void prepareToPlay(double rate,int blockSize) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "CRIS VOX EQ"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }
    int getNumPrograms() override { return 7; }
    int getCurrentProgram() override { return program.load(); }
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    cris::Settings settings() const;
    cris::Settings audibleSettings() const;
    static juce::String bandId(int band,const char* field);
    void setParameter(const juce::String&,float,bool gesture=true);
    void writeSettings(const cris::Settings&);
    void analyse();
    int freeBand() const;
    bool preview(const cris::Suggestion&);
    bool apply(const cris::Suggestion&);
    void stopPreview() { previewBand.store(-1); }
    bool isPreviewing() const { return previewBand.load()>=0; }
    int currentBank() const { return bank.load(); }
    void selectBank(int);
    void copyBank();
    std::uint64_t revision() const { return settingsRevision.load(); }
    juce::AudioProcessorValueTreeState state;
    AnalysisWorker analyser;
    std::atomic<float> inputPeak {0},outputPeak {0},matchDb {0};
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();
    void process(juce::AudioBuffer<float>&,bool);
    void parameterChanged(const juce::String&,float) override;
    struct RawBand { std::atomic<float>* frequency=nullptr;std::atomic<float>* gain=nullptr;std::atomic<float>* q=nullptr;std::atomic<float>* type=nullptr;std::atomic<float>* enabled=nullptr; };
    std::array<RawBand,8> raw;
    std::atomic<float>* output=nullptr;std::atomic<float>* match=nullptr;std::atomic<float>* bypass=nullptr;std::atomic<float>* context=nullptr;
    cris::Equalizer eq;
    std::atomic<int> program {0},previewBand {-1},bank {0};
    std::atomic<double> previewFrequency {280},previewGain {-2},previewQ {1.2};
    std::atomic<std::uint64_t> settingsRevision {0},previewRevision {0};
    mutable std::mutex bankMutex;
    std::array<juce::ValueTree,2> banks;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrisVoxEQProcessor)
};
