#include "gtest/gtest.h"

#include <type_traits>

#include "expu/containers/fuple.hpp"
#include "expu/containers/static_map.hpp"

//Empty structure used for testing purposes
struct empty_struct {};

/*
* Simple test ensuring that fuple is indeed a standard_layout type, hence,
* accordint to the standard, one may reinterpret cast another
* standard_layout_type pointer to and from fuple.
*/
TEST(fuple_tests, traits)
{
    using fuple_t = expu::fuple<int, float, double, empty_struct>;

    ASSERT_TRUE(std::is_standard_layout_v<fuple_t>);
}

TEST(fuple_tests, get)
{
    auto value = expu::linear_smap(
        std::make_pair(1, 1.4f), 
        std::make_pair(2, 1.6));

    std::cout << value.at(1) << std::endl;
}