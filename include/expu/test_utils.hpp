#ifndef EXPU_TEST_UTILS_HPP_INCLUDED
#define EXPU_TEST_UTILS_HPP_INCLUDED

#include <type_traits>
#include <iostream>

#include "expu/meta/meta_utils.hpp"
#include "expu/meta/function_traits.hpp"

namespace expu {

#define EXPU_FUND_WRAPPER_BIVARIATE_OVERLOAD_IF_false

#define EXPU_FUND_WRAPPER_BIVARIATE_OVERLOAD_IF_true \
    requires(std::is_integral_v<OtherFundamentalType> && std::is_integral_v<FundamentalType>)


#define EXPU_FUND_WRAPPER_BIVARIATE_OVERLOAD(symbol, requires_integral)                                                             \
    template<typename OtherFundamentalType>                                                                                         \
    constexpr friend auto operator symbol(const fundamental_wrapper& lhs, OtherFundamentalType rhs) noexcept                        \
        EXPU_FUND_WRAPPER_BIVARIATE_OVERLOAD_IF_##requires_integral                                                                 \
    {                                                                                                                               \
        auto result = lhs.unwrapped symbol rhs;                                                                                     \
        return fundamental_wrapper<std::decay_t<decltype(result)>>(result);                                                         \
    }                                                                                                                               \
                                                                                                                                    \
    template<typename OtherFundamentalType>                                                                                         \
    constexpr friend auto operator symbol(OtherFundamentalType lhs, const fundamental_wrapper& rhs) noexcept                        \
        EXPU_FUND_WRAPPER_BIVARIATE_OVERLOAD_IF_##requires_integral                                                                 \
    {                                                                                                                               \
        auto result = lhs symbol rhs.unwrapped;                                                                                     \
        return fundamental_wrapper<std::decay_t<decltype(result)>>(result);                                                         \
    }                                                                                                                               \
                                                                                                                                    \
    template<typename OtherFundamentalType>                                                                                         \
    constexpr friend auto operator symbol(const fundamental_wrapper& lhs, const fundamental_wrapper<OtherFundamentalType>& rhs)     \
        EXPU_FUND_WRAPPER_BIVARIATE_OVERLOAD_IF_##requires_integral                                                                 \
                                                                                                                                    \
    {                                                                                                                               \
        auto result = lhs.unwrapped symbol rhs.unwrapped;                                                                           \
        return fundamental_wrapper<std::decay_t<decltype(result)>>(result);                                                         \
    }                                                                                                                               \
                                                                                                                                    \
    template<typename OtherFundamentalType>                                                                                         \
    constexpr fundamental_wrapper& operator symbol##=(const fundamental_wrapper<OtherFundamentalType>& rhs) noexcept                \
        EXPU_FUND_WRAPPER_BIVARIATE_OVERLOAD_IF_##requires_integral                                                                 \
    {                                                                                                                               \
        unwrapped symbol##= rhs.unwrapped;                                                                                          \
        return *this;                                                                                                               \
    }                                                                                                                               \
                                                                                                                                    \
    template<typename OtherFundamentalType>                                                                                         \
    constexpr fundamental_wrapper& operator symbol##=(OtherFundamentalType rhs) noexcept                                            \
        EXPU_FUND_WRAPPER_BIVARIATE_OVERLOAD_IF_##requires_integral                                                                 \
    {                                                                                                                               \
        unwrapped symbol##= rhs;                                                                                                    \
        return *this;                                                                                                               \
    }

    template<typename FundamentalType>
    requires(std::is_fundamental_v<FundamentalType>)
    struct fundamental_wrapper {
    public:
        constexpr fundamental_wrapper() noexcept:
            unwrapped() {}

        constexpr fundamental_wrapper(FundamentalType fundamental) noexcept :
            unwrapped(fundamental) {}

    public: //Binary arithmetic operator overloads
        EXPU_FUND_WRAPPER_BIVARIATE_OVERLOAD(+ , false);
        EXPU_FUND_WRAPPER_BIVARIATE_OVERLOAD(- , false);
        EXPU_FUND_WRAPPER_BIVARIATE_OVERLOAD(* , false);
        EXPU_FUND_WRAPPER_BIVARIATE_OVERLOAD(/ , false);
        EXPU_FUND_WRAPPER_BIVARIATE_OVERLOAD(% , true);
        EXPU_FUND_WRAPPER_BIVARIATE_OVERLOAD(& , true);
        EXPU_FUND_WRAPPER_BIVARIATE_OVERLOAD(^ , true);
        EXPU_FUND_WRAPPER_BIVARIATE_OVERLOAD(| , true);
        EXPU_FUND_WRAPPER_BIVARIATE_OVERLOAD(>>, true);
        EXPU_FUND_WRAPPER_BIVARIATE_OVERLOAD(<<, true);

#undef EXPU_FUND_WRAPPER_BIVARIATE_OVERLOAD_IF_false
#undef EXPU_FUND_WRAPPER_BIVARIATE_OVERLOAD_IF_true
#undef EXPU_FUND_WRAPPER_BIVARIATE_OVERLOAD

    public:
        template<typename OtherFundamentalType>
        constexpr bool operator==(const fundamental_wrapper<OtherFundamentalType>& other) const noexcept
        {
            return unwrapped == other.unwrapped;
        }

        template<typename OtherFundamentalType>
        requires std::is_fundamental_v<OtherFundamentalType>
        constexpr bool operator==(OtherFundamentalType other) const noexcept
        {
            return unwrapped == other;
        }

        template<typename OtherFundamentalType>
        constexpr auto operator<=>(const fundamental_wrapper<OtherFundamentalType>& other) const noexcept
        {
            return unwrapped <=> other.unwrapped;
        }

        template<typename OtherFundamentalType>
        requires std::is_fundamental_v<OtherFundamentalType>
        constexpr auto operator<=>(OtherFundamentalType other) const noexcept
        {
            return unwrapped <=> other;
        }

    public:
        FundamentalType unwrapped;
    };

    template<typename FundamentalType>
    fundamental_wrapper(FundamentalType)->fundamental_wrapper<std::decay_t<FundamentalType>>;

    template<typename FundamentalType>
    std::ostream& operator<<(std::ostream& stream, const fundamental_wrapper<FundamentalType>& wrapper)
    {
        return stream << wrapper.unwrapped;
    }

    template<typename FundamentalType>
    constexpr fundamental_wrapper<FundamentalType>& operator++(fundamental_wrapper<FundamentalType>& wrapper, int)
        noexcept
    {
        ++wrapper.unwrapped;
        return *this;
    }

    template<typename FundamentalType>
    constexpr fundamental_wrapper<FundamentalType> operator++(fundamental_wrapper<FundamentalType>& wrapper)
        noexcept
    {
        fundamental_wrapper<FundamentalType> copy(wrapper);
        ++wrapper.unwrapped;
        return copy;
    }

    template<typename FundamentalType>
    constexpr fundamental_wrapper<FundamentalType> operator~(const fundamental_wrapper<FundamentalType>& wrapper)
        noexcept requires(std::is_integral_v<FundamentalType>)
    {
        return fundamental_wrapper(~wrapper.unwrapped);
    }   

    enum class test_type_props {
#ifdef EXPU_ALLOW_TRIVIAL_TEST_TYPE
        inherit_trivially_copyable,
#endif // EXPU_ALLOW_TRIVIAL_TEST_TYPE
        not_trivially_destructible,
        disable_copy_ctor,
        disable_move_ctor,
        disable_copy_asgn,
        disable_move_asgn
    };

    template<test_type_props Property, test_type_props ... Properties>
    constexpr bool contains = ((Property == Properties) || ...);

    template<typename Base, test_type_props ... Properties>
    class TestTypeBase : public std::conditional_t<std::is_fundamental_v<Base>, expu::fundamental_wrapper<Base>, Base>
    {
    private:
        using _base_t = std::conditional_t<std::is_fundamental_v<Base>, expu::fundamental_wrapper<Base>, Base>;

    public:
        using _base_t::_base_t;

        constexpr TestTypeBase(const TestTypeBase& other)
            noexcept(std::is_nothrow_copy_constructible_v<_base_t>)
            requires(!contains<test_type_props::disable_copy_ctor, Properties...>) :
            _base_t(other)
        {
            _copy_ctor_called = true;
        }

        constexpr TestTypeBase(TestTypeBase&& other)
            noexcept(std::is_nothrow_copy_constructible_v<_base_t>)
            requires(!contains<test_type_props::disable_move_ctor, Properties...>) :
            _base_t(std::move(other))
        {
            _move_ctor_called = true;
        }

    public:
        constexpr TestTypeBase& operator=(const TestTypeBase& other)
            noexcept(std::is_nothrow_copy_assignable_v<_base_t>)
            requires(!contains<test_type_props::disable_copy_asgn, Properties...>)
        {
            if constexpr (std::is_copy_assignable_v<_base_t>)
                _base_t::operator=(other);

            ++_copy_asgn_calls;
            return *this;
        }

        constexpr TestTypeBase& operator=(TestTypeBase&& other)
            noexcept(std::is_nothrow_move_assignable_v<_base_t>)
            requires(!contains<test_type_props::disable_move_asgn, Properties...>)
        {
            if constexpr (std::is_move_assignable_v<_base_t>)
                _base_t::operator=(std::move(other));

            ++_move_asgn_calls;
            return *this;
        }

    public:
        [[nodiscard]] constexpr bool copy_ctor_called() const noexcept { return _copy_ctor_called; }
        [[nodiscard]] constexpr bool move_ctor_called() const noexcept { return _move_ctor_called; }

        [[nodiscard]] constexpr size_t copy_asgn_calls() const noexcept { return _copy_asgn_calls; }
        [[nodiscard]] constexpr size_t move_asgn_calls() const noexcept { return _move_asgn_calls; }


    private:
        //Note: Here we use default initialisation to avoid issues with using Base constructors
        bool _copy_ctor_called  = false;
        bool _move_ctor_called  = false;
        size_t _copy_asgn_calls = 0;
        size_t _move_asgn_calls = 0;
    };

    template<typename Base, test_type_props ... Properties>
    struct TestType : public TestTypeBase<Base, Properties...> 
    {
        using TestTypeBase<Base, Properties...>::TestTypeBase;
    };

    template<typename Base, test_type_props ... Properties>
    requires(contains<test_type_props::not_trivially_destructible, Properties...>)
    struct TestType<Base, Properties...> : public TestTypeBase<Base, Properties...> 
    {
    public:
        using TestTypeBase<Base, Properties...>::TestTypeBase;

    public:
        //Explicitely defining an empty destructor here to set trivially_destructible to false.
        constexpr ~TestType() noexcept(std::is_nothrow_destructible_v<Base>) {}
    };
}

/*
* Technically, specialisation of is_trivially_copyable(_v) is undefined behaviour, 
* according to the standard. However, this should function as intended for most 
* compilers. USE AT YOUR OWN RISK. 
*/
#ifdef EXPU_ALLOW_TRIVIAL_TEST_TYPE

template<typename Base, expu::test_type_props ... Properties>
struct std::is_trivially_copyable<expu::TestType<Base, Properties...>> :
    std::bool_constant<std::is_trivially_copyable_v<Base> && expu::contains<expu::test_type_props::inherit_trivially_copyable, Properties...>> {};

template<typename Base, expu::test_type_props ... Properties>
constexpr bool std::is_trivially_copyable_v<expu::TestType<Base, Properties...>> =
    std::is_trivially_copyable_v<Base> && expu::contains<expu::test_type_props::inherit_trivially_copyable, Properties... >;

#endif // EXPU_ALLOW_TRIVIAL_TEST_TYPE

#endif // !EXPU_TEST_UTILS_HPP_INCLUDED