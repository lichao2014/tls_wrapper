
#include <string>

#include "tls_wrapper.h"


TLSPtr<std::string> g_tls;

std::string* get()
{
    return g_tls.get([] {
        return new std::string;
    });
}

int main()
{
    TLSReleaseScoped scoped;

    get()->assign("123");

    auto t = create_thread([] {

        auto x = get();
    });

    t.join();
}
