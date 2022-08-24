#ifndef EXPU_throw_on_HPP_INCLUDED
#define EXPU_throw_on_HPP_INCLUDED

#include <atomic>
#include <exception>
#include <concepts>
#include <optional>

#include "expu/meta/meta_utils.hpp"

#define EXPU_NO_THROW_ON(type_name, operation)                         \
    type_name::condition = ::expu::throw_conditions::do_not_throw;     \
    operation;                                                         \
    type_name::condition = ::expu::throw_conditions::throw_on_call     \


#define EXPU_GUARDED_THROW_ON_TYPE(alias, base_type, callable) \
    using alias = expu::throw_on<base_type, callable>;         \
    expu::_throw_on_guard<alias> _##alias##_guard;             \


namespace expu {

    enum class throw_conditions 
    {
        throw_on_call, 
        do_not_throw, 
        call_do_not_throw
    };

    template<class Base, class Callable>
    class _throw_on_trivially_copyable: public Base 
    {
    public:
        inline static std::atomic<throw_conditions> condition = throw_conditions::throw_on_call;

    public:
        using callable_type = Callable;

    protected:
        template<class ... Args>
        static constexpr bool _will_call_on = std::predicate<Callable, Args...>;

    protected:
        template<class ... Args>
        void _try_throw(Args&& ... args) 
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
        _throw_on_trivially_copyable(Args&& ... args)
            noexcept(std::is_nothrow_constructible_v<Base, decltype(args)...> && !_will_call_on<decltype(args)...>):
            Base(std::forward<Args>(args)...)
        {
            //Todo: Potentially forwarding objects that have already been moved from on call to delegating construct! Fix.
            _try_throw(std::forward<Args>(args)...);
        }

    public:
        _throw_on_trivially_copyable& operator=(const _throw_on_trivially_copyable&) = default;
        _throw_on_trivially_copyable& operator=(_throw_on_trivially_copyable&&) = default;

    public:
        static void reset() 
            requires requires { Callable::reset(); }
        {
            Callable::reset();
        }

        static void reset() {}

    };

    template<class Callable, class Base>
    struct _throw_on: public _throw_on_trivially_copyable<Callable, Base>
    {
    private:
        using _base_type = _throw_on_trivially_copyable<Callable, Base>;

    public:
        using _base_type::_base_type;

        _throw_on(const _throw_on& other)
            noexcept(std::is_nothrow_copy_constructible_v<Base> && !_base_type::template _will_call_on<decltype(other)>)
            requires(std::is_copy_constructible_v<Base>) :
            _base_type(other)
        {
            _base_type::_try_throw(other);
        }

        _throw_on(_throw_on&& other)
            noexcept(std::is_nothrow_move_constructible_v<Base> && !_base_type::template _will_call_on<decltype(other)>)
            requires(std::is_move_constructible_v<Base>) :
            _base_type(std::move(other))
        {
            //Note: Since other gets moved from, instead pass current instance
            _base_type::_try_throw(*this);
        }

    public:
        _throw_on& operator=(const _throw_on&) = default;
        _throw_on& operator=(_throw_on&&) = default;
    };

    template<class Base, class Callable>
    using throw_on = std::conditional_t<std::is_trivially_copyable_v<Base> && !std::predicate<Callable, const _throw_on_trivially_copyable<Base, Callable>&>,
        _throw_on_trivially_copyable<Base, Callable>,
        _throw_on<Base, Callable>>;

    template<class Type>
    concept _uses_throw_on_type = template_of<Type, _throw_on> || template_of<Type, _throw_on_trivially_copyable>;

    template<_uses_throw_on_type ThrowOnType>
    struct _throw_on_guard {
        constexpr _throw_on_guard()  noexcept { ThrowOnType::reset(); }
        constexpr ~_throw_on_guard() noexcept { ThrowOnType::reset(); }
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

#endif // !EXPU_throw_on_HPP_INCLUDED