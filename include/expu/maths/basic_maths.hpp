#ifndef EXPU_BASIC_MATHS_HPP_INCLUDED
#define EXPU_BASIC_MATHS_HPP_INCLUDED

#include <concepts>

#ifdef _MSC_VER
#include <intrin.h>
#endif


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

    template<class IntegralType>
    constexpr IntegralType right_shift_round_up(IntegralType value, unsigned char shift_by)
    {
        if (value)
            return ((value - 1) >> shift_by) + 1;
        else
            return IntegralType{};
    }

    template<std::unsigned_integral Type>
    constexpr unsigned char int_log2(Type value)
    {
        if (value == 0)
            return 0xFF;

#ifdef _MSC_VER 
        unsigned long result = 0;

        if constexpr (sizeof(Type) == sizeof(unsigned __int64))
            _BitScanReverse64(&result, value);
        else
            _BitScanReverse(&result, value);

        return static_cast<unsigned char>(result);

#elif defined __GNUC__
        static constexpr int _type_bit_count = (sizeof(Type) << 3) - 1;

        if constexpr (std::is_same_v<Type, unsigned long long int>)
            return _type_bit_count - __builtin_clzll(value);
        if constexpr (std::is_same_v<Type, unsigned long int>)
            return _type_bit_count - __builtin_clzl(value);
        else
            return _type_bit_count - __builtin_clz(value);

#else
        unsigned char result = 0;
        while (value >>= 1)
            ++result;

        return result;

#endif // _MSVC
    }
}

#endif // !EXPU_BASIC_MATHS_HPP_INCLUDED