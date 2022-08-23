#ifndef EXPU_BASIC_MATHS_HPP_INCLUDED
#define EXPU_BASIC_MATHS_HPP_INCLUDED

#include <concepts>

namespace expu {

    //Todo: provide overload for functions that have module operator defined
    template<class IntegralType>
    constexpr bool is_even(IntegralType value) noexcept
    {
        return (value & 1) == 0;
    }

    template<class IntegralType>
    constexpr bool is_odd(IntegralType value) noexcept
    {
        return !is_even(value);
    }
}

#endif // !EXPU_BASIC_MATHS_HPP_INCLUDED