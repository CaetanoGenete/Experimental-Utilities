#include "gtest/gtest.h"

#include <string>
#include <algorithm>

#include "expu/containers/darray.hpp"
#include "expu/iterators/seq_iter.hpp"
#include "expu/test_utils.hpp"

#include "counters.hpp"

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

    size_t expected_copies       = 0;
    size_t expected_moves        = 0;
    size_t expected_destructions = 0;
    size_t expected_allocations  = 0;
    size_t expected_capacity     = arr.capacity();

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

    for (FwdIt it = begin; it != end; ++it)
        arr.emplace_back(*it);

    std::string error = expected_capacity != arr.capacity() ?
                        "Unexpected array capacity!\n" : "";

    if (!std::equal(arr.end() - range_size, arr.end(), begin, end))
        error += "Unexpected tail elements of array!\n";

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

template<typename Type, template <typename ...> typename Allocator, typename ... AdditionalAllocatorTArgs>
struct _darray_tests_base
{
public:
    using test_type  = Type;
    using alloc_type = expu_tests::counted_allocator<Allocator<test_type, AdditionalAllocatorTArgs...>>;

    using darray_type = expu::darray<test_type, alloc_type>;

    using base_type = _darray_tests_base;

public:
    constexpr static size_t init_size = 10000;

public:
    constexpr auto SetUp() 
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
    public _darray_tests_base<std::conditional_t<IsTrivial::value,
        expu::TestType<int, expu::test_type_props::inherit_trivially_copyable>,
        expu::TestType<int>
    >, std::allocator>,
    public testing::TestWithParam<IsTrivial> 
{
public:
    constexpr static bool is_trivial = std::is_trivially_copyable_v<darray_trivially_copyable_tests::test_type>;

    static_assert(is_trivial == IsTrivial::value, "Not testing a trivially copyable type!");
public:
    //Not inherited, much sad
    constexpr auto SetUp() { return darray_trivially_copyable_tests::base_type::SetUp(); }
};

using darray_tests_cases = testing::Types<std::false_type, std::true_type>;
TYPED_TEST_SUITE(darray_trivially_copyable_tests, darray_tests_cases);

/*
* Test to ensure that stateless allocators do not 
* take extra space in darray's memory footprint. Here
* the test is performed using the standard allocator.
*/
TEST(darray_trivially_copyable_tests, sizeof_darray_with_stateless_allocator) 
{
    using darray_type = expu::darray<int>;

    //sizeof darray should be equivalent to 3 pointers. 
    ASSERT_EQ(sizeof(darray_type), 3 * sizeof(darray_type::pointer));
}

/*
* Testing emplace_back. Due to Fixture (darray_tests), test passes
* if darray_tests::SetUp() passes.
*/
TYPED_TEST(darray_trivially_copyable_tests, emplace_back) {}

/*
* Test copy construction, expect no calls to copy constructor
* if container value_type is trivially copyable, otherwise one
* copy per element. In either case, expecting only one allocation.
*/
TYPED_TEST(darray_trivially_copyable_tests, copy_construct)
{
    typename TestFixture::darray_type& test_array = this->test_array;
    typename TestFixture::darray_type copy_array(this->test_array);

    ASSERT_TRUE(std::equal(
        copy_array.cbegin(), copy_array.cend(),   //Copied range
        test_array.cbegin(), test_array.cend())); //Original range

    std::string err;

    if constexpr (std::is_trivially_copyable_v<typename TestFixture::test_type>)
        //Expecting no calls to copy constructor if trivially copyable
        err = expu_tests::check_counters(copy_array.get_allocator(), { 0, 0, 0, 1, 0 });
    else
        err = expu_tests::check_counters(copy_array.get_allocator(), { TestFixture::init_size, 0, 0, 1, 0 });


    ASSERT_FALSE(err.size()) << err;
}

using darray_non_trivial_type_tests = darray_trivially_copyable_tests<std::false_type>;
/*
* Testing move constructor without change in allocator. Since allocator
* is moved to new instance, memory can be safely deallocated hence expecting
* pointers of darray to be stolen, therefore no calls to any constructor or
* allocations.
*/
TEST_F(darray_non_trivial_type_tests, move_construct_case_1_same_allocator)
{
    const auto delta = this->test_array.get_allocator().data();

    darray_type moved_arr(std::move(this->test_array));

    //Expecting no new calls, content should be stolen instead.
    std::string err = expu_tests::check_counters(moved_arr.get_allocator(), delta, { 0, 0, 0, 0, 0 });
    ASSERT_FALSE(err.size()) << err;

    ASSERT_TRUE(std::equal(
        moved_arr.begin(), moved_arr.end(),
        this->emplace_begin, this->emplace_end)
    ) << "Elements have not been correctly moved!";
}

/*
* Testing move constructor with change of allocator that compares equal.
*/
TEST_F(darray_non_trivial_type_tests, move_construct_case_2_new_allocator_comp_equal)
{
    const auto new_alloc = alloc_type();
    darray_type moved_arr(std::move(this->test_array), new_alloc);

    //Expecting no calls to any counter. Counters should be the same as new_alloc counters
    //as data should have been stolen.
    std::string err = expu_tests::check_counters(moved_arr.get_allocator(), new_alloc.data());
    ASSERT_FALSE(err.size()) << err;

    ASSERT_TRUE(std::equal(
        moved_arr.begin(), moved_arr.end(),
        this->emplace_begin, this->emplace_end)
    ) << "Elements have not been correctly moved!";
}

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
    public _darray_tests_base<std::conditional_t<IsTrivial::value,
        expu::TestType<int, expu::test_type_props::inherit_trivially_copyable>,
        expu::TestType<int>
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
    //Not inherited, much sad
    constexpr auto SetUp() { return darray_comp_false_no_prop_alloc_tests::base_type::SetUp(); }
};
TYPED_TEST_SUITE(darray_comp_false_no_prop_alloc_tests, darray_tests_cases);

/*
* Testing move constructor with change of allocator that does not compare equal. Expecting
* the container to individually move all its elements over. If type is trivially copyable,
* should not call move constructor. Otherwise, one call to move constructor for each
* element.
*/
TYPED_TEST(darray_comp_false_no_prop_alloc_tests, move_construct_case_3_new_allocator_not_comp_equal)
{
    const auto new_alloc = typename TestFixture::alloc_type();
    typename TestFixture::darray_type moved_arr(std::move(this->test_array), new_alloc);

    std::string err;

    if constexpr (std::is_trivially_copyable_v<typename TestFixture::test_type>)
        err = expu_tests::check_counters(moved_arr.get_allocator(), { 0, 0, 0, 1, 0 });
    else
        err = expu_tests::check_counters(moved_arr.get_allocator(), { 0, TestFixture::init_size, 0, 1, 0 });

    ASSERT_FALSE(err.size()) << err;

    ASSERT_TRUE(std::equal(
        moved_arr.begin(), moved_arr.end(),
        this->emplace_begin, this->emplace_end)
    ) << "Elements have not been correctly moved!";
}

template<typename IsTrivial>
struct darray_trivially_destructible_tests :
    public _darray_tests_base<std::conditional_t<IsTrivial::value,
        expu::TestType<int>,
        expu::TestType<int, expu::test_type_props::not_trivially_destructible>
    >, std::allocator>,
    public testing::TestWithParam<IsTrivial>
{
public:
    constexpr static bool is_trivial = std::is_trivially_destructible_v<darray_trivially_destructible_tests::test_type>;

    static_assert(is_trivial == IsTrivial::value, "Type is not trivially destructible!");
public:
    //Not inherited, much sad
    constexpr auto SetUp() { return darray_trivially_destructible_tests::base_type::SetUp(); }
};
TYPED_TEST_SUITE(darray_trivially_destructible_tests, darray_tests_cases);

/*
* Test assign for the case where the iterator is a
*/
TYPED_TEST(darray_trivially_destructible_tests, assign_case_1_forward_iterator_assign_size_greater_than_capacity)
{
    constexpr size_t assign_size = TestFixture::init_size << 1;

    const auto assign_begin = expu::seq_iter<size_t>(TestFixture::init_size);
    const auto assign_end = assign_begin + assign_size;

    const auto delta = this->test_array.get_allocator().data();

    this->test_array.assign(assign_begin, assign_end);

    typename TestFixture::alloc_type::map_type expected_calls;
    expected_calls.add<size_t&>(assign_size);

    std::string err;

    if constexpr (std::is_trivially_destructible_v<typename TestFixture::test_type>)
        std::string err = expu_tests::check_counters(this->test_array.get_allocator(), delta, { 0, 0, 0, 1, 1, expected_calls });
    else
        std::string err = expu_tests::check_counters(this->test_array.get_allocator(), delta, { 0, 0, TestFixture::init_size, 1, 1, expected_calls });

    ASSERT_FALSE(err.size()) << err;

    ASSERT_TRUE(std::equal(
        this->test_array.begin(), this->test_array.end(),
        assign_begin, assign_end)
    ) << "Elements have not been assigned correctly!";

    ASSERT_EQ(this->test_array.capacity(), assign_size);
}

template<typename Type, typename Allocator>
constexpr std::string checked_darray_reserve(
    expu::darray<Type, Allocator>& arr,
    typename std::allocator_traits<Allocator>::size_type new_size)
{
    const auto delta = arr.get_allocator().data();
    const auto old_cap = arr.capacity();
    const auto old_size = arr.size();

    Type* copied_data = new Type[old_size];
    //Todo: Create bespoke
    std::uninitialized_copy(arr.begin(), arr.end(), copied_data);

    arr.reserve(new_size);

    size_t expected_copies = 0;
    size_t expected_moves = 0;
    size_t expected_allocations = 0;

    std::string error;

    if (new_size > old_cap) {
        expected_allocations = 1;

        if constexpr (!std::is_trivially_copyable_v<Type>) {
            if constexpr (std::is_nothrow_move_constructible_v<Type>)
                expected_moves = old_size;
            else
                expected_copies = old_size;
        }

        if (arr.capacity() != new_size)
            error += "Unexpected new capacity!";
    }

    if (arr.size() != old_size)
        error += "Expected size to be invariant under reserve!";

    size_t expected_destructions = std::is_trivially_destructible_v<Type> ?
                                   0 : expected_copies + expected_moves;

    error += expu_tests::check_counters(arr.get_allocator(), delta,
        {
            expected_copies,
            expected_moves,
            expected_destructions,
            expected_allocations,
            expected_allocations  //Expected deallocations
        });

    if (!std::equal(arr.begin(), arr.end(), copied_data))
        error += "Expected data to be invariant under reserve!";

    delete[] copied_data;

    return error;
}

TYPED_TEST(darray_trivially_destructible_tests, assign_case_2_forward_iterator_assign_size_between_size_and_capacity)
{
    constexpr size_t assign_size = TestFixture::init_size << 1;

    const auto assign_begin = expu::seq_iter<size_t>(TestFixture::init_size);
    const auto assign_end = assign_begin + assign_size;

    std::string error = checked_darray_reserve(this->test_array, TestFixture::init_size * 3);
    ASSERT_FALSE(error.size()) << error;

    const auto delta = this->test_array.get_allocator().data();

    this->test_array.assign(assign_begin, assign_end);

    typename TestFixture::alloc_type::map_type expected_calls;
    expected_calls.add<size_t&>(assign_size - TestFixture::init_size);

    error = expu_tests::check_counters(this->test_array.get_allocator(), delta, { 0, 0, 0, 0, 0, expected_calls });
    ASSERT_FALSE(error.size()) << error;

    ASSERT_TRUE(std::equal(this->test_array.begin(), this->test_array.end(), assign_begin, assign_end))
        << "Elements were not correctly assigned!";
}

TYPED_TEST(darray_trivially_destructible_tests, assign_case_3_forward_iterator_assign_size_less_than_size)
{
    constexpr size_t assign_size = TestFixture::init_size >> 1;

    const auto assign_begin = expu::seq_iter<size_t>(TestFixture::init_size);
    const auto assign_end = assign_begin + assign_size;

    const auto delta = this->test_array.get_allocator().data();

    this->test_array.assign(assign_begin, assign_end);

    std::string error;

    if constexpr (std::is_trivially_destructible_v<TestFixture::test_type>)
        error = expu_tests::check_counters(this->test_array.get_allocator(), delta, { 0, 0, 0, 0, 0 });
    else
        error = expu_tests::check_counters(this->test_array.get_allocator(), delta, { 0, 0, TestFixture::init_size - assign_size, 0, 0 });

    ASSERT_FALSE(error.size()) << error;

    ASSERT_TRUE(std::equal(this->test_array.begin(), this->test_array.end(), assign_begin, assign_end))
        << "Elements were not correctly assigned!";
}

//Todo: Write tests for darray::assign with an input_iterator
