#include "EQGraph.h"
namespace { const juce::Colour gold(0xffd9b76d),muted(0xff979ca6); }
EQGraph::EQGraph(CrisVoxEQProcessor& p):processor(p) { setMouseCursor(juce::MouseCursor::CrosshairCursor); }
juce::Rectangle<float> EQGraph::area() const { return {52,30,static_cast<float>(getWidth()-102),static_cast<float>(getHeight()-65)}; }
float EQGraph::frequencyX(double f) const { auto r=area();return r.getX()+static_cast<float>(std::log(std::clamp(f,20.0,20000.0)/20)/std::log(1000.0))*r.getWidth(); }
double EQGraph::xFrequency(float x) const { auto r=area();return std::clamp(20*std::pow(1000.0,(x-r.getX())/r.getWidth()),20.0,20000.0); }
float EQGraph::gainY(double db) const { auto r=area();return r.getCentreY()-static_cast<float>(db/24)*r.getHeight(); }
double EQGraph::yGain(float y) const { auto r=area();return std::clamp((r.getCentreY()-y)/r.getHeight()*24.0,-12.0,12.0); }
juce::Point<float> EQGraph::node(const cris::Band& b) const
{
    const bool cut=b.type==cris::FilterType::highPass || b.type==cris::FilterType::lowPass || b.type==cris::FilterType::notch;
    return {frequencyX(b.frequency),gainY(cut && b.enabled?-3.0:b.gain)};
}
void EQGraph::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff14161a));auto r=area();
    g.setFont(juce::FontOptions(11));
    for(int db=-12;db<=12;db+=3)
    {
        const auto y=gainY(db);g.setColour(db==0?juce::Colour(0xff55534b):juce::Colour(0xff303239));
        g.drawHorizontalLine(static_cast<int>(y),r.getX(),r.getRight());g.setColour(muted);
        g.drawText((db>0?"+":"")+juce::String(db)+" dB",0,static_cast<int>(y)-8,46,16,juce::Justification::centredRight);
    }
    for(double f:{20,50,100,200,500,1000,2000,5000,10000,20000})
    {
        const auto x=frequencyX(f);g.setColour(juce::Colour(0xff34363c));g.drawVerticalLine(static_cast<int>(x),r.getY(),r.getBottom());
        g.setColour(muted);g.drawText(f>=1000?juce::String(f/1000,0)+"k":juce::String(f,0),static_cast<int>(x)-22,static_cast<int>(r.getBottom())+9,44,18,juce::Justification::centred);
    }
    g.setColour(muted);g.setFont(juce::FontOptions(9));
    g.drawText("dBFS",static_cast<int>(r.getRight())+5,10,38,16,juce::Justification::centredLeft);
    for(int db:{0,-30,-60,-90})
    {
        const float y=r.getY()-static_cast<float>(db)/90*r.getHeight();
        g.drawText(juce::String(db),static_cast<int>(r.getRight())+5,static_cast<int>(y)-7,38,14,juce::Justification::centredLeft);
    }
    juce::Graphics::ScopedSaveState save(g);g.reduceClipRegion(r.toNearestInt());
    if(result)
    {
        auto spectrum=[&](const auto& values,juce::Colour colour,bool fill)
        {
            juce::Path path;path.startNewSubPath(r.getX(),r.getBottom());
            for(int i=0;i<cris::displayBins;++i)
            {
                const float x=r.getX()+static_cast<float>(i)*r.getWidth()/(cris::displayBins-1);
                const float y=r.getBottom()-juce::jlimit(0.0f,1.0f,(values[i]+90)/90)*r.getHeight();path.lineTo(x,y);
            }
            if(fill){path.lineTo(r.getRight(),r.getBottom());path.closeSubPath();g.setColour(colour.withAlpha(0.13f));g.fillPath(path);}
            else {g.setColour(colour.withAlpha(0.45f));g.strokePath(path,juce::PathStrokeType(1));}
        };
        spectrum(result->spectrum.input,muted,true);spectrum(result->spectrum.output,gold,false);
    }
    const auto settings=processor.audibleSettings();const double rate=processor.getSampleRate()>0?processor.getSampleRate():48000;
    std::array<cris::Coefficients,8> coefficients;
    for(std::size_t i=0;i<8;++i)coefficients[i]=cris::Coefficients::design(settings.bands[i],rate);
    juce::Path curve;
    for(int point=0;point<384;++point)
    {
        const double f=20*std::pow(1000.0,point/383.0);const auto z=std::polar(1.0,-2*cris::pi*f/rate);
        double magnitude=1;
        for(const auto& c:coefficients)magnitude*=std::abs((c.b0+c.b1*z+c.b2*z*z)/(1.0+c.a1*z+c.a2*z*z));
        const auto x=frequencyX(f),y=gainY(20*std::log10(std::max(1.0e-12,magnitude)));
        if(point==0)curve.startNewSubPath(x,y);else curve.lineTo(x,y);
    }
    g.setColour(gold);g.strokePath(curve,juce::PathStrokeType(2.2f));
    for(int i=0;i<8;++i)
    {
        const auto point=node(settings.bands[i]);const float radius=i==selected?9.5f:7.5f;
        g.setColour(settings.bands[i].enabled?gold:juce::Colour(0xff5b5e67));g.fillEllipse(point.x-radius,point.y-radius,radius*2,radius*2);
        g.setColour(juce::Colour(0xff17181c));g.setFont(juce::FontOptions(11,juce::Font::bold));
        g.drawText(juce::String(i+1),juce::Rectangle<float>(point.x-radius,point.y-radius,radius*2,radius*2).toNearestInt(),juce::Justification::centred);
    }
}
void EQGraph::mouseDown(const juce::MouseEvent& e)
{
    const auto s=processor.settings();float best=18;int hit=-1;
    for(int i=0;i<8;++i){const float distance=node(s.bands[i]).getDistanceFrom(e.position);if(distance<best){best=distance;hit=i;}}
    if(hit<0)return;selected=hit;if(onSelect)onSelect(selected);dragging=true;
    for(auto field:{"freq","gain"})processor.state.getParameter(CrisVoxEQProcessor::bandId(selected,field))->beginChangeGesture();
    processor.stopPreview();repaint();
}
void EQGraph::mouseDrag(const juce::MouseEvent& e)
{
    if(!dragging)return;const auto b=processor.settings().bands[selected];
    processor.setParameter(CrisVoxEQProcessor::bandId(selected,"freq"),static_cast<float>(xFrequency(e.position.x)),false);
    if(b.type==cris::FilterType::bell || b.type==cris::FilterType::lowShelf || b.type==cris::FilterType::highShelf)
        processor.setParameter(CrisVoxEQProcessor::bandId(selected,"gain"),static_cast<float>(yGain(e.position.y)),false);
    repaint();
}
void EQGraph::mouseUp(const juce::MouseEvent&)
{ if(dragging){for(auto field:{"freq","gain"})processor.state.getParameter(CrisVoxEQProcessor::bandId(selected,field))->endChangeGesture();dragging=false;} }
void EQGraph::mouseWheelMove(const juce::MouseEvent&,const juce::MouseWheelDetails& w)
{ const auto b=processor.settings().bands[selected];processor.setParameter(CrisVoxEQProcessor::bandId(selected,"q"),static_cast<float>(b.q*std::exp(w.deltaY)),true);repaint(); }
