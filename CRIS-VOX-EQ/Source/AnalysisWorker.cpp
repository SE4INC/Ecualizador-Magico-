#include "AnalysisWorker.h"
AnalysisWorker::AnalysisWorker() : juce::Thread("CRIS EQ analysis")
{ published=std::make_shared<AnalysisResult>();startThread(juce::Thread::Priority::low); }
AnalysisWorker::~AnalysisWorker() { signalThreadShouldExit();notify();stopThread(-1); }
void AnalysisWorker::prepare(double rate) { sampleRate.store(rate);rateRevision.fetch_add(1); }
void AnalysisWorker::request(int context,std::uint64_t revision)
{
    requestedContext.store(context);settingsRevision.store(revision);analysing.store(true);requestId.fetch_add(1);notify();
}
std::shared_ptr<const AnalysisResult> AnalysisWorker::snapshot() const
{ std::lock_guard<std::mutex> lock(mutex);return published; }
void AnalysisWorker::run()
{
    std::array<float,cris::fftSize> input {},output {},orderedInput {},orderedOutput {};
    std::array<cris::AudioPair,1024> block {};
    std::array<double,7> accumulated {};
    cris::SpectrumFrame lastSpectrum;
    lastSpectrum.input.fill(-120);lastSpectrum.output.fill(-120);
    std::uint64_t lastRate=0,lastRequest=0,captured=0,droppedAtStart=0,analysisRevision=0;
    int write=0,filled=0,since=0,validFrames=0,context=0;
    double rate=48000,startTime=0;
    bool collecting=false;
    std::vector<cris::Suggestion> suggestions;
    juce::String message="Reproduce tu voz y pulsa ANALIZAR.";
    bool complete=false;
    auto publish=[&]
    {
        auto value=std::make_shared<AnalysisResult>();value->spectrum=lastSpectrum;
        value->suggestions=suggestions;value->message=message;value->complete=complete;
        value->resultId=lastRequest;value->settingsRevision=analysisRevision;
        std::lock_guard<std::mutex> lock(mutex);published=std::move(value);
    };
    while(!threadShouldExit())
    {
        wait(12);
        const auto currentRate=rateRevision.load();
        if(currentRate!=lastRate)
        {
            lastRate=currentRate;rate=std::clamp(sampleRate.load(),8000.0,384000.0);
            tap.discard();write=filled=since=0;input.fill(0);output.fill(0);
            if(collecting) { collecting=false;analysing.store(false);message=juce::String::fromUTF8("Cambió la frecuencia de sesión. Analiza de nuevo.");complete=false;publish(); }
        }
        const auto request=requestId.load();
        if(request!=lastRequest)
        {
            lastRequest=request;context=requestedContext.load();analysisRevision=settingsRevision.load();
            captured=0;validFrames=0;accumulated.fill(0);suggestions.clear();
            analysing.store(true);
            tap.discard();write=filled=since=0;collecting=true;complete=false;
            droppedAtStart=tap.lost();startTime=juce::Time::getMillisecondCounterHiRes();
            message="Analizando unos 4 segundos de audio. Reproduce tu voz...";publish();
        }
        int count=0;
        while((count=tap.pop(block.data(),static_cast<int>(block.size())))>0)
        {
            for(int i=0;i<count;++i)
            {
                input[write]=block[i].input;output[write]=block[i].output;
                write=(write+1)%cris::fftSize;filled=std::min(cris::fftSize,filled+1);++since;
                if(collecting) ++captured;
                if(filled==cris::fftSize && since>=static_cast<int>(rate/20))
                {
                    since=0;
                    for(int n=0;n<cris::fftSize;++n) { orderedInput[n]=input[(write+n)%cris::fftSize];orderedOutput[n]=output[(write+n)%cris::fftSize]; }
                    lastSpectrum=fft.calculate(orderedInput.data(),orderedOutput.data(),rate);
                    if(collecting && lastSpectrum.rmsDb>-45)
                    {
                        ++validFrames;
                        for(std::size_t r=0;r<7;++r) accumulated[r]+=std::pow(10.0,lastSpectrum.regionDb[r]/10.0);
                    }
                    publish();
                }
                if(collecting && captured>=static_cast<std::uint64_t>(rate*4))
                {
                    collecting=false;analysing.store(false);complete=true;
                    if(tap.lost()!=droppedAtStart) message=juce::String::fromUTF8("Se perdió audio de análisis. Repite con menos carga de sesión.");
                    else if(validFrames<15) message=juce::String::fromUTF8("No hubo suficiente señal útil. Reproduce una toma más clara y analiza de nuevo.");
                    else
                    {
                        std::array<double,7> regions {};
                        for(std::size_t r=0;r<7;++r) regions[r]=10*std::log10(std::max(1.0e-12,accumulated[r]/validFrames));
                        suggestions=cris::recommend(regions,context,-20);
                        message=suggestions.empty()?juce::String::fromUTF8("No destaca un desequilibrio claro según estas reglas. Eso no certifica la mezcla; compara con tu instrumental."):juce::String::fromUTF8("Posibles ajustes para comparar. No son errores obligatorios ni una evaluación artística.");
                    }
                    publish();
                }
            }
            if(threadShouldExit()) return;
        }
        if(collecting && juce::Time::getMillisecondCounterHiRes()-startTime>15000)
        {
            collecting=false;analysing.store(false);complete=true;suggestions.clear();
            message=juce::String::fromUTF8("Faltó audio continuo. Reproduce la voz durante 4 segundos y vuelve a analizar.");publish();
        }
    }
}
