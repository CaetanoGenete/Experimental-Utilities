#ifndef EXPU_MEM_UTILS_HPP_INCLUDED
#define EXPU_MEM_UTILS_HPP_INCLUDED

#include <type_traits> //For access to is_nothrow_x, is_trivially_x, etc traits
#include <iterator>    //For access to iterator_traits and iterator concepts
#include <memory>      //For access to memcpy and memmove

#include "expu/maths/basic_maths.hpp"

#include "expu/meta/meta_utils.hpp"

namespace expu {

    struct zero_then_variadic{};
    struct one_then_variadic{};

    
    //////////////////////////////////////COMPRESSED PAIR ///////////////////////////////////////////////////////////////////////////////


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


    //////////////////////////////////////CHECKED ALLOCATOR HELPER FUNCTIONS///////////////////////////////////////////////////////////////////////////////


    template<class Alloc, bool>
    class _checked_allocator;

    //Note: Takes advantage of polymorphism and CTAD to check whether
    //Alloc inherits from some template of checked_allocator. Works
    //because _checked_allocator has a copy constructor defined for
    //all template arguments.
    template<class Alloc>
    concept _uses_checked_allocator = requires(Alloc & alloc) {
        { _checked_allocator(alloc) } -> expu::base_of<Alloc>;
    };


    template<class Alloc>
    constexpr void _mark_initialised_if_checked_allocator(Alloc& alloc, const void* const first, const void* const last, bool value)
        noexcept
    {
        (void)alloc; (void)first; (void)last; (void)value;
    }

    template<_uses_checked_allocator Alloc>
    constexpr void _mark_initialised_if_checked_allocator(Alloc& alloc, void* const first, const void* const last, bool value)
    {
        alloc._mark_initialised(first, last, value);
    }


    //////////////////////////////////////HELPER EXPECTION HANDLING FUNCTIONS//////////////////////////////////////////////////////////////////


    template<class Alloc, class Type> 
    constexpr void destroy_range(Alloc& alloc, Type* first, Type* last)
        noexcept(std::is_trivially_destructible_v<Type>);

    template<class Alloc> 
    constexpr void destroy_range(Alloc& alloc, const _alloc_ptr_t<Alloc> first, const _alloc_ptr_t<Alloc> last)
        noexcept(std::is_trivially_destructible_v<_alloc_value_t<Alloc>>);

    template<class Alloc, class Type>
    struct _partial_range 
    {
    public:
        using value_type = _alloc_value_t<Alloc>;
        using pointer    = Type*;

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

    template<class Alloc, class Type>
    struct _partial_backward_range
    {
    public:
        using value_type = _alloc_value_t<Alloc>;
        using pointer    = Type*;

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

    /////////////////////////////////////WRAPPED ITERATOR HELPERS///////////////////////////////////////////////////////////////////


    template<class Type>
    constexpr auto _unwrapped(const Type type)
        noexcept(std::is_copy_constructible_v<Type>)
    {
        return type; //By default, do nothing.
    }

    template<class Iter>
    constexpr auto _unwrapped(const std::move_iterator<Iter> move_iter)
        noexcept(noexcept(move_iter.base()))
    {
        return move_iter.base();
    }

    template<class Sentinel>
    constexpr auto _unwrapped(const std::move_sentinel<Sentinel> move_sentinel)
        noexcept(noexcept(move_sentinel.base()))
    {
        return move_sentinel.base();
    }


    template<class Type>
    using _unwrapped_t = decltype(_unwrapped(std::declval<Type>()));

    template<class Iter>
    concept _ctg_or_ctg_move_iter = std::contiguous_iterator<_unwrapped_t<Iter>>;


    template<class Sentinel>
    constexpr auto _make_move_sentinel(const Sentinel sentinel)
        noexcept(std::is_nothrow_move_constructible_v<Sentinel>)
    {
        return std::move_sentinel(std::move(sentinel));
    }

    template<class Iter>
    constexpr auto _make_move_sentinel(const std::move_iterator<Iter> move_iter)
        noexcept(std::is_nothrow_copy_constructible_v<Iter>)
    {
        return move_iter; //Do nothing if move iterator, to avoid unnecessary wrapping
    }


    /////////////////////////////////////RANGEIFIED C-FUNCTIONS///////////////////////////////////////////////////////////////////


    template<class Alloc, class Type>
    constexpr void destroy_range(Alloc& alloc, Type* first, Type* last)
        noexcept(std::is_trivially_destructible_v<Type>)
    {
        if constexpr (!std::is_trivially_destructible_v<Type>) {
            for (; first != last; ++first)
                std::allocator_traits<Alloc>::destroy(alloc, first);
        }
        else
            _mark_initialised_if_checked_allocator(alloc, first, last, false);
    }

    template<class Alloc>
    requires(!std::is_pointer_v<_alloc_ptr_t<Alloc>>)
    constexpr void destroy_range(Alloc& alloc, const _alloc_ptr_t<Alloc> first, const _alloc_ptr_t<Alloc> last)
        noexcept(std::is_trivially_destructible_v<_alloc_value_t<Alloc>>)
    {
        return destroy_range(alloc, std::to_address(first), std::to_address(last));
    }


    template<
        std::input_or_output_iterator SrcIter,
        std::input_or_output_iterator DestIter,
        std::sentinel_for<SrcIter> SrcSentinel = SrcIter>
    struct _actually_trivially
    {
    private:
        using _src_type  = unwrap_if_enum_t<std::iter_value_t<SrcIter>>;
        using _dest_type = unwrap_if_enum_t<std::iter_value_t<DestIter>>;
        
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
            std::is_trivially_copyable_v<_src_type>         &&
            std::contiguous_iterator<_unwrapped_t<SrcIter>> &&
            std::sized_sentinel_for<SrcSentinel, SrcIter>   &&
            std::contiguous_iterator<DestIter>              &&
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
    void* _memcpy_or_memmove(void* dest, const void* src, size_t size) noexcept
    {
        if constexpr (not_overlapping)
            return std::memcpy(dest, src, size);
        else
            return std::memmove(dest, src, size);
    }

    template<bool not_overlapping>
    struct _range_memcpy_or_memmove : private _not_quite_object
    {
        using _not_quite_object::_not_quite_object;

        template<
            std::contiguous_iterator SrcCtgIt,
            std::sized_sentinel_for<SrcCtgIt> SrcSizedSentinel,
            std::contiguous_iterator DestCtgIt>
        DestCtgIt operator()(SrcCtgIt first, SrcSizedSentinel last, DestCtgIt output) const noexcept
        {
            auto output_chr = const_cast<char*>(reinterpret_cast<const volatile char*>(std::to_address(output)));

            size_t count = static_cast<size_t>(last - first);
            size_t size = count * sizeof(std::iter_value_t<SrcCtgIt>);

            _memcpy_or_memmove<not_overlapping>(output_chr, std::to_address(first), size);
            return output + count;
        }
    };     

    inline constexpr _range_memcpy_or_memmove<true>  _range_memcpy {_not_quite_object::construct_tag{}};
    inline constexpr _range_memcpy_or_memmove<false> _range_memmove{_not_quite_object::construct_tag{}};


    template<bool not_overlapping>
    struct _range_backward_memcpy_or_memmove : private _not_quite_object
    {
        using _not_quite_object::_not_quite_object;

        template<
            std::contiguous_iterator SrcCtgIt,
            std::sized_sentinel_for<SrcCtgIt> SizedSentinel,
            std::contiguous_iterator DestCtgIt>
        DestCtgIt operator()(SizedSentinel first, SrcCtgIt last, DestCtgIt output) const noexcept
        {
            char* output_chr = const_cast<char*>(reinterpret_cast<volatile char*>(std::to_address(output)));

            size_t count = static_cast<size_t>(last - first);
            size_t size = count * sizeof(std::iter_value_t<SrcCtgIt>);

            //No need to calculate address of first range element. 
            if constexpr (std::contiguous_iterator<SizedSentinel>)
                _memcpy_or_memmove<not_overlapping>(output_chr - size, std::to_address(first), size);
            else
                _memcpy_or_memmove<not_overlapping>(output_chr - size, std::to_address(last) - count, size);

            return output - count;
        }
    };

    inline constexpr _range_backward_memcpy_or_memmove<true>  _range_backward_memcpy {_not_quite_object::construct_tag{}};
    inline constexpr _range_backward_memcpy_or_memmove<false> _range_backward_memmove{_not_quite_object::construct_tag{}};  


    template<
        std::contiguous_iterator CtgIt,
        std::sized_sentinel_for<CtgIt> SizedSentinel>
    void* set_bits(void* const bits, CtgIt first, const SizedSentinel last)
    {
        const size_t bytes_count = right_shift_round_up(std::ranges::distance(first, last), 3);

        auto bytes = static_cast<char*>(bits);

        for (size_t index = 0; index < bytes_count - 1; ++index, ++bytes) {
            unsigned char value = 0;
            for (unsigned char mask = 1; mask != 0; mask <<= 1, ++first)
                value |= mask * static_cast<bool>(*first);

            std::memset(bytes, value, 1);
        }

        unsigned char value = 0;
        for (unsigned char mask = 1; first != last; mask <<= 1, ++first)
            value |= mask * static_cast<bool>(*first);

        std::memset(bytes, value, 1);
        return ++bytes;
    }

    template<
        class Alloc,
        std::contiguous_iterator CtgIt,
        std::sized_sentinel_for<CtgIt> SizedSentinel> 
    void* set_bits(Alloc& alloc, void* const bits, CtgIt first, const SizedSentinel last)
    {
        void* const result = set_bits(bits, first, last);
        _mark_initialised_if_checked_allocator(alloc, bits, result, true);

        return result;
    }

/////////////////////////////////////UNINITIALISED RANGE FUNCTIONS///////////////////////////////////////////////////////////////////

    template<
        class Alloc, 
        class Type,
        std::input_iterator InputIt,
        std::sentinel_for<InputIt> Sentinel>
    constexpr auto uninitialised_copy(Alloc& alloc, InputIt first, Sentinel last, Type* output)
        noexcept(std::is_nothrow_constructible_v<Type, std::iter_reference_t<InputIt>>)
    {
        if constexpr (_actually_trivially<InputIt, Type*, Sentinel>::constructible) {
            if (!std::is_constant_evaluated()) {
                auto result = _range_memcpy(_unwrapped(first), _unwrapped(last), output);
                _mark_initialised_if_checked_allocator(alloc, output, result, true);
                return result;
            }
        }

        _partial_range<Alloc, Type> partial_range(alloc, output);
        for (; first != last; ++first)
            partial_range.emplace_back(*first);

        return partial_range.release();
    }

    template<
        class Alloc,
        class Type,
        std::bidirectional_iterator BiDirIt,
        std::sentinel_for<BiDirIt> Sentinel>
    constexpr auto uninitialised_backward_copy(Alloc& alloc, Sentinel first, BiDirIt last, Type* output)
        noexcept(std::is_nothrow_constructible_v<Type, std::iter_reference_t<BiDirIt>>)
    {
        if constexpr (_actually_trivially<BiDirIt, Type*, Sentinel>::constructible) {
            if (!std::is_constant_evaluated()) {
                auto result = _range_backward_memcpy(_unwrapped(first), _unwrapped(last), output);
                _mark_initialised_if_checked_allocator(alloc, result, output, true);
                return result;
            }
        }

        _partial_backward_range<Alloc, Type> partial_range(alloc, output);
        while (first != last)
            partial_range.emplace_back(*(--last));

        return partial_range.release();
    }

    template<
        class Alloc,
        class Type,
        std::input_iterator InputIt,
        std::sentinel_for<InputIt> Sentinel>
    constexpr auto uninitialised_move(Alloc& alloc, InputIt first, Sentinel last, Type* output)
        noexcept(std::is_nothrow_constructible_v<Type, std::iter_reference_t<InputIt>>)
    {
        //Todo: Check performance penalty
        return uninitialised_copy(
            alloc,
            std::make_move_iterator(first),
            _make_move_sentinel(last),
            output);
    }

    //Todo: memset for compatible types
    template<class Type, class Alloc, class DestType>
    constexpr void uninitialised_fill(Alloc& alloc, DestType* first, const DestType* const last, const Type& value)
        noexcept(std::is_nothrow_constructible_v<DestType, Type>)
    {
        _partial_range<Alloc, Type> partial_range(alloc, first);
        for (; first != last; ++first)
            partial_range.emplace_back(value);

        partial_range.release();
    }

    //Todo: memset for compatible types
    template<class Type, class Alloc, class DestType>
    constexpr auto uninitialised_fill_n(Alloc& alloc, DestType* first, size_t n, const Type& value)
        noexcept(std::is_nothrow_constructible_v<DestType, Type>)
    {
        _partial_range<Alloc, Type> partial_range(alloc, first);
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
                return _range_memmove(_unwrapped(first), _unwrapped(last), output);
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
            _make_move_sentinel(last),
            output);
    }

    template<
        class BiDirIt,
        std::sentinel_for<BiDirIt> Sentinel,
        _output_iterator_for<BiDirIt> OutIt>
    constexpr OutIt backward_copy(Sentinel first, BiDirIt last, OutIt output) 
        requires(std::bidirectional_iterator<_unwrapped_t<BiDirIt>>)
    {
        if constexpr (_actually_trivially<BiDirIt, OutIt, Sentinel>::assignable) {
            if (!std::is_constant_evaluated())
                return _range_backward_memmove(_unwrapped(first), _unwrapped(last), output);
        }

        while (first != last)
            *(--output) = *(--last);

        return output;
    }

    template<
        class BiDirIt,
        std::sentinel_for<BiDirIt> Sentinel,
        _output_iterator_for<BiDirIt> OutIt>
    constexpr OutIt backward_move(Sentinel first, BiDirIt last, OutIt output) 
        requires(std::bidirectional_iterator<_unwrapped_t<BiDirIt>>)
    {
        return backward_copy(
            _make_move_sentinel(first),
            std::make_move_iterator(last),
            output);
    }

    template<
        std::input_iterator InputIt,
        _output_iterator_for<InputIt> OutIt,
        std::sentinel_for<OutIt> Sentinel>
    constexpr InputIt copy_until_sentinel(InputIt first, OutIt out_first, Sentinel out_last)
    {
        if constexpr (_actually_trivially<InputIt, OutIt>::assignable && std::sized_sentinel_for<Sentinel, OutIt>) {
            if (!std::is_constant_evaluated()) 
                return _range_memmove(_unwrapped(first), _unwrapped(first) + (out_last - out_first), out_first);
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
            output = nullptr; //Not strictly necessary
            throw;
        }

    }
}

#endif // !EXPU_MEM_UTILS_HPP_INCLUDED
