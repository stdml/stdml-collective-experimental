# https://cmake.org/cmake/help/latest/command/install.html

ADD_BINARY(apps/stdml_collective_bench_send_recv.cpp)
INSTALL(TARGETS stdml-collective-bench-send-recv DESTINATION bin)

ADD_BINARY(apps/stdml_collective_bench_allreduce.cpp)
INSTALL(TARGETS stdml-collective-bench-allreduce DESTINATION bin)
