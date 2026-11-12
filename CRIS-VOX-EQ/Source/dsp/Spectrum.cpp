#include "Spectrum.h"
namespace cris
{
void AudioTap::push(const float* in,const float* out,int count)
{
    const auto w=write.load(std::memory_order_relaxed),r=read.load(std::memory_order_acquire);
    const auto available=capacity-(w-r);
    const auto n=std::min(static_cast<std::uint32_t>(count),available);
    for (std::uint32_t i=0;i<n;++i) storage[(w+i)&(capacity-1)]={in[i],out[i]};
    write.store(w+n,std::memory_order_release);
    if(n<static_cast<std::uint32_t>(count)) dropped.fetch_add(static_cast<std::uint32_t>(count)-n,std::memory_order_relaxed);
}
int AudioTap::pop(AudioPair* dest,int max)
{
    const auto r=read.load(std::memory_order_relaxed),w=write.load(std::memory_order_acquire);
    const auto n=std::min(w-r,static_cast<std::uint32_t>(max));
    for(std::uint32_t i=0;i<n;++i) dest[i]=storage[(r+i)&(capacity-1)];
    read.store(r+n,std::memory_order_release); return static_cast<int>(n);
}
void AudioTap::discard() { read.store(write.load(std::memory_order_acquire),std::memory_order_release); }
Spectrum::Spectrum()
{
    for(int i=0;i<fftSize;++i) { window[i]=static_cast<float>(0.5-0.5*std::cos(2*pi*i/(fftSize-1))); windowSum+=window[i]; }
    for(int i=0;i<fftSize/2;++i) twiddles[i]=std::polar(1.0f,static_cast<float>(-2*pi*i/fftSize));
}
void Spectrum::transform(const float* in)
{
    for(int i=0;i<fftSize;++i) buffer[i]={in[i]*window[i],0};
    for(int i=1,j=0;i<fftSize;++i)
    {
        int bit=fftSize>>1;
        for(;j&bit;bit>>=1) j^=bit;
        j^=bit; if(i<j) std::swap(buffer[i],buffer[j]);
    }
    for(int length=2;length<=fftSize;length*=2)
        for(int start=0;start<fftSize;start+=length)
            for(int j=0;j<length/2;++j)
            {
                const auto a=buffer[start+j],b=buffer[start+j+length/2]*twiddles[j*(fftSize/length)];
                buffer[start+j]=a+b; buffer[start+j+length/2]=a-b;
            }
    const float scale=4/(windowSum*windowSum);
    for(int i=0;i<=fftSize/2;++i) power[i]=std::norm(buffer[i])*scale;
}
void Spectrum::display(std::array<float,displayBins>& result,double rate) const
{
    for(int i=0;i<displayBins;++i)
    {
        const double freq=20*std::pow(1000.0,static_cast<double>(i)/(displayBins-1));
        if(freq>rate*0.5) { result[i]=-120; continue; }
        const double bin=freq*fftSize/rate;
        const int k=std::clamp(static_cast<int>(bin),0,fftSize/2-1);
        const float p=power[k]+static_cast<float>(bin-k)*(power[k+1]-power[k]);
        result[i]=static_cast<float>(10*std::log10(std::max(1.0e-12f,p)));
    }
}
SpectrumFrame Spectrum::calculate(const float* in,const float* out,double rate)
{
    SpectrumFrame result;
    transform(in); display(result.input,rate);
    transform(out); display(result.output,rate);
    constexpr std::array<double,8> edges {80,180,450,1000,2000,4500,9000,16000};
    for(std::size_t region=0;region<7;++region)
    {
        double sum=0; int bins=0;
        for(int i=1;i<=fftSize/2;++i)
        {
            const double f=i*rate/fftSize;
            if(f>=edges[region] && f<edges[region+1]) { sum+=power[i];++bins; }
        }
        result.regionDb[region]=10*std::log10(std::max(1.0e-12,sum/std::max(1,bins)));
    }
    double energy=0;
    for(int i=0;i<fftSize;++i) { energy+=out[i]*static_cast<double>(out[i]);result.peak=std::max(result.peak,std::abs(static_cast<double>(out[i]))); }
    result.rmsDb=10*std::log10(std::max(1.0e-12,energy/fftSize));
    return result;
}
std::vector<Suggestion> recommend(const std::array<double,7>& r,int context,double rmsDb)
{
    std::vector<Suggestion> result;
    if(rmsDb<-45) return result;
    const double mud=r[1]-(r[0]+r[2])*0.5;
    const double nasal=r[3]-(r[2]+r[4])*0.5;
    const double harsh=r[5]-r[4];
    const double presence=r[4]-r[2];
    if(mud>(context==0?5.0:4.0))
        result.push_back({"Posible acumulación en graves medios","Se observa más energía relativa entre 180 y 450 Hz. Escucha este recorte solo si notas la voz encerrada. No significa que el cuerpo de tu voz esté mal.",280,-2,1.2});
    if(nasal>4.0)
        result.push_back({"Posible resonancia en medios","La zona de 1 a 2 kHz destaca frente a sus vecinas. Prueba una reducción suave si percibes un carácter nasal; compara también con el instrumental.",1400,-1.5,1.5});
    if(harsh>1.5)
        result.push_back({"Revisar brillo o aspereza","La energía entre 4,5 y 9 kHz es relativamente alta. Un recorte amplio puede suavizarla. Esta propuesta estática no sustituye a un de-esser.",6000,-1.5,1.1});
    if(presence<(context==1?-15.0:-20.0) && result.size()<3)
        result.push_back({"Explorar presencia vocal","La presencia es baja respecto a los medios. Si falta claridad dentro de la mezcla, compara este realce pequeño. Una voz oscura también puede ser una elección artística.",3000,1.5,0.8});
    if(result.size()>3) result.resize(3);
    return result;
}
}
