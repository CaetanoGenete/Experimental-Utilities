#include "gtest/gtest.h"

#include <memory>
#include <algorithm>
#include <sstream>

#include <variant>

#include "expu/containers/darray.hpp"
#include "expu/containers/fixed_array.hpp"

#include "expu/iterators/concatenated_iterator.hpp"
#include "expu/iterators/seq_iter.hpp"

#include "expu/testing/checked_allocator.hpp"
#include "expu/testing/test_allocator.hpp"
#include "expu/testing/throw_on_type.hpp"
#include "expu/testing/iterator_downcast.hpp"
#include "expu/testing/test_type.hpp"
#include "expu/testing/gtest_utils.hpp"

template<class Type, template<class ...> class Alloc, class ... ExtraArgs>
using checked_darray = expu::darray<Type, expu::checked_allocator<Alloc<Type, ExtraArgs...>>>;

//////////////////////////////////////DARRAY CHECKS///////////////////////////////////////////////////////////////////////////////

template<class Type, class Alloc, std::input_iterator InputIt, std::sentinel_for<InputIt> Sentinel>
testing::AssertionResult is_equal(const expu::darray<Type, Alloc>& arr, InputIt first, const Sentinel last) 
{
    size_t range_size = 0;

    auto arr_first = arr.begin();
    for (; arr_first != arr.end() && first != last; ++arr_first, ++first, ++range_size) {
        if(*arr_first != *first)
            return testing::AssertionFailure() 
                << "At index: (" << range_size << "). " << *arr_first << " != " << *first
                << ". expu::darray elements did not compare equal to range!";
    }

    if (arr_first == arr.end() && first == last)
        return testing::AssertionSuccess();
    else
        return testing::AssertionFailure() << "expu::darray size (" << arr.size() << ") != range size (" << range_size << ")";
}

template<class Type, class Alloc>
testing::AssertionResult is_darray_valid(const expu::darray<Type, expu::checked_allocator<Alloc>>& darray)
{
    if (darray.capacity() < darray.size())
        return testing::AssertionFailure() 
            << darray.capacity() << "(capacity) < " << darray.size() << " (size). " 
            << "expu::darray capacity should not be less than its size!";

    if(darray.begin() != darray.end())
    if (!darray.get_allocator().initialised(darray.begin()._unwrapped(), darray.end()._unwrapped()))
        return testing::AssertionFailure() << "Uninitialised gaps found in darray after function call!";

    if(darray.size() < darray.capacity())
    if (darray.get_allocator().atleast_one_initiliased_in(darray.end()._unwrapped(), (darray.begin() + darray.capacity())._unwrapped()))
        return testing::AssertionFailure() << "Initialised object found past end of expu::darray!";

    return testing::AssertionSuccess();
}

//Todo: Make general for container types if needed
template<class Type, class Alloc, class Callable, class ... Args>
testing::AssertionResult provides_weak_guarantee(
    expu::darray<Type, expu::checked_allocator<Alloc>>& darray, Callable&& function, Args&& ... args) 
{
    try {
        std::invoke(std::forward<Callable>(function), darray, std::forward<Args>(args)...);
    }
    catch (...) { return is_darray_valid(darray); }
}

template<expu::template_of<expu::darray> ArrayType, std::input_iterator InputIt, std::sentinel_for<InputIt> Sentinel>
testing::AssertionResult _verify_unchanged(const ArrayType& arr, const InputIt old_first, const Sentinel old_last, const typename ArrayType::size_type old_capacity)
{
    //expu::darray must always be in a valid state!
    if constexpr (expu::template_of<typename ArrayType::allocator_type, expu::checked_allocator>) {
        auto is_valid_result = is_darray_valid(arr);
        if (!is_valid_result)
            return is_valid_result;
    }

    if (arr.capacity() != old_capacity)
        return testing::AssertionFailure() << "New capacity (" << arr.capacity() << ") != old capacity (" << old_capacity << ")";

    return is_equal(arr, old_first, old_last);
}

template<class Type, class Alloc, class Callable, class ... Args>
testing::AssertionResult provides_strong_guarantee(
    expu::darray<Type, expu::checked_allocator<Alloc>>& darray, Callable&& function, Args&& ... args)
{
    EXPU_NO_THROW_ON(Type, const expu::fixed_array<Type> original_data(darray.begin(), darray.end()));

    const auto old_capacity = darray.capacity();
    const auto old_begin    = darray.begin();
    const auto old_end      = darray.end();

    try {
        std::invoke(std::forward<Callable>(function), darray, std::forward<Args>(args)...);
    }
    catch (...) {
        if (old_begin != darray.begin())
            return testing::AssertionFailure() << "Iterators fully invalidated!";

        if (old_end != darray.end())
            return testing::AssertionFailure() << "Iterators partially invalidated!";

        //Strong-guarantee: elements of expu::darray before function call should compare equal after call. 
        return _verify_unchanged(darray, original_data.begin(), original_data.end(), old_capacity);;
    }

    return testing::AssertionFailure() << "Function did not throw!";
}


//////////////////////////////////////DARRAY TEST FIXTURES//////////////////////////////////////////////////////////////////////////

template<class TrivialPair>
struct darray_trivial_tests;

template<
    std::convertible_to<bool> TriviallyCopyable, 
    std::convertible_to<bool> TriviallyDestructible>
struct darray_trivial_tests<std::tuple<TriviallyCopyable,TriviallyDestructible>>:
    public testing::Test
{
private:
    static constexpr bool _is_trivially_copyable     = TriviallyCopyable{};
    static constexpr bool _is_trivially_destructible = TriviallyDestructible{};

public:
    using value_type =
        std::conditional_t <_is_trivially_copyable && !_is_trivially_destructible,
        expu::test_type<int,
            expu::test_type_props::throw_on_copy_ctor,
            expu::test_type_props::throw_on_move_ctor,
            expu::test_type_props::inherit_trivially_copyable,
            expu::test_type_props::not_trivially_destructible>,
        std::conditional_t<_is_trivially_copyable,
        expu::test_type<int,
            expu::test_type_props::throw_on_copy_ctor,
            expu::test_type_props::throw_on_move_ctor,
            expu::test_type_props::inherit_trivially_copyable>,
        std::conditional_t<!_is_trivially_destructible,
            expu::test_type<int, expu::test_type_props::not_trivially_destructible>,
            expu::test_type<int>>>>;


    static_assert(std::is_trivially_destructible_v<value_type> == _is_trivially_destructible);
    static_assert(std::is_trivially_copyable_v<value_type>     == _is_trivially_copyable);
};

template<class IsTrivial>
struct darray_trivially_copyable_tests :
    public darray_trivial_tests<std::tuple<IsTrivial, std::true_type>> {};

template<class IsTrivial>
struct darray_trivially_destructible_tests :
    public darray_trivial_tests<std::tuple<std::false_type, IsTrivial>> {};

//Explicitely writting structs for nice console log information.
struct is_trivially_copyable      : public std::true_type {};
struct not_trivially_copyable     : public std::false_type {};
struct is_trivially_destructible  : public std::true_type {};
struct not_trivially_destructible : public std::false_type {};

using trivially_copyable_test_types     = testing::Types<not_trivially_copyable, is_trivially_copyable>;
using trivially_destructible_test_types = testing::Types<not_trivially_destructible, is_trivially_destructible>;
using trivial_test_types                = expu::cartesian_product_t<trivially_copyable_test_types, trivially_destructible_test_types>;

TYPED_TEST_SUITE(darray_trivially_copyable_tests, trivially_copyable_test_types);
TYPED_TEST_SUITE(darray_trivially_destructible_tests, trivially_destructible_test_types);
TYPED_TEST_SUITE(darray_trivial_tests, trivial_test_types);


template<class Params>
struct darray_trivial_iterator_tests;

template<
    class TrviallyConstructible, 
    class TriviallyDestructible, 
    class IteratorCategory>
struct darray_trivial_iterator_tests<std::tuple<TrviallyConstructible, TriviallyDestructible, IteratorCategory>>:
    public darray_trivial_tests<std::tuple<TrviallyConstructible, TriviallyDestructible>>
{
public:
    using iterator_category = IteratorCategory;

public:
    template<std::input_iterator Iterator>
    static constexpr bool satisfies =
        (std::forward_iterator<Iterator>       || !std::is_base_of_v<std::forward_iterator_tag, iterator_category>)       &&
        (std::bidirectional_iterator<Iterator> || !std::is_base_of_v<std::bidirectional_iterator_tag, iterator_category>) &&
        (std::random_access_iterator<Iterator> || !std::is_base_of_v<std::random_access_iterator_tag, iterator_category>);

    template<std::input_iterator Iterator>
    using iter_cast_t = expu::iterator_downcast<Iterator, iterator_category>;
};

using darray_trivial_iterator_test_types =
    expu::cartesian_product_t<
        trivially_copyable_test_types,
        trivially_destructible_test_types,
        testing::Types<
            std::input_iterator_tag, 
            std::forward_iterator_tag, 
            std::bidirectional_iterator_tag,
            std::random_access_iterator_tag>>;

TYPED_TEST_SUITE(darray_trivial_iterator_tests, darray_trivial_iterator_test_types);

//////////////////////////////////////DARRAY TRAITS TESTS///////////////////////////////////////////////////////////////////////////////


TEST(darray_tests, traits_test) 
{
    using darray_type = expu::darray<int>;

    ASSERT_EQ(sizeof(darray_type), 3 * sizeof(darray_type::pointer)) <<
        "expu::darray should be (when using an empty allocator) exactly three times ints pointer type in size.";
}


//////////////////////////////////////DARRAY ITERATOR CONSTRUCTION TESTS//////////////////////////////////////////////////////////////////////////


TYPED_TEST(darray_trivial_iterator_tests, construct_with_input_iterator)
{
    constexpr size_t test_size = 10000;

    using iter_type = typename TestFixture::template iter_cast_t<expu::seq_iter<int>>;
    const iter_type first(0), last(test_size);

    const checked_darray<TestFixture::value_type, std::allocator> arr(first, last);
    ASSERT_TRUE(is_darray_valid(arr));

    //Note: here the iterators are re-constructed because iterator_downcast may invalidates.
    ASSERT_TRUE(is_equal(arr, iter_type(0), iter_type(test_size)));

    if constexpr(std::forward_iterator<iter_type>)
        ASSERT_EQ(test_size, arr.capacity());
}


//////////////////////////////////////DARRAY SPECIAL CONSTRUCTION TESTS//////////////////////////////////////////////////////////////////////////


TYPED_TEST(darray_trivial_tests, copy_construct)
{
    using darray_type = checked_darray<TestFixture::value_type, std::allocator>;

    expu::seq_iter first(0), last(10000);

    const darray_type original(first, last);
    const darray_type copied(original);

    ASSERT_TRUE(is_darray_valid(copied));
    ASSERT_TRUE(is_darray_valid(original));

    ASSERT_TRUE(is_equal(copied, first, last));
    ASSERT_TRUE(std::ranges::equal(original, copied));
}

TYPED_TEST(darray_trivially_destructible_tests, move_construct_with_defaulted_allocator)
{
    using value_type   = expu::throw_on_type<TestFixture::value_type, expu::always_throw>;
    using darray_type = checked_darray<value_type, std::allocator>;

    const expu::seq_iter first(0), last(10000);

    EXPU_NO_THROW_ON(value_type, darray_type original(first, last));

    const darray_type moved(std::move(original));

    ASSERT_TRUE(is_darray_valid(original));
    ASSERT_TRUE(is_darray_valid(moved));
    ASSERT_TRUE(is_equal(moved, first, last));
}

TYPED_TEST(darray_trivial_tests, move_construct_with_comp_false_allocator)
{
    using alloc_type  = expu::test_allocator<TestFixture::value_type, expu::test_alloc_props::always_comp_false>;
    using darray_type = expu::darray<TestFixture::value_type, expu::checked_allocator<alloc_type>>;

    const expu::seq_iter first(0), last(10000);

    darray_type original(first, last);
    const darray_type moved(std::move(original), alloc_type());

    ASSERT_TRUE(is_darray_valid(original));
    ASSERT_TRUE(is_darray_valid(moved));
    ASSERT_TRUE(is_equal(moved, first, last));
}


//////////////////////////////////////DARRAY RESERVE TESTS///////////////////////////////////////////////////////////////////////////////


TYPED_TEST(darray_trivial_tests, reserve_requires_resize)
{
    using darray_type = checked_darray<TestFixture::value_type, std::allocator>;

    constexpr int test_size = 10000;
    const expu::seq_iter first(0), last(test_size);

    darray_type arr(first, last);

    ASSERT_EQ(arr.capacity(), test_size);

    constexpr int resize_size = test_size * 2;
    arr.reserve(resize_size);

    ASSERT_EQ(resize_size, arr.capacity());
    ASSERT_TRUE(is_equal(arr, first, last));
    ASSERT_TRUE(is_darray_valid(arr));
}

TYPED_TEST(darray_trivially_destructible_tests, reserve_with_enough_capacity)
{
    using value_type  = expu::throw_on_type<TestFixture::value_type, expu::always_throw>;
    using darray_type = checked_darray<value_type, std::allocator>;

    static_assert(!std::is_trivially_copyable_v<value_type>, 
        "Ensure no calls to mem_x functions.");

    constexpr int test_size = 10000;
    //Note: Using input iterator here to ensure darray emplaces back range elements.
    const expu::input_iterator_cast<expu::seq_iter<int>> first(0), last(test_size);

    EXPU_NO_THROW_ON(value_type, darray_type arr(first, last));  

    const auto old_capacity = static_cast<size_t>(arr.capacity());
    //Ensure new capacity is more than size but less than capacity.
    ASSERT_GT(old_capacity - 1, arr.size());

    //Note: Choosing to loop within test rather than define parameterised test to keep test log clean. 
    for (size_t new_capacity = 0; new_capacity < old_capacity; new_capacity += 200) {
        arr.reserve(new_capacity);

        EXPECT_TRUE(_verify_unchanged(arr, expu::seq_iter(0), expu::seq_iter(test_size), old_capacity))
            << "expu::darray::reserve failed with arugment: " << new_capacity;
    }
}

TYPED_TEST(darray_trivially_destructible_tests, reserve_strong_guarantee) {
    constexpr int test_size = 10000;

    //Throw after test_size/2 calls to any constructor.
    EXPU_GUARDED_THROW_ON_TYPE(value_type, TestFixture::value_type, expu::always_throw_after_x<test_size/2>);
    using darray_type = checked_darray<value_type, std::allocator>;

    static_assert(!std::is_trivially_copyable_v<value_type>,
        "Expecting type not to be trivially copyable for this test!");

    static_assert(std::is_move_constructible_v<value_type>,
        "Expecting type to be move constructible for this test!");

    EXPU_NO_THROW_ON(value_type, darray_type arr(expu::seq_iter(0), expu::seq_iter(test_size)));

    ASSERT_TRUE(provides_strong_guarantee(arr, &darray_type::reserve, arr.capacity() * 2));
}


//////////////////////////////////////DARRAY EMPLACE/PUSH_BACK TESTS///////////////////////////////////////////////////////////////////////////////


template<class ... Args, class ReturnType, class ClassType>
auto _disambiguate(ReturnType(ClassType::* func)(Args...)) { return func; }

template<class ... Args, class ReturnType>
auto _disambiguate(ReturnType(*func)(Args...)) { return func; }

template<class ArrayType, class Callable>
testing::AssertionResult _emplace_or_push_back_sg_tests_common(Callable&& test_callable) 
{
    using value_type = typename ArrayType::value_type;

    constexpr int test_size = 10000;
    EXPU_NO_THROW_ON(value_type, ArrayType arr(expu::seq_iter(0), expu::seq_iter(test_size)));

    constexpr int emplace_value = test_size * 2;
    value_type::callable_type::value = emplace_value;

    //Constructing int value ensure r-value is passed as argument.
    return provides_strong_guarantee(arr, std::forward<Callable>(test_callable), int{ emplace_value });
}

//Note: Not using darray_trivial_tests because expu::throw_on_type cannot be trivially_copyable
TYPED_TEST(darray_trivially_destructible_tests, emplace_back_strong_guarantee)
{
    EXPU_GUARDED_THROW_ON_TYPE(value_type, TestFixture::value_type, expu::throw_on_comp_equal<int&&>);
    using darray_type = checked_darray<value_type, std::allocator>;

    _emplace_or_push_back_sg_tests_common<darray_type>(&darray_type::template emplace_back<int>);

}

//Note: Not using darray_trivial_tests because expu::throw_on_type cannot be trivially_copyable
TYPED_TEST(darray_trivially_destructible_tests, push_back_strong_guarantee)
{
    using base_type = typename TestFixture::value_type;

    EXPU_GUARDED_THROW_ON_TYPE(value_type, base_type, EXPU_MACRO_ARG(expu::throw_on_comp_equal<int, base_type&&>));
    using darray_type = checked_darray<value_type, std::allocator>;

    _emplace_or_push_back_sg_tests_common<darray_type>(_disambiguate<value_type&&>(&darray_type::push_back));
    
}

template<class ArrayType, class Callable>
testing::AssertionResult _emplace_tests_common(int test_size, int step, size_t capacity, Callable&& pre_check) 
{
    constexpr int emplace_value = -10;

    if(test_size % step != 0) 
        return testing::AssertionFailure() << "Test does not emplace at back!";

    expu::seq_iter<int> first(0), last(test_size);
    expu::seq_iter<int> emplace_iter(-10);

    for (expu::seq_iter<int> at = first; at != last; at += step) {
        ArrayType arr(first, last);
        arr.reserve(capacity);

        auto check_result = std::invoke(std::forward<Callable>(pre_check), arr);
        if (!check_result)
            return check_result;

        const typename ArrayType::iterator constructed_at = arr.emplace(arr.cbegin() + (at - first), emplace_value);
        if (constructed_at != (arr.begin() + *at))
            return testing::AssertionFailure() << "Unexpected emplace return iterator!";

        auto check_iter = expu::concatenate(first, at, emplace_iter, emplace_iter + 1, at);

        auto is_valid_result = is_darray_valid(arr);
        if(!is_valid_result)
            return is_valid_result << ". Failed at: " << *at;

        return is_equal(arr, check_iter, last) << ". Failed at: " << *at;
    }

    return testing::AssertionSuccess();
}

template<class ArrayType, bool enough_capacity>
struct _emplace_pre_check
{
    constexpr auto operator()(const ArrayType& arr)
    {
        if ((arr.capacity() < arr.size() + 1) == enough_capacity)
            return testing::AssertionFailure()
            << "Test requires expu::darray to "
            << (enough_capacity ? "" : "NOT ")
            << "have enough capacity to insert one!";
        else
            return testing::AssertionSuccess();
    }
};

TYPED_TEST(darray_trivially_destructible_tests, emplace_with_capacity) 
{
    constexpr int test_size = 10000;
    constexpr int step      = test_size/10;
    
    using darray_type = checked_darray<TestFixture::value_type, std::allocator>;
    ASSERT_TRUE(_emplace_tests_common<darray_type>(test_size, step, test_size * 2, _emplace_pre_check<darray_type, true>{}));
}

TYPED_TEST(darray_trivially_destructible_tests, emplace_requires_resize) 
{
    constexpr int test_size = 10000;
    constexpr int step      = test_size/10;

    using darray_type = checked_darray<TestFixture::value_type, std::allocator>;
    ASSERT_TRUE(_emplace_tests_common<darray_type>(test_size, step, test_size, _emplace_pre_check<darray_type, false>{}));
}

TYPED_TEST(darray_trivially_copyable_tests, emplace_back) 
{
    checked_darray<TestFixture::value_type, std::allocator> arr;

    constexpr int test_size = 10000;
    expu::seq_iter first(0), last(test_size);

    for (auto it = first; it != last; ++it)
        arr.emplace_back(*it);

    EXPECT_TRUE(is_darray_valid(arr));
    EXPECT_TRUE(is_equal(arr, first, last));
}

TYPED_TEST(darray_trivially_destructible_tests, emplace_with_enough_capacity_strong_guarantee) 
{
    EXPU_GUARDED_THROW_ON_TYPE(value_type, TestFixture::value_type, expu::throw_on_comp_equal<int&&>);
    using darray_type = checked_darray<value_type, std::allocator>;

    static_assert(std::is_nothrow_move_assignable_v<value_type> &&
                 (std::is_nothrow_move_constructible_v<value_type> ||
                  std::is_nothrow_copy_constructible_v<value_type>),
        "For emplace to provide strong guarantee (always), The type must not throw on either move or copy constructor and be nothrow move assignable!");

    constexpr int test_size     = 10000;
    constexpr int step          = test_size / 10;
    constexpr int emplace_value = -10;

    value_type::callable_type::value = emplace_value;

    darray_type arr(expu::seq_iter(0), expu::seq_iter(test_size));
    arr.reserve(test_size * 2);

    for (size_t at = 0; at < test_size; at += step) {
        ASSERT_TRUE(provides_strong_guarantee(arr, &darray_type::template emplace<int&&>, arr.begin() + at, int{ emplace_value })) <<
            "Failed trying to emplace at index: " << at;
    }
}

//////////////////////////////////////DARRAY ASSIGN TESTS///////////////////////////////////////////////////////////////////////////////

template<class IteratorCategory = std::forward_iterator_tag, class ArrayType, std::input_iterator Iterator>
testing::AssertionResult _darray_assign_tests_common(ArrayType& arr, Iterator first, Iterator last) 
{
    using down_iter_type = expu::iterator_downcast<Iterator, IteratorCategory>;

    arr.assign(down_iter_type(first), down_iter_type(last));

    auto failure_message = std::stringstream()
        << "Failed trying to assign range: "
        << "[" << *first << ", " << *last << ")";

    auto darray_valid_result = is_darray_valid(arr);
    if (!darray_valid_result)
        return darray_valid_result << failure_message.str();

    auto is_equal_result = is_equal(arr, first, last);
    if(!is_equal_result)
        return is_equal_result << failure_message.str();

    return testing::AssertionSuccess();
}

TYPED_TEST(darray_trivial_tests, assign_with_forward_iterator_with_enough_capacity)
{
    constexpr int test_size       = 10000;
    constexpr int max_assign_size = test_size * 2;
    constexpr int step            = max_assign_size / 10;

    static_assert(max_assign_size % step == 0, "Insertion at end never occurs!");

    for (int assign_size = 0; assign_size <= max_assign_size; assign_size += step)
    {
        checked_darray<TestFixture::value_type, std::allocator> arr(expu::seq_iter(0), expu::seq_iter(test_size));
        arr.reserve(max_assign_size);

        ASSERT_LE(assign_size, arr.capacity())
            << "For this test, array must have enough capacity to accept assign range.";

        ASSERT_TRUE(_darray_assign_tests_common(arr, expu::seq_iter(-assign_size), expu::seq_iter(0)));

        //Optimisation (kinda): forward iterator allows for calculation of assign range size.
        ASSERT_EQ(arr.capacity(), max_assign_size)
            << "Expected no change in container capacity!";
    }
}

TYPED_TEST(darray_trivial_tests, assign_with_forward_iterator_requires_resize)
{
    constexpr int test_size = 10000;
    constexpr int assign_size = test_size * 2;

    checked_darray<TestFixture::value_type, std::allocator> arr(expu::seq_iter(0), expu::seq_iter(test_size));
    ASSERT_LT(arr.capacity(), assign_size)
        << "For this test, array must NOT have enough capacity to accept assign range.";

    ASSERT_TRUE(_darray_assign_tests_common(arr, expu::seq_iter(-assign_size), expu::seq_iter(0)));

    //Optimisation (kinda): forward iterator allows for calculation of assign range size.
    ASSERT_EQ(arr.capacity(), assign_size)
        << "Expected capacity to be equal to that of assigned range!";
}


//////////////////////////////////////DARRAY INSERTION TESTS///////////////////////////////////////////////////////////////////////////////


template<
    class ArrayType, 
    class IteratorCategory = std::forward_iterator_tag, 
    class Callable>
void _insert_iterator_test_common(int initial_size, int insert_size, size_t capacity, int number_of_tests, Callable pre_check)
{
    using iter_type = expu::seq_iter<int>;

    //If need for input iterator, downcast
    using insert_iter_t =
        std::conditional_t<
            std::is_same_v<std::input_iterator_tag, IteratorCategory>,
            expu::iterator_downcast<iter_type, IteratorCategory>,
            iter_type> ;

    ASSERT_TRUE((initial_size % number_of_tests) == 0)
        << "number_of_tests not a multiple of initial_size! Insertion at end never occurs!";

    int step = initial_size / number_of_tests;

    iter_type init_first(0), init_last(initial_size);
    iter_type insert_first(-insert_size), insert_last(0);

    //Note: range goes to init_last + step so that init_last is included.
    for (auto insert_at = init_first; insert_at != (init_last + step); insert_at += step)
    {
        size_t at = static_cast<size_t>(insert_at - init_first);

        ArrayType arr(init_first, init_last);
        arr.reserve(capacity);

        auto failure_message = std::stringstream() 
            << "Failed trying to insert at index:" << at;

        ASSERT_TRUE(pre_check(arr)) 
            << failure_message.str();

        arr.insert(arr.cbegin() + at, insert_first, insert_last);

        //Expected values of arr post insertion
        auto check_iter = expu::concatenate(init_first, insert_at, insert_first, insert_last, insert_at);

        ASSERT_TRUE(is_darray_valid(arr)) 
            << failure_message.str();
        ASSERT_TRUE(is_equal(arr, check_iter, init_last)) 
            << failure_message.str();
    }
}

template<class ArrayType, bool more_than_capacity, size_t insert_size>
struct _insert_pre_check 
{
    constexpr auto operator()(const ArrayType& arr)
    {
        if ((arr.capacity() - arr.size() < insert_size) != more_than_capacity)
            return testing::AssertionFailure() 
                << "Insertion size must be " 
                << (more_than_capacity ? "greater than" : " less or equal to")
                << " unused capacity for this test!";
        else
            return testing::AssertionSuccess();
    }
};

TYPED_TEST(darray_trivial_iterator_tests, insert_requires_resize)
{
    using array_type = checked_darray<TestFixture::value_type, std::allocator>;

    constexpr int test_size = 10000;
    constexpr int insert_size = 2500;

    _insert_iterator_test_common<array_type, TestFixture::iterator_category>(
        test_size, insert_size, test_size, 10, _insert_pre_check<array_type, true, insert_size>{});
}

TYPED_TEST(darray_trivial_iterator_tests, insert_with_enough_capacity)
{
    using array_type = checked_darray<TestFixture::value_type, std::allocator>;

    constexpr int test_size = 10000;
    constexpr int insert_size = 2500;

    _insert_iterator_test_common<array_type, TestFixture::iterator_category>(
        test_size, insert_size, test_size * 2, 10, _insert_pre_check<array_type, false, insert_size>{});
}
