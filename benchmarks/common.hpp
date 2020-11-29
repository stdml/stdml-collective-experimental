#pragma once
#include <vector>

template <typename T>
struct fake_cpu_buffer_t {
    using value_type = T;

    const std::string name;
    const size_t count;

    std::vector<T> send_buf;
    std::vector<T> recv_buf;

    fake_cpu_buffer_t(std::string name, size_t count)
        : name(std::move(name)), count(count), send_buf(count), recv_buf(count)
    {
        std::fill(send_buf.begin(), send_buf.end(), 1);
        std::fill(recv_buf.begin(), recv_buf.end(), -1);
    }
};

template <typename T>
struct fake_cpu_model {
    std::vector<fake_cpu_buffer_t<T>> buffers;

    fake_cpu_model(const std::vector<size_t> &sizes)
    {
        for (auto i : std::views::iota((size_t)0, sizes.size())) {
            std::string name = "variable:" + std::to_string(i);
            buffers.emplace_back(std::move(name), sizes[i]);
        }
    }
};
