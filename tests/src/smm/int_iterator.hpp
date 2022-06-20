#ifndef INT_ITERATOR_HPP_INCLUDED
#define INT_ITERATOR_HPP_INCLUDED

#include <type_traits>
#include <iterator>

namespace smm_tests {
    template<class IntType, class DiffType = std::make_signed_t<IntType>>
    struct intit {
    public:
        //Note: Does not satisfy random_access_iterator concept because of indexing operator.
        //However, is functionally identical to a random access iterator.
        using iterator_category = std::random_access_iterator_tag;
        using value_type = IntType;
        using reference = IntType&;
        using pointer = IntType*;
        using difference_type = DiffType;

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
}

#endif // !INT_ITERATOR_HPP_INCLUDED