#include <cstdio>

#include <stdml/bits/collective/build_info.hpp>

#ifndef STDML_COLLECTIVE_GIT_COMMIT
    #define STDML_COLLECTIVE_GIT_COMMIT ""
#endif

#ifndef STDML_COLLECTIVE_BUILD_TIME
    #define STDML_COLLECTIVE_BUILD_TIME 0
#endif

#ifndef STDML_COLLECTIVE_BUILD_CC
    #define STDML_COLLECTIVE_BUILD_CC ""
#endif

#ifndef STDML_COLLECTIVE_BUILD_CXX
    #define STDML_COLLECTIVE_BUILD_CXX ""
#endif

#define DEF_MACRO_VALUE(T, name, K) constexpr T name = K;

namespace stdml::collective
{
DEF_MACRO_VALUE(const char *, git_commit, STDML_COLLECTIVE_GIT_COMMIT);
DEF_MACRO_VALUE(int, build_time, STDML_COLLECTIVE_BUILD_TIME);
DEF_MACRO_VALUE(const char *, build_cc, STDML_COLLECTIVE_BUILD_CC);
DEF_MACRO_VALUE(const char *, build_cxx, STDML_COLLECTIVE_BUILD_CXX);

void show_build_info()
{
    printf("git commit: %s\n", git_commit);
    printf("build time: %d\n", build_time);

    printf("build cc: %s\n", build_cc);
    printf("build cxx: %s\n", build_cxx);
}
}  // namespace stdml::collective
