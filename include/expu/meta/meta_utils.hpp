#ifndef SMM_TRAITS_EXT_HPP_INCLUDED
#define SMM_TRAITS_EXT_HPP_INCLUDED

#include <type_traits>
#include <string>
#include <memory>
#include <tuple>
#include <array>
#include <concepts>

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


    //////////////TUPLE META HELPERS/////////////////////////////////////////////////////////////////////////


    template<class ... Tuples>
    using tuple_concat_t = decltype(std::tuple_cat(std::declval<Tuples>()...));
    

    template<class ... Types, size_t ... Indicies>
    constexpr auto tuple_subset(std::tuple<Types...>&& tuple, std::index_sequence<Indicies...>)
    {
        return std::make_tuple(std::get<Indicies>(tuple)...);
    }

    template<class Tuple, class Seq>
    using tuple_subset_t = decltype(tuple_subset(std::declval<Tuple&&>(), std::declval<Seq>()));


    template<class FirstType, class ... Types>
    using first_t = FirstType;

    /*
    LEGACY: log(n) instantiation depth cartesian product on std::tuple

    template<class Tuple>
    struct halve_tuple {
    private:
        static constexpr size_t _tuple_size = std::tuple_size_v<Tuple>;
        static constexpr size_t _half_size  = _tuple_size / 2;

    public:
        using lhs_type = tuple_subset_t<Tuple, std::make_index_sequence<_half_size>>;
        using rhs_type = tuple_subset_t<Tuple, make_index_sequence_from<_half_size, _tuple_size>>;

    };

    template<class LhsTuple, class RhsTuple>
    struct _cartesian_merge;

    template<class ... LhsTypes, class ... RhsTuples>
    struct _cartesian_merge<order_twople<LhsTypes...>, std::tuple<RhsTuples...>>
    {
        using type = std::tuple<tuple_concat_t<std::tuple<LhsTypes...>, RhsTuples>...>;
    };

    template<class ... LhsTuples, class ... RhsTuples>
    struct _cartesian_merge<std::tuple<LhsTuples...>, std::tuple<RhsTuples...>>
    {
        using type = tuple_concat_t<typename _cartesian_merge<std::tuple<LhsTuples>, std::tuple<RhsTuples...>>::type...>;
    };

    template<class Tuple>
    struct _cartesian_product;

    template<class ... Types>
    struct _cartesian_product<order_twople<Types...>>
    {
        using type = std::tuple<std::tuple<Types>...>;
    };

    template<class ... Tuples>
    struct _cartesian_product<std::tuple<Tuples...>>
    {
    private:
        using _halved_tuple = halve_tuple<std::tuple<Tuples...>>;

    public:
        //Divide and conquer
        using type =
            typename _cartesian_merge<
                typename _cartesian_product<typename _halved_tuple::lhs_type>::type,
                typename _cartesian_product<typename _halved_tuple::rhs_type>::type
            >::type;
    };
    */
}

#endif // SMM_TRAITS_EXT_HPP_INCLUDED
