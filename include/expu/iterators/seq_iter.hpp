#ifndef EXPU_SEQ_ITER_HPP_INCLUDED
#define EXPU_SEQ_ITER_HPP_INCLUDED

#include <type_traits>
#include <iterator>

namespace expu {

    template<class IntType, class DiffType = std::make_signed_t<IntType>>
    struct seq_iter {
    public:
        //Note: Does not satisfy random_access_iterator concept because of indexing operator.
        //However, is functionally identical to a random access iterator.
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = IntType;
        using reference         = IntType&;
        using pointer           = IntType*;
        using difference_type   = DiffType;

    public:
        constexpr seq_iter() noexcept : _curr() {}

        constexpr seq_iter(IntType value)
            noexcept(std::is_nothrow_copy_constructible_v<IntType>) :
            _curr(value) {}

        constexpr seq_iter(const seq_iter&) = default;
        constexpr seq_iter(seq_iter&&) noexcept = default;
        constexpr ~seq_iter() noexcept = default;

    public:
        constexpr seq_iter& operator=(const seq_iter&) = default;
        constexpr seq_iter& operator=(seq_iter&&) noexcept = default;

    public:
        [[nodiscard]] constexpr reference operator*() const noexcept {
            return _curr;
        }

        [[nodiscard]] constexpr pointer operator->() const noexcept {
            return &_curr;
        }

#ifndef EXPU_MAKE_SEQ_ITER_RANDOM_ACCESS
        [[nodiscard]] constexpr value_type operator[](const difference_type n) const noexcept {
            return static_cast<value_type>(_curr + n);
        }
#else
        [[nodiscard]] constexpr reference operator[](const difference_type n) const noexcept {
            place_holder = return static_cast<value_type>(_curr + n);
            return place_holder;
        }
#endif // !EXPU_MAKE_SEQ_ITER_RANDOM_ACCESS



    public:
        constexpr seq_iter& operator++() noexcept {
            ++_curr;
            return *this;
        }

        [[nodiscard("Prefer pre-increment operator.")]] 
        constexpr seq_iter operator++(int) noexcept {
            const seq_iter copy = _curr;
            ++_curr;
            return copy;
        }

        constexpr seq_iter& operator--() noexcept {
            --_curr;
            return *this;
        }

        [[nodiscard("Prefer pre-increment operator.")]] 
        constexpr seq_iter operator--(int) noexcept {
            const seq_iter copy = _curr;
            --_curr;
            return copy;
        }

    public:
        constexpr seq_iter& operator+=(const difference_type n) noexcept {
            _curr += n;
            return *this;
        }

        constexpr seq_iter& operator-=(const difference_type n) noexcept {
            _curr -= n;
            return *this;
        }

    public:
        constexpr void swap(seq_iter& other)
            noexcept(std::is_nothrow_swappable_v<IntType>)
        {
            using std::swap;
            swap(other._curr, _curr);
        }

    private:
        mutable value_type _curr;
#ifdef EXPU_MAKE_SEQ_ITER_RANDOM_ACCESS
        value_type place_holder;
#endif // EXPU_MAKE_SEQ_ITER_RANDOM_ACCESS

    };

    template<class IntType, class DiffType>
    [[nodiscard]] constexpr auto operator<=>(const seq_iter<IntType, DiffType>& lhs, const seq_iter<IntType, DiffType>& rhs) noexcept
    {
        return *lhs <=> *rhs;
    }

    template<class IntType, class DiffType>
    [[nodiscard]] constexpr bool operator==(const seq_iter<IntType, DiffType>& lhs, const seq_iter<IntType, DiffType>& rhs) noexcept
    {
        return *lhs == *rhs;
    }

    template<class IntType, class DiffType>
    constexpr void swap(seq_iter<IntType, DiffType>& lhs, seq_iter<IntType, DiffType>& rhs)
        noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }

    template<class IntType, class DiffType>
    constexpr auto operator+(seq_iter<IntType, DiffType> lhs, const std::iter_difference_t<seq_iter<IntType, DiffType>> n) noexcept
    {
        lhs += n;
        return lhs;
    }

    template<class IntType, class DiffType>
    constexpr auto operator+(const std::iter_difference_t<seq_iter<IntType, DiffType>> n, seq_iter<IntType, DiffType> rhs) noexcept
    {
        rhs += n;
        return rhs;
    }

    template<class IntType, class DiffType>
    constexpr auto operator-(seq_iter<IntType, DiffType> lhs, const std::iter_difference_t<seq_iter<IntType, DiffType>> n) noexcept
    {
        lhs -= n;
        return lhs;
    }

    template<class IntType, class DiffType>
    constexpr auto operator-(const seq_iter<IntType, DiffType>& lhs, const seq_iter<IntType, DiffType>& rhs) noexcept
    {
        return static_cast<seq_iter<IntType, DiffType>::difference_type>(*lhs - *rhs);
    }

}

#endif // !EXPU_SEQ_ITER_HPP_INCLUDED