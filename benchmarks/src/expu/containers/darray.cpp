#include "benchmark/benchmark.h"

#include <vector>

#include "expu/containers/darray.hpp"

template<class Container>
static void BM_push_back(benchmark::State& state) {
    const size_t push_back_count = 1 << state.range(0);

    for (auto _ : state) {
        Container arr;

        for (size_t i = 0; i < push_back_count; ++i)
            arr.push_back(i);
    }

    const size_t bytes = push_back_count * sizeof(size_t);
    state.SetBytesProcessed(bytes);
    state.SetLabel(std::to_string(bytes >> 10));
}

//BENCHMARK(BM_push_back<std::vector<int>>)->DenseRange(8, 23);
BENCHMARK(BM_push_back<expu::darray<int>>)->DenseRange(8, 23);

BENCHMARK_MAIN();
