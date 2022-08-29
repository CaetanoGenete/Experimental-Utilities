#ifndef EXPU_TYPELIST_SET_OPERATIONS_HPP_INCLUDED
#define EXPU_TYPELIST_SET_OPERATIONS_HPP_INCLUDED

#include <utility> // For access to integer_sequence
#include <tuple>
#include <array>

namespace expu {

    template<template<class ...> class CastTo, class TypeList>
    struct typelist_cast;

    template<template<class ...> class CastTo, class TypeList>
    using typelist_cast_t = typename typelist_cast<CastTo, TypeList>::type;

    template<template<class ...> class CastTo, template<class...> class CastFrom, class ... FromTypes>
    struct typelist_cast<CastTo, CastFrom<FromTypes...>> 
    {
        using type = CastTo<FromTypes...>;
    };


    template<class TypeList>
    struct typelist_size;

    template<class TypeList>
    constexpr size_t typelist_size_v = typelist_size<TypeList>::value;

    template<template<class ...> class TypeList, class ... Args>
    struct typelist_size<TypeList<Args...>> 
    {
        static constexpr size_t value = sizeof...(Args);
    };


    template<size_t Index, class TypeList>
    struct typelist_element;

    template<size_t Index, class TypeList>
    using typelist_element_t = typename typelist_element<Index, TypeList>::type;

    template<size_t Index, template<class ...> class TypeList, class FirstType, class ... Types>
    struct typelist_element<Index, TypeList<FirstType, Types...>>
    {
        static_assert(Index < sizeof...(Types) + 1, "Index out of bounds!");

        using type = typelist_element_t<Index - 1, TypeList<Types...>>;
    };

    template<template<class ...> class TypeList, class FirstType, class ... Types>
    struct typelist_element<0, TypeList<FirstType, Types...>>
    {
        using type = FirstType;
    };


    template<template<class ...> class TypeList, class ... TypeLists>
    struct union_c;

    template<template<class ...> class TypeList, class ... TypeLists>
    using union_ct = typename union_c<TypeList, TypeLists...>::type;

    template<class ... TypeLists>
    struct _union;

    template<class ... TypeLists>
    using union_t = typename _union<TypeLists...>::type;

    template<
        template<class ...> class TypeList, 
        template<class ...> class LhsTypeList,
        template<class ...> class RhsTypeList,
        class ... LhsTypes, 
        class ... RhsTypes>
    struct union_c<TypeList, LhsTypeList<LhsTypes...>, RhsTypeList<RhsTypes...>>
    {
        using type = TypeList<LhsTypes..., RhsTypes...>;
    };
    
    template<template<class ...> class TypeList, class FirstTypeList, class SecondTypeList, class ... OtherTypeLists>
    struct union_c<TypeList, FirstTypeList, SecondTypeList, OtherTypeLists...> 
    {
        using type = union_ct<TypeList, union_ct<TypeList, FirstTypeList, SecondTypeList>, OtherTypeLists...>;
    };

    template<template<class ...> class TypeList, class OnlyTypeList>
    struct union_c<TypeList, OnlyTypeList>: 
        public typelist_cast<TypeList, OnlyTypeList> {};


    template<template<class ...> class TypeList, class ... FirstTypes, class ... OtherTypeLists>
    struct _union<TypeList<FirstTypes...>, OtherTypeLists...>:
        public union_c<TypeList, TypeList<FirstTypes...>, OtherTypeLists...> {};


    template<class Sequence, class List>
    struct subset;

    template<class Sequence, class List>
    using subset_t = typename subset<Sequence, List>::type;

    template<template<class ...> class TypeList, class ... Types, size_t ... Indicies>
    struct subset<std::index_sequence<Indicies...>, TypeList<Types...>>
    {
        using type = TypeList<typelist_element_t<Indicies, TypeList<Types...>>...>;
    };


    template<template<class ...> class ResultTypeList, class Sequence, class ... TypeLists>
    struct from_typelists_c;

    template<class Sequence, class ... TypeLists>
    struct from_typelists;

    template<class Sequence, class ... TypeLists>
    using from_typelists_t = typename from_typelists<Sequence, TypeLists...>::type;

    template<template<class ...> class ResultTypeList, class Sequence, class ... TypeLists>
    using from_typelists_ct = typename from_typelists_c<ResultTypeList, Sequence, TypeLists...>::type;

    template<template<class ...> class ResultTypeList, size_t ... Indicies, class ... TypeLists>
    struct from_typelists_c<ResultTypeList, std::index_sequence<Indicies...>, TypeLists...> 
    {
        static_assert(sizeof...(Indicies) == sizeof...(TypeLists), "Number of indicies does not match number of type lists!");

    public:
        using type = ResultTypeList<typelist_element_t<Indicies, TypeLists>...>;
    };

    //Note: TypeList template is deduced from first passed in type list.
    template<template<class ...> class TypeList, size_t ... Indicies, class ... FirstListArgs, class ... OtherTypeLists>
    struct from_typelists<std::index_sequence<Indicies...>, TypeList<FirstListArgs...>, OtherTypeLists...>:
        public from_typelists_c<TypeList, std::index_sequence<Indicies...>, TypeList<FirstListArgs...>, OtherTypeLists...> {};


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
    
    template<template<class ...> class TypeList, class SequenceTuple, class ... TypeLists>
    struct _cartesian_product_c_helper;

    template<template<class ...> class TypeList, class ... Sequences, class ... TypeLists>
    struct _cartesian_product_c_helper<TypeList, std::tuple<Sequences...>, TypeLists...>
    {
        using type = union_t<TypeList<from_typelists_ct<TypeList, Sequences, TypeLists...>>...>;
    };
    
    template<template<class ...> class TypeList, class ... TypeLists>
    struct cartesian_product_c
    {
        using type = typename _cartesian_product_c_helper<
            TypeList, decltype(_cartesian_indicies<typelist_size_v<TypeLists>...>()), TypeLists...>::type;
    };
    
    template<template<class ...> class TypeList, class ... TypeLists>
    using cartesian_product_ct = typename cartesian_product_c<TypeList, TypeLists...>::type;

    template<class ... TypeLists>
    struct cartesian_product;

    template<class ... TypeLists>
    using cartesian_product_t = typename cartesian_product<TypeLists...>::type;

    template<template<class ...> class TypeList, class ... FirstTypes,  class ... TypeLists>
    struct cartesian_product<TypeList<FirstTypes...>, TypeLists...>: 
        public cartesian_product_c<TypeList, TypeList<FirstTypes...>, TypeLists...> {};


    template<class TypeList, class Type>
    struct has_type;

    template<template<class...> class TypeList, class Type>
    struct has_type<TypeList<>, Type>:
        public std::false_type {};

    template<template<class ...> class TypeList, class FirstType, class ... Types, class Type>
    struct has_type<TypeList<FirstType, Types...>, Type> :
        public std::bool_constant<(std::is_same_v<Types, Type> || ...)> {};

    template<class TypeList, class Type>
    constexpr bool has_type_v = has_type<TypeList, Type>::value;


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
        }({ Indicies... });

        return[]<size_t ... Indicies2>(std::index_sequence<Indicies2...>) {
            return std::index_sequence<sequence[Indicies2]...>{};
        }(std::make_index_sequence<sequence.size()>{});
    }

    template<class Sequence, class TypeList>
    struct _typelist_unique_mask;

    template<size_t ... Indicies, template<class...> class TypeList, class ... Types>
    struct _typelist_unique_mask<std::index_sequence<Indicies...>, TypeList<Types...>>
    {
    private:
        using _typelist_t = TypeList<Types...>;

    public:
        using type = std::index_sequence<!has_type_v<subset_t<std::make_index_sequence<Indicies>, _typelist_t>, Types>...>;
    };

    template<class TypeList>
    struct unique_typelist
    {
    private:
        using _typelist_index_seq = std::make_index_sequence<typelist_size_v<TypeList>>;

    public:
        using type = subset_t<decltype(_mask_to_sequence(_typelist_unique_mask<_typelist_index_seq, TypeList>::type())), TypeList>;
    };

    template<class Tuple>
    using unique_typelist_t = typename unique_typelist<Tuple>::type;
}

#endif // !EXPU_TYPELIST_SET_OPERATIONS_HPP_INCLUDED