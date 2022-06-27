#include "gtest/gtest.h"

#include "wrappers/counted_allocator.hpp"

namespace smm_tests {

    TEST(counted_allocator_tests, allocator_traits) {
        using base_alloc = std::allocator<int>;
        using base_alloc_traits = std::allocator_traits<base_alloc>;
        using counted_alloc_traits = std::allocator_traits<counted_allocator<base_alloc>>;

        ASSERT_EQ(base_alloc_traits::propagate_on_container_copy_assignment::value,
            counted_alloc_traits::propagate_on_container_copy_assignment::value);

        ASSERT_EQ(base_alloc_traits::propagate_on_container_move_assignment::value,
            counted_alloc_traits::propagate_on_container_move_assignment::value);

        ASSERT_EQ(base_alloc_traits::propagate_on_container_swap::value,
            counted_alloc_traits::propagate_on_container_swap::value);

        ASSERT_EQ(base_alloc_traits::is_always_equal::value,
            counted_alloc_traits::is_always_equal::value);
    }

}