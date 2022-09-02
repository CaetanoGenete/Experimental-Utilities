#ifndef SMM_TRAITS_EXT_HPP_INCLUDED
#define SMM_TRAITS_EXT_HPP_INCLUDED

#include <type_traits>
#include <memory>

namespace expu {

#define EXPU_MACRO_ARG(...) \
    ::expu::_marcro_arg_type<void(__VA_ARGS__)>::type

    template<class T> 
    struct _marcro_arg_type;

    template<class Wrapper, class Type> 
    struct _marcro_arg_type<Wrapper(Type)> { using type = Type; };


    template<class Type, template<class ...> class Base>
    struct is_template_of : public std::bool_constant<false> {};

    template<template <class ...> class Base, class ... Types>
    struct is_template_of<Base<Types...>, Base> : public std::bool_constant<true> {};

    template<class Type, template<class ...> class Base>
    constexpr bool is_template_of_v = is_template_of<Type, Base>::value;

    template<class Type, template<class ...> class Base>
    concept template_of = is_template_of_v<Type, Base>;


    template<class Base, class Derived>
    concept base_of = std::is_base_of_v<Base, Derived>;


    //////////////ALLOCATOR HELPERS///////////////////////////////////////////////////////////////


    template<class Alloc>
    using _alloc_value_t = typename std::allocator_traits<Alloc>::value_type;

    template<class Alloc>
    using _alloc_ptr_t = typename std::allocator_traits<Alloc>::pointer;

    template<class Alloc>
    using _alloc_size_t = typename std::allocator_traits<Alloc>::size_type;

    /*
    //Todo: Add checks for optional features!
    template<class Alloc>
    concept allocator_type = 
        std::copyable<Alloc>            &&
        std::equality_comparable<Alloc> &&
        requires(Alloc& a, const Alloc& a1, const Alloc& a2, _alloc_size_t<Alloc> n, _alloc_ptr_t<Alloc> p) {
            { a.allocate(n)      } -> std::same_as<_alloc_ptr_t<Alloc>>;
            { a.deallocate(p, n) };
            Alloc::value_type;
        };
    */


    //////////////INDEX SEQUENCE META HELPERS///////////////////////////////////////////////////////////////


    template<size_t shift_by, class Seq>
    struct _shift_sequence_by;

    template<size_t shift_by, size_t ... indicies>
    struct _shift_sequence_by<shift_by, std::index_sequence<indicies...>>
    {
        using type = std::index_sequence<(shift_by + indicies) ...>;
    };

    template<size_t from, size_t to>
    using make_index_sequence_from = typename _shift_sequence_by<from, std::make_index_sequence<to - from>>::type;


    //////////////MISCLELLANEOUS///////////////////////////////////////////////////////////////


    //Note: Heavily heavily inspired from equivalent dinkleware object.
    class _not_quite_object {
    public:

        struct construct_tag {
            explicit construct_tag() = default;
        };

        _not_quite_object() = delete;

        constexpr explicit _not_quite_object(construct_tag) noexcept {}

        _not_quite_object(const _not_quite_object&) = delete;
        _not_quite_object& operator=(const _not_quite_object&) = delete;

        void operator&() const = delete;

    protected:
        ~_not_quite_object() = default;
    };


    //////////////MISCLELLANEOUS///////////////////////////////////////////////////////////////


    template<class Type>
    struct unwrap_if_enum { using type = Type; };

    template<class Type>
        requires(std::is_enum_v<Type>)
    struct unwrap_if_enum<Type> { using type = std::underlying_type_t<Type>; };

    template<class Type>
    using unwrap_if_enum_t = typename unwrap_if_enum<Type>::type;


    template<class FirstType, class ... Types>
    using first_t = FirstType;


    template<class Type>
    constexpr decltype(auto) remove_const_cast(Type&& type) noexcept
    {
        return const_cast<std::remove_const_t<decltype(type)>>(std::forward<Type>(type));
    }
}

#endif // SMM_TRAITS_EXT_HPP_INCLUDED
