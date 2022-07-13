#include "gtest/gtest.h"

#include <string>

#include "expu/containers/darray.hpp"
#include "expu/iterators/seq_iter.hpp"

#include "counters.hpp"

/* darray fixture */

template<typename FundamentalType, bool trivially_copyable = true>
    requires(std::is_fundamental_v<FundamentalType>)
struct fundamental_wrapper
{
    constexpr fundamental_wrapper(FundamentalType var) noexcept :
        var(var) {}

    FundamentalType var;
};

template<typename FundamentalType>
struct ::std::is_trivially_copyable<fundamental_wrapper<FundamentalType, false>> :
    std::false_type {};

template<typename FundamentalType>
constexpr bool ::std::is_trivially_copyable_v<fundamental_wrapper<FundamentalType, false>> = false;

struct trivial_test {};
struct non_trivial_test {};

template<typename IsTrivial>
struct darray_tests : testing::TestWithParam<IsTrivial>
{
    constexpr static bool is_trivial = std::is_same_v<IsTrivial, trivial_test>;

    using test_t   = fundamental_wrapper<int, is_trivial>;
    using alloc_t  = expu_tests::counted_allocator<std::allocator<test_t>>;
};

using darray_tests_cases = testing::Types<trivial_test, non_trivial_test>;
TYPED_TEST_SUITE(darray_tests, darray_tests_cases);

/*
* Emplaces (back) elements of passed in range into passed into darray;
* this is performed sequentially, hence the darray may need to grow
* multiple times. Verifys the number of calls to constructors,
* destructor, number of allocations, deallocations are correct.
* Furthermore, verifys that the capacity of darray at the end.
*
*/
template<typename Type, typename Alloc, std::forward_iterator FwdIt>
std::string populate_darray(expu::darray<Type, expu_tests::counted_allocator<Alloc>>& arr, FwdIt begin, FwdIt end)
{
    const auto range_size = std::distance(begin, end);

    const auto delta = arr.get_allocator().data();

    size_t expected_copies = 0;
    size_t expected_moves = 0;
    size_t expected_destructions = 0;
    size_t expected_allocations = 0;
    size_t expected_capacity = arr.capacity();

    while (expected_capacity < range_size) {
        ++expected_allocations;

        /*
        *If the darray stored type is trivially copyable, then there
        *should be no constructor calls, instead underlying mem should
        *be copied.
        */
        if constexpr (!std::is_trivially_copyable_v<Type>) {
            //If can be safely moved, then move constructor should be called.
            if constexpr (std::is_nothrow_move_constructible_v<Type>)
                expected_moves += expected_capacity;
            else
                expected_copies += expected_capacity;
        }

        expected_capacity = std::max(
            expected_capacity + expected_capacity / 2,
            expected_capacity + 1);
    }

    //Expecting as many destructions as constructions.
    expected_destructions = std::is_trivially_destructible_v<Type> ?
        0 : expected_copies + expected_moves;

    /**
    * If dereferenced type calls either move or copy constructor on
    * emplace, then these need to be accounted for.
    */
    if constexpr (expu::calls_move_ctor_v<Type, std::iter_reference_t<FwdIt>>) {
        expected_moves += range_size;
    }
    else if constexpr (expu::calls_copy_ctor_v<Type, std::iter_reference_t<FwdIt>>) {
        expected_copies += range_size;
    }

    for (; begin != end; ++begin)
        arr.emplace_back(*begin);

    std::string error = expected_capacity != arr.capacity() ?
        "Unexpected array capacity!\n" : "";

    error += expu_tests::check_counters(
        arr.get_allocator(),
        delta,
        {
            expected_copies,
            expected_moves,
            expected_destructions,
            expected_allocations,
            expected_allocations - 1
        });

    return error;
}

/*
* Test to ensure that stateless allocators do not 
* take extra space in darray's memory footprint. Here
* the test is performed using the standard allocator.
*/
TEST(darray_tests, sizeof_darray_with_stateless_allocator) 
{
    using darray_t = expu::darray<int>;

    //sizeof darray should be equivalent to 3 pointers. 
    ASSERT_EQ(sizeof(darray_t), 3 * sizeof(darray_t::pointer));
}

/*
* Testing emplace_back
*/
TYPED_TEST(darray_tests, emplace_back) 
{
    expu::darray<TestFixture::test_t, TestFixture::alloc_t> test_arr;

    std::string err = populate_darray(
        test_arr,
        expu::seq_iter(0),
        expu::seq_iter(100000));

    ASSERT_FALSE(err.size()) << err;
}

TYPED_TEST(darray_tests, copy_construct) 
{
    constexpr size_t test_size = 100000;

    using darray_t = expu::darray<TestFixture::test_t, TestFixture::alloc_t>;
    darray_t test_arr;

    std::string err = populate_darray(
        test_arr,
        expu::seq_iter(0ull),
        expu::seq_iter(test_size));

    ASSERT_FALSE(err.size()) << err;

    darray_t copy_arr(test_arr);

    if constexpr (std::is_trivially_copyable_v<TestFixture::test_t>)
        //Expecting no calls to copy constructor if trivially copyable
        err = expu_tests::check_counters(copy_arr.get_allocator(), { 0, 0, 0, 1, 0 });
    else
        err = expu_tests::check_counters(copy_arr.get_allocator(), { test_size, 0, 0, 1, 0 });


    ASSERT_FALSE(err.size()) << err;
}

//TYPED_TEST(darray_tests, move_construct)