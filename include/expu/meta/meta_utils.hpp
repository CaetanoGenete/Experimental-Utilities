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


    template<class Sequence, class ... Tuples>
    struct from_tuples;

    template<size_t ... Indicies, class ... Tuples>
    struct from_tuples<std::index_sequence<Indicies...>, Tuples...>
    {
        static_assert(sizeof...(Indicies) == sizeof...(Tuples), "Number of indicies does not match number of tuples!");

    public:
        using type = std::tuple<std::tuple_element_t<Indicies, Tuples>...>;
    };


    template<class Tuple, class Type>
    struct tuple_has_type;

    template<class Type>
    struct tuple_has_type<std::tuple<>, Type> :
        public std::false_type {};

    template<class ... Types, class Type>
    struct tuple_has_type<std::tuple<Types...>, Type> :
        public std::bool_constant<(std::is_same_v<Types, Type> || ...)> {};

    template<class Tuple, class Type>
    constexpr bool tuple_has_type_v = tuple_has_type<Tuple, Type>::value;


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


    //////////////Constant instantiation depth cartesian product/////////////////////////////////////////////////////


    template<size_t ... Sizes>
    consteval auto _cartesian_indicies() 
    {
        constexpr size_t tuples_count = (Sizes * ...);
        constexpr std::array bases = { Sizes... };

        constexpr auto _indicies_array = []() {
            std::array<std::array<size_t, bases.size()>, tuples_count> result = {};

            for (size_t elem_index = 0; elem_index < tuples_count; ++elem_index) {
                size_t value = elem_index;

                for (size_t base_index = 0; base_index < bases.size(); ++base_index) {
                    result[elem_index][base_index] = value % bases[base_index];
                    value /= bases[base_index];
                }
            }

            return result;
        }();

        constexpr auto row_to_index_sequence = []<size_t row, size_t ... Indicies>(std::index_sequence<Indicies...>) {
            return std::index_sequence<_indicies_array[row][Indicies]...>{};
        };

        return[]<size_t ... RowIndicies, size_t ... ColumnIndicies>(
            std::index_sequence<RowIndicies...>, 
            std::index_sequence<ColumnIndicies...> indicies)
        {
            return std::make_tuple(row_to_index_sequence.template operator()<RowIndicies>(indicies)...);
        }
        (std::make_index_sequence<tuples_count>{}, std::make_index_sequence<sizeof...(Sizes)>{});
    }

    template<class SequenceTuple, class ... Tuples>
    struct _cartesian_product_helper;

    template<class ... Sequences, class ... Tuples>
    struct _cartesian_product_helper<std::tuple<Sequences...>, Tuples...>
    {
        using type = tuple_concat_t<std::tuple<typename from_tuples<Sequences, Tuples...>::type>...>;
    };

    template<class ... Tuples>
    struct _cartesian_product 
    {
        using type = typename _cartesian_product_helper<decltype(_cartesian_indicies<std::tuple_size_v<Tuples>...>()), Tuples...>::type;
    };

    template<class ... Tuples>
    using cartesian_product_t = typename _cartesian_product<Tuples...>::type;


    //////////////Constant instantiation depth unique tuple types///////////////////////////////////////////


    template<size_t ... Indicies>
    consteval auto _mask_to_sequence(std::index_sequence<Indicies...>) 
    {
        constexpr auto sequence = [](std::array<size_t, sizeof...(Indicies)> mask) {
            std::array<size_t, (Indicies + ...)> sequence = {};

            size_t counter = 0;
            for (size_t index = 0; index < mask.size(); ++index) {
                if (mask[index] == 1)
                    sequence[counter++] = index;
            }

            return sequence;
        }({Indicies...});

        return[]<size_t ... Indicies2>(std::index_sequence<Indicies2...>) {
            return std::index_sequence<sequence[Indicies2]...>{};
        }(std::make_index_sequence<sequence.size()>{});
    }

    template<class Tuple, class Sequence>
    struct _tuple_unique_mask;

    template<class ... Types, size_t ... Indicies>
    struct _tuple_unique_mask<std::tuple<Types...>, std::index_sequence<Indicies...>> 
    {
    private:
        using tuple_t = std::tuple<Types...>;

    public:
        using type = std::index_sequence<!tuple_has_type_v<tuple_subset_t<tuple_t, std::make_index_sequence<Indicies>>, Types>...>;
    };

    template<class Tuple>
    struct _unique_tuple
    {
    private:
        using _tuple_seq = std::make_index_sequence<std::tuple_size_v<Tuple>>;

    public:
        using type = tuple_subset_t<Tuple, decltype(_mask_to_sequence(_tuple_unique_mask<Tuple, _tuple_seq>::type()))>;
    };

    template<class Tuple>
    using make_unique_tuple = typename _unique_tuple<Tuple>::type;

    template<class ... Types>
    using unique_tuple = typename _unique_tuple<std::tuple<Types...>>::type;

}

#endif // SMM_TRAITS_EXT_HPP_INCLUDED
