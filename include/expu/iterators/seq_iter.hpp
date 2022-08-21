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

        [[nodiscard]] constexpr value_type operator[](const difference_type n) const noexcept {
            return static_cast<value_type>(_curr + n);
        }

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

    template<template_of<seq_iter> SeqIter>
    constexpr void swap(SeqIter& lhs, SeqIter& rhs)
        noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }

    template<template_of<seq_iter> SeqIter>
    constexpr SeqIter operator+(SeqIter lhs, const typename SeqIter::difference_type n) noexcept
    {
        lhs += n;
        return lhs;
    }

    template<template_of<seq_iter> SeqIter>
    constexpr SeqIter operator+(const typename SeqIter::difference_type n, SeqIter rhs) noexcept
    {
        rhs += n;
        return rhs;
    }

    template<template_of<seq_iter> SeqIter>
    constexpr SeqIter operator-(SeqIter lhs, const typename SeqIter::difference_type n) noexcept
    {
        lhs -= n;
        return lhs;
    }

    template<template_of<seq_iter> SeqIter>
    constexpr auto operator-(const SeqIter& lhs, const SeqIter& rhs) noexcept
    {
        return static_cast<SeqIter::difference_type>(*lhs - *rhs);
    }

}

#endif // !EXPU_SEQ_ITER_HPP_INCLUDED