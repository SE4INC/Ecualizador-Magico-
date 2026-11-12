#pragma once
#include "PluginProcessor.h"
#include <functional>
class EQGraph final : public juce::Component
{
public:
    explicit EQGraph(CrisVoxEQProcessor& p);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&,const juce::MouseWheelDetails&) override;
    void setSelected(int index) { selected=juce::jlimit(0,7,index);repaint(); }
    void setSpectrum(std::shared_ptr<const AnalysisResult> value) { result=std::move(value);repaint(); }
    std::function<void(int)> onSelect;
private:
    juce::Rectangle<float> area() const;
    float frequencyX(double hz) const;
    double xFrequency(float x) const;
    float gainY(double db) const;
    double yGain(float y) const;
    juce::Point<float> node(const cris::Band&) const;
    CrisVoxEQProcessor& processor;
    std::shared_ptr<const AnalysisResult> result;
    int selected=2;
    bool dragging=false;
};
