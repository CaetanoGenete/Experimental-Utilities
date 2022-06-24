#ifndef SMM_MEM_UTILS_HPP_INCLUDED
#define SMM_MEM_UTILS_HPP_INCLUDED

#include <type_traits>

#include "defines.hpp"

namespace smm {

    struct zero_then_variadic{};
    struct one_then_variadic{};

    template<typename Type1, typename Type2, bool = std::is_empty_v<Type1>>
    class compressed_pair final
    {
    public:
        template<typename ... SecondArgs>
        constexpr compressed_pair(zero_then_variadic, SecondArgs&& ... args)
            noexcept(std::is_nothrow_default_constructible_v<Type1>      &&
                     std::is_nothrow_constructible_v<Type2, SecondArgs...>)
            :
            _first(),
            _second(std::forward<SecondArgs>(args)...)
        {}

        template<typename FirstArg, typename ... SecondArgs>
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

    template<typename Type1, typename Type2>
    class compressed_pair<Type1, Type2, true> final : public Type1 
    {
    private:
        using _base_t = Type1;

    public:
        template<typename ... SecondArgs>
        constexpr compressed_pair(zero_then_variadic, SecondArgs&& ... args)
            noexcept(std::is_nothrow_default_constructible_v<_base_t>    &&
                     std::is_nothrow_constructible_v<Type2, SecondArgs...>)
            :
            _base_t(), 
            _second(std::forward<SecondArgs>(args)...)
        {}

        template<typename FirstArg, typename ... SecondArgs>
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
    using _alloc_value_t = typename std::allocator_traits<Alloc>::value_type;

    template<class Alloc>
    using _alloc_ptr_t = typename std::allocator_traits<Alloc>::pointer;

    template<class Alloc>
    using _alloc_size_t = typename std::allocator_traits<Alloc>::size_type;

    template<typename Alloc>
    constexpr void range_destroy_al(Alloc& alloc, _alloc_value_t<Alloc>* begin, _alloc_value_t<Alloc>* end) 
        noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<_alloc_value_t<Alloc>>) {
            for (; begin != end; ++begin)
                std::allocator_traits<Alloc>::destroy(alloc, begin);
        }
    }

    template<typename Alloc>
    struct _partial_range_al 
    {
    public:
        using pointer = _alloc_value_t<Alloc>*;

    private:
        pointer _first, _last;
        Alloc& _alloc;

    public:
        constexpr _partial_range_al(Alloc& _alloc, pointer first) noexcept:
            _first(first), _last(first), _alloc(_alloc) {}

        constexpr ~_partial_range_al() noexcept
        {
            range_destroy_al(_alloc, _first, _last);
        }

    public:
        template<typename ... Args>
        constexpr void emplace_back(Args&& ... args) 
            noexcept(std::is_nothrow_constructible_v<Args...>) 
        {
            std::allocator_traits<Alloc>::construct(_alloc, _last, std::forward<Args>(args)...);
            ++_last;
        }

        constexpr pointer release() noexcept {
            return (_first = _last);
        }
    };

}

#endif // !SMM_MEM_UTILS_HPP_INCLUDED
