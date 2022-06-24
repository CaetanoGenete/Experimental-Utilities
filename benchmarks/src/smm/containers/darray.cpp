#include "benchmark/benchmark.h"

#include <vector>

#include "smm/containers/darray.hpp"


//static void bm_darray_push_back(benchmark::State& state) {
//    const size_t push_back_count = state.range(0);
//
//    for (auto _ : state) {
//        smm::darray<size_t> arr;
//
//        for (size_t i = 0; i < push_back_count; ++i)
//            arr.push_back(i);
//    }
//}
//
//static void bm_vector_push_back(benchmark::State& state) {
//    const size_t push_back_count = state.range(0);
//
//    for (auto _ : state) {
//        std::vector<size_t> arr;
//
//        for (size_t i = 0; i < push_back_count; ++i)
//            arr.push_back(i);
//    }
//}
//
//BENCHMARK(bm_darray_push_back)->Range(8, 1 << 24);
//BENCHMARK(bm_vector_push_back)->Range(8, 1 << 24);

static void bm_darray_default_construct(benchmark::State& state) {
    for (auto _ : state)
        benchmark::DoNotOptimize(smm::darray<size_t>());
}

BENCHMARK(bm_darray_default_construct);

BENCHMARK_MAIN();
