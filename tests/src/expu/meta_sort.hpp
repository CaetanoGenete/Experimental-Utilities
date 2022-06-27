#ifndef SMM_TESTS_META_SORT_HPP_INCLUDED
#define SMM_TESTS_META_SORT_HPP_INCLUDED

#include "gtest/gtest.h"

#include "expu/int_seq.hpp"
#include "expu/iterators/sorting.hpp"

#include <array>

namespace smm_tests {

    template<typename IntType, IntType ... Seq>
    consteval auto _bubble_sort_int_seq(expu::int_seq<IntType, Seq...>)  
    {
        constexpr size_t seq_size = sizeof...(Seq);

        //Abuse lambdas to sort 
        constexpr auto sorted_arr = [](std::array<IntType, seq_size> arr) {
            expu::bubble_sort(arr.begin(), arr.end());
            return arr;
        }(std::array<IntType, seq_size>{Seq...});

        //Templated lambda to expand sorted_arr
        return []<size_t ... Indicies>(expu::int_seq<size_t, Indicies...>) {
            return expu::int_seq<IntType, sorted_arr[Indicies]...>{};
        }(expu::make_int_seq<size_t, seq_size>{});
    }

    template<typename Seq>
    using sorted_int_seq = decltype(_bubble_sort_int_seq(Seq{}));

    /* LEGACY: Template only bubble sort on int_seq

    template<typename IntType, IntType...>
    struct bubble_sort_pass;

    template<typename IntType, IntType First, IntType Second, IntType ... Other>
    struct bubble_sort_pass<IntType, First, Second, Other...> 
    {
    private:
        static constexpr IntType _least = std::min<IntType>(First, Second);
        static constexpr IntType _great = std::max<IntType>(First, Second);

    public:
        using next = bubble_sort_pass<IntType, _great, Other...>;
        using seq  = concat_int_seq<int_seq<IntType, _least>, typename next::seq>;

        static constexpr IntType max = next::max;
    };

    template<typename IntType, IntType Last>
    struct bubble_sort_pass<IntType, Last>
    {
        static constexpr IntType max = Last;

        using seq = int_seq<IntType>;
    };

    template<typename>
    struct meta_bubble_sort;

    template<typename IntType, IntType ... Seq> 
    struct meta_bubble_sort<int_seq<IntType, Seq...>>
    {
    private:
        using _pass = bubble_sort_pass<IntType, Seq...>;

    public:
        using seq = concat_int_seq<sorted_int_seq<typename _pass::seq>, int_seq<IntType, _pass::max>>;
    };

    template<typename IntType, IntType Int>
    struct meta_bubble_sort<int_seq<IntType, Int>> {
        using seq = int_seq<IntType, Int>;
    };

    */

}

#endif // !SMM_TESTS_META_SORT_HPP_INCLUDED