#ifndef EXPU_THROW_ON_TYPE_HPP_INCLUDED
#define EXPU_THROW_ON_TYPE_HPP_INCLUDED

#include <atomic>
#include <exception>
#include <concepts>
#include <optional>

#include "expu/meta/meta_utils.hpp"

#define EXPU_NO_THROW_ON(type_name, operation)                         \
    if constexpr(::expu::_uses_throw_on_type<type_name>)               \
        type_name::condition = ::expu::throw_conditions::do_not_throw; \
                                                                       \
    operation;                                                         \
                                                                       \
    if constexpr (::expu::_uses_throw_on_type<type_name>)              \
        type_name::condition = ::expu::throw_conditions::throw_on_call \


#define EXPU_GUARDED_THROW_ON_TYPE(alias, base_type, callable) \
    using alias = expu::throw_on_type<base_type, callable>;    \
    expu::_throw_on_type_guard<alias> _##alias##_guard;        \


namespace expu {

    enum class throw_conditions 
    {
        throw_on_call, 
        do_not_throw, 
        call_do_not_throw
    };

    template<class Base, class Callable>
    class throw_on_type: public Base 
    {
    public:
        inline static std::atomic<throw_conditions> condition = throw_conditions::throw_on_call;

    public:
        using callable_type = Callable;

    private:
        template<class ... Args>
        static constexpr bool _will_call_on = std::predicate<Callable, Args...>;

    private:
        template<class ... Args>
        void try_throw(Args&& ... args) 
            noexcept(!_will_call_on<decltype(args)...>)
        {
            if constexpr (_will_call_on<decltype(args)...>) {
                switch (condition)
                {
                case throw_conditions::throw_on_call:
                    if (std::invoke(Callable(), std::forward<Args>(args)...))
                        throw std::exception("Expected throw!");

                    break;
                case throw_conditions::call_do_not_throw:
                    std::invoke(Callable(), std::forward<Args>(args)...);

                default:
                    break;
                }
            }
        }

    public:
        template<class ... Args>
        requires(std::is_constructible_v<Base, Args...>)
        throw_on_type(Args&& ... args)
            noexcept(std::is_nothrow_constructible_v<Base, decltype(args)...> && !_will_call_on<decltype(args)...>):
            Base(std::forward<Args>(args)...)
        {
            try_throw(std::forward<Args>(args)...);
        }

        throw_on_type(const throw_on_type& other)
            noexcept(std::is_nothrow_copy_constructible_v<Base> && !_will_call_on<decltype(other)>)
            requires(std::is_copy_constructible_v<Base>):
            Base(other)
        {
            try_throw(other);
        }

        throw_on_type(throw_on_type&& other) 
            noexcept(std::is_nothrow_move_constructible_v<Base> && !_will_call_on<decltype(other)>)
            requires(std::is_move_constructible_v<Base>) :
            Base(std::move(other))
        {
            try_throw(std::move(other));
        }

    public:
        constexpr throw_on_type& operator=(const throw_on_type&) = default;
        constexpr throw_on_type& operator=(throw_on_type&&) = default;

    public:
        static void reset() 
            requires requires { Callable::reset(); }
        {
            Callable::reset();
        }

        static void reset() {}

    };

    template<expu::template_of<throw_on_type> ThrowOnType>
    struct _throw_on_type_guard {
        constexpr _throw_on_type_guard()  noexcept { ThrowOnType::reset(); }
        constexpr ~_throw_on_type_guard() noexcept { ThrowOnType::reset(); }
    };

    template<class Type>
    concept _uses_throw_on_type = requires(Type& type) { 
        { throw_on_type(type) } -> base_of<Type>; 
    };

    struct always_throw {
        template<class ... Args>
        constexpr bool operator()(Args&& ...) noexcept {
            return true;
        }
    };

    template<size_t x, class ... Args>
    struct throw_after_x {
    private:
        inline static std::atomic_size_t _counter = 0;

    public:
        constexpr bool operator()(Args&& ...) 
        {
            static size_t counter = 0;

            if (x < ++counter)
                return true;
            else
                return false;

        }

    public:
        static void reset() noexcept { _counter = 0; }
    };

    template<size_t x>
    struct always_throw_after_x {
    private:
        inline static std::atomic_size_t _counter = 0;

    public:
        template<class ... Args>
        constexpr bool operator()(Args&& ...) noexcept
        {
            if (x < ++_counter)
                return true;
            else
                return false;

        }

    public:
        static void reset() noexcept { _counter = 0; }
    };

    template<size_t x, class ... Args>
    struct throw_every_x {
    private:
        inline static std::atomic_size_t _counter = 0;

    public:
        constexpr bool operator()(Args&& ...) noexcept
        {
            if (x < ++_counter) {
                _counter = 0;
                return true;
            }
            else
                return false;

        }

    public:
        static void reset() noexcept { _counter = 0; }
    };

    template<class Type, class Arg = Type>
    struct throw_on_comp_equal {
    public:
        inline static std::optional<std::decay_t<Type>> value = std::nullopt;

    public:
        constexpr bool operator()(Arg&& arg) 
            requires std::equality_comparable_with<Type, Arg>
        {
            if (value)
                return value == arg;
            else
                return false;
        }
    public:
        static void reset() { value = std::nullopt; }
    };
}

#endif // !EXPU_THROW_ON_TYPE_HPP_INCLUDED