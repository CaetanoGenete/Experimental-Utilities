#include "gtest/gtest.h"

namespace cess_tests {

    template<typename Type>
    struct disable_wrapper_tests : testing::TestWithParam<Type> {};

    struct _disabled_copy_ctor { static constexpr special_member code = special_member::copy_ctor; };
    struct _disabled_move_ctor { static constexpr special_member code = special_member::move_ctor; };
    struct _disabled_copy_asgn { static constexpr special_member code = special_member::copy_asgn; };
    struct _disabled_move_asgn { static constexpr special_member code = special_member::move_asgn; };

    template<typename ... Disables>
    struct _disabled_int_type {
        using type = disabled_wrapper_t<int, Disables::code...>;
    };

    using disable_tests_types = prod_t<
        _disabled_int_type,
        subsets_t<testing::Types<
        _disabled_copy_ctor,
        _disabled_move_ctor,
        _disabled_copy_asgn,
        _disabled_move_asgn
        >>>;
    TYPED_TEST_SUITE(disable_wrapper_tests, disable_tests_types);

    TYPED_TEST(disable_wrapper_tests, disables)
    {
        using wrapped_type = typename TypeParam::type;

        ASSERT_EQ(sizeof(wrapped_type), sizeof(wrapped_type::type));

        ASSERT_NE(std::is_copy_constructible_v<wrapped_type>, (is_disabled<wrapped_type, special_member::copy_ctor>));
        ASSERT_NE(std::is_move_constructible_v<wrapped_type>, (is_disabled<wrapped_type, special_member::move_ctor>));
        ASSERT_NE(std::is_copy_assignable_v<wrapped_type>, (is_disabled<wrapped_type, special_member::copy_asgn>));
        ASSERT_NE(std::is_move_assignable_v<wrapped_type>, (is_disabled<wrapped_type, special_member::move_asgn>));

        //Verify trivial copy is inherited
        ASSERT_TRUE(std::is_trivially_copyable_v<wrapped_type>);
        ASSERT_TRUE(std::is_trivially_copyable<wrapped_type>::value);

    }
}