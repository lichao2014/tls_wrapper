#pragma once

#include <thread>

#include "list.h"

#ifdef _WIN32
#include <wtypes.h>

class PlatformTLS
{
public:
    PlatformTLS() noexcept : _index(::TlsAlloc()) {}
    ~PlatformTLS() noexcept { ::TlsFree(_index); }

    PlatformTLS(const PlatformTLS&) = delete;
    void operator=(const PlatformTLS&) = delete;

    void* get() const noexcept { return ::TlsGetValue(_index); }
    void set(void* value) noexcept { ::TlsSetValue(_index, value); }
private:
    DWORD _index = 0;
};
#endif

template<typename T>
class TLSObject
{
public:
    T* get() const noexcept { return static_cast<T*>(_tls.get()); }
    void set(T* value) noexcept { _tls.set(value); }

    template<typename F>
    T* get_or_create(F f)
    {
        auto p = get();
        if (!p)
        {
            p = f();
            set(p);
        }

        return p;
    }

    T* release()
    {
        auto p = get();
        if (p)
        {
            set(nullptr);
        }

        return p;
    }
private:
    PlatformTLS _tls;
};

struct TLSReleaseNode
{
    TLSReleaseNode(void (*dtor)(void *), void* ud) : on_release(dtor), user_data(ud) {}
    TLSReleaseNode* next = nullptr;
    void (*on_release)(void *) = nullptr;
    void* user_data = nullptr;

    void operator()()
    {
        if (this->on_release)
        {
            this->on_release(this->user_data);
        }
    }
};

using TLSReleaseList = List<TLSReleaseNode, &TLSReleaseNode::next>;

class TLSReleaseManager : private TLSObject<TLSReleaseList>
{
public:
    TLSReleaseList* get()
    {
        return get_or_create([] { return new TLSReleaseList; });
    }

    void clear()
    {
        std::unique_ptr<TLSReleaseList> p(release());
        if (p)
        {
            p->visit([](TLSReleaseNode& node) { node(); });
        }
    }

    static TLSReleaseManager& get_instance()
    {
        static TLSReleaseManager s_tls_release_manager;
        return s_tls_release_manager;
    }
};

struct TLSReleaseScoped
{
    ~TLSReleaseScoped()
    {
        TLSReleaseManager::get_instance().clear();
    }
};


template<typename ... Args>
std::thread create_thread(Args&& ... args)
{
    return std::thread([=] {
        TLSReleaseScoped scoped;
        return std::invoke(args...);
    });
}

template<typename T>
class TLSPtr : public TLSObject<T>
{
public:
    template<typename F>
    T *get(F f)
    {
        return this->get_or_create([f, this] {
            auto ptr = f();
            this->set(ptr);
            TLSReleaseManager::get_instance().get()->push_back(
                [](void* p) {delete static_cast<T*>(p);},
                ptr
            );
            return ptr;
        });
    }
};


