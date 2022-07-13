#ifndef EXPU_SEARCHING_HPP_INCLUDED
#define EXPU_SEARCHING_HPP_INCLUDED

#include <iterator>

namespace expu {

    template<std::forward_iterator FwdIt, typename Predicate>
    [[nodiscard]] constexpr FwdIt find_if(FwdIt begin, FwdIt end, Predicate pred)
    {
        for (; begin != end; ++begin)
            if (pred(*begin)) return begin;

        return end;
    }

    template<std::forward_iterator FwdIt, typename Type>
    [[nodiscard]] constexpr FwdIt find_first(FwdIt begin, FwdIt end, const Type& type)
    {
        return find_if(begin, end, [&](std::iter_reference_t<FwdIt> curr) {
            return curr == type;
        });
    }

}

#endif // !EXPU_SEARCHING_HPP_INCLUDED