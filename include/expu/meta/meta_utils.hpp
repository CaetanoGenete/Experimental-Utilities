#ifndef SMM_TRAITS_EXT_HPP_INCLUDED
#define SMM_TRAITS_EXT_HPP_INCLUDED

#include <type_traits>
#include <string_view>

namespace expu {

    template<typename Type, template<typename ...> typename Base>
    struct is_template_of : public std::bool_constant<false> {};

    template<template <typename ...> typename Base, typename ... Types>
    struct is_template_of<Base<Types...>, Base> : public std::bool_constant<true> {};

    template<typename Type, template<typename ...> typename Base>
    constexpr bool is_template_of_v = is_template_of<Type, Base>::value;

    template<typename Type, template<typename ...> typename Base>
    concept template_of = is_template_of_v<Type, Base>;


    template<typename Type>
    constexpr std::string type_name()
    {
        std::string prefix = "";

        if constexpr (std::is_const_v<std::remove_reference_t<Type>>)
            prefix += "const ";

        if constexpr (std::is_volatile_v<std::remove_reference_t<Type>>)
            prefix += "volatile ";

        constexpr auto postfix = std::is_rvalue_reference_v<Type> ? "&&" :
            std::is_lvalue_reference_v<Type> ? "&" : "";

        return prefix + typeid(Type).name() + postfix;
    }

    template<typename Type>
    constexpr std::string type_name(Type&& type)
    {
        return type_name<decltype(type)>();
    }


}

#endif // SMM_TRAITS_EXT_HPP_INCLUDED
