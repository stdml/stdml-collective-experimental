ADD_EXECUTABLE(bench-all-reduce benchmarks/bench_all_reduce.cpp)
TARGET_LINK_LIBRARIES(bench-all-reduce stdml-collective)
