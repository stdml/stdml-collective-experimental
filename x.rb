line = "    void group_all_reduce(std::vector<std::future<workspace>> ws);"

m = line.scan(/(std::[:\w]+|size_t|uint8_t|uint16_t|uint32_t|uint64_t|int8_t|int16_t|int32_t|int64_t|printf|fprintf)/)
p m
m = m.flatten
p m
