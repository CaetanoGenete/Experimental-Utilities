#include "gtest/gtest.h"

#include "smm/debug.hpp"
#include "smm/containers/darray.hpp"
#include "smm/iter_utils.hpp"

#include "test_utils.hpp"
#include "meta_sort.hpp"

#include "wrappers/disabled_wrapper.hpp"
#include "wrappers/counted_wrapper.hpp"
#include "wrappers/counted_allocator.hpp"

namespace smm_tests {

    template<class IntType, class DiffType = std::make_signed_t<IntType>>
    struct intit {
    public:
        //Note: Does not satisfy random_access_iterator concept because of indexing operator.
        //However, is functionally identical to a random access iterator.
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = IntType;
        using reference         = IntType&;
        using pointer           = IntType*;
        using difference_type   = DiffType;

    public:

        constexpr intit() noexcept : _curr() {}

        constexpr intit(IntType value)
            noexcept(std::is_nothrow_copy_constructible_v<IntType>) :
            _curr(value) {}

        constexpr intit(const intit&) = default;
        constexpr intit(intit&&) noexcept = default;
        constexpr ~intit() noexcept = default;

    public:

        constexpr intit& operator=(const intit&) = default;
        constexpr intit& operator=(intit&&) noexcept = default;

    public:

        [[nodiscard]] constexpr reference operator*() const noexcept {
            return _curr;
        }

        [[nodiscard]] constexpr pointer operator->() const noexcept {
            return &_curr;
        }

        [[nodiscard]] constexpr value_type operator[](difference_type n) const noexcept {
            return static_cast<value_type>(_curr + n);
        }

    public:

        constexpr intit& operator++() noexcept {
            ++_curr;
            return *this;
        }

        constexpr intit operator++(int) noexcept {
            intit copy = _curr;
            ++_curr;
            return copy;
        }

        constexpr intit& operator--() noexcept {
            --_curr;
            return *this;
        }

        constexpr intit operator--(int) noexcept {
            intit copy = _curr;
            --_curr;
            return copy;
        }

    public:

        constexpr intit& operator+=(difference_type n) noexcept {
            _curr += n;
            return *this;
        }

        constexpr intit& operator-=(difference_type n) noexcept {
            _curr -= n;
            return *this;
        }

    public:

        constexpr void swap(intit& other) 
            noexcept(std::is_nothrow_swappable_v<IntType>) 
        {
            using std::swap;
            swap(other._curr, _curr);
        }

    private:
        mutable value_type _curr;
    };

    template<class IntType, class DiffType>
    [[nodiscard]] constexpr auto operator<=>(const intit<IntType, DiffType>& lhs, const intit<IntType, DiffType>& rhs) noexcept 
    {
        return *lhs <=> *rhs;
    }

    template<class IntType, class DiffType>
    [[nodiscard]] constexpr bool operator==(const intit<IntType, DiffType>& lhs, const intit<IntType, DiffType>& rhs) noexcept 
    {
        return *lhs == *rhs;
    }

    template<class IntType, class DiffType>
    constexpr void swap(intit<IntType, DiffType>& lhs, intit<IntType, DiffType>& rhs) 
        noexcept(noexcept(lhs.swap(rhs))) 
    {
        lhs.swap(rhs);
    }

    template<class IntType, class DiffType>
    constexpr intit<IntType, DiffType> operator+(intit<IntType, DiffType> lhs, DiffType n) noexcept 
    {
        lhs += n;
        return lhs;
    }

    template<class IntType, class DiffType>
    constexpr intit<IntType, DiffType> operator+(DiffType n, intit<IntType, DiffType> rhs) noexcept 
    {
        rhs += n;
        return rhs;
    }

    template<class IntType, class DiffType>
    constexpr intit<IntType, DiffType> operator-(intit<IntType, DiffType> lhs, DiffType n) noexcept 
    {
        lhs -= n;
        return lhs;
    }

    template<class IntType, class DiffType>
    constexpr DiffType operator-(const intit<IntType, DiffType>& lhs, const intit<IntType, DiffType>& rhs) noexcept 
    {
        return *lhs - *rhs;
    }

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
            ASSERT_EQ(moved.get_allocator().deallocations(), 1);
        }

        ASSERT_EQ(TestFixture::counted_int::destructions_counts(), test_size);
    }

    TYPED_TEST(darray_tests, assign_case_1) {
        using alloc = counted_allocator<std::allocator<TestFixture::counted_int>>;

        constexpr size_t init_size = 50000;
        constexpr size_t assign_size = init_size * 2;
        {
            smm::darray<TestFixture::counted_int, alloc> test;
            test.reserve(init_size);

            ASSERT_TRUE(TestFixture::populate_darray(test, intit<int>(0), intit<int>(init_size)));
            
            smm::darray<TestFixture::counted_int, alloc> temp_cont;
            temp_cont.reserve(assign_size);

            TestFixture::populate_darray(temp_cont, intit<int>(0), intit<int>(assign_size));
            test.assign(temp_cont.begin(), temp_cont.end());

            ASSERT_EQ(test.get_allocator().allocations(), 2);
            ASSERT_EQ(test.get_allocator().deallocations(), 2);
            
            if constexpr (std::is_trivially_copyable_v<TestFixture::counted_int>) {
                SMM_TESTS_ASSERT_COUNTS(TestFixture::counted_int, 0, 0, 0, 0, init_size);
            }
            else
                SMM_TESTS_ASSERT_COUNTS(TestFixture::counted_int, assign_size, 0, 0, 0, init_size);

            ASSERT_TRUE(std::equal(test.begin(), test.end(), temp_cont.begin(), temp_cont.end()));


        }
    }

    TYPED_TEST(darray_tests, assign_case_2) {
        using alloc = counted_allocator<std::allocator<TestFixture::counted_int>>;

        constexpr size_t init_size = 100000;
        constexpr size_t assign_size = init_size / 2;
        {
            smm::darray<TestFixture::counted_int, alloc> test;
            test.reserve(init_size);

            ASSERT_TRUE(TestFixture::populate_darray(test, intit<int>(0), intit<int>(init_size)));

            smm::darray<TestFixture::counted_int, alloc> temp_cont;
            temp_cont.reserve(assign_size);

            TestFixture::populate_darray(temp_cont, intit<int>(0), intit<int>(assign_size));
            test.assign(temp_cont.begin(), temp_cont.end());

            ASSERT_EQ(test.get_allocator().allocations(), 1);
            ASSERT_EQ(test.get_allocator().deallocations(), 1);

            if constexpr (std::is_trivially_copyable_v<TestFixture::counted_int>) {
                SMM_TESTS_ASSERT_COUNTS(TestFixture::counted_int, 0, 0, assign_size, 0, 0);
            }
            else
                SMM_TESTS_ASSERT_COUNTS(TestFixture::counted_int, assign_size, 0, assign_size, 0, 0);

            ASSERT_TRUE(std::equal(test.begin(), test.end(), temp_cont.begin(), temp_cont.end()));


        }
    }
}
