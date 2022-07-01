#ifndef SMM_TRAITS_EXT_HPP_INCLUDED
#define SMM_TRAITS_EXT_HPP_INCLUDED

#include <type_traits>

namespace expu {

    template<typename Type, template<typename ...> typename Base>
    struct is_template_of : public std::bool_constant<false> {};

    template<template <typename ...> typename Base, typename ... Types>
    struct is_template_of<Base<Types...>, Base> : public std::bool_constant<true> {};

    template<typename Type, template<typename ...> typename Base>
    constexpr bool is_template_of_v = is_template_of<Type, Base>::value;

    template<typename Type, template<typename ...> typename Base>
    concept template_of = is_template_of_v<Type, Base>;

}

#endif // SMM_TRAITS_EXT_HPP_INCLUDED
