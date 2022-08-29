#include "gtest/gtest.h"

#include "expu/meta/meta_utils.hpp"
#include "expu/meta/typelist_set_operations.hpp"
#include "expu/meta/function_utils.hpp"

#include <type_traits>
#include <tuple>


//////////////////////////////////////TYPELIST TEST FIXTURES//////////////////////////////////////////////////////////////////////////


struct test_with_gtest_types {
    template<class ... Types>
    using type = ::testing::Types<Types...>;
};

struct test_with_tuple {
    template<class ... Types>
    using type = std::tuple<Types...>;
};

template<class Type>
struct typelist_tests: testing::Test {
    template<class ... Types>
    using type = Type::template type<Types...>;
};

using typelist_types = testing::Types<test_with_gtest_types, test_with_tuple>;
TYPED_TEST_SUITE(typelist_tests, typelist_types);


//////////////////////////////////////TYPELIST COMMON CHECKS//////////////////////////////////////////////////////////////////////////


template<class Expected, class Actual>
testing::AssertionResult same_as() 
{
    if (!std::is_same_v<Expected, Actual>) {
        return testing::AssertionFailure()
            << "Expected: " << typeid(Expected).name()
            << ", Actual: " << typeid(Actual).name();
    }
    else
        return testing::AssertionSuccess();
}

template<expu::template_of<std::tuple> ExpectedTuple, class TypeList>
testing::AssertionResult have_same_elements() 
{
    constexpr size_t expected_size = std::tuple_size_v<ExpectedTuple>;
    constexpr size_t actual_size   = expu::typelist_size_v<TypeList>;
    if constexpr(expected_size != actual_size)
        return testing::AssertionFailure() 
            << "TypeList size does not match with expected"
            << "! Expected: " << expected_size
            << ", Actual: " << actual_size;

    testing::AssertionResult result = testing::AssertionSuccess();

    expu::indexed_unroll_n<std::tuple_size_v<ExpectedTuple>>([&]<size_t index>(std::integral_constant<size_t, index>) {
        using expected = std::tuple_element_t<index, ExpectedTuple>;
        using actual   = expu::typelist_element_t<index, TypeList>;

        auto same_as_succeeded = same_as<expected, actual>();
        if (result && !same_as_succeeded)
            result = same_as_succeeded << "Failed to get correct element at index: " << index;
    });

    return result;
}


//////////////////////////////////////TYPELIST TESTS//////////////////////////////////////////////////////////////////////////


TYPED_TEST(typelist_tests, _union) 
{
    using set1 = TestFixture::template type<int, float, bool, double, double, int>;
    using set2 = TestFixture::template type<char, short, unsigned char>;

    using expected = std::tuple<int, float, bool, double, double, int, char, short, unsigned char>;
    using actual   = expu::union_t<set1, set2>;
    
    ASSERT_TRUE((have_same_elements<expected, actual>()));
    ASSERT_TRUE((std::is_same_v<expu::typelist_cast_t<TestFixture::type, expected>, actual>))
        << "concat did not provide typelist of expected type!";
}

TYPED_TEST(typelist_tests, typelist_element)
{
    using set = TestFixture::template type<int, float, bool, double, double, int, char, short, unsigned char>;
    using expected            = std::tuple<int, float, bool, double, double, int, char, short, unsigned char>;
    
    ASSERT_TRUE((have_same_elements<expected, set>()));
}

template<size_t Index, class IntegerSequence>
constexpr size_t integer_sequence_element_v;

template<size_t Index, class IntegralType, IntegralType ... Indicies>
constexpr size_t integer_sequence_element_v<Index, std::integer_sequence<IntegralType, Indicies...>> = std::array{Indicies...}[Index];

TYPED_TEST(typelist_tests, subset)
{
    using list = TestFixture::template type<int, float, bool, double, double, int, int*, float, char>;
    using seq1 = std::index_sequence<0, 3, 4, 7, 2, 8>;

    using list_subset = expu::subset_t<seq1, list>;

    ASSERT_EQ(seq1::size(), expu::typelist_size_v<list_subset>)
        << "Unexpected subset size!";

    testing::AssertionResult result = testing::AssertionSuccess();
    expu::indexed_unroll_n<seq1::size()>([&]<size_t Index>(std::integral_constant<size_t, Index>) {
        using expected = expu::typelist_element_t<integer_sequence_element_v<Index, seq1>, list>;
        using actual   = expu::typelist_element_t<Index, list_subset>;

        auto same_as_succeded = same_as<expected, actual>();
        if (result && !same_as_succeded)
            result = same_as_succeded << "Failed at index: " << Index;
    });

    ASSERT_TRUE(result);
}

TYPED_TEST(typelist_tests, from_typelists) 
{
    using seq = std::index_sequence<3, 1, 4, 2, 0>;

    using actual = expu::from_typelists_t<seq,
        TestFixture::template type<int           , bool           , double, float, char*, const char* >,
        TestFixture::template type<bool          , unsigned char  , int*                              >,
        TestFixture::template type<long double   , unsigned int   , char         , bool , size_t      >,
        TestFixture::template type<int           , float*, double*, bool                              >,
        TestFixture::template type<long long int>>;

    using expected = TestFixture::template type<float, unsigned char, size_t, double*, long long int>;

    ASSERT_TRUE((same_as<expected, actual>));
}

TYPED_TEST(typelist_tests, has_type)
{
    using list = TestFixture::template type<int, float, bool, double, double, int, int*, float, char>;

    testing::AssertionResult true_positive_result = testing::AssertionSuccess();
    expu::indexed_unroll_n<expu::typelist_size_v<list>>([&]<size_t Index>(std::integral_constant<size_t, Index>) {
        using check_type = expu::typelist_element_t<Index, list>;

        if constexpr(!expu::has_type_v<list, check_type >) if(true_positive_result)
            true_positive_result = testing::AssertionFailure()
                << "List contains " << typeid(check_type).name() 
                << ", yet expu::has_type_v was false!";
    });

    ASSERT_TRUE(true_positive_result);
    ASSERT_FALSE((expu::has_type_v<list, unsigned char>))
        << "List does not contain unsigned char, yet expu::has_type_v was true!";
}

TYPED_TEST(typelist_tests, unique)
{
    using list = TestFixture::template type<int, float, bool, double, double, int, int*, float, char>;
    using unique_list = expu::unique_typelist_t<list>;

    testing::AssertionResult result = testing::AssertionSuccess();
    expu::indexed_unroll_n<expu::typelist_size_v<list>>([&]<size_t Index>(std::integral_constant<size_t, Index>) {
        using check_type = expu::typelist_element_t<Index, list>;

        if constexpr (!expu::has_type_v<unique_list, check_type >) if (result)
            result = testing::AssertionFailure()
                << "List contains " << typeid(check_type).name()
                << ", yet expu::has_type_v was false!";
    });

    ASSERT_TRUE(result);
}
