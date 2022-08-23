#ifndef EXPU_MEM_UTILS_HPP_INCLUDED
#define EXPU_MEM_UTILS_HPP_INCLUDED

#include <type_traits> //For access to is_nothrow_x, is_trivially_x, etc traits
#include <iterator>    //For access to iterator_traits and iterator concepts
#include <memory>      //For access to memcpy and memmove

#include "expu/meta/meta_utils.hpp"

namespace expu {

    struct zero_then_variadic{};
    struct one_then_variadic{};

    template<class Type1, class Type2, bool = std::is_empty_v<Type1>>
    class compressed_pair final
    {
    public:
        template<class ... SecondArgs>
        constexpr compressed_pair(zero_then_variadic, SecondArgs&& ... args)
            noexcept(std::is_nothrow_default_constructible_v<Type1>      &&
                     std::is_nothrow_constructible_v<Type2, SecondArgs...>)
            :
            _first(),
            _second(std::forward<SecondArgs>(args)...)
        {}

        template<class FirstArg, class ... SecondArgs>
        constexpr compressed_pair(one_then_variadic, FirstArg&& first_arg, SecondArgs&& ... second_args)
            noexcept(std::is_nothrow_constructible_v<Type1, FirstArg>    &&
                     std::is_nothrow_constructible_v<Type2, SecondArgs...>)
            :
            _first(std::forward<FirstArg>(first_arg)),
            _second(std::forward<SecondArgs>(second_args)...)
        {}

    public:
        [[nodiscard]] constexpr Type1& first() noexcept
        {
            return _first;
        }

        [[nodiscard]] constexpr const Type1& first() const noexcept
        {
            return _first;
        }

        [[nodiscard]] constexpr Type2& second() noexcept
        {
            return _second;
        }

        [[nodiscard]] constexpr const Type2& second() const noexcept
        {
            return _second;
        }

    private:
        Type1 _first;
        Type2 _second;
    };

    template<class Type1, class Type2>
    class compressed_pair<Type1, Type2, true> final : public Type1 
    {
    private:
        using _base_t = Type1;

    public:
        template<class ... SecondArgs>
        constexpr compressed_pair(zero_then_variadic, SecondArgs&& ... args)
            noexcept(std::is_nothrow_default_constructible_v<_base_t>    &&
                     std::is_nothrow_constructible_v<Type2, SecondArgs...>)
            :
            _base_t(), 
            _second(std::forward<SecondArgs>(args)...)
        {}

        template<class FirstArg, class ... SecondArgs>
        constexpr compressed_pair(one_then_variadic, FirstArg&& first_arg, SecondArgs&& ... second_args)
            noexcept(std::is_nothrow_constructible_v<_base_t, FirstArg>  && 
                     std::is_nothrow_constructible_v<Type2, SecondArgs...>)
            :
            _base_t(std::forward<FirstArg>(first_arg)), 
            _second(std::forward<SecondArgs>(second_args)...)
        {}

    public:
        [[nodiscard]] constexpr Type1& first() noexcept
        {
            return *this;
        }

        [[nodiscard]] constexpr const Type1& first() const noexcept
        {
            return *this;
        }

        [[nodiscard]] constexpr Type2& second() noexcept
        {
            return _second;
        }

        [[nodiscard]] constexpr const Type2& second() const noexcept
        {
            return _second;
        }

    private:
        Type2 _second;
    };

    template<class Alloc>
    class _checked_allocator;

    //Note: Takes advantage of polymorphism and CTAD to check whether
    //Alloc inherits from some template of checked_allocator.
    template<class Alloc>
    concept _uses_checked_allocator = requires(Alloc & alloc) {
        { _checked_allocator(alloc) } -> expu::base_of<Alloc>;
    };


    template<class Alloc>
    constexpr void _mark_initialised_if_checked_allocator(Alloc& alloc, _alloc_value_t<Alloc>* first, _alloc_size_t<Alloc> size, bool value)
        noexcept
    {
        (void)alloc; (void)first; (void)size; (void)value;
    }

    template<_uses_checked_allocator Alloc>
    constexpr void _mark_initialised_if_checked_allocator(Alloc& alloc, _alloc_value_t<Alloc>* first, _alloc_size_t<Alloc> size, bool value)
    {
        alloc._mark_initialised(first, first + size, value);
    }

    template<class Alloc>
    constexpr void destroy_range(Alloc& alloc, _alloc_value_t<Alloc>* first, _alloc_value_t<Alloc>* last)
        noexcept(std::is_trivially_destructible_v<_alloc_value_t<Alloc>>)
    {
        if constexpr (!std::is_trivially_destructible_v<_alloc_value_t<Alloc>>) {
            for (; first != last; ++first)
                std::allocator_traits<Alloc>::destroy(alloc, first);
        }
        else
            _mark_initialised_if_checked_allocator(alloc, first, last - first, false);
    }

    template<class Alloc>
    requires(!std::same_as<_alloc_ptr_t<Alloc>, _alloc_value_t<Alloc>*>)
    constexpr void destroy_range(Alloc& alloc, const _alloc_ptr_t<Alloc> first, const _alloc_ptr_t<Alloc> last)
        noexcept(std::is_trivially_destructible_v<_alloc_value_t<Alloc>>)
    {
        return destroy_range(alloc, std::to_address(first), std::to_address(last));
    }

    template<class Alloc>
    struct _partial_range 
    {
    public:
        using value_type = _alloc_value_t<Alloc>;
        using pointer    = value_type*;

    private:
        pointer _first, _last;
        Alloc& _alloc;

    public:
        constexpr _partial_range(Alloc& _alloc, pointer first) noexcept:
            _first(first), _last(first), _alloc(_alloc) {}

        constexpr ~_partial_range() noexcept
        {
            destroy_range(_alloc, _first, _last);
        }

    public:
        template<class ... Args>
        constexpr void emplace_back(Args&& ... args) 
            //noexcept(std::is_nothrow_constructible_v<value_type, Args...>)
        {
            std::allocator_traits<Alloc>::construct(_alloc, _last, std::forward<Args>(args)...);
            ++_last;
        }

        constexpr pointer release() noexcept 
        {
            _first = _last;
            return _first;
        }
    };

    template<class Alloc>
    struct _partial_backward_range
    {
    public:
        using value_type = _alloc_value_t<Alloc>;
        using pointer    = value_type*;

    private:
        pointer _first, _last;
        Alloc& _alloc;

    public:
        constexpr _partial_backward_range(Alloc& _alloc, pointer last) noexcept :
            _first(last), _last(last), _alloc(_alloc) {}

        constexpr ~_partial_backward_range() noexcept
        {
            destroy_range(_alloc, _first, _last);
        }

    public:
        template<class ... Args>
        constexpr void emplace_back(Args&& ... args)
            //noexcept(std::is_nothrow_constructible_v<value_type, Args...>)
        {
            std::allocator_traits<Alloc>::construct(_alloc, --_first, std::forward<Args>(args)...);
        }

        constexpr pointer release() noexcept
        {
            _last = _first;
            return _first;
        }
    };


    /////////////////////////////////////RANGEIFIED C-FUNCTIONS///////////////////////////////////////////////////////////////////


    template<class Type>
    struct _unwrap_if_enum { using type = Type; };

    template<class Type>
    requires(std::is_enum_v<Type>)
    struct _unwrap_if_enum<Type> { using type = std::underlying_type_t<Type>; };
    
    template<class Type>
    using _unwrap_if_enum_t = typename _unwrap_if_enum<Type>::type;

    template<class Iter>
    constexpr bool _is_ctg_move_iter = false;

    template<class Iter>
    constexpr bool _is_ctg_move_iter<std::move_iterator<Iter>> = std::contiguous_iterator<Iter>;

    template<class Iter>
    concept _ctg_or_ctg_move_iter = _is_ctg_move_iter<Iter> || std::contiguous_iterator<Iter>;

    template<std::input_or_output_iterator Iter, std::sentinel_for<Iter> Sentinel>
    constexpr bool _is_ctg_sized_range = _ctg_or_ctg_move_iter<Iter> && std::sized_sentinel_for<Sentinel, Iter>;

    template<
        std::input_or_output_iterator SrcIter,
        std::input_or_output_iterator DestIter,
        std::sentinel_for<SrcIter> SrcSentinel = SrcIter>
    struct _actually_trivially
    {
    private:
        using _src_type  = _unwrap_if_enum_t<std::iter_value_t<SrcIter>>;
        using _dest_type = _unwrap_if_enum_t<std::iter_value_t<DestIter>>;
        
        //Todo: Add support for aggregate types as much as possible
        //Types are compatible iff they are either:
        // 1. The same type.
        // 2. Both integral and of the same size (exception: if _dest_type is bool, _src_type must also be bool).
        // 3. Both floating point numbers and of the same size.
        // 4. If either are enums, then 1-3 must hold for their respect underlying type(s).
        static constexpr bool _compatible =
                std::is_same_v<_src_type, _dest_type> || (sizeof(_src_type) == sizeof(_dest_type) &&
                std::is_same_v<_src_type, bool>       >= std::is_same_v<_dest_type, bool>         &&
                std::is_integral_v<_src_type>         == std::is_integral_v<_dest_type>           &&
                std::is_floating_point_v<_src_type>   == std::is_floating_point_v<_dest_type>);

        //Note: mem-x functions can only be used with trivially_constructible types and contiguous iterators
        static constexpr bool _potentially_trivial =
            std::is_trivially_copyable_v<_src_type>   &&
            _is_ctg_sized_range<SrcIter, SrcSentinel> &&
            std::contiguous_iterator<DestIter>        &&
            _compatible;

    public:
        static constexpr bool constructible = 
            _potentially_trivial &&
            std::is_constructible_v<
                std::iter_value_t<DestIter>, 
                std::iter_reference_t<SrcIter>>;

        static constexpr bool assignable =
            _potentially_trivial &&
            std::is_assignable_v<
                std::iter_reference_t<DestIter>,
                std::iter_reference_t<SrcIter>>;
    };

    template<bool not_overlapping>
    void* _memcpy_or_memmove(void* dest, const void* src, size_t size) 
    {
        if constexpr (not_overlapping)
            return std::memcpy(dest, src, size);
        else
            return std::memmove(dest, src, size);
    }

    //Todo: Consider adding optimisation in the case where SrcSizedSentinel is a contiguous iterator or do away with sized_sentinel (if found to not be needed)
    template<
        bool not_overlapping,
        std::contiguous_iterator SrcCtgIt,
        std::sized_sentinel_for<SrcCtgIt> SrcSizedSentinel,
        std::contiguous_iterator DestCtgIt>
    DestCtgIt _range_memcpy_or_memmove(SrcCtgIt first, SrcSizedSentinel last, DestCtgIt output)
    {
        auto output_chr = const_cast<char*>(reinterpret_cast<const volatile char*>(std::to_address(output)));

        size_t count = static_cast<size_t>(last - first);
        size_t size  = count * sizeof(std::iter_value_t<SrcCtgIt>);

        _memcpy_or_memmove<not_overlapping>(output_chr, std::to_address(first), size);
        return output + count;
    }

    template<class Type>
    constexpr bool _is_move_wrapper = is_template_of_v<Type, std::move_iterator> || 
                                      is_template_of_v<Type, std::move_sentinel>;

    template<
        bool not_overlapping,
        std::contiguous_iterator SrcCtgIt,
        std::sized_sentinel_for<std::move_iterator<SrcCtgIt>> SrcSizedSentinel,
        std::contiguous_iterator DestCtgIt>
    DestCtgIt _range_memcpy_or_memmove(std::move_iterator<SrcCtgIt> first, SrcSizedSentinel last, DestCtgIt output)
    {
        if constexpr (_is_move_wrapper<SrcSizedSentinel>)
            return _range_memcpy_or_memmove<not_overlapping>(first.base(), last.base(), output);
        else
            return _range_memcpy_or_memmove<not_overlapping>(first.base(), last, output);
    }

    template<
        _ctg_or_ctg_move_iter SrcCtgIt,
        std::sized_sentinel_for<SrcCtgIt> SizedSentinel,
        std::contiguous_iterator DestCtgIt>
    DestCtgIt _range_memcpy(SrcCtgIt first, SizedSentinel last, DestCtgIt output)
    {
        return _range_memcpy_or_memmove<true>(first, last, output);
    }

    template<
        _ctg_or_ctg_move_iter SrcCtgIt,
        std::sized_sentinel_for<SrcCtgIt> SizedSentinel,
        std::contiguous_iterator DestCtgIt>
    DestCtgIt _range_memmove(SrcCtgIt first, SizedSentinel last, DestCtgIt output)
    {
        return _range_memcpy_or_memmove<false>(first, last, output);
    }

    template<
        bool not_overlapping,
        std::contiguous_iterator SrcCtgIt,
        std::sized_sentinel_for<SrcCtgIt> SizedSentinel,
        std::contiguous_iterator DestCtgIt>
    DestCtgIt _range_backward_memcpy_or_memmove(SizedSentinel first, SrcCtgIt last, DestCtgIt output)
    {
        char* output_chr = const_cast<char*>(reinterpret_cast<volatile char*>(std::to_address(output)));

        size_t count = static_cast<size_t>(last - first);
        size_t size  = count * sizeof(std::iter_value_t<SrcCtgIt>);

        if constexpr (std::contiguous_iterator<SizedSentinel>)
            _memcpy_or_memmove<not_overlapping>(output_chr - size, std::to_address(first), size);
        else
            _memcpy_or_memmove<not_overlapping>(output_chr - size, std::to_address(last) - count, size);

        return output - count;
    }

    template<
        bool not_overlapping,
        std::contiguous_iterator SrcCtgIt,
        std::sized_sentinel_for<std::move_iterator<SrcCtgIt>> SizedSentinel,
        std::contiguous_iterator OutputCtgIt>
    OutputCtgIt _range_backward_memcpy_or_memmove(SizedSentinel first, std::move_iterator<SrcCtgIt> last, OutputCtgIt output)
    {
        if constexpr (_is_move_wrapper<SizedSentinel>)
            return _range_backward_memcpy_or_memmove<not_overlapping>(first.base(), last.base(), output);
        else
            return _range_backward_memcpy_or_memmove<not_overlapping>(first, last.base(), output);
    }

    template<
        std::contiguous_iterator SrcCtgIt,
        std::sized_sentinel_for<SrcCtgIt> SizedSentinel,
        std::contiguous_iterator DestCtgIt>
    DestCtgIt _range_backward_memcpy(SizedSentinel first, SrcCtgIt last, DestCtgIt output) 
    {
        return _range_backward_memcpy_or_memmove<true>(first, last, output);
    }

    template<
        std::contiguous_iterator SrcCtgIt,
        std::sized_sentinel_for<SrcCtgIt> SizedSentinel,
        std::contiguous_iterator DestCtgIt>
    DestCtgIt _range_backward_memmove(SizedSentinel first, SrcCtgIt last, DestCtgIt output)
    {
        return _range_backward_memcpy_or_memmove<false>(first, last, output);
    }


/////////////////////////////////////UNINITIALISED RANGE FUNCTIONS///////////////////////////////////////////////////////////////////

    template<
        std::input_iterator InputIt,
        std::sentinel_for<InputIt> Sentinel,
        class Alloc>
    constexpr auto uninitialised_copy(Alloc& alloc, InputIt first, Sentinel last, _alloc_value_t<Alloc>* output)
        noexcept(std::is_nothrow_constructible_v<_alloc_value_t<Alloc>, std::iter_reference_t<InputIt>>)
    {
        if constexpr (_actually_trivially<InputIt, _alloc_value_t<Alloc>*, Sentinel>::constructible) {
            if (!std::is_constant_evaluated()) {
                _mark_initialised_if_checked_allocator(alloc, output, last - first, true);
                return _range_memcpy(first, last, output);
            }
        }

        _partial_range<Alloc> partial_range(alloc, output);
        for (; first != last; ++first)
            partial_range.emplace_back(*first);

        return partial_range.release();
    }

    template<
        std::bidirectional_iterator BiDirIt,
        std::sentinel_for<BiDirIt> Sentinel,
        class Alloc>
    constexpr auto uninitialised_backward_copy(Alloc& alloc, Sentinel first, BiDirIt last, _alloc_value_t<Alloc>* output)
        noexcept(std::is_nothrow_constructible_v<_alloc_value_t<Alloc>, std::iter_reference_t<BiDirIt>>)
    {
        if constexpr (_actually_trivially<BiDirIt, _alloc_value_t<Alloc>*, Sentinel>::constructible) {
            if (!std::is_constant_evaluated()) {
                size_t range_size = static_cast<size_t>(last - first);
                _mark_initialised_if_checked_allocator(alloc, output - range_size, range_size, true);

                return _range_backward_memcpy(first, last, output);
            }
        }

        _partial_backward_range<Alloc> partial_range(alloc, output);
        while (first != last)
            partial_range.emplace_back(*(--last));

        return partial_range.release();
    }

    template<
        std::input_iterator InputIt,
        std::sentinel_for<InputIt> Sentinel,
        class Alloc>
    constexpr auto uninitialised_move(Alloc& alloc, InputIt first, Sentinel last, _alloc_value_t<Alloc>* output)
        noexcept(std::is_nothrow_move_constructible_v<std::iter_value_t<InputIt>>)
    {
        //Todo: Check performance penalty
        return uninitialised_copy(
            alloc,
            std::make_move_iterator(first),
            std::move_sentinel(last),
            output);
    }

    //Todo: memset for compatible types
    template<class Type, class Alloc>
    requires(std::is_constructible_v<_alloc_value_t<Alloc>, Type>)
    constexpr void uninitialised_fill(Alloc& alloc, _alloc_value_t<Alloc>* first, const _alloc_value_t<Alloc>* const last, const Type& value) 
        noexcept(std::is_nothrow_constructible_v<_alloc_value_t<Alloc>, Type>)
    {
        _partial_range<Alloc> partial_range(alloc, first);
        for (; first != last; ++first)
            partial_range.emplace_back(value);

        partial_range.release();
    }

    //Todo: memset for compatible types
    template<class Type, class Alloc>
    requires(std::is_constructible_v<_alloc_value_t<Alloc>, Type>)
    constexpr auto uninitialised_fill_n(Alloc& alloc, _alloc_value_t<Alloc>* first, size_t n, const Type& value)
        noexcept(std::is_nothrow_constructible_v<_alloc_value_t<Alloc>, Type>)
    {
        _partial_range<Alloc> partial_range(alloc, first);
        while(n--)
            partial_range.emplace_back(value);

        return partial_range.release();
    }


/////////////////////////////////////INITIALISED RANGE FUNCTIONS///////////////////////////////////////////////////////////////////


    template<class OutIt, class InputIt>
    concept _output_iterator_for = std::input_iterator<InputIt> && std::output_iterator<OutIt, std::iter_reference_t<InputIt>>;

    template<
        std::input_iterator InputIt,
        std::sentinel_for<InputIt> Sentinel,
        _output_iterator_for<InputIt> OutIt>
    constexpr OutIt copy(InputIt first, Sentinel last, OutIt output) {
        if constexpr (_actually_trivially<InputIt, OutIt, Sentinel>::assignable) {
            if (!std::is_constant_evaluated())
                return _range_memmove(first, last, output);
        }

        for (; first != last; ++first, ++output)
            *output = *first;

        return output;
    }

    template<
        std::input_iterator InputIt,
        std::sentinel_for<InputIt> Sentinel,
        _output_iterator_for<InputIt> OutIt>
    constexpr OutIt move(InputIt first, Sentinel last, OutIt output) 
    {
        return copy(
            std::make_move_iterator(first),
            std::move_sentinel(last),
            output);
    }

    template<
        std::bidirectional_iterator BiDirIt,
        std::sentinel_for<BiDirIt> Sentinel,
        _output_iterator_for<BiDirIt> OutIt>
    constexpr OutIt backward_copy(Sentinel first, BiDirIt last, OutIt output) 
    {
        if constexpr (_actually_trivially<BiDirIt, OutIt, Sentinel>::assignable) {
            if (!std::is_constant_evaluated())
                return _range_backward_memmove(first, last, output);
        }

        while (first != last)
            *(--output) = *(--first);

        return output;
    }

    template<
        std::bidirectional_iterator BiDirIt,
        std::sentinel_for<BiDirIt> Sentinel,
        _output_iterator_for<BiDirIt> OutIt>
    constexpr OutIt backward_move(Sentinel first, BiDirIt last, OutIt output) 
    {
        if constexpr (_actually_trivially<BiDirIt, OutIt, Sentinel>::assignable) {
            if (!std::is_constant_evaluated())
                return _range_backward_memmove(first, last, output);
        }

        while (first != last)
            *(--output) = std::move(*(--last));

        return output;
    }

    template<
        std::input_iterator InputIt,
        _output_iterator_for<InputIt> OutIt,
        std::sentinel_for<OutIt> Sentinel>
    constexpr InputIt copy_until_sentinel(InputIt first, OutIt out_first, Sentinel out_last)
    {
        if constexpr (_actually_trivially<InputIt, OutIt>::assignable && std::sized_sentinel_for<Sentinel, OutIt>) {
            if (!std::is_constant_evaluated())
                return _range_memmove(first, first + (out_last - out_first), out_first);
        }

        for (; out_first != out_last; ++first, ++out_first)
            *out_first = *first;

        return first;
    }

    template<
        std::input_iterator InputIt,
        std::sentinel_for<InputIt> Sentinel,
        class Alloc>
    [[nodiscard]] constexpr auto _ctg_duplicate(Alloc& alloc, InputIt first, Sentinel last, _alloc_ptr_t<Alloc>& output, _alloc_size_t<Alloc> capacity)
    {
        output = std::allocator_traits<Alloc>::allocate(alloc, capacity);

        try {
            return uninitialised_copy(alloc, first, last, std::to_address(output));
        }
        catch (...) {
            std::allocator_traits<Alloc>::deallocate(alloc, output, capacity);
            output = nullptr;
            throw;
        }

    }
}

#endif // !EXPU_MEM_UTILS_HPP_INCLUDED
