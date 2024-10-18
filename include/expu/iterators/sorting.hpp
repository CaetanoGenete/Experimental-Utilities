#ifndef SMM_ITERATORS_SORTING_HPP_INCLUDED
#define SMM_ITERATORS_SORTING_HPP_INCLUDED

#include <iterator>

namespace expu {

    template<
        std::forward_iterator FwdIt,
        std::sentinel_for<FwdIt> Sentinel,
        class Projection,
        std::indirect_binary_predicate < 
            std::projected<std::iter_reference_t<FwdIt>, Projection>, 
            std::projected<std::iter_reference_t<FwdIt>, Projection> 
        > Predicate>
    constexpr FwdIt _bubble_pass(FwdIt begin, Sentinel end, Predicate pred, Projection proj)
    {
        for (FwdIt next(std::next(begin)); next != end; ++begin, ++next) {
            if (std::invoke(pred, std::invoke(proj, *next), std::invoke(proj, *begin)))
                std::iter_swap(begin, next);
        }

        return begin;
    }

    template<
        std::forward_iterator FwdIt,
        std::sentinel_for<FwdIt> Sentinel,
        class Projection = std::identity,
        std::indirect_binary_predicate <
            std::projected<std::iter_reference_t<FwdIt>, Projection>,
            std::projected<std::iter_reference_t<FwdIt>, Projection>
        > Predicate = std::ranges::less>    
    constexpr void bubble_sort(FwdIt begin, Sentinel end, Predicate pred = {}, Projection proj = {})
    {
        //Do nothing if range size is zero
        if (begin != end) {
            FwdIt stop = _bubble_pass(begin, end, proj, pred);

            while (begin != stop)
                stop = _bubble_pass(begin, end, proj, pred);
        }
    }
}

#endif // !SMM_ITERATORS_SORTING_HPP_INCLUDED
