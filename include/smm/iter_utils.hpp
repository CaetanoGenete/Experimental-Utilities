#ifndef SMM_ITER_UTILS_HPP_INCLUDED
#define SMM_ITER_UTILS_HPP_INCLUDED

#include <type_traits>
#include <iterator>
#include <memory>

#include "smm/mem_utils.hpp"

namespace smm {

    template<typename>
    constexpr bool _is_memcpyable_v = false;

    template<std::contiguous_iterator Iter>
    constexpr bool _is_memcpyable_v<Iter> = std::is_trivially_copyable<std::iter_value_t<Iter>>::value;

    template<std::contiguous_iterator Iter>
    constexpr bool _is_memcpyable_v<std::move_iterator<Iter>> = _is_memcpyable_v<Iter>;

    template<typename FromIter, typename ToIter>
    constexpr bool _is_memcpyable_to_v = std::is_same_v<std::iter_value_t<FromIter>, std::iter_value_t<ToIter>> &&
                                         _is_memcpyable_v<FromIter>                                             &&
                                         _is_memcpyable_v<ToIter>;
    
    //Todo: Consider supporting volatility
    template<std::contiguous_iterator InputCtgIt, std::contiguous_iterator OutCtgIt>
    OutCtgIt _range_memcpy(InputCtgIt begin, InputCtgIt end, OutCtgIt output)
    {
        const char* begin_chr  = reinterpret_cast<const char*>(std::to_address(begin));
        const char* end_chr    = reinterpret_cast<const char*>(std::to_address(end));
        char* output_chr       = reinterpret_cast<char*>(std::to_address(output));

        size_t size = static_cast<size_t>(end_chr - begin_chr);
        std::memcpy(output_chr, begin_chr, size);
        
        //Save like 1 instruction LOL!
        if constexpr (std::is_pointer_v<OutCtgIt>)
            return reinterpret_cast<OutCtgIt>(output_chr + size);
        else
            return output + (end - begin);
    }

    template<std::contiguous_iterator InputCtgIt, std::contiguous_iterator OutputCtgIt>
    OutputCtgIt _range_memcpy(std::move_iterator<InputCtgIt> begin, std::move_iterator<InputCtgIt> end, OutputCtgIt output)
    {
        return _range_memcpy(begin.base(), end.base(), output);
    }

    //Todo: Consider supporting volatility
    template<std::contiguous_iterator InputCtgIt, std::contiguous_iterator OutCtgIt>
    OutCtgIt _range_memmove(InputCtgIt begin, InputCtgIt end, OutCtgIt output)
    {
        const char* begin_chr = reinterpret_cast<const char*>(std::to_address(begin));
        const char* end_chr = reinterpret_cast<const char*>(std::to_address(end));
        char* output_chr = reinterpret_cast<char*>(std::to_address(output));

        size_t size = static_cast<size_t>(end_chr - begin_chr);
        std::memmove(output_chr, begin_chr, size);

        //Save like 1 instruction LOL!
        if constexpr (std::is_pointer_v<OutCtgIt>)
            return reinterpret_cast<OutCtgIt>(output_chr + size);
        else
            return output + (end - begin);
    }

    template<std::contiguous_iterator InputCtgIt, std::contiguous_iterator OutputCtgIt>
    OutputCtgIt _range_memmove(std::move_iterator<InputCtgIt> begin, std::move_iterator<InputCtgIt> end, OutputCtgIt output)
    {
        return _range_memmove(begin.base(), end.base(), output);
    }

    template<std::input_iterator InputIt, typename Alloc>
    constexpr auto uninitialised_copy(Alloc& alloc, InputIt begin, InputIt end, _alloc_value_t<Alloc>* output)
        noexcept(std::is_nothrow_constructible_v<_alloc_value_t<Alloc>, std::iter_reference_t<InputIt>>)
    {
        if constexpr (_is_memcpyable_to_v<InputIt, _alloc_value_t<Alloc>*>) {
            if (!std::is_constant_evaluated())
                return _range_memcpy(begin, end, output);
        }

        _partial_range_al<Alloc> partial_range(alloc, output);
        for (; begin != end; ++begin)
            partial_range.emplace_back(*begin);

        return partial_range.release();
    }

    template<std::input_iterator InputIt, typename Alloc>
    constexpr auto uninitialised_move(Alloc& alloc, InputIt begin, InputIt end, _alloc_value_t<Alloc>* output) 
        noexcept(std::is_nothrow_move_constructible_v<std::iter_value_t<InputIt>>)
    {
        //Todo: Check performance penalty
        return uninitialised_copy(
            alloc, 
            std::make_move_iterator(begin), 
            std::make_move_iterator(end), 
            output);
    }

    template<std::input_iterator InputIt, std::input_iterator OutIt>
    constexpr OutIt copy(InputIt begin, InputIt end, OutIt output) {
        if constexpr (_is_memcpyable_to_v<InputIt, OutIt>) {
            if (!std::is_constant_evaluated())
                return _range_memmove(begin, end, output);
        }

        for (; begin != end; ++begin, ++output)
            *output = *begin;

        return output;
    }

    //IMPORTANT: High risk of memory leak! Returned pointer must be deallocated!
    //Copies range into newly allocated contiguous memory of specified capacity.
    template<std::input_iterator InputIt, typename Alloc>
    [[nodiscard("Memory allocated, but not released!")]] 
    constexpr auto _ctg_duplicate(Alloc& alloc, InputIt begin, InputIt end, _alloc_size_t<Alloc> capacity)
    {
        _alloc_ptr_t<Alloc> first = std::allocator_traits<Alloc>::allocate(alloc, capacity);

        try {
            auto last = uninitialised_copy(alloc, begin, end, std::to_address(first));
            //Todo: Determine performance cost (if any)
            return std::make_pair(first, last);
        }
        catch (...) {
            std::allocator_traits<Alloc>::deallocate(alloc, first, capacity);
            throw;
        }

    }

}

#endif //!SMM_ITER_UTILS_HPP_INCLUDED 