#include "PluginProcessor.h"
#include "AudioAllocationProbe.h"
#include <iostream>
#include <stdexcept>
namespace
{
void require(bool value,const char* text){if(!value)throw std::runtime_error(text);}
template<class F>bool waitFor(F fn,int milliseconds=5000)
{
    const auto end=juce::Time::getMillisecondCounter()+static_cast<juce::uint32>(milliseconds);
    while(juce::Time::getMillisecondCounter()<end){if(fn())return true;juce::Thread::sleep(20);}return false;
}
void screenshot(juce::AudioProcessorEditor& editor,int width,int height,const char* name)
{
    editor.setSize(width,height);const auto file=juce::File::getCurrentWorkingDirectory().getChildFile(name);file.deleteFile();
    auto stream=file.createOutputStream();juce::PNGImageFormat format;
    require(stream && format.writeImageToStream(editor.createComponentSnapshot(editor.getLocalBounds(),true,1),*stream),"Screenshot failed");
}
}
int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        auto processor=std::make_unique<CrisVoxEQProcessor>();processor->prepareToPlay(48000,2048);
        require(processor->getNumPrograms()==7,"Missing factory programs");require(processor->getLatencySamples()==0,"EQ reports unexpected latency");
        processor->setCurrentProgram(1);require(processor->settings().bands[2].gain==-2,"Preset not connected");
        juce::AudioFormatManager formats;formats.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(juce::File(CRIS_TEST_VOCAL)));
        require(reader!=nullptr,"Missing real singing fixture");
        juce::AudioBuffer<float> real(2,static_cast<int>(reader->lengthInSamples));
        require(reader->read(&real,0,real.getNumSamples(),0,true,true),"Cannot read real vocal");
        juce::AudioBuffer<float> original(real);juce::MidiBuffer midi;
        processor->prepareToPlay(reader->sampleRate,real.getNumSamples());processor->processBlock(real,midi);
        double difference=0;for(int c=0;c<2;++c)for(int i=0;i<real.getNumSamples();++i)
        {const double d=real.getSample(c,i)-original.getSample(c,i);difference+=d*d;require(std::isfinite(real.getSample(c,i)),"Non-finite vocal output");}
        require(difference>0.001,"Real vocal not processed");
        std::cout<<"Real singing fixture: "<<reader->lengthInSamples/reader->sampleRate<<" s, processed with RAP GRAVE; difference energy="<<difference<<"\n";
        processor->setParameter("b3_gain",-4);processor->copyBank();processor->selectBank(1);
        require(std::abs(processor->settings().bands[2].gain+4)<0.01,"A/B copy wrong");
        processor->setParameter("b3_gain",1);processor->selectBank(0);
        require(std::abs(processor->settings().bands[2].gain+4)<0.01,"A/B recall wrong");
        juce::MemoryBlock state;processor->getStateInformation(state);
        {
            auto restored=std::make_unique<CrisVoxEQProcessor>();restored->setStateInformation(state.getData(),static_cast<int>(state.getSize()));
            require(std::abs(restored->settings().bands[2].gain+4)<0.01,"State restore wrong");
            restored->selectBank(1);require(std::abs(restored->settings().bands[2].gain-1)<0.01,"A/B slot not saved");
        }
        processor->setCurrentProgram(1);processor->prepareToPlay(48000,2048);
        // Feed synthetic diagnostic audio at a paced rate so the worker is tested
        // without overflowing its bounded real-time FIFO.
        juce::AudioBuffer<float> block(2,2048);processor->analyse();juce::Thread::sleep(40);
        for(int b=0;b<110;++b)
        {
            for(int i=0;i<2048;++i)
            {
                const double t=(b*2048+i)/48000.0;
                const auto x=static_cast<float>(0.05*std::sin(2*cris::pi*110*t)+0.20*std::sin(2*cris::pi*280*t)
                    +0.025*std::sin(2*cris::pi*800*t)+0.02*std::sin(2*cris::pi*3000*t));
                block.setSample(0,i,x);block.setSample(1,i,x);
            }
            audioProbe::count=0;audioProbe::enabled=true;
            processor->processBlock(block,midi);
            audioProbe::enabled=false;
            require(audioProbe::count==0,"Heap allocation in processBlock while analysing");
            juce::Thread::sleep(25);
        }
        require(waitFor([&]{return !processor->analyser.busy();}),"Analysis did not finish");
        auto result=processor->analyser.snapshot();require(result->complete && !result->suggestions.empty(),"No expected spectral recommendation");
        const auto& suggestion=result->suggestions.front();const auto before=processor->settings();
        require(processor->preview(suggestion),"Preview rejected");
        require(std::abs(processor->audibleSettings().bands[processor->freeBand()].gain-suggestion.gain)<0.01,"Preview not connected to audible settings");
        require(processor->settings().bands[3].gain==before.bands[3].gain,"Preview changed stored parameters");
        require(processor->isPreviewing(),"Preview flag missing");processor->stopPreview();require(!processor->isPreviewing(),"Preview not stopped");
        {
            std::unique_ptr<juce::AudioProcessorEditor> editor(processor->createEditor());
            screenshot(*editor,1340,860,"actual-eq-interface.png");screenshot(*editor,1100,760,"actual-eq-interface-small.png");
        }
        const int available=processor->freeBand();require(available>=0,"No free band");
        require(processor->apply(suggestion),"Apply rejected");
        require(std::abs(processor->settings().bands[available].gain-suggestion.gain)<0.01,"Apply not connected to band");
        require(processor->revision()!=result->settingsRevision,"Old analysis not invalidated");
        std::cout<<"Assistant: "<<suggestion.title<<" / "<<suggestion.frequency<<" Hz, "<<suggestion.gain<<" dB\n";
        std::cout<<"PASS: JUCE presets, actual vocal processing, A/B, state, worker analysis, preview/apply, callback allocations=0 and UI snapshots.\n";
        return 0;
    }
    catch(const std::exception& e){audioProbe::enabled=false;std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
