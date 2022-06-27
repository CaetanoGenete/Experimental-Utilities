#ifndef SMM_TESTS_TEST_UTILS_HPP_INCLUDED
#define SMM_TESTS_TEST_UTILS_HPP_INCLUDED

#include "gtest/gtest.h"

namespace smm_tests {
    template<typename...>
    struct concat_types_list;

    template<typename ... Lists>
    using concat_types_list_t = typename concat_types_list<Lists...>::type;

    template<typename ... ListTypes>
    struct concat_types_list<testing::Types<ListTypes...>> 
    { 
        using type = testing::Types<ListTypes...>;
    };

    template<typename ... LhsTypes, typename ... RhsTypes, typename ... Lists>
    struct concat_types_list<testing::Types<LhsTypes...>, testing::Types<RhsTypes...>, Lists...> 
    {
        using type = concat_types_list_t<testing::Types<LhsTypes..., RhsTypes...>, Lists...>;
    };

    template<typename, typename>
    struct cart_prod_helper;

    template<typename Type, typename ... Lists>
    struct cart_prod_helper<Type, testing::Types<Lists...>> {
        using type = testing::Types<concat_types_list_t<testing::Types<Type>, Lists>...>;
    };

    template<typename ...>
    struct cart_prod;

    template<typename ... Lists>
    using cart_prod_t = typename cart_prod<Lists...>::type;

    template<typename ... ListTypes> 
    struct cart_prod<testing::Types<ListTypes...>>
    {
        using type = testing::Types<testing::Types<ListTypes>...>;
    };

    template<typename ... LhsTypes, typename ... OtherLists>
    struct cart_prod<testing::Types<LhsTypes...>, OtherLists...>
    {
        using type = concat_types_list_t<typename cart_prod_helper<LhsTypes, cart_prod_t<OtherLists...>>::type...>;
    };


    template<template<typename...> typename, typename>
    struct prod;

    template<template<typename...> typename, typename>
    struct prod_helper;

    template<template<typename...> typename Template, typename ... ListTypes>
    struct prod_helper<Template, testing::Types<ListTypes...>> {
        using type = testing::Types<Template<ListTypes...>>;
    };

    template<template<typename...> typename Template, typename ... Lists>
    struct prod<Template, testing::Types<Lists...>> 
    {
        using type = concat_types_list_t<typename prod_helper<Template, Lists>::type...>;
    };

    template<template<typename...> typename Template, typename Lists>
    using prod_t = typename prod<Template, Lists>::type;


    template<typename>
    struct subsets;

    template<typename List>
    using subsets_t = typename subsets<List>::type;

    template<typename Type>
    struct subsets<testing::Types<Type>> 
    {
        using type = cart_prod_t<testing::Types<Type>>;
    };

    template<typename LhsType, typename ... ListTypes>
    struct subsets<testing::Types<LhsType, ListTypes...>>
    {
        using singleton = testing::Types<LhsType>;
        using subsubset = subsets_t<testing::Types<ListTypes...>>;

        using type = concat_types_list_t<
            testing::Types<singleton>,
            subsubset,
            typename cart_prod_helper<LhsType, subsubset>::type>;
    };

}

#endif // !SMM_TESTS_TEST_UTILS_HPP_INCLUDED
