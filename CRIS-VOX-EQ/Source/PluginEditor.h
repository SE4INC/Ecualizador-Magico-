#pragma once
#include "PluginProcessor.h"
#include "ui/EQGraph.h"
class EQStyle final : public juce::LookAndFeel_V4
{
public:
    EQStyle();
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
};
class CrisVoxEQEditor final : public juce::AudioProcessorEditor,private juce::Timer
{
public:
    explicit CrisVoxEQEditor(CrisVoxEQProcessor&);
    ~CrisVoxEQEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void selectBand(int);
    void refreshAssistant();
    const cris::Suggestion* selectedSuggestion() const;
    CrisVoxEQProcessor& processor;
    EQStyle style;
    EQGraph graph;
    juce::ComboBox presets,context,type,recommendations;
    std::array<juce::TextButton,8> bandButtons;
    std::array<juce::Slider,4> knobs;
    std::array<juce::Label,4> knobLabels;
    juce::TextButton a {"A"},b {"B"},copy {"COPIAR A/B"},analyse {"ANALIZAR"},listen {"ESCUCHAR"},apply {"APLICAR"},dismiss {"DESCARTAR"};
    juce::ToggleButton bypass {"BYPASS"},match {"IGUALAR VOLUMEN"},enabled {"BANDA ACTIVA"};
    juce::Label assistantStatus,description,adjustment,footer,subtitle;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,4> sliderAttachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttachment,contextAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enabledAttachment,matchAttachment,bypassAttachment;
    std::shared_ptr<const AnalysisResult> analysis;
    std::array<bool,3> discarded {};
    std::uint64_t shownResult=0;
    int selected=2,shownProgram=-1;
    bool priorPreview=false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrisVoxEQEditor)
};
