#pragma once

template <typename T>
struct fake_cpu_buffer_t {
    using value_type = T;

    const std::string name;
    const size_t count;

    std::vector<T> send_buf;
    std::vector<T> recv_buf;

    fake_cpu_buffer_t(const std::string &name, size_t count)
        : name(name), count(count), send_buf(count), recv_buf(count)
    {
        std::fill(send_buf.begin(), send_buf.end(), 1);
        std::fill(recv_buf.begin(), recv_buf.end(), -1);
    }
};
