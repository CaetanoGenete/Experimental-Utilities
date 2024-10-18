#include "gtest/gtest.h"

#include <string>
#include <algorithm>
#include <vector>

#include "expu/containers/darray.hpp"
#include "expu/iterators/seq_iter.hpp"
#include "expu/test_utils.hpp"

#include <ranges>

#include "expu/testing/checked_allocator.hpp"
#include "counters.hpp"


/////////////////////////////////////////////////DARRAY TESTS HELPER FUNCTIONS////////////////////////////////////////////////////////////////////


template<typename Type, typename Alloc>
constexpr std::string check_counters(
    const expu::darray<Type, Alloc>& test_array,
    const expu_tests::alloc_counters& delta,
          expu_tests::alloc_counters expected)
{
    if constexpr (std::is_trivially_copyable_v<Type>) {
        expected.copy_ctor_calls = 0;
        expected.move_ctor_calls = 0;
    }

    if constexpr (std::is_trivially_destructible_v<Type>) {
        expected.destructor_calls = 0;
    }

    return expu_tests::check_counters(test_array.get_allocator(), delta, expected);
}

template<typename Type, typename Alloc>
constexpr std::string check_counters(
    const expu::darray<Type, Alloc>& test_array,
    typename expu_tests::alloc_counters expected)
{
    return check_counters(test_array, { 0, 0, 0, 0, 0 }, expected);
}

template<typename Type, typename FwdIt>
constexpr expu_tests::alloc_counters expected_calls_on_emplace_range(size_t& expected_capacity, size_t range_size) 
{
    expu_tests::alloc_counters expected;
    expected.copy_ctor_calls = 0;
    expected.move_ctor_calls = 0;
    expected.allocations     = 0;

    while (expected_capacity < range_size) {
        ++expected.allocations;

        //If can be safely moved, then move constructor should be called.
        if constexpr (std::is_nothrow_move_constructible_v<Type>)
            expected.move_ctor_calls += expected_capacity;
        else
            expected.copy_ctor_calls += expected_capacity;

        expected_capacity = std::max(
            expected_capacity + expected_capacity / 2,
            expected_capacity + 1);
    }

    //Expecting as many destructions as constructions.
    expected.destructor_calls = expected.copy_ctor_calls + expected.move_ctor_calls;
    expected.deallocations    = expected.allocations;

    /*
    * If dereferenced type calls either move or copy constructor on
    * emplace, then these need to be accounted for.
    */
    if constexpr (expu::calls_move_ctor_v<Type, std::iter_reference_t<FwdIt>>) {
        expected.move_ctor_calls += range_size;
    }
    else if constexpr (expu::calls_copy_ctor_v<Type, std::iter_reference_t<FwdIt>>) {
        expected.copy_ctor_calls += range_size;
    }
    else
        expected.calls.add<std::iter_reference_t<FwdIt>>(range_size);

    return expected;
}

/*
* Emplaces (back) elements of passed in range into passed into darray;
* this is performed sequentially, hence the darray may need to grow
* multiple times. Verifys the number of calls to constructors,
* destructor, number of allocations, deallocations are correct.
* Furthermore, verifys that the capacity of darray at the end.
*
*/
template<typename Type, typename Alloc, std::forward_iterator FwdIt>
std::string populate_darray(expu::darray<Type, Alloc>& arr, FwdIt begin, FwdIt end)
{
    const auto range_size = std::distance(begin, end);

    size_t expected_capacity = arr.capacity();

    const auto delta = arr.get_allocator().data();
    auto expected = expected_calls_on_emplace_range<Type, FwdIt>(expected_capacity, range_size);

    //darray shouldn't deallocate if empty
    if (arr.size() == 0)
        --expected.deallocations;

    for (FwdIt it = begin; it != end; ++it)
        arr.emplace_back(*it);

    std::string error = expected_capacity != arr.capacity() ?
                        "Unexpected array capacity!\n" : "";

    if (!std::equal(arr.end() - range_size, arr.end(), begin, end))
        error += "Unexpected tail elements of array!\n";

    error += check_counters(arr, delta, expected);

    return error;
}

/*
* Reserves desired amount of memory for the use of the specified darray. Verifies
* that construction is only performed once per each element, that all such elements
* are destroyed, that relocated elements compare equal with the elements of the darray
* before the call to reserve and that old memory is deallocated.
*/
template<typename Type, typename Allocator>
constexpr std::string checked_darray_reserve(
    expu::darray<Type, Allocator>& arr,
    typename std::allocator_traits<Allocator>::size_type new_size)
{
    const expu_tests::alloc_counters delta = arr.get_allocator().data();
          expu_tests::alloc_counters expected;

    const auto old_cap  = arr.capacity();
    const auto old_size = arr.size();

    std::allocator<Type> alloc;
    Type* copied_first = nullptr;
    Type* copied_last  = expu::_ctg_duplicate(alloc, arr.begin(), arr.end(), copied_first, old_size);

    arr.reserve(new_size);

    std::string error;

    if (new_size > old_cap) {
        expected.allocations = 1;

        if constexpr (std::is_nothrow_move_constructible_v<Type>) {
            expected.move_ctor_calls = old_size;
            expected.copy_ctor_calls = 0;
        }
        else {
            expected.copy_ctor_calls = old_size;
            expected.move_ctor_calls = 0;
        }

        if (arr.capacity() != new_size)
            error += "Unexpected new capacity!";
    }
    else {
        expected.copy_ctor_calls = 0;
        expected.move_ctor_calls = 0;
        expected.allocations     = 0;
    }

    expected.destructor_calls = expected.copy_ctor_calls + expected.move_ctor_calls;
    expected.deallocations    = expected.allocations;

    if (arr.size() != old_size)
        error += "Expected size to be invariant under reserve!";

    error += check_counters(arr, delta, expected);

    if (!std::equal(arr.begin(), arr.end(), copied_first, copied_last))
        error += "Expected data to be invariant under reserve!";

    alloc.deallocate(copied_first, old_size);

    return error;
}


/////////////////////////////////////////////////DARRAY TESTS FIXTURES AND HELPER CLASSES///////////////////////////////////////////////////////


template<typename Type, template <typename ...> typename Allocator, typename ... AdditionalAllocatorTArgs>
struct _darray_tests_base
{
public:
    using test_type   = Type;
    using alloc_type  = expu::checked_allocator<expu_tests::counted_allocator<Allocator<test_type, AdditionalAllocatorTArgs...>>>;
    using darray_type = expu::darray<test_type, alloc_type>;

    using base_type = _darray_tests_base;

public:
    constexpr static size_t init_size = 10000;

public:
    constexpr void SetUp() 
    {
        emplace_begin = expu::seq_iter<size_t>(0);
        emplace_end   = expu::seq_iter<size_t>(init_size);

        std::string error = populate_darray(test_array, emplace_begin, emplace_end);
        ASSERT_FALSE(error.size()) << error;
    }

public:
    darray_type test_array;

    expu::seq_iter<size_t> emplace_begin;
    expu::seq_iter<size_t> emplace_end;
};


template<typename IsTrivial>
struct darray_trivially_copyable_tests : 
    public _darray_tests_base<std::conditional_t<
    /*If  */ IsTrivial::value,                                                          
    /*Then*/ expu::TestType<size_t, expu::test_type_props::inherit_trivially_copyable>, 
    /*Else*/ expu::TestType<size_t>                                                    
    >, std::allocator>,
    public testing::TestWithParam<IsTrivial> 
{
public:
    constexpr static bool is_trivial = std::is_trivially_copyable_v<darray_trivially_copyable_tests::test_type>;

    static_assert(is_trivial == IsTrivial::value, "Not testing a trivially copyable type!");
public:
    constexpr void SetUp() override { return darray_trivially_copyable_tests::base_type::SetUp(); }
};

using darray_tests_cases = testing::Types<std::false_type, std::true_type>;
TYPED_TEST_SUITE(darray_trivially_copyable_tests, darray_tests_cases);

using darray_non_trivial_type_tests = darray_trivially_copyable_tests<std::false_type>;


template<typename Type, typename POCCA, typename POCMA, typename POCS, typename Comp>
class TestSTDAllocator : public std::allocator<Type>
{
public:
    using propagate_on_container_copy_assignment = POCCA;
    using propagate_on_container_move_assignment = POCMA;
    using propagate_on_container_swap = POCS;
    using is_always_equal = Comp;

    constexpr bool operator!=(const TestSTDAllocator&) noexcept
    {
        return !Comp::value;
    }

    constexpr bool operator==(const TestSTDAllocator&) noexcept
    {
        return Comp::value;
    }
};

template<typename IsTrivial>
struct darray_comp_false_no_prop_alloc_tests :
    public _darray_tests_base<std::conditional_t<
        IsTrivial::value,                                                          //If
        expu::TestType<size_t, expu::test_type_props::inherit_trivially_copyable>, //Then
        expu::TestType<size_t>                                                     //Else
    >, TestSTDAllocator,
    std::false_type, //POCCA
    std::false_type, //POCMA
    std::false_type, //POCS
    std::false_type>,//Comp
    public testing::TestWithParam<IsTrivial>
{
public:
    constexpr static bool is_trivial = std::is_trivially_copyable_v<darray_comp_false_no_prop_alloc_tests::test_type>;

    static_assert(is_trivial == IsTrivial::value, "Not testing a trivially copyable type!");
public:
    constexpr void SetUp() override { return darray_comp_false_no_prop_alloc_tests::base_type::SetUp(); }
};
TYPED_TEST_SUITE(darray_comp_false_no_prop_alloc_tests, darray_tests_cases);


template<typename IsTrivial>
struct darray_trivially_destructible_tests :
    public _darray_tests_base<std::conditional_t<
        IsTrivial::value,                                                         //If
        expu::TestType<size_t>,                                                   //Then
        expu::TestType<size_t, expu::test_type_props::not_trivially_destructible> //Else
    >, std::allocator>,
    public testing::TestWithParam<IsTrivial>
{
public:
    constexpr static bool is_trivial = std::is_trivially_destructible_v<darray_trivially_destructible_tests::test_type>;

    static_assert(is_trivial == IsTrivial::value, "Type is not trivially destructible!");
public:
    constexpr void SetUp() { return darray_trivially_destructible_tests::base_type::SetUp(); }
};
TYPED_TEST_SUITE(darray_trivially_destructible_tests, darray_tests_cases);

using darray_non_trivially_destructible_tests = darray_trivially_destructible_tests<std::false_type>;


/////////////////////////////////////////////////DARRAY TRAITS TEST////////////////////////////////////////////////////////////////////////////


/*
* Test to ensure that stateless allocators do not 
* take extra space in darray's memory footprint. Here
* the test is performed using the standard allocator.
* 
* Testing criteria:
* 1. darray size is equivalent to three pointer
*/
TEST(darray_trivially_copyable_tests, sizeof_darray_with_stateless_allocator) 
{
    using darray_type = expu::darray<int>;

    //sizeof darray should be equivalent to 3 pointers. 
    ASSERT_EQ(sizeof(darray_type), 3 * sizeof(darray_type::pointer));
}


/////////////////////////////////////////////////DARRAY REAR INSERTION TESTS////////////////////////////////////////////////////////////////////


/*
* Testing emplace_back. Due to Fixture (darray_tests), test passes
* if darray_tests::SetUp() passes.
*/
TYPED_TEST(darray_trivially_copyable_tests, emplace_back) {}


/////////////////////////////////////////////////DARRAY CONSTRUCTION TESTS//////////////////////////////////////////////////////////////////////


/*
* Test copy construction.
* 
* Testing criteria:
* 1. copied darray contents are equal to original darray
* 2. Only one construction call per element (if not trivially copyable otherwise 0).
* 3. Only one allocation.
*/
TYPED_TEST(darray_trivially_copyable_tests, copy_construct)
{
    typename TestFixture::darray_type& test_array = this->test_array;
    typename TestFixture::darray_type copy_array(this->test_array);

    //Criterion 1
    ASSERT_TRUE(std::equal(
        copy_array.cbegin(), copy_array.cend(),   //Copied range
        test_array.cbegin(), test_array.cend())); //Original range

    expu_tests::alloc_counters expected;
    expected.copy_ctor_calls = TestFixture::init_size;  //Criterion 2
    expected.allocations     = 1;                       //Criterion 3

    std::string err = check_counters(copy_array, expected);
    ASSERT_FALSE(err.size()) << err;
}

/*
* Testing move constructor without change in allocator. Since allocator
* is moved to new instance, memory can be safely deallocated hence expecting
* pointers of darray to be stolen, therefore no calls to any constructor or
* allocations.
* 
* Testing criteria:
* 1. Moved darray contents are equal to original darray
* 2. No calls to move/copy constructor
*/
TEST_F(darray_non_trivial_type_tests, move_construct_case_1_same_allocator)
{
    const auto delta = this->test_array.get_allocator().data();

    darray_type moved_arr(std::move(this->test_array));

    //Criterion 1
    ASSERT_TRUE(std::equal(
        moved_arr.begin(), moved_arr.end(),
        this->emplace_begin, this->emplace_end)
    ) << "Elements have not been correctly moved!";

    //Expecting no new calls, content should be stolen instead. Criterion 2
    std::string err = expu_tests::check_counters(moved_arr.get_allocator(), delta, { 0, 0, 0, 0, 0 });
    ASSERT_FALSE(err.size()) << err;

}

/*
* Testing move constructor with change of allocator that compares equal.
* 
* Testing criteria:
* 1. Moved darray contents are equal to original darray
* 2. No calls to move/copy constructor
* 3. No calls to allocate/deallocate. 
*/
TEST(darray_tests, move_construct_case_2_new_allocator_comp_equal)
{
    constexpr size_t test_size = 10000;

    using test_t   = expu::TestType<size_t>;
    using alloc_t  = expu_tests::counted_allocator<std::allocator<test_t>>;
    using darray_t = expu::darray<test_t, alloc_t>;

    using iter_t = expu::seq_iter<size_t>;
    const auto first = iter_t(0);
    const auto last  = iter_t(test_size);

    darray_t test_array(first, last);

    const auto new_alloc = alloc_t();
    darray_t moved_arr(std::move(test_array), new_alloc);
    
    //Criterion 1
    ASSERT_TRUE(std::equal(moved_arr.begin(), moved_arr.end(), first, last)) 
        << "Elements have not been correctly moved!";

    //Expecting no calls to any counter. Counters should be the same as new_alloc counters
    //as data should have been stolen. Criterion 2 and Criterion 3
    std::string err = expu_tests::check_counters(moved_arr.get_allocator(), new_alloc.data());
    ASSERT_FALSE(err.size()) << err;

}

/*
* Testing move constructor with change of allocator that does not compare equal. Expecting
* the container to individually move all its elements over. If type is trivially copyable,
* should not call any constructor. Otherwise, one call to move constructor for each
* element is expected.
* 
* Testing criteria:
* 1. Moved darray contents are equal to original darray
* 2. Expecting one call to move per element (If not trivially copyable otherwise 0)
* 3. Expecting only one new allocation
*/
TYPED_TEST(darray_comp_false_no_prop_alloc_tests, move_construct_case_3_new_allocator_not_comp_equal)
{
    const auto new_alloc = typename TestFixture::alloc_type();
    typename TestFixture::darray_type moved_arr(std::move(this->test_array), new_alloc);

    //Criterion 1
    ASSERT_TRUE(std::equal(
        moved_arr.begin(), moved_arr.end(),
        this->emplace_begin, this->emplace_end)
    ) << "Elements have not been correctly moved!";

    expu_tests::alloc_counters expected;
    expected.move_ctor_calls = TestFixture::init_size;  //Criterion 2
    expected.allocations     = 1;                       //Criterion 3

    std::string err = check_counters(moved_arr, expected);
    ASSERT_FALSE(err.size()) << err;

}


/////////////////////////////////////////////////DARRAY RESERVE TESTS///////////////////////////////////////////////////////////////////////////


/*
* Testing strong guarantee of darray::reserve. On throw,
* darray should be unchanged from before call to reserve.
* 
* Testing criteria:
* 1. Deallocate all memory allocated during reserve
* 2. Destroy all elements constructed during reserve
* 3. Provide strong guarantee (darray should be unchanged from before call to reserve)
*/
TEST(darray_throw_on_copy_tests, resize_strong_guarantee)
{
    //Note: Here type is set to not be trivially constructible so that
    using test_type = expu::TestType<int, 
        expu::test_type_props::throw_on_move_ctor, 
        expu::test_type_props::throw_on_copy_ctor,
        expu::test_type_props::not_trivially_destructible>;

    using alloc_type = expu::checked_allocator<expu_tests::counted_allocator<std::allocator<test_type>>>;
    using array_type = expu::darray<test_type, alloc_type>;

    constexpr int test_size = 10000;
    auto seq_begin = expu::seq_iter<int>(0);
    auto seq_end   = expu::seq_iter<int>(test_size);

    array_type test_array(seq_begin, seq_end);

    auto pre_calls = test_array.get_allocator().data();
    //Ensure that darray::reserve truly throws
    EXPECT_THROW(test_array.reserve(test_size * 2), expu::test_type_exception);
    auto post_calls = test_array.get_allocator().data();

    expu_tests::alloc_counters expected;
    expected.deallocations    = post_calls.allocations - pre_calls.allocations;   //Criterion 1
    expected.destructor_calls = post_calls.ctor_calls() - pre_calls.ctor_calls(); //Criterion 2

    std::string err = check_counters(test_array, pre_calls, expected);
    ASSERT_FALSE(err.size()) << err;

    //Expecting darray be in the same state as before reserve was called. //Criterion 3
    ASSERT_EQ(test_array.capacity(), test_size);
    ASSERT_TRUE(std::equal(test_array.begin(), test_array.end(), seq_begin, seq_end));
}


/////////////////////////////////////////////////DARRAY ASSIGN TESTS///////////////////////////////////////////////////////////////////////////


/*
* Test assign for the case where the iterator is atleast an input_iterator and
* the range to be assigned is greater than the capacity of darray array. Expecting
* optimised assignment and growing of darray.
* 
* Testing criteria:
* 1. All allocations during assignment must be paired with a deallocation (no memory leaks)
* 2. One call to destructor per element in darray (if not trivially destructible otherwise 0)
* 3. One constructor call per element in range
* 4. darray contents match assignment range
* 
* Todo: Consider whether it is important to have new darray capacity equal to assign range size.
*/
TYPED_TEST(darray_trivially_destructible_tests, assign_case_1_forward_iterator_assign_size_greater_than_capacity)
{
    constexpr size_t assign_size = TestFixture::init_size << 1;

    const auto assign_begin = expu::seq_iter<size_t>(TestFixture::init_size);
    const auto assign_end   = assign_begin + assign_size;

    using iter_ref_t = std::iter_reference_t<std::decay_t<decltype(assign_begin)>>;

    const auto pre_calls = this->test_array.get_allocator().data();
    this->test_array.assign(assign_begin, assign_end);
    const auto post_calls = this->test_array.get_allocator().data();

    expu_tests::alloc_counters expected;
    expected.destructor_calls = TestFixture::init_size;                         //Criterion 2
    expected.deallocations    = post_calls.allocations - pre_calls.allocations; //Criterion 1
    expected.calls.add<iter_ref_t>(assign_size);                           //Criterion 3

    std::string err = check_counters(this->test_array, pre_calls, expected);
    ASSERT_FALSE(err.size()) << err;

    //Criterion 4
    ASSERT_TRUE(std::equal(this->test_array.begin(), this->test_array.end(), assign_begin, assign_end)) 
        << "Elements have not been assigned correctly!";

    //ASSERT_EQ(this->test_array.capacity(), assign_size); Revision: Capacity may be anything as long as content is properly assigned
}

/*
* Test assign for the case where the iterator is atleast a forward_iterator and
* the range to be assigned is less than the capacity array but greater
* than the size of the darray. Expecting optimised assignment.
* 
* Test criteria:
* 1. (Optimisation) No new allocations or deallocations.
* 2. ATMOST one construction call per element in assignment range.
* 3. darray contents match assignment range
*/
TYPED_TEST(darray_trivially_destructible_tests, assign_case_2_forward_iterator_assign_size_between_size_and_capacity)
{
    constexpr size_t assign_size = TestFixture::init_size << 1;

    const auto assign_begin = expu::seq_iter<size_t>(TestFixture::init_size);
    const auto assign_end   = assign_begin + assign_size;

    using iter_ref_t = std::iter_reference_t<std::decay_t<decltype(assign_begin)>>;

    //Resize array, guaranteeing darray capacity is greater than assignement range size.
    std::string error = checked_darray_reserve(this->test_array, TestFixture::init_size * 3);
    ASSERT_FALSE(error.size()) << error;

    //Explicit check for the above 
    ASSERT_LT(assign_size, this->test_array.capacity())
        << "This test requires that the capacity of the darray be greater than the assignment range size!";

    const auto pre_calls = this->test_array.get_allocator().data();
    this->test_array.assign(assign_begin, assign_end);
    const auto post_calls = this->test_array.get_allocator().data();

    expu_tests::alloc_counters expected;
    expected.allocations   = 0; //Criterion 1
    expected.deallocations = 0; //Criterion 1

    error = check_counters(this->test_array, pre_calls, expected);
    ASSERT_FALSE(error.size()) << error;

    //Criterion 2
    ASSERT_LE(post_calls.calls.get<iter_ref_t&>() - pre_calls.calls.get<iter_ref_t&>(), assign_size)
        << "Expecting less calls to constructor than assignment range size!";

    //Criterion 3
    ASSERT_TRUE(std::equal(this->test_array.begin(), this->test_array.end(), assign_begin, assign_end))
        << "Elements were not assigned correctly!";
}

/*
* Test assign for the case where the iterator is atleast a forward_iterator and
* the range to be assigned is less than the size of the darray. Expecting 
* optimised assignment.
* 
* Test criteria:
* 1. (Optimisation) No new allocations or deallocations.
* 2. ATMOST one construction call per element in assignment range.
* 3. darray contents match assignment range.
* 4. One call to destructor per each element whose index is greater than assignment range size
*    (if not trivially destructible otherwise 0)
*/
TYPED_TEST(darray_trivially_destructible_tests, assign_case_3_forward_iterator_assign_size_less_than_size)
{
    constexpr size_t assign_size = TestFixture::init_size >> 1;

    const auto assign_begin = expu::seq_iter<size_t>(TestFixture::init_size);
    const auto assign_end   = assign_begin + assign_size;

    using iter_ref_t = std::iter_reference_t<std::decay_t<decltype(assign_begin)>>;

    const auto pre_calls = this->test_array.get_allocator().data();
    this->test_array.assign(assign_begin, assign_end);
    const auto post_calls = this->test_array.get_allocator().data();

    expu_tests::alloc_counters expected;
    expected.destructor_calls = TestFixture::init_size - assign_size; //Criterion 3
    expected.allocations      = 0; //Criterion 1
    expected.deallocations    = 0; //Criterion 1

    std::string error = check_counters(this->test_array, pre_calls, expected);
    ASSERT_FALSE(error.size()) << error;

    //Criterion 2
    ASSERT_LE(post_calls.calls.get<iter_ref_t&>() - pre_calls.calls.get<iter_ref_t&>(), assign_size)
        << "Expecting less calls to constructor than assignment range size!";

    //Criterion 3
    ASSERT_TRUE(std::equal(this->test_array.begin(), this->test_array.end(), assign_begin, assign_end))
        << "Elements were not correctly assigned!";
}

/*
* Test assign for the case where the iterator is an input_iterator and
* the range to be assigned is greater than the capacity of the darray.
* Expecting assignment followed by call to push/emplace_back until 
* assignment range reaches the end.
* 
* Test criteria:
* 1. All allocations during assignment must be paired with a deallocation (no memory leaks)
* 2. All copy/move constructions during assignment must be paired with a destruction
* 3. ATMOST one call to constructor per element in assignment range
* 4. darray contents match assignment range. 
*/
TEST_F(darray_non_trivially_destructible_tests, assign_case_4_1_input_iterator_assign_size_greater_than_capacity)
{
    constexpr size_t assign_size = init_size << 1;

    //Need to use input iterator
    using iter_type = expu::iterator_downcast<expu::seq_iter<size_t>, std::input_iterator_tag>;
    using iter_ref_t = std::iter_reference_t<iter_type>;

    //Explicit check for above requirement
    ASSERT_TRUE(std::input_iterator<iter_type>)    << "For this test, iterator category must be input_iterator!";
    ASSERT_FALSE(std::forward_iterator<iter_type>) << "For this test, iterator category must be input_iterator!";

    const auto assign_begin = iter_type(init_size);
    const auto assign_end   = iter_type(init_size + assign_size);

    const auto pre_calls = test_array.get_allocator().data();
    this->test_array.assign(assign_begin, assign_end);
    const auto post_calls = test_array.get_allocator().data();

    expu_tests::alloc_counters expected;
    expected.deallocations    = post_calls.allocations - pre_calls.allocations;   //Criterion 1
    expected.destructor_calls = post_calls.ctor_calls() - pre_calls.ctor_calls(); //Criterion 2

    std::string err = check_counters(test_array, pre_calls, expected);
    ASSERT_FALSE(err.size()) << err;

    //Criterion 3
    ASSERT_LE(post_calls.calls.get<iter_ref_t&>() - pre_calls.calls.get<iter_ref_t&>(), assign_size)
        << "Expecting less calls to constructor than assignment range size!";

    //Criterion 4
    ASSERT_TRUE(std::equal(this->test_array.begin(), this->test_array.end(), assign_begin, assign_end))
        << "Elements have not been assigned correctly!";
}

/*
* Test assign for the case where the iterator is an input_iterator and
* the range to be assigned is less than the size of the darray.
* Expecting assignment until end of assignment range is reached, then 
* destruction of remaining darray elements.
* 
* Testing focus: Correct destruction of non-overlapping darray elements
* 
* Testing criteria:
* 1. ATMOST one call to constructor per element in assignment range
* 2. No new calls to move/copy constructor
* 3. No new calls to allocate and deallocate
* 4. One call to destructor per non-overlapping darray element.
* 5. darray contents match assignment range. 

*/
TYPED_TEST(darray_trivially_destructible_tests, assign_case_4_2_input_iterator_assign_size_less_than_size)
{
    constexpr size_t assign_size = TestFixture::init_size >> 1;

    //Need to use input iterator
    using iter_type = expu::iterator_downcast<expu::seq_iter<size_t>, std::input_iterator_tag>;
    using iter_ref_t = std::iter_reference_t<iter_type>;

    ASSERT_TRUE(std::input_iterator<iter_type>)    << "For this test, iterator category must be input_iterator!";
    ASSERT_FALSE(std::forward_iterator<iter_type>) << "For this test, iterator category must be input_iterator!";

    const auto assign_begin = iter_type(TestFixture::init_size);
    const auto assign_end   = iter_type(TestFixture::init_size + assign_size);

    expu_tests::alloc_counters expected;
    expected.copy_ctor_calls = 0; //Criterion 1 
    expected.move_ctor_calls = 0; //Criterion 1
    expected.allocations     = 0; //Criterion 3
    expected.deallocations   = 0; //Criterion 3
    //Since assign range size is smaller than darray::size(), tail end elements should be
    //deleted, the rest should be assigned or memcopied. //Criterion 4
    expected.destructor_calls = TestFixture::init_size - assign_size;

    const auto pre_calls = this->test_array.get_allocator().data();
    this->test_array.assign(assign_begin, assign_end);
    const auto post_calls = this->test_array.get_allocator().data();

    //Criterion 3
    ASSERT_LE(post_calls.calls.get<iter_ref_t&>() - pre_calls.calls.get<iter_ref_t&>(), assign_size)
        << "Expecting less calls to constructor than assignment range size!";

    std::string err = check_counters(this->test_array, pre_calls, expected);
    ASSERT_FALSE(err.size()) << err;

    //Criterion 5
    ASSERT_TRUE(std::equal(this->test_array.begin(), this->test_array.end(), assign_begin, assign_end))
        << "Elements have not been assigned correctly!";
}


/////////////////////////////////////////////////DARRAY INSERTION TESTS////////////////////////////////////////////////////////////////////


class darray_insertion_tests : 
    public _darray_tests_base<expu::TestType<size_t>, std::allocator>,
    public testing::TestWithParam<size_t>
{
public:
    constexpr void SetUp() override { return darray_insertion_tests::base_type::SetUp(); }

    template<std::input_iterator InputIt>
    constexpr void check_insertion(InputIt begin, InputIt end, size_t insert_size) const
    {
       const auto at = insert_at();
       EXPECT_TRUE(std::equal(test_array.cbegin(), at, expu::seq_iter<size_t>(0), expu::seq_iter(GetParam())))
            << "Insertion altered elements before insert location!";

        const auto darray_insert_end = at + insert_size;
        EXPECT_TRUE(std::equal(at, darray_insert_end, begin, end))
            << "Elements have not been inserted correctly!";

        EXPECT_TRUE(std::equal(darray_insert_end, test_array.end(), expu::seq_iter(GetParam()), expu::seq_iter(init_size)))
            << "Elements have not been assigned correctly!";
    }

    template<std::forward_iterator FwdIt>
    constexpr void check_insertion(FwdIt begin, FwdIt end) const
    {
        check_insertion(begin, end, std::ranges::distance(begin, end));
    }

public:
    constexpr auto insert_at() const 
    {
        return test_array.cbegin() + GetParam();
    }

};

INSTANTIATE_TEST_SUITE_P(
    darray_tests,
    darray_insertion_tests,
    ::testing::Range(
        0ull, 
        darray_insertion_tests::init_size + 1, 
        darray_insertion_tests::init_size / 5));

/*
* Test insertion edge case where insertion range has zero size. In this scenario,
* insertion should be cancelled and iterators should remain valid.
* 
* Testing criteria:
* 1. iterators are preserved.
* 2. No calls to allocation or deallocation
* 3. No calls to move/copy constructor or destructor
* 4. darray is left unchanged after insertion.
*/
TEST_P(darray_insertion_tests, darray_insert_do_not_invalidate_iterators)
{
    const auto last_begin = test_array.begin();
    const auto last_end = test_array.end();

    const auto pre_calls = test_array.get_allocator().data();
    test_array.insert(insert_at(), expu::seq_iter(0), expu::seq_iter(0));

    //Criterion 1
    ASSERT_EQ(last_begin, test_array.begin()) << "Iterators fully invalidated!";
    ASSERT_EQ(last_end, test_array.end())     << "Iterators partially invalidated!";

    expu_tests::alloc_counters expected;
    expected.copy_ctor_calls  = 0; //Criterion 3
    expected.move_ctor_calls  = 0; //Criterion 3
    expected.destructor_calls = 0; //Criterion 3
    expected.allocations      = 0; //Criterion 2
    expected.deallocations    = 0; //Criterion 2

    std::string err = check_counters(this->test_array, pre_calls, expected);
    ASSERT_FALSE(err.size()) << err;

    //Criterion 4
    ASSERT_TRUE(std::equal(this->test_array.begin(), this->test_array.end(), emplace_begin, emplace_end))
        << "Elements have not been assigned correctly!";
}

/*
* Test insertion for the case where the iterator is atleast a forward_iterator
* and the insertion range plus the size of the darray is greater than the capacity
* of the darray, hence a resize is needed.
* 
* Testing criteria:
* 1. All allocations during assignment must be paired with a deallocation (no memory leaks).
* 2. Only one call to copy/move constructor per element of darray (if not trivially copyable otherwise 0).
* 3. Only one call to constructor per element of insertion range (if not trivially copyable otherwise 0).
* 4. Correct insertion
*/
TEST_P(darray_insertion_tests, insert_case_1_forward_iterator_insert_requires_resize)
{
    constexpr size_t insert_size = 10000;

    using iter_t     = expu::seq_iter<size_t>;
    using iter_ref_t = std::iter_reference_t<iter_t>;

    const auto insert_begin = iter_t(init_size);
    const auto insert_end   = iter_t(init_size + insert_size);

    const auto pre_calls = this->test_array.get_allocator().data();
    this->test_array.insert(insert_at(), insert_begin, insert_end);
    const auto post_calls = this->test_array.get_allocator().data();

    expu_tests::alloc_counters expected;
    expected.deallocations = post_calls.allocations - pre_calls.allocations; //Criterion 1
    expected.calls.add<iter_ref_t>(insert_size);                             //Criterion 3

    std::string err = check_counters(this->test_array, pre_calls, expected);
    ASSERT_FALSE(err.size()) << err;

    //Criterion 2
    ASSERT_EQ(post_calls.ctor_calls() - pre_calls.ctor_calls(), init_size)
        << "Expecting only one call to copy/move constructor per original element in darray!";

    //Criterion 4
    check_insertion(insert_begin, insert_end);
}

/*
* Test insertion for the case where the iterator is atleast a forward_iterator
* and the darray has sufficient capacity to include range. Here two cases can
* occur: (1) the insertion region does not overlapp uninitialised portion of
* the darray.
* 
* Illustration:
* Legend: (#) initialised element, (-) uninitialised element, (@) newly inserted element,
*         (~) initialised but has been moved 
* 
* To insert: @@@@@@@
* 
* |######################--------------|
*          ^insert here
* 
* Step 1 (shift elements into uninitialised space):
*
* |################~~~~~~######--------|
*          ^insert here  ^previously end of darray
* 
* Step 2 (back fill elements):
*
* |########~~~~~~##############--------|
*                        ^previously end of darray
* 
* Step 3 (insert desired elements though assignment, no inserted element overlapps previously uninitialised area)
* 
* |########@@@@@@##############--------|
*                        ^previously end of darray
* 
* (2) The 
* 
* Illustration:
* Legend: (#) initialised element, (-) uninitialised element, (@) newly inserted element (assignment), 
*         (~) initialised but has been moved, (+) newly inserted element (construction)
* 
* To insert: @@@@@++ (Note: due to using a forward iterator, elements needing construction can be
*                           deduced ahead of time)
* 
* |######################--------------|
*                    ^ insert here
* 
* Step 1 (shift elements):
*
* |##################~~~~  ####--------|
*                        ^ previously end of darray
* 
* Step 2 (insert desired elements though construction in uninitialised region)
* 
* |##################~~~~++####--------|
*                        ^ previously end of darray
* 
* Step 3 (insert remaining elements though assignment)
* 
* |##################@@@@++####--------|
*                        ^ previously end of darray
* 
* Testing criteria:
* 1. All allocations during assignment must be paired with a deallocation (no memory leaks).
* 2. ATMOST one call to copy/move constructor per element of darray (if not trivially copyable otherwise 0).
* 3. ATMOST one call to constructor per element of insertion range (if not trivially copyable otherwise 0).
* 4. Correct insertion
*/
TEST_P(darray_insertion_tests, insert_case_2_forward_iterator_no_resize_non_overlapping) 
{
    using iter_t     = expu::seq_iter<size_t>;
    using iter_ref_t = std::iter_reference_t<iter_t>;

    const size_t unused_capacity = test_array.capacity() - test_array.size();
    const size_t insert_size     = unused_capacity - 100;

    const auto insert_begin = iter_t(init_size);
    const auto insert_end   = iter_t(init_size + insert_size);

    const auto pre_calls = this->test_array.get_allocator().data();
    test_array.insert(insert_at(), insert_begin, insert_end);
    const auto post_calls = this->test_array.get_allocator().data();

    expu_tests::alloc_counters expected;
    expected.deallocations = post_calls.allocations - pre_calls.allocations; //Criterion 1

    std::string err = check_counters(this->test_array, pre_calls, expected);
    ASSERT_FALSE(err.size()) << err;

    //Criterion 2
    ASSERT_LE(post_calls.ctor_calls() - pre_calls.ctor_calls(), init_size)
        << "Expecting atmost one call to copy/move constructor per original element in darray!";

    //Criterion 3
    ASSERT_LE(post_calls.calls.get<iter_ref_t>() - pre_calls.calls.get<iter_ref_t>(), insert_size)
        << "Expecting atmost one call to constructor per array insertion!";

    //Criterion 4
    check_insertion(insert_begin, insert_end);
}

void darray_input_iterator_insertion_test(size_t insert_size, darray_insertion_tests& fixture)
{
    using iter_t = expu::iterator_downcast<expu::seq_iter<size_t>, std::input_iterator_tag>;
    using iter_ref_t = std::iter_reference_t<iter_t>;

    const auto insert_begin = iter_t(darray_insertion_tests::init_size);
    const auto insert_end   = iter_t(darray_insertion_tests::init_size + insert_size);

    auto& test_array = fixture.test_array;

    const auto pre_calls = test_array.get_allocator().data();
    test_array.insert(fixture.insert_at(), insert_begin, insert_end);
    const auto post_calls = test_array.get_allocator().data();

    expu_tests::alloc_counters expected;
    expected.deallocations = post_calls.allocations - pre_calls.allocations; //Criterion 1

    std::string err = check_counters(test_array, pre_calls, expected);
    ASSERT_FALSE(err.size()) << err;

    //Criterion 2
    ASSERT_LE(post_calls.calls.get<iter_ref_t>() - pre_calls.calls.get<iter_ref_t>(), insert_size)
        << "Expecting atmost one call to constructor per array insertion!";

    //Criterion 3
    fixture.check_insertion(insert_begin, insert_end, insert_size);
}

/*
* Testing insertion using an input_iterator for the case where the darray
* has enough capacity to contain the inserted range.
* 
* Testing criteria:
* 1. All allocations during assignment must be paired with a deallocation (no memory leaks).
* 2. ATMOST one call to constructor per element in insertion range
* 3. Correct insertion
*/
TEST_P(darray_insertion_tests, insert_case_3_input_iterator_enough_capacity)
{
    const size_t unused_capacity = test_array.capacity() - test_array.size();
    //See documentation for criteria
    darray_input_iterator_insertion_test(unused_capacity - 100, *this);
}

/*
* Testing insertion using an input_iterator for the case where the darray
* does not enough capacity to contain the inserted range.
*
* Testing criteria:
* 1. All allocations during assignment must be paired with a deallocation (no memory leaks).
* 2. ATMOST one call to constructor per element in insertion range
* 3. Correct insertion
*/
TEST_P(darray_insertion_tests, insert_case_4_input_iterator_not_enough_capacity)
{
    const size_t unused_capacity = test_array.capacity() - test_array.size();
    //See documentation for criteria
    darray_input_iterator_insertion_test(unused_capacity * 2, *this);
}


/////////////////////////////////////////////////DARRAY INSERTION EXCEPTION TESTS////////////////////////////////////////////////////////////////////


struct darray_insertion_throw_tests:
    public _darray_tests_base<
        expu::throw_on_spcl_after_x<expu::TestType<size_t>>,
        std::allocator>,
    public testing::TestWithParam<size_t> {};

INSTANTIATE_TEST_SUITE_P(
    darray_tests,
    darray_insertion_throw_tests,
    ::testing::Range(
        0ull,
        darray_insertion_throw_tests::init_size + 1,
        darray_insertion_throw_tests::init_size / 5));

/*
* Tests for strong guarantee on insertion
*/
template<typename Type, typename Alloc, std::forward_iterator FwdIt>
void darray_insert_test_strong_guarantee(
    expu::darray<Type, Alloc>& test_array,
    size_t at_index, 
    FwdIt begin, 
    FwdIt end)
{
    //Store previous sentinels for throwing on copies
    size_t prev_throw_after_ctor = 0;
    size_t prev_throw_after_asgn = 0;

    if constexpr (expu::template_of<Type, expu::throw_on_spcl_after_x>) {
        /*
        * If using throw_on_spcl_after_x type then ensure type does not
        * (expectedly) throw during copying of the range.
        */

        Type::reset();
        prev_throw_after_ctor = Type::throw_after_ctor;
        prev_throw_after_asgn = Type::throw_after_asgn;

        Type::throw_after_ctor = std::numeric_limits<size_t>::max();
        Type::throw_after_asgn = std::numeric_limits<size_t>::max();
    }

    std::allocator<Type> alloc;
    size_t prev_size = test_array.size();

    Type* copied_first = nullptr;
    Type* copied_last  = expu::_ctg_duplicate(alloc, test_array.begin(), test_array.end(), copied_first, prev_size);

    if constexpr (expu::template_of<Type, expu::throw_on_spcl_after_x>) {
        //Set sentinels back to what they were before
        Type::reset();
        Type::throw_after_ctor = prev_throw_after_ctor;
        Type::throw_after_asgn = prev_throw_after_asgn;
    }

    const auto at = test_array.begin() + at_index;

    const auto prev_begin = test_array.begin();
    const auto prev_end = test_array.end();

    ASSERT_THROW(test_array.insert(at, begin, end), expu::test_type_exception)
        << "Expecting the function to throw here";

    //Criterion 1
    ASSERT_EQ(prev_begin, test_array.begin()) << "Iterators fully invalidated!";
    ASSERT_EQ(prev_end, test_array.end())     << "Iterators partially invalidated!";

    //Criterion 2
    ASSERT_TRUE(std::equal(test_array.begin(), test_array.end(), copied_first, copied_last))
        << "Strong guarantee not provided!";

    //Cleanup
    if constexpr (expu::template_of<Type, expu::throw_on_spcl_after_x>)
        Type::reset();

    alloc.deallocate(copied_first, prev_size);
}

/*
* Testing failure on move/copy construction during insertion that requires a resize of the container.
* 
* Testing criteria:
* 1. Expecting iterators to remain valid
* 2. Expecting elements to be left invariant (strong guarantee).
*/
TEST_P(darray_insertion_throw_tests, insertion_exceptions_1_forward_iterator_insert_requires_resize_throw_on_ctor)
{
    constexpr size_t insert_size = 10000;

    using iter_t = expu::seq_iter<size_t>;

    darray_type arr(iter_t(0), iter_t(init_size));

    test_type::throw_after_ctor = 10;
    //See description for criteria
    darray_insert_test_strong_guarantee(arr, GetParam(), iter_t(init_size), iter_t(init_size + insert_size));

}

struct throw_on_construction : public expu::TestType<size_t, expu::test_type_props::not_trivially_destructible> {
private:
    using _base_t = expu::TestType<size_t, expu::test_type_props::not_trivially_destructible>;
public:
    inline static std::atomic_bool disabled = false;
    
public:
    throw_on_construction(size_t num):
        _base_t(num)
    {
        if (!disabled)
            throw expu::test_type_exception();
    }
};

/*
* Testing failure on move/copy construction during insertion that requires a resize of the container.
*
* Testing criteria:
* 1. Expecting iterators to remain valid
* 2. Expecting elements to be left invariant (strong guarantee).
*/
TEST_P(darray_insertion_throw_tests, insertion_exceptions_2_forward_iterator_insert_with_enough_capacity)
{

    using iter_t = expu::seq_iter<size_t>;
    using test_t = throw_on_construction;

    using alloc_t = expu::checked_allocator<expu_tests::counted_allocator<std::allocator<test_t>>>;

    //Disable throw on construction until darray is initialised
    test_t::disabled = true;

    expu::darray<test_t, alloc_t> arr(iter_t(0), iter_t(init_size));
    checked_darray_reserve(arr, init_size * 2);

    test_t::disabled = false;

    size_t insert_size = arr.capacity() - arr.size() - 100;
    darray_insert_test_strong_guarantee(arr, GetParam(), iter_t(init_size), iter_t(init_size + insert_size));
}
