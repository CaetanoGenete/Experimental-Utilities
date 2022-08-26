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

    template<class Callable, class Arg, class ... TemplateArgs>
    concept _predicate_with_deduced_arg = requires(Arg&& arg, Callable callable)
    {
        { callable.template operator()<TemplateArgs...>(std::forward<Arg>(arg)) } -> std::convertible_to<bool>;
    };

    template<class Base, class Callable>
    class _trivially_copyable_throw_on: public Base 
    {
    public:
        inline static std::atomic<throw_conditions> condition = throw_conditions::throw_on_call;

    public:
        using callable_type = Callable;

    private:
        template<class Callable2, class ... Args>
        void _unchecked_try_call(Callable2&& callable, Args&& ... args) 
        {
            if (condition != throw_conditions::do_not_throw) {
                const bool marked_to_throw = std::invoke(std::forward<Callable2>(callable), std::forward<Args>(args)...);

                if(condition == throw_conditions::throw_on_call && marked_to_throw)
                    throw std::exception("Expected throw!");
            }
        }

    protected:
        template<class ... TemplateArgs, class Arg>
        void _try_throw(Arg&& arg) 
            requires _predicate_with_deduced_arg<Callable, Arg, TemplateArgs...>
        {
            //Callable has an argument that needs to be deduced, cast function pointer
            auto func_ptr = static_cast<bool(Callable::*)(const std::decay_t<Arg>&)>(&Callable::template operator()<TemplateArgs...>);
            _unchecked_try_call(func_ptr, Callable(), std::forward<Arg>(arg));
        }

        template<class ... TemplateArgs, class Arg>
        void _try_throw(Arg&& arg) 
            requires _predicate_with_deduced_arg<Callable, Arg, decltype(arg), TemplateArgs...>
        {
            //Template argument doesn't need to be deduced, just pass in.
            _unchecked_try_call(&Callable::template operator()<decltype(arg), TemplateArgs...>, Callable(), std::forward<Arg>(arg));
        }

        template<class ... TemplateArgs, class Arg>
        void _try_throw(Arg&& arg) 
         requires std::predicate<decltype(&Callable::template operator()<TemplateArgs...>), Callable> &&
                  !_predicate_with_deduced_arg<Callable, Arg, decltype(arg), TemplateArgs...>         &&
                  !_predicate_with_deduced_arg<Callable, Arg, TemplateArgs...>
                 
        {
            _unchecked_try_call(&Callable::template operator()<TemplateArgs...> , Callable());
            //Disables 'unused' warning
            (void)arg;
        }

        template<class ... TemplateArgs, class Arg>
        void _try_throw(Arg&& arg) 
            noexcept(!std::predicate<Callable, Arg> && !std::predicate<Callable>)
        {
            if constexpr (std::predicate<Callable, Arg>) {
                _unchecked_try_call(Callable(), std::forward<Arg>(arg));
            }
            else if constexpr (std::predicate<Callable>) {
                _unchecked_try_call(Callable());
                //Disables 'unused' warning
                (void)arg;
            }
        }

    public:
        template<class ... Args>
        requires(std::is_constructible_v<Base, Args...>)
        _trivially_copyable_throw_on(Args&& ... args)
            noexcept(std::is_nothrow_constructible_v<Base, decltype(args)...> && noexcept(_try_throw<decltype(args)...>(*this))):
            Base(std::forward<Args>(args)...)
        {
            _try_throw<decltype(args)...>(*this);
        }

    public:
        _trivially_copyable_throw_on& operator=(const _trivially_copyable_throw_on&) = default;
        _trivially_copyable_throw_on& operator=(_trivially_copyable_throw_on&&) = default;

    public:
        static void reset() 
            requires requires { Callable::reset(); }
        {
            Callable::reset();
        }

        static void reset() {}

    };

    template<class Callable, class Base>
    struct _throw_on: public _trivially_copyable_throw_on<Callable, Base>
    {
    private:
        using _base_type = _trivially_copyable_throw_on<Callable, Base>;

    public:
        using _base_type::_base_type;

        _throw_on(const _throw_on& other)
            noexcept(std::is_nothrow_copy_constructible_v<Base> && noexcept(_base_type::template _try_throw<decltype(other)>(*this)))
            requires(std::is_copy_constructible_v<Base>) :
            _base_type(other)
        {
            _base_type::template _try_throw<decltype(other)>(*this);
        }

        _throw_on(_throw_on&& other)
            noexcept(std::is_nothrow_move_constructible_v<Base> && noexcept(_base_type::template _try_throw<decltype(other)>(*this)))
            requires(std::is_move_constructible_v<Base>) :
            _base_type(std::move(other))
        {
            //Note: Since other gets moved from, instead pass current instance
            _base_type::template _try_throw<decltype(other)>(*this);
        }

    public:
        _throw_on& operator=(const _throw_on&) = default;
        _throw_on& operator=(_throw_on&&) = default;
    };


    template<class Base, class Callable>
    using throw_on = std::conditional_t<
        std::is_trivially_copyable_v<Base> && !std::predicate<Callable, const _trivially_copyable_throw_on<Base, Callable>&>,
        _trivially_copyable_throw_on<Base, Callable>,
        _throw_on<Base, Callable>>;


    template<class Type>
    concept _uses_throw_on_type = template_of<Type, _throw_on> || template_of<Type, _trivially_copyable_throw_on>;


    template<_uses_throw_on_type ThrowOnType>
    struct _throw_on_guard 
    {
        constexpr _throw_on_guard()  noexcept { ThrowOnType::reset(); }
        constexpr ~_throw_on_guard() noexcept { ThrowOnType::reset(); }
    };


    struct always_throw 
    {
        template<class ... Args>
        constexpr bool operator()() noexcept {
            return true;
        }
    };


    template<size_t x, class ... Args>
    struct throw_after_x 
    {
    private:
        inline static std::atomic_size_t _counter = 0;

    public:
        template<class ... Args2>
        requires (std::same_as<Args, Args2> && ...)
        constexpr bool operator()() 
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
    struct always_throw_after_x 
    {
    private:
        inline static std::atomic_size_t _counter = 0;

    public:
        template<class ... Args>
        constexpr bool operator()() noexcept
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
    struct throw_every_x 
    {
    private:
        inline static std::atomic_size_t _counter = 0;

    public:
        template<class ... Args2>
        requires (std::same_as<Args, Args2> && ...)
        constexpr bool operator()() noexcept
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


    template<class Type, class ... Args>
    struct throw_on_comp_equal 
    {
    public:
        inline static std::optional<std::decay_t<Type>> value = std::nullopt;

    public:
        template<class Comparator, class ... Args2>
        requires (std::same_as<Args, Args2> && ...)
        bool operator()(Comparator&& arg)
        {
            if (value)
                return value == std::forward<Comparator>(arg);
            else
                return false;
        }

    public:
        static void reset() { value = std::nullopt; }
    };
}

#endif // !EXPU_throw_on_HPP_INCLUDED