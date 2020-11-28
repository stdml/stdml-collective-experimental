#include <iostream>
#include <string>

namespace stdml::collective
{
constexpr const char *git_commit =
#ifdef STDML_COLLECTIVE_GIT_COMMIT
    // FIXME: extract string
    // STDML_COLLECTIVE_GIT_COMMIT
    ""
#else
    "unknown"
#endif
    ;

void show_build_info()
{
    std::cout << "git commit: " << git_commit;
}
}  // namespace stdml::collective
