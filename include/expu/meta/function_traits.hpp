#ifndef EXPU_META_FUNCTION_TRAITS_HPP_INCLUDED
#define EXPU_META_FUNCTION_TRAITS_HPP_INCLUDED

#include <type_traits>

namespace expu {
    template<typename BaseType>
    struct _all_special_ctor_deleted_type : public BaseType
    {
        using BaseType::BaseType;

        _all_special_ctor_deleted_type(const _all_special_ctor_deleted_type&) = delete;
        _all_special_ctor_deleted_type(_all_special_ctor_deleted_type&&) = delete;

        _all_special_ctor_deleted_type& operator=(const _all_special_ctor_deleted_type&) = delete;
        _all_special_ctor_deleted_type& operator=(_all_special_ctor_deleted_type&&) = delete;
    };

    template<typename Type, typename ... Args>
    constexpr bool calls_special_ctor_v = false;

    template<typename Type, typename Arg>
    requires (!std::is_final_v<Type>)
    constexpr bool calls_special_ctor_v<Type, Arg> = !std::is_constructible_v<_all_special_ctor_deleted_type<Type>, Arg> &&
                                                      std::is_convertible_v<Arg, Type>;

    template<typename Type> constexpr bool calls_special_ctor_v<Type, Type> = true;
    template<typename Type> constexpr bool calls_special_ctor_v<Type, Type&> = true;
    template<typename Type> constexpr bool calls_special_ctor_v<Type, const Type&> = true;
    template<typename Type> constexpr bool calls_special_ctor_v<Type, Type&&> = true;

    template<typename Type, typename Arg>
    constexpr bool calls_move_ctor_v = calls_special_ctor_v<Type, Arg>  &&
                                       std::is_rvalue_reference_v<Arg>  &&
                                       std::is_move_constructible_v<Type>;

    template<typename Type, typename Arg>
    constexpr bool calls_copy_ctor_v = calls_special_ctor_v<Type, Arg> &&
                                       !calls_move_ctor_v<Type, Arg>   &&
                                       std::is_copy_constructible_v<Type>;
}

#endif // !EXPU_META_FUNCTION_TRAITS_HPP_INCLUDED