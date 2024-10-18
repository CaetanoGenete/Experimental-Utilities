#ifndef EXPU_FUNCTION_UTILS_HPP_INCLUDED
#define EXPU_FUNCTION_UTILS_HPP_INCLUDED

#include <utility>

namespace expu {

    class no_return_value {};

    template<class Callable, class ... Args>
    constexpr decltype(auto) _return_guard(Callable&& callable, Args&& ... args)
    {
        if constexpr (std::is_void_v<std::invoke_result_t<Callable, decltype(args)...>>) {
            std::invoke(std::forward<Callable>(callable), std::forward<Args>(args)...);
            return no_return_value{};
        }
        else
            return std::invoke(std::forward<Callable>(callable), std::forward<Args>(args)...);
    }

    //Todo: allow partial returnables
    template<size_t ... Indicies, class Callable, class ... Args>
    constexpr auto indexed_unroll(std::index_sequence<Indicies...>, const Callable& callable, Args&& ... args)
    {
        //Using integral_constant here to allow for constexpr deduction of Index.
        return std::make_tuple(
            _return_guard(callable, std::integral_constant<size_t, Indicies>{}, std::forward<Args>(args)...)...);
    }

    template<size_t n, class Callable, class ... Args>
    constexpr auto indexed_unroll_n(const Callable& callable, Args&& ... args)
    {
        return indexed_unroll(std::make_index_sequence<n>{}, callable, std::forward<Args>(args)...);
    }



    template<class ... Args, class ReturnType, class ClassType>
    auto disambiguate(ReturnType(ClassType::* func)(Args...)) { return func; }

    template<class ... Args, class ReturnType>
    auto disambiguate(ReturnType(*func)(Args...)) { return func; }
}

#endif // !EXPU_FUNCTION_UTILS_HPP_INCLUDED