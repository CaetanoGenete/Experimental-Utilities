#ifndef EXPU_GTEST_UTILS_HPP_INCLUDED
#define EXPU_GTEST_UTILS_HPP_INCLUDED

#include "expu/meta/meta_utils.hpp"

namespace testing::internal {
    template <class ... Ts>
    struct ProxyTypeList;
}

namespace expu {

    template<class TypeList>
    struct as_tuple;

    template<class ... Args>
    struct as_tuple<::testing::internal::ProxyTypeList<Args...>> {
        using type = std::tuple<Args...>;
    };

    template<class TypeList>
    using as_tuple_t = typename as_tuple<TypeList>::type;


    template<class Tuple>
    struct as_gtest_TypeList;

    template<class ... Args>
    struct as_gtest_TypeList<std::tuple<Args...>> {
        using type = ::testing::internal::ProxyTypeList<Args...>;
    };

    template<class TypeList>
    using as_gtest_TypeList_t = typename as_gtest_TypeList<TypeList>::type;

    
    template<template_of<::testing::internal::ProxyTypeList> ... TypeLists>
    struct _cartesian_product<std::tuple<TypeLists...>>
    {
        using type = as_gtest_TypeList_t<cartesian_product_t<as_tuple_t<TypeLists>...>>;
    };
}

#endif // !EXPU_GTEST_UTILS_HPP_INCLUDED