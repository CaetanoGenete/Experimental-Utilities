#ifndef SMM_ITERATORS_SORTING_HPP_INCLUDED
#define SMM_ITERATORS_SORTING_HPP_INCLUDED

#include <iterator>

namespace expu {

    template<std::forward_iterator FwdIt, typename Predicate>
    constexpr void bubble_sort(FwdIt begin, FwdIt end, Predicate pred)
    {
        FwdIt stop = end;

        while (stop != begin) {
            FwdIt curr = begin;
            for (FwdIt next = std::next(curr); next != stop; ++curr, ++next) {
                if (pred(*next, *curr))
                    std::iter_swap(next, curr);
            }

            stop = curr;
        }
    }

    template<std::forward_iterator FwdIt>
    constexpr void bubble_sort(FwdIt begin, FwdIt end)
    {
        bubble_sort(begin, end, std::less<>{});
    }

}

#endif // !SMM_ITERATORS_SORTING_HPP_INCLUDED
