#ifndef EXPU_LEGACY_MEM_UTILS_HPP_INCLUDED
#define EXPU_LEGACY_MEM_UTILS_HPP_INCLUDED

#include "../mem_utils.hpp"

namespace expu::legacy {

    template<
        bool not_overlapping,
        std::contiguous_iterator SrcCtgIt,
        std::sized_sentinel_for<SrcCtgIt> SizedSentinel,
        std::contiguous_iterator DestCtgIt>
    DestCtgIt _range_backward_memcpy_or_memmove(SizedSentinel first, SrcCtgIt last, DestCtgIt output)
    {
        char* output_chr = const_cast<char*>(reinterpret_cast<volatile char*>(std::to_address(output)));

        size_t count = static_cast<size_t>(last - first);
        size_t size = count * sizeof(std::iter_value_t<SrcCtgIt>);

        if constexpr (std::contiguous_iterator<SizedSentinel>)
            _memcpy_or_memmove<not_overlapping>(output_chr - size, std::to_address(first), size);
        else
            _memcpy_or_memmove<not_overlapping>(output_chr - size, std::to_address(last) - count, size);

        return output - count;
    }

    template<
        std::contiguous_iterator SrcCtgIt,
        std::sized_sentinel_for<SrcCtgIt> SizedSentinel,
        std::contiguous_iterator DestCtgIt>
    DestCtgIt _range_backward_memcpy(SizedSentinel first, SrcCtgIt last, DestCtgIt output)
    {
        return _range_backward_memcpy_or_memmove<true>(_unwrapped(first), _unwrapped(last), output);
    }

    template<
        std::contiguous_iterator SrcCtgIt,
        std::sized_sentinel_for<SrcCtgIt> SizedSentinel,
        std::contiguous_iterator DestCtgIt>
    DestCtgIt _range_backward_memmove(SizedSentinel first, SrcCtgIt last, DestCtgIt output)
    {
        return _range_backward_memcpy_or_memmove<false>(_unwrapped(first), _unwrapped(last), output);
    }

    template<
        bool not_overlapping,
        std::contiguous_iterator SrcCtgIt,
        std::sized_sentinel_for<SrcCtgIt> SrcSizedSentinel,
        std::contiguous_iterator DestCtgIt>
    DestCtgIt _range_memcpy_or_memmove(SrcCtgIt first, SrcSizedSentinel last, DestCtgIt output)
    {
        auto output_chr = const_cast<char*>(reinterpret_cast<const volatile char*>(std::to_address(output)));

        size_t count = static_cast<size_t>(last - first);
        size_t size = count * sizeof(std::iter_value_t<SrcCtgIt>);

        _memcpy_or_memmove<not_overlapping>(output_chr, std::to_address(first), size);
        return output + count;
    }

    template<
        _ctg_or_ctg_move_iter SrcCtgIt,
        std::sized_sentinel_for<SrcCtgIt> SizedSentinel,
        std::contiguous_iterator DestCtgIt>
    DestCtgIt _range_memcpy(SrcCtgIt first, SizedSentinel last, DestCtgIt output)
    {
        return _range_memcpy_or_memmove<true>(_unwrapped(first), _unwrapped(last), output);
    }

    template<
        _ctg_or_ctg_move_iter SrcCtgIt,
        std::sized_sentinel_for<SrcCtgIt> SizedSentinel,
        std::contiguous_iterator DestCtgIt>
    DestCtgIt _range_memmove(SrcCtgIt first, SizedSentinel last, DestCtgIt output)
    {
        return _range_memcpy_or_memmove<false>(_unwrapped(first), _unwrapped(last), output);
    }
}

#endif // !EXPU_LEGACY_MEM_UTILS_HPP_INCLUDED