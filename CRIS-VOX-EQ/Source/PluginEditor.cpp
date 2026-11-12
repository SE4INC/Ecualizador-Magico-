#include "PluginEditor.h"
namespace { const juce::Colour gold(0xffd9b76d),text(0xffeeeadf),muted(0xff9b9eaa),panel(0xff1c1e23); }
EQStyle::EQStyle()
{
    setColour(juce::ComboBox::backgroundColourId,panel);setColour(juce::ComboBox::textColourId,text);
    setColour(juce::ComboBox::outlineColourId,juce::Colour(0xff4c4b46));setColour(juce::PopupMenu::backgroundColourId,panel);
    setColour(juce::PopupMenu::textColourId,text);setColour(juce::PopupMenu::highlightedBackgroundColourId,juce::Colour(0xff6b5633));
    setColour(juce::TextButton::buttonColourId,juce::Colour(0xff2a2c32));setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff9a7d43));
    setColour(juce::TextButton::textColourOffId,text);setColour(juce::ToggleButton::textColourId,text);setColour(juce::ToggleButton::tickColourId,gold);
    setColour(juce::Slider::textBoxTextColourId,text);setColour(juce::Slider::textBoxOutlineColourId,juce::Colour(0xff4c4b46));
}
void EQStyle::drawRotarySlider(juce::Graphics& g,int x,int y,int width,int height,float position,float start,float end,juce::Slider&)
{
    const auto bounds=juce::Rectangle<float>(static_cast<float>(x),static_cast<float>(y),static_cast<float>(width),static_cast<float>(height)).reduced(9);
    const auto centre=bounds.getCentre();const float radius=std::min(bounds.getWidth(),bounds.getHeight())/2;
    juce::Path track,arc;track.addCentredArc(centre.x,centre.y,radius,radius,0,start,end,true);
    arc.addCentredArc(centre.x,centre.y,radius,radius,0,start,start+position*(end-start),true);
    g.setColour(juce::Colour(0xff3d3f46));g.strokePath(track,juce::PathStrokeType(3));g.setColour(gold);g.strokePath(arc,juce::PathStrokeType(3));
    auto face=juce::Rectangle<float>(centre.x-radius+7,centre.y-radius+7,(radius-7)*2,(radius-7)*2);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffbaa16c),face.getTopLeft(),juce::Colour(0xff3c3831),face.getBottomRight(),false));g.fillEllipse(face);
    g.setColour(gold.withAlpha(0.7f));g.drawEllipse(face,1);
    const float angle=start+position*(end-start);g.setColour(text);
    g.drawLine(centre.x+std::sin(angle)*radius*0.4f,centre.y-std::cos(angle)*radius*0.4f,
        centre.x+std::sin(angle)*radius*0.74f,centre.y-std::cos(angle)*radius*0.74f,2);
}
CrisVoxEQEditor::CrisVoxEQEditor(CrisVoxEQProcessor& p):AudioProcessorEditor(p),processor(p),graph(p)
{
    setLookAndFeel(&style);setResizable(true,true);setResizeLimits(1100,760,1700,1100);setSize(1340,860);
    addAndMakeVisible(graph);graph.onSelect=[this](int n){selectBand(n);};
    for(auto* c:{&presets,&context,&type,&recommendations})addAndMakeVisible(c);
    for(int i=0;i<processor.getNumPrograms();++i)presets.addItem(processor.getProgramName(i),i+1);
    presets.onChange=[this]{processor.setCurrentProgram(presets.getSelectedId()-1);};
    context.addItemList({"RAP - VOZ GRAVE","CLARIDAD Y PRESENCIA","CANTO NATURAL"},1);
    contextAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"context",context);
    type.addItemList({"CAMPANA","LOW SHELF","HIGH SHELF","PASA ALTOS","PASA BAJOS","NOTCH"},1);
    recommendations.setTextWhenNothingSelected(juce::String::fromUTF8("Esperando análisis"));
    recommendations.onChange=[this]{processor.stopPreview();refreshAssistant();};
    for(int i=0;i<8;++i){auto& button=bandButtons[i];button.setButtonText("BANDA "+juce::String(i+1));button.onClick=[this,i]{selectBand(i);};addAndMakeVisible(button);}
    const std::array<const char*,4> names {"FRECUENCIA","GANANCIA","Q","OUTPUT"};
    for(std::size_t i=0;i<4;++i)
    {
        knobs[i].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);knobs[i].setTextBoxStyle(juce::Slider::TextBoxBelow,false,95,25);addAndMakeVisible(knobs[i]);
        knobLabels[i].setText(names[i],juce::dontSendNotification);knobLabels[i].setColour(juce::Label::textColourId,text);
        knobLabels[i].setFont(juce::FontOptions(12));knobLabels[i].setJustificationType(juce::Justification::centred);addAndMakeVisible(knobLabels[i]);
    }
    knobs[0].setTextValueSuffix(" Hz");knobs[0].setNumDecimalPlacesToDisplay(0);
    knobs[1].setTextValueSuffix(" dB");knobs[1].setNumDecimalPlacesToDisplay(1);
    knobs[2].setNumDecimalPlacesToDisplay(2);knobs[3].setTextValueSuffix(" dB");knobs[3].setNumDecimalPlacesToDisplay(1);
    sliderAttachments[3]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,"output",knobs[3]);
    for(auto* button:{&a,&b,&copy,&analyse,&listen,&apply,&dismiss})addAndMakeVisible(button);
    analyse.setColour(juce::TextButton::buttonColourId,gold);
    analyse.setColour(juce::TextButton::textColourOffId,juce::Colour(0xff181818));
    a.onClick=[this]{processor.selectBank(0);};b.onClick=[this]{processor.selectBank(1);};copy.onClick=[this]{processor.copyBank();};
    analyse.onClick=[this]{processor.analyse();};
    listen.setClickingTogglesState(true);listen.onClick=[this]
    {if(listen.getToggleState()){if(auto* s=selectedSuggestion())processor.preview(*s);}else processor.stopPreview();refreshAssistant();};
    apply.onClick=[this]{if(auto* s=selectedSuggestion())processor.apply(*s);refreshAssistant();};
    dismiss.onClick=[this]
    {
        processor.stopPreview();const int index=recommendations.getSelectedId()-1;
        if(index>=0 && index<3){discarded[static_cast<std::size_t>(index)]=true;recommendations.setItemEnabled(index+1,false);}
        int next=0;
        for(int i=0;i<recommendations.getNumItems();++i)if(!discarded[static_cast<std::size_t>(i)]){next=i+1;break;}
        recommendations.setSelectedId(next,juce::dontSendNotification);
        if(next==0)recommendations.setTextWhenNothingSelected("Sin propuestas pendientes");
        refreshAssistant();
    };
    for(auto* button:{&bypass,&match,&enabled})addAndMakeVisible(button);
    bypassAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"bypass",bypass);
    matchAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"match",match);
    for(auto* label:{&assistantStatus,&description,&adjustment,&footer,&subtitle})
    {label->setFont(juce::FontOptions(13));label->setColour(juce::Label::textColourId,muted);label->setJustificationType(juce::Justification::topLeft);addAndMakeVisible(label);}
    description.setColour(juce::Label::textColourId,text);description.setFont(juce::FontOptions(15));
    adjustment.setColour(juce::Label::textColourId,gold);subtitle.setText(juce::String::fromUTF8("Tu oído decide. Compara al mismo volumen."),juce::dontSendNotification);
    selectBand(2);processor.analyser.setVisible(true);timerCallback();startTimerHz(20);
}
CrisVoxEQEditor::~CrisVoxEQEditor() {stopTimer();processor.stopPreview();processor.analyser.setVisible(false);setLookAndFeel(nullptr);}
void CrisVoxEQEditor::selectBand(int n)
{
    selected=juce::jlimit(0,7,n);graph.setSelected(selected);
    for(int i=0;i<8;++i)bandButtons[i].setToggleState(i==selected,juce::dontSendNotification);
    typeAttachment.reset();enabledAttachment.reset();for(int i=0;i<3;++i)sliderAttachments[i].reset();
    const std::array<const char*,3> ids {"freq","gain","q"};
    for(int i=0;i<3;++i)sliderAttachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,CrisVoxEQProcessor::bandId(selected,ids[i]),knobs[i]);
    typeAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state,CrisVoxEQProcessor::bandId(selected,"type"),type);
    enabledAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,CrisVoxEQProcessor::bandId(selected,"on"),enabled);
}
const cris::Suggestion* CrisVoxEQEditor::selectedSuggestion() const
{
    const int i=recommendations.getSelectedId()-1;
    if(!analysis || !analysis->complete || analysis->settingsRevision!=processor.revision() || i<0 || i>=static_cast<int>(analysis->suggestions.size()) || discarded[static_cast<std::size_t>(i)])return nullptr;
    return &analysis->suggestions[static_cast<std::size_t>(i)];
}
void CrisVoxEQEditor::refreshAssistant()
{
    if(!analysis)return;const auto* s=selectedSuggestion();
    const bool stale=analysis->complete && analysis->settingsRevision!=processor.revision();
    assistantStatus.setText(stale?"Los ajustes cambiaron. Analiza de nuevo antes de aplicar otra propuesta.":analysis->message,juce::dontSendNotification);
    const int free=processor.freeBand();
    description.setText(s?juce::String(s->description):juce::String::fromUTF8("El asistente usa reglas de balance espectral. No escucha el instrumental ni decide si tu interpretación o mezcla es correcta."),juce::dontSendNotification);
    adjustment.setText(s?(juce::String(s->frequency,0)+" Hz   "+juce::String(s->gain,1)+" dB   Q "+juce::String(s->q,2)+"\n"+(free>=0?juce::String::fromUTF8("Se usará la banda ")+juce::String(free+1):"No hay banda libre; libera una primero.")):juce::String::fromUTF8("Sin ajustes automáticos obligatorios."),juce::dontSendNotification);
    listen.setEnabled(s && free>=0);apply.setEnabled(s && free>=0);dismiss.setEnabled(s!=nullptr);
    listen.setToggleState(processor.isPreviewing(),juce::dontSendNotification);listen.setButtonText(processor.isPreviewing()?"DETENER":"ESCUCHAR");
}
void CrisVoxEQEditor::timerCallback()
{
    if(shownProgram!=processor.getCurrentProgram()){shownProgram=processor.getCurrentProgram();presets.setSelectedId(shownProgram+1,juce::dontSendNotification);}
    analysis=processor.analyser.snapshot();graph.setSpectrum(analysis);
    if(analysis && (shownResult!=analysis->resultId || recommendations.getNumItems()!=static_cast<int>(analysis->suggestions.size())))
    {
        shownResult=analysis->resultId;discarded.fill(false);recommendations.clear(juce::dontSendNotification);
        recommendations.setTextWhenNothingSelected("Sin propuestas pendientes");
        for(std::size_t i=0;i<analysis->suggestions.size();++i)recommendations.addItem(juce::String(analysis->suggestions[i].title),static_cast<int>(i+1));
        if(!analysis->suggestions.empty())recommendations.setSelectedId(1,juce::dontSendNotification);
    }
    if(analysis && analysis->settingsRevision!=processor.revision())processor.stopPreview();
    analyse.setEnabled(!processor.analyser.busy());analyse.setButtonText(processor.analyser.busy()?"ANALIZANDO...":"ANALIZAR");
    a.setToggleState(processor.currentBank()==0,juce::dontSendNotification);b.setToggleState(processor.currentBank()==1,juce::dontSendNotification);
    copy.setButtonText(processor.currentBank()==0?"COPIAR A > B":"COPIAR B > A");
    const auto settings=processor.settings();const auto t=settings.bands[selected].type;
    knobs[1].setEnabled(t==cris::FilterType::bell || t==cris::FilterType::lowShelf || t==cris::FilterType::highShelf);
    knobs[2].setEnabled(t!=cris::FilterType::lowShelf && t!=cris::FilterType::highShelf);
    footer.setText(juce::String::fromUTF8("FASE MÍNIMA  /  0 muestras añadidas     Compensación: ")+juce::String(processor.matchDb.load(),1)+" dB",juce::dontSendNotification);
    refreshAssistant();repaint();
}
void CrisVoxEQEditor::resized()
{
    const int w=getWidth(),h=getHeight(),side=std::max(340,w/3),split=w-side-20;
    presets.setBounds(370,23,w-750,34);a.setBounds(w-350,23,42,34);b.setBounds(w-302,23,42,34);
    copy.setBounds(w-250,23,130,34);bypass.setBounds(w-111,23,100,34);
    graph.setBounds(16,104,split-24,h-363);
    const int bandY=h-247,bw=(split-32)/8;
    for(int i=0;i<8;++i)bandButtons[i].setBounds(20+i*bw,bandY,bw-5,29);
    const int controlsY=h-198,cw=(split-40)/5;
    for(int i=0;i<3;++i){knobLabels[i].setBounds(20+i*cw,controlsY,cw,22);knobs[i].setBounds(20+i*cw,controlsY+22,cw,120);}
    type.setBounds(24+3*cw,controlsY+34,cw-8,30);enabled.setBounds(22+3*cw,controlsY+77,cw,35);
    knobLabels[3].setBounds(20+4*cw,controlsY,cw,22);knobs[3].setBounds(20+4*cw,controlsY+22,cw,120);
    const int x=split+24,inner=side-35;
    context.setBounds(x,139,inner,34);analyse.setBounds(x,190,inner,48);assistantStatus.setBounds(x,254,inner,90);
    const int descriptionHeight=juce::jlimit(80,150,h-650);
    recommendations.setBounds(x,355,inner,38);description.setBounds(x,410,inner,descriptionHeight);adjustment.setBounds(x,418+descriptionHeight,inner,48);
    const int buttonsY=h-177,buttonW=(inner-12)/3;
    listen.setBounds(x,buttonsY,buttonW,37);apply.setBounds(x+buttonW+6,buttonsY,buttonW,37);dismiss.setBounds(x+2*(buttonW+6),buttonsY,buttonW,37);
    subtitle.setBounds(x,h-120,inner,45);match.setBounds(20,h-42,190,28);footer.setBounds(225,h-40,w-500,27);
}
void CrisVoxEQEditor::paint(juce::Graphics& g)
{
    const int w=getWidth(),h=getHeight(),side=std::max(340,w/3),split=w-side-20;
    g.fillAll(juce::Colour(0xff111216));g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2a2724),0,0,juce::Colour(0xff111216),static_cast<float>(w),100,false));g.fillRect(0,0,w,80);
    g.setColour(gold);for(int i=0;i<5;++i){const float height=i==2?32.0f:(i==1 || i==3?21.0f:10.0f);g.fillRoundedRectangle(24.0f+i*7,40-height/2,3,height,1.5f);}
    g.setColour(text);g.setFont(juce::FontOptions(29,juce::Font::bold));g.drawText("CRIS VOX EQ",72,15,270,48,juce::Justification::centredLeft);
    g.setColour(gold);g.setFont(juce::FontOptions(12));g.drawText("1.0",315,32,35,20,juce::Justification::centredLeft);
    g.setColour(panel);g.fillRoundedRectangle(static_cast<float>(split+8),94,static_cast<float>(side),static_cast<float>(h-150),10);
    g.setColour(text);g.setFont(juce::FontOptions(22,juce::Font::bold));g.drawText("ASISTENTE VOCAL",split+25,101,side-35,32,juce::Justification::centredLeft);
    g.setFont(juce::FontOptions(11));g.setColour(muted);g.drawText("ENTRADA",54,81,85,20,juce::Justification::centredLeft);
    g.setColour(gold);g.drawText("SALIDA / CURVA EQ",160,81,180,20,juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff35363c));g.drawHorizontalLine(h-53,20,static_cast<float>(w-20));
    auto meter=[&](float peak,int x,const char* label)
    {
        g.setColour(muted);g.drawText(label,x,h-39,25,22,juce::Justification::centredLeft);
        const float db=juce::Decibels::gainToDecibels(peak,-60.0f);const int active=juce::jlimit(0,12,static_cast<int>((db+60)/5));
        for(int i=0;i<12;++i){g.setColour(i<active?(peak>1?juce::Colours::orangered:gold):juce::Colour(0xff33363c));g.fillRect(x+27+i*5,h-33,3,12);}
    };
    meter(processor.inputPeak.load(),w-225,"IN");meter(processor.outputPeak.load(),w-112,"OUT");
}
