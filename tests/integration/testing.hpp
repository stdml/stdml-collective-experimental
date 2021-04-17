#pragma once

template <typename F, typename... Args, typename T>
T for_all_types(const F &f, T init, const Args &... args)
{
    init += f.template operator()<int8_t>(args...);
    init += f.template operator()<int16_t>(args...);
    init += f.template operator()<int32_t>(args...);
    init += f.template operator()<int64_t>(args...);

    init += f.template operator()<uint8_t>(args...);
    init += f.template operator()<uint16_t>(args...);
    init += f.template operator()<uint32_t>(args...);
    init += f.template operator()<uint64_t>(args...);

    init += f.template operator()<float>(args...);
    init += f.template operator()<double>(args...);
    return init;
}
