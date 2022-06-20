#ifndef SMM_TESTS_COUNTED_WRAPPER_HPP_INCLUDED
#define SMM_TESTS_COUNTED_WRAPPER_HPP_INCLUDED

#include "gtest/gtest.h"

#include <type_traits>

namespace smm_tests {

    template<typename PrimitiveType>
    struct _primitive_wrapper
    {
        PrimitiveType value;
    };

    template<typename LhsPrimitiveType, typename RhsPrimitiveType>
    constexpr bool operator==(const _primitive_wrapper<LhsPrimitiveType>& lhs, const _primitive_wrapper<RhsPrimitiveType>& rhs)
    {
        return lhs.value == rhs.value;
    }

    template<typename LhsPrimitiveType, typename RhsPrimitiveType>
    constexpr bool operator!=(const _primitive_wrapper<LhsPrimitiveType>& lhs, const _primitive_wrapper<RhsPrimitiveType>& rhs)
    {
        return !(lhs == rhs);
    }

    //If primitive type, then returns wrapper
    template<typename Type>
    using _ensure_inheritable_t = std::conditional_t<std::is_fundamental_v<Type>, _primitive_wrapper<Type>, Type>;

    template<typename Base, bool _inherit_trivially_copyable = false>
    struct _counted_type : public _ensure_inheritable_t<Base>
    {
#define SMM_TESTS_COUNTER(name)                                                 \
    private:                                                                    \
        inline static std::atomic_size_t _##name##_count = 0;                   \
                                                                                \
    public:                                                                     \
        [[nodiscard]] constexpr static size_t name##_counts() noexcept          \
        {                                                                       \
            return _##name##_count;                                             \
        }                                                                       \
                                                                                \
        constexpr static void reset_##name##_count() noexcept                   \
        {                                                                       \
            _##name##_count = 0;                                                \
        }                                                                       \
                                                                                \
        static void print_##name##_count() {                                    \
            std::cout << #name << "_counts: " << _##name##_count << std::endl;  \
        }                                                                       \

    private:
        using _base_t = _ensure_inheritable_t<Base>;

    public:
        using _base_t::_base_t;

        constexpr _counted_type() : _base_t() {}

        constexpr _counted_type(const _counted_type& type)
            noexcept(std::is_nothrow_copy_constructible_v<Base>) :
            _base_t(type)
        {
            ++_counted_type::_copy_ctors_count;
        }

        constexpr _counted_type(const Base& type)
            noexcept(std::is_nothrow_copy_constructible_v<Base>) :
            _base_t(type)
        {
            //Note: Do nothing if simply wrapping 
        }

        constexpr _counted_type(_counted_type&& type)
            noexcept(std::is_nothrow_move_constructible_v<Base>) :
            _base_t(std::move(type))
        {
            if constexpr (std::is_move_constructible_v<Base>)
                ++_counted_type::_move_ctors_count;
            else
                ++_counted_type::_copy_ctors_count;
        }

        constexpr _counted_type(Base&& type)
            noexcept(std::is_nothrow_move_constructible_v<Base>) :
            _base_t(std::move(type))
        {
            //Once again, do nothing if only wrapping
        }

        constexpr ~_counted_type()
            noexcept(std::is_nothrow_destructible_v<Base>)
        {
            ++_counted_type::_destructions_count;
        }

    public:
        constexpr _counted_type& operator=(const _counted_type& other)
            noexcept(std::is_nothrow_copy_assignable_v<Base>)
        {
            _base_t::operator=(other);
            ++_counted_type::_copy_asgns_count;

            return *this;
        }

        constexpr _counted_type& operator=(_counted_type&& other)
            noexcept(std::is_nothrow_move_assignable_v<Base>)
        {
            _base_t::operator=(std::move(other));

            if constexpr (std::is_move_assignable_v<Base>)
                ++_counted_type::_move_asgns_count;
            else
                ++_counted_type::_copy_asgns_count;

            return *this;
        }

    public:
        SMM_TESTS_COUNTER(move_ctors);
        SMM_TESTS_COUNTER(copy_ctors);
        SMM_TESTS_COUNTER(move_asgns);
        SMM_TESTS_COUNTER(copy_asgns);
        SMM_TESTS_COUNTER(destructions);

#undef SMM_TESTS_COUNTER

        constexpr static void reset_all() noexcept
        {
            reset_copy_ctors_count();
            reset_move_ctors_count();
            reset_copy_asgns_count();
            reset_move_asgns_count();
            reset_destructions_count();
        }
    };



    template<typename Type>
    struct _counted_type_guard {
    public:
        constexpr _counted_type_guard()
        {
            Type::reset_all();
        }

        constexpr ~_counted_type_guard() noexcept
        {
            Type::reset_all();
        }

        constexpr _counted_type_guard(const _counted_type_guard&) = delete;
        constexpr _counted_type_guard(_counted_type_guard&&) = delete;

        constexpr _counted_type_guard& operator=(const _counted_type_guard&) = delete;
        constexpr _counted_type_guard& operator=(_counted_type_guard&&) = delete;
    };

    // Provides wrapped class that counts number of calls to each special member function, under the specified
    // alias. Moreover, ensures counters are reset upon macro expansion and at the end of the scope, wherein the
    // expansion occured.
#define SMM_TESTS_CREATE_COUNTED(type, alias, ...)                  \
    using alias = _counted_type<type __VA_OPT__(,) __VA_ARGS__>;    \
    _counted_type_guard<##alias##>()


        //Todo: Consider moving to static function
#define SMM_TESTS_ASSERT_COUNTS(counted_type, copy_ctors_count, move_ctors_count, copy_asgns_count, move_asgns_count, destrs_count) \
    ASSERT_EQ(counted_type##::copy_ctors_counts(), copy_ctors_count);                                                               \
    ASSERT_EQ(counted_type##::move_ctors_counts(), move_ctors_count);                                                               \
    ASSERT_EQ(counted_type##::copy_asgns_counts(), copy_asgns_count);                                                               \
    ASSERT_EQ(counted_type##::move_asgns_counts(), move_asgns_count);                                                               \
    ASSERT_EQ(counted_type##::destructions_counts(), destrs_count);                                                                 \

}

//Hack: Terrible practice but necessary evil, may be undefine behaviour.
template<typename Base>
struct std::is_trivially_copyable<smm_tests::_counted_type<Base, true>>:
    std::bool_constant<std::is_trivially_copyable_v<Base>> {};

//Hack: Terrible, may be undefine behaviour
template<typename Base>
constexpr bool std::is_trivially_copyable_v<smm_tests::_counted_type<Base, true>> = std::is_trivially_copyable_v<Base>;

#endif // !SMM_TESTS_COUNTED_WRAPPER_HPP_INCLUDED
