INCLUDE(FetchContent)

FETCHCONTENT_DECLARE(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v2.13.1)

FETCHCONTENT_MAKEAVAILABLE(Catch2)
# TARGET_LINK_LIBRARIES(tests Catch2::Catch2)
