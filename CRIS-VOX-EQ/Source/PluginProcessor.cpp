#include "PluginProcessor.h"
#include "PluginEditor.h"
juce::String CrisVoxEQProcessor::bandId(int n,const char* name) { return "b"+juce::String(n+1)+"_"+name; }
juce::AudioProcessorValueTreeState::ParameterLayout CrisVoxEQProcessor::makeLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout result;
    auto add=[&](juce::String id,juce::String name,juce::NormalisableRange<float> range,float initial)
    { result.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,range,initial)); };
    cris::Settings defaults;
    for(int b=0;b<8;++b)
    {
        const auto& v=defaults.bands[b];const auto name="Band "+juce::String(b+1)+" ";
        auto frequency=juce::NormalisableRange<float>(20,20000,0.1f);frequency.setSkewForCentre(1000);
        add(bandId(b,"freq"),name+"Frequency",frequency,static_cast<float>(v.frequency));
        add(bandId(b,"gain"),name+"Gain",{ -12,12,0.01f },0);
        auto q=juce::NormalisableRange<float>(0.15f,12,0.01f);q.setSkewForCentre(1.2f);
        add(bandId(b,"q"),name+"Q",q,static_cast<float>(v.q));
        result.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{bandId(b,"type"),1},name+"Type",
            juce::StringArray{"Campana","Low Shelf","High Shelf","Pasa Altos","Pasa Bajos","Notch"},static_cast<int>(v.type)));
        result.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{bandId(b,"on"),1},name+"Enabled",v.enabled));
    }
    add("output","Output (dB)",{-24,12,0.01f},0);
    result.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"match",1},"Igualar volumen",true));
    result.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"bypass",1},"Bypass",false));
    result.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"context",1},"Contexto vocal",
        juce::StringArray{"Rap - voz grave","Claridad y presencia","Canto natural"},0));
    return result;
}
CrisVoxEQProcessor::CrisVoxEQProcessor()
    : AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true)
                                     .withOutput("Output",juce::AudioChannelSet::stereo(),true)),
      state(*this,nullptr,"CRIS_EQ_PARAMETERS",makeLayout())
{
    for(int i=0;i<8;++i)
    {
        raw[i]={state.getRawParameterValue(bandId(i,"freq")),state.getRawParameterValue(bandId(i,"gain")),
            state.getRawParameterValue(bandId(i,"q")),state.getRawParameterValue(bandId(i,"type")),state.getRawParameterValue(bandId(i,"on"))};
        for(auto field:{"freq","gain","q","type","on"}) state.addParameterListener(bandId(i,field),this);
    }
    output=state.getRawParameterValue("output");match=state.getRawParameterValue("match");
    bypass=state.getRawParameterValue("bypass");context=state.getRawParameterValue("context");
    for(auto id:{"output","match","bypass","context"}) state.addParameterListener(id,this);
    banks[0]=state.copyState();banks[1]=banks[0].createCopy();
}
CrisVoxEQProcessor::~CrisVoxEQProcessor()
{
    for(int i=0;i<8;++i) for(auto field:{"freq","gain","q","type","on"}) state.removeParameterListener(bandId(i,field),this);
    for(auto id:{"output","match","bypass","context"}) state.removeParameterListener(id,this);
}
void CrisVoxEQProcessor::parameterChanged(const juce::String&,float) { settingsRevision.fetch_add(1); }
cris::Settings CrisVoxEQProcessor::settings() const
{
    cris::Settings s;
    for(std::size_t i=0;i<8;++i) s.bands[i]={raw[i].frequency->load(),raw[i].gain->load(),raw[i].q->load(),
        static_cast<cris::FilterType>(static_cast<int>(raw[i].type->load())),raw[i].enabled->load()>0.5f};
    s.outputDb=output->load();s.match=match->load()>0.5f;s.bypass=bypass->load()>0.5f;return s;
}
void CrisVoxEQProcessor::prepareToPlay(double rate,int)
{ stopPreview();settingsRevision.fetch_add(1);eq.prepare(rate);eq.setSettings(settings());analyser.prepare(rate);setLatencySamples(0); }
cris::Settings CrisVoxEQProcessor::audibleSettings() const
{
    auto s=settings();const int audition=previewBand.load();
    if(audition>=0 && audition<8 && previewRevision.load()==revision())
        s.bands[static_cast<std::size_t>(audition)]={previewFrequency.load(),previewGain.load(),previewQ.load(),cris::FilterType::bell,true};
    return s;
}
bool CrisVoxEQProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{ const auto o=l.getMainOutputChannelSet();return (o==juce::AudioChannelSet::mono() || o==juce::AudioChannelSet::stereo()) && o==l.getMainInputChannelSet(); }
void CrisVoxEQProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer& m)
{ juce::ScopedNoDenormals nd;m.clear();process(b,false); }
void CrisVoxEQProcessor::processBlockBypassed(juce::AudioBuffer<float>& b,juce::MidiBuffer& m)
{ juce::ScopedNoDenormals nd;m.clear();process(b,true); }
void CrisVoxEQProcessor::process(juce::AudioBuffer<float>& b,bool hostBypass)
{
    if(b.getNumSamples()==0 || b.getNumChannels()==0) return;
    auto s=audibleSettings();
    s.bypass=s.bypass || hostBypass;eq.setSettings(s);
    inputPeak.store(b.getMagnitude(0,b.getNumSamples()));
    if(analyser.needsAudio())
    {
        std::array<float,128> input {};
        for(int offset=0;offset<b.getNumSamples();offset+=128)
        {
            const int count=std::min(128,b.getNumSamples()-offset);
            std::copy_n(b.getReadPointer(0,offset),count,input.data());
            float* channels[2] {b.getWritePointer(0,offset),b.getWritePointer(b.getNumChannels()>1?1:0,offset)};
            eq.process(channels,b.getNumChannels(),count);analyser.push(input.data(),b.getReadPointer(0,offset),count);
        }
    }
    else eq.process(b.getArrayOfWritePointers(),b.getNumChannels(),b.getNumSamples());
    outputPeak.store(b.getMagnitude(0,b.getNumSamples()));matchDb.store(static_cast<float>(eq.matchGainDb()));
}
void CrisVoxEQProcessor::setParameter(const juce::String& id,float value,bool gesture)
{
    if(auto* p=state.getParameter(id)) { if(gesture)p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(value));if(gesture)p->endChangeGesture(); }
}
void CrisVoxEQProcessor::writeSettings(const cris::Settings& s)
{
    for(int i=0;i<8;++i)
    {
        const auto& b=s.bands[i];setParameter(bandId(i,"freq"),static_cast<float>(b.frequency),false);
        setParameter(bandId(i,"gain"),static_cast<float>(b.gain),false);setParameter(bandId(i,"q"),static_cast<float>(b.q),false);
        setParameter(bandId(i,"type"),static_cast<float>(b.type),false);setParameter(bandId(i,"on"),b.enabled?1.0f:0.0f,false);
    }
}
void CrisVoxEQProcessor::setCurrentProgram(int index)
{ index=juce::jlimit(0,6,index);program.store(index);stopPreview();writeSettings(cris::presets()[static_cast<std::size_t>(index)].settings); }
const juce::String CrisVoxEQProcessor::getProgramName(int index) { return cris::presets()[static_cast<std::size_t>(juce::jlimit(0,6,index))].name; }
void CrisVoxEQProcessor::analyse() { stopPreview();analyser.request(static_cast<int>(context->load()),revision()); }
int CrisVoxEQProcessor::freeBand() const
{
    const auto s=settings();
    for(int i=2;i<8;++i) if(!s.bands[i].enabled || (s.bands[i].type==cris::FilterType::bell && std::abs(s.bands[i].gain)<0.05)) return i;
    for(int i=0;i<2;++i) if(!s.bands[i].enabled) return i;
    return -1;
}
bool CrisVoxEQProcessor::preview(const cris::Suggestion& suggestion)
{
    const int bandIndex=freeBand();if(bandIndex<0) return false;
    stopPreview();previewFrequency.store(suggestion.frequency);previewGain.store(suggestion.gain);previewQ.store(suggestion.q);
    previewRevision.store(revision());previewBand.store(bandIndex);return true;
}
bool CrisVoxEQProcessor::apply(const cris::Suggestion& suggestion)
{
    const int i=freeBand();if(i<0) return false;stopPreview();
    setParameter(bandId(i,"freq"),static_cast<float>(suggestion.frequency));setParameter(bandId(i,"gain"),static_cast<float>(suggestion.gain));
    setParameter(bandId(i,"q"),static_cast<float>(suggestion.q));setParameter(bandId(i,"type"),0);setParameter(bandId(i,"on"),1);return true;
}
void CrisVoxEQProcessor::selectBank(int next)
{
    next=juce::jlimit(0,1,next);if(next==bank.load())return;stopPreview();
    std::lock_guard<std::mutex> lock(bankMutex);
    banks[bank.load()]=state.copyState();state.replaceState(banks[next].createCopy());bank.store(next);
}
void CrisVoxEQProcessor::copyBank()
{ std::lock_guard<std::mutex> lock(bankMutex);banks[1-bank.load()]=state.copyState(); }
void CrisVoxEQProcessor::getStateInformation(juce::MemoryBlock& data)
{
    std::lock_guard<std::mutex> lock(bankMutex);
    juce::ValueTree root("CRIS_VOX_EQ_STATE");root.setProperty("program",program.load(),nullptr);root.setProperty("bank",bank.load(),nullptr);
    const auto current=state.copyState();root.appendChild(current.createCopy(),nullptr);
    for(int i=0;i<2;++i) { juce::ValueTree slot(i==0?"A":"B");slot.appendChild((i==bank.load()?current:banks[i]).createCopy(),nullptr);root.appendChild(slot,nullptr); }
    if(auto xml=root.createXml())copyXmlToBinary(*xml,data);
}
void CrisVoxEQProcessor::setStateInformation(const void* data,int size)
{
    if(auto xml=getXmlFromBinary(data,size))
    {
        const auto root=juce::ValueTree::fromXml(*xml);if(!root.hasType("CRIS_VOX_EQ_STATE"))return;
        const auto parameters=root.getChildWithName("CRIS_EQ_PARAMETERS");if(!parameters.isValid())return;
        std::lock_guard<std::mutex> lock(bankMutex);
        stopPreview();state.replaceState(parameters.createCopy());
        program.store(juce::jlimit(0,6,static_cast<int>(root.getProperty("program",0))));bank.store(juce::jlimit(0,1,static_cast<int>(root.getProperty("bank",0))));
        for(int i=0;i<2;++i) { const auto saved=root.getChildWithName(i==0?"A":"B").getChild(0);banks[i]=saved.isValid()?saved.createCopy():parameters.createCopy(); }
    }
}
juce::AudioProcessorEditor* CrisVoxEQProcessor::createEditor() { return new CrisVoxEQEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new CrisVoxEQProcessor(); }
