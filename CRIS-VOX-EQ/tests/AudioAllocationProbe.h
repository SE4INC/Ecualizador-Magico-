#pragma once
#include <algorithm>
#include <cstdlib>
#include <new>
#if defined(_MSC_VER)
#include <malloc.h>
#endif
// Thread-local measurement excludes legitimate allocations by the analysis worker.
namespace audioProbe { inline thread_local bool enabled=false;inline thread_local int count=0; }
void* operator new(std::size_t n)
{
    if(audioProbe::enabled)++audioProbe::count;
    if(auto* p=std::malloc(n?n:1))return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p) noexcept{std::free(p);}
void operator delete[](void* p) noexcept{std::free(p);}
void operator delete(void* p,std::size_t) noexcept{std::free(p);}
void operator delete[](void* p,std::size_t) noexcept{std::free(p);}
void* operator new(std::size_t n,std::align_val_t alignment)
{
    if(audioProbe::enabled)++audioProbe::count;
    const auto a=static_cast<std::size_t>(alignment);
#if defined(_MSC_VER)
    void* p=_aligned_malloc(n?n:1,a);
#else
    void* p=std::aligned_alloc(a,((std::max<std::size_t>(1,n)+a-1)/a)*a);
#endif
    if(p)return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t n,std::align_val_t a){return ::operator new(n,a);}
void operator delete(void* p,std::align_val_t) noexcept
{
#if defined(_MSC_VER)
    _aligned_free(p);
#else
    std::free(p);
#endif
}
void operator delete[](void* p,std::align_val_t a) noexcept{::operator delete(p,a);}
void operator delete(void* p,std::size_t,std::align_val_t a) noexcept{::operator delete(p,a);}
void operator delete[](void* p,std::size_t,std::align_val_t a) noexcept{::operator delete(p,a);}
