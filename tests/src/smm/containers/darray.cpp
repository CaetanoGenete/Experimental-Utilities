#include "gtest/gtest.h"

#include "smm/debug.hpp"
#include "smm/containers/darray.hpp"
#include "smm/iter_utils.hpp"

#include "test_utils.hpp"
#include "meta_sort.hpp"
#include "int_iterator.hpp"

#include "wrappers/disabled_wrapper.hpp"
#include "wrappers/counted_wrapper.hpp"
#include "wrappers/counted_allocator.hpp"

#include <vector>

namespace smm_tests {

    struct inherit_trivial: public std::true_type {};
    struct dont_inherit_trivial : public std::false_type {};

    template<typename InheritTrival>
    class darray_tests : public testing::Test {
    public:
        using counted_int = _counted_type<int, InheritTrival::value>;

    public:
        constexpr darray_tests() noexcept  { counted_int::reset_all(); }
        constexpr ~darray_tests() noexcept { counted_int::reset_all(); }

    public:

        template<typename Alloc, std::input_iterator InIt>
        constexpr static bool populate_darray(smm::darray<counted_int, counted_allocator<Alloc>>& arr, InIt begin, InIt end)
        {
            size_t init_size = arr.size();
            size_t init_cap  = arr.capacity();

            size_t expected_moves    = counted_int::move_ctors_counts();
            size_t expected_destrs   = counted_int::destructions_counts();
            size_t expected_allocs   = arr.get_allocator().allocations();
            size_t expected_deallocs = arr.get_allocator().deallocations();
            size_t expected_capacity = arr.capacity();

            size_t grow_to = init_size;
            for (; begin != end; ++begin, ++grow_to)
                arr.emplace_back(*begin);

            while (expected_capacity < grow_to) {
                ++expected_allocs;
                ++expected_deallocs;

                expected_destrs += expected_capacity;

                if constexpr (!std::is_trivially_copyable_v<counted_int>)
                    expected_moves += expected_capacity;

                //Growth factor = 1.5
                size_t new_capacity = expected_capacity + expected_capacity / 2;
                expected_capacity = std::max(expected_capacity + 1, new_capacity);
            }

            if (init_cap == 0)
                --expected_deallocs;

            if (grow_to           != arr.size())                          return false;
            if (expected_moves    != counted_int::move_ctors_counts())    return false;
            if (expected_destrs   != counted_int::destructions_counts())  return false;
            if (expected_allocs   != arr.get_allocator().allocations())   return false;
            if (expected_deallocs != arr.get_allocator().deallocations()) return false;
            if (expected_capacity != arr.capacity())                      return false;

            return true;
        }

        template<typename Alloc>
        constexpr static bool darray_reserve(smm::darray<counted_int, counted_allocator<Alloc>>& arr, size_t capacity) 
        {
            size_t init_size = arr.size();

            size_t expected_moves    = counted_int::move_ctors_counts();
            size_t expected_destrs   = counted_int::destructions_counts();
            size_t expected_allocs   = arr.get_allocator().allocations();
            size_t expected_deallocs = arr.get_allocator().deallocations();
            size_t expected_capacity = arr.capacity();

            if (arr.capacity() < capacity) {
                if constexpr (!std::is_trivially_copyable_v<counted_int>)
                    expected_moves += init_size;

                expected_destrs += init_size;
                ++expected_allocs;
                ++expected_deallocs;
                expected_capacity = capacity;
            }

            arr.reserve(capacity);

            if (init_size         != arr.size())                          return false;
            if (expected_moves    != counted_int::move_ctors_counts())    return false;
            if (expected_destrs   != counted_int::destructions_counts())  return false;
            if (expected_allocs   != arr.get_allocator().allocations())   return false;
            if (expected_deallocs != arr.get_allocator().deallocations()) return false;
            if (expected_capacity != arr.capacity())                      return false;

            return true;
        }
    };

    using darray_grow_tests_types = testing::Types<dont_inherit_trivial, inherit_trivial>;
    TYPED_TEST_SUITE(darray_tests, darray_grow_tests_types);

    TEST(darray_misc_tests, traits_test) {
        using type = smm::darray<int>;

        ASSERT_EQ(sizeof(type), 24);
    }

    TEST(darray_misc_tests, misc) {
        smm::darray<int> arr1;
        std::vector<int> arr2;

        ASSERT_EQ(arr1.capacity(), arr2.capacity());

        for (int i = 0; i < (1 << 13); ++i) {
            arr1.push_back(i);
            arr2.push_back(i);

            ASSERT_EQ(arr1.capacity(), arr2.capacity());
        }
    }

    TYPED_TEST(darray_tests, u_emplace_back_nogrow)
    {
        using alloc = counted_allocator<std::allocator<TestFixture::counted_int>>;

        constexpr size_t test_size = 100000;
        {
            smm::darray<TestFixture::counted_int, alloc> test;
            TestFixture::darray_reserve(test, test_size);

            ASSERT_TRUE(TestFixture::populate_darray(test, intit<int>(0), intit<int>(test_size)));
        }

        ASSERT_EQ(TestFixture::counted_int::destructions_counts(), test_size);
    }
  
    TYPED_TEST(darray_tests, emplace_back_grow)
    {
        using alloc = counted_allocator<std::allocator<TestFixture::counted_int>>;

        constexpr size_t test_size = 100000;

        size_t expected_destrs = 0;
        {//new scope to ensure destruction of darray
            smm::darray<TestFixture::counted_int, alloc> test;
            ASSERT_TRUE(TestFixture::populate_darray(test, intit<int>(0), intit<int>(test_size)));

            expected_destrs = TestFixture::counted_int::destructions_counts() + test.size();
        }

        ASSERT_EQ(TestFixture::counted_int::destructions_counts(), expected_destrs);
    }

    TYPED_TEST(darray_tests, copy_construct) 
    {
        using alloc = counted_allocator<std::allocator<TestFixture::counted_int>>;

        constexpr size_t test_size = 100000;
        {
            smm::darray<TestFixture::counted_int, alloc> to_be_copied;
            to_be_copied.reserve(test_size);

            ASSERT_TRUE(TestFixture::populate_darray(to_be_copied, intit<int>(0), intit<int>(test_size)));

            smm::darray<TestFixture::counted_int, alloc> copy(to_be_copied);
            ASSERT_EQ(to_be_copied.size(), copy.size());
            ASSERT_EQ(to_be_copied.capacity(), copy.capacity());

            ASSERT_EQ(copy.get_allocator().allocations(), 1);
            ASSERT_EQ(copy.get_allocator().deallocations(), 0);

            //If trivially copyable, then not expecting copy constructor calls!
            constexpr size_t expected_copies = std::is_trivially_copyable_v<TestFixture::counted_int> ? 0 : test_size;

            SMM_TESTS_ASSERT_COUNTS(TestFixture::counted_int, expected_copies, 0, 0, 0, 0);

            ASSERT_TRUE(std::equal(copy.begin(), copy.end(), to_be_copied.begin()));
            //Check addresses don't match
            ASSERT_TRUE(std::equal(copy.begin(), copy.end(), to_be_copied.begin(), 
            [](TestFixture::counted_int& lhs, TestFixture::counted_int& rhs) {
                return &lhs != &rhs;
            }));
        }

        ASSERT_EQ(TestFixture::counted_int::destructions_counts(), 2 * test_size);
    }

    TYPED_TEST(darray_tests, move_construct) 
    {
        using alloc = counted_allocator<std::allocator<TestFixture::counted_int>>;

        constexpr size_t test_size = 100000;
        {
            smm::darray<TestFixture::counted_int, alloc> to_be_moved;
            to_be_moved.reserve(test_size);

            ASSERT_TRUE(TestFixture::populate_darray(to_be_moved, intit<int>(0), intit<int>(test_size)));

            smm::darray<TestFixture::counted_int, alloc> moved(std::move(to_be_moved));
            ASSERT_EQ(moved.size(), test_size);
            ASSERT_EQ(moved.capacity(), test_size);

            SMM_TESTS_ASSERT_COUNTS(TestFixture::counted_int, 0, 0, 0, 0, 0);

            //darray left in a 'null' state.
            ASSERT_EQ(to_be_moved.size(), 0);
            ASSERT_EQ(to_be_moved.capacity(), 0);

            //Here expecting no new (de)allocations.
            ASSERT_EQ(moved.get_allocator().allocations(), 1);
            ASSERT_EQ(moved.get_allocator().deallocations(), 0);
        }

        ASSERT_EQ(TestFixture::counted_int::destructions_counts(), test_size);
    }

    TYPED_TEST(darray_tests, assign_case_1) {
        using alloc = counted_allocator<std::allocator<TestFixture::counted_int>>;

        constexpr size_t init_size = 50000;
        constexpr size_t assign_size = init_size * 2;

        //Ensures call to copy_constructor
        smm::darray<TestFixture::counted_int, alloc> temp_cont;
        temp_cont.reserve(assign_size);

        ASSERT_TRUE(TestFixture::populate_darray(temp_cont, intit<int>(0), intit<int>(assign_size)));

        {
            smm::darray<TestFixture::counted_int, alloc> test;
            test.reserve(init_size);

            ASSERT_TRUE(TestFixture::populate_darray(test, intit<int>(0), intit<int>(init_size)));

            test.assign(temp_cont.begin(), temp_cont.end());

            ASSERT_EQ(test.capacity(), assign_size);

            ASSERT_EQ(test.get_allocator().allocations(), 2);
            ASSERT_EQ(test.get_allocator().deallocations(), 1);
            
            constexpr size_t expected_copies = std::is_trivially_copyable_v<TestFixture::counted_int> ? 0 : assign_size;
            SMM_TESTS_ASSERT_COUNTS(TestFixture::counted_int, expected_copies, 0, 0, 0, init_size);

            ASSERT_TRUE(std::equal(test.begin(), test.end(), temp_cont.begin(), temp_cont.end()));
        }

        ASSERT_EQ(TestFixture::counted_int::destructions_counts(), init_size + assign_size);
    }

    TYPED_TEST(darray_tests, assign_case_2) {
        using alloc = counted_allocator<std::allocator<TestFixture::counted_int>>;

        constexpr size_t init_size = 50000;
        constexpr size_t assign_size = init_size * 2;

        //Ensures call to copy_assignment
        smm::darray<TestFixture::counted_int, alloc> temp_cont;
        temp_cont.reserve(assign_size);

        ASSERT_TRUE(TestFixture::populate_darray(temp_cont, intit<int>(0), intit<int>(assign_size)));

        {
            smm::darray<TestFixture::counted_int, alloc> test;
            test.reserve(assign_size * 2);

            ASSERT_TRUE(TestFixture::populate_darray(test, intit<int>(0), intit<int>(init_size)));

            test.assign(temp_cont.begin(), temp_cont.end());

            ASSERT_EQ(test.capacity(), assign_size * 2);

            //Expecting NO new allocations
            ASSERT_EQ(test.get_allocator().allocations(), 1);
            ASSERT_EQ(test.get_allocator().deallocations(), 0);

            constexpr size_t expected_copies = std::is_trivially_copyable_v<TestFixture::counted_int> ? 0 : assign_size - init_size;
            SMM_TESTS_ASSERT_COUNTS(TestFixture::counted_int, expected_copies, 0, init_size, 0, 0);

            ASSERT_TRUE(std::equal(test.begin(), test.end(), temp_cont.begin(), temp_cont.end()));
        }

        ASSERT_EQ(TestFixture::counted_int::destructions_counts(), assign_size);
    }

    TYPED_TEST(darray_tests, assign_case_3) {
        using alloc = counted_allocator<std::allocator<TestFixture::counted_int>>;

        constexpr size_t init_size = 100000;
        constexpr size_t assign_size = init_size / 2;

        //Ensures call to copy_assignment
        smm::darray<TestFixture::counted_int, alloc> temp_cont;
        temp_cont.reserve(assign_size);

        ASSERT_TRUE(TestFixture::populate_darray(temp_cont, intit<int>(0), intit<int>(assign_size)));

        {
            smm::darray<TestFixture::counted_int, alloc> test;
            test.reserve(init_size);

            ASSERT_TRUE(TestFixture::populate_darray(test, intit<int>(0), intit<int>(init_size)));

            test.assign(temp_cont.begin(), temp_cont.end());

            ASSERT_EQ(test.capacity(), init_size);

            //Expecting NO new allocations
            ASSERT_EQ(test.get_allocator().allocations(), 1);
            ASSERT_EQ(test.get_allocator().deallocations(), 0);

            constexpr size_t expected_assigns = std::is_trivially_copyable_v<TestFixture::counted_int> ? 0 : assign_size;
            SMM_TESTS_ASSERT_COUNTS(TestFixture::counted_int, 0, 0, expected_assigns, 0, init_size - assign_size);

            ASSERT_TRUE(std::equal(temp_cont.begin(), temp_cont.end(), test.begin(), test.end()));
        }

        ASSERT_EQ(TestFixture::counted_int::destructions_counts(), init_size);
    }
}
