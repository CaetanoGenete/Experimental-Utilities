#include <type_traits>
#include <iterator>

#include "gtest/gtest.h"

#include "smm/debug.hpp"
#include "smm/containers/darray.hpp"

namespace smm_tests {

    template<class Iter>
    class darray_iter_tests : public testing::TestWithParam<Iter> {};

    using darray_iter_tests_types = testing::Types<
        smm::darray<int>::iterator,
        smm::darray<int>::const_iterator>;

    TYPED_TEST_SUITE(darray_iter_tests, darray_iter_tests_types);

    TYPED_TEST(darray_iter_tests, traits) {
        using iterator = TypeParam;

        ASSERT_TRUE(std::input_or_output_iterator<iterator>);
        ASSERT_TRUE(std::input_iterator<iterator>);
        ASSERT_TRUE(std::forward_iterator<iterator>);
        ASSERT_TRUE(std::bidirectional_iterator<iterator>);
        ASSERT_TRUE(std::random_access_iterator<iterator>);
    }

}