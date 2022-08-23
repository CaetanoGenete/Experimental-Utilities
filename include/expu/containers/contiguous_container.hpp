#ifndef CONTIGUOUS_CONTAINER_HPP_INCLUDED
#define CONTIGUOUS_CONTAINER_HPP_INCLUDED

#include <iterator>
#include <type_traits>

#include "expu/debug.hpp"
#include "expu/meta/meta_utils.hpp"

#if EXPU_ITERATOR_DEBUG_LEVEL > 0
#define EXPU_L1_ITER_VERIFY(condition, message) \
    EXPU_VERIFY(condition, message)
#else
#define EXPU_L1_ITER_VERIFY(condition, message)
#endif //EXPU_ITERATOR_DEBUG_LEVEL > 0

namespace expu {

    template<class DataType>
    concept _ctg_range_pair =
        std::contiguous_iterator<typename DataType::pointer>                              &&
        std::contiguous_iterator<typename DataType::const_pointer>                        &&
        std::convertible_to<typename DataType::pointer, typename DataType::const_pointer> &&
        std::convertible_to<decltype(DataType::first),  typename DataType::const_pointer> &&
        std::convertible_to<decltype(DataType::last),   typename DataType::const_pointer>;

    template<_ctg_range_pair CtgRangePair>
    class ctg_const_iterator
    {
    public:
        using iterator_concept  = std::contiguous_iterator_tag;
        using iterator_category = std::random_access_iterator_tag;
        using pointer           = typename CtgRangePair::const_pointer;
        using value_type        = std::iter_value_t<pointer>;
        using reference         = std::iter_reference_t<pointer>;
        using difference_type   = std::iter_difference_t<pointer>;

    private:
        using _ctg_range_ptr_t = const CtgRangePair*;
        using _member_ptr_t    = typename CtgRangePair::pointer;

    public:
        constexpr ctg_const_iterator()                          = default;
        constexpr ctg_const_iterator(const ctg_const_iterator&) = default;
        constexpr ctg_const_iterator(ctg_const_iterator&&)      = default;
        constexpr ~ctg_const_iterator()                         = default;

        constexpr ctg_const_iterator(_member_ptr_t ptr, _ctg_range_ptr_t data_type)
            noexcept(std::is_nothrow_copy_constructible_v<_member_ptr_t>) :
            _ptr(ptr)
#if EXPU_ITERATOR_DEBUG_LEVEL > 0
            , _data_ptr(data_type)
#endif
        //Solves 'unused' warning.
        { (void)data_type; }

    public:
        constexpr ctg_const_iterator& operator=(const ctg_const_iterator&) = default;
        constexpr ctg_const_iterator& operator=(ctg_const_iterator&&)      = default;

    protected:
        constexpr void _verify_dereferencable() const noexcept
        {
            EXPU_L1_ITER_VERIFY(_ptr, "Container is empty, invalid derefence.");
        }

    public:
        [[nodiscard]] constexpr reference operator*() const noexcept
        {
            _verify_dereferencable();
            return *_ptr;
        }

        [[nodiscard]] constexpr pointer operator->() const noexcept
        {
            _verify_dereferencable();
            //let c++ deal with the conversion here
            return _ptr;
        }

        [[nodiscard]] constexpr reference operator[](const difference_type n) const noexcept
        {
            return *(*this + n);
        }

    public:
        constexpr void swap(ctg_const_iterator& other)
            noexcept(std::is_nothrow_swappable_v<pointer>)
        {
            std::swap(_ptr, other._ptr);
        }

    public:
        constexpr ctg_const_iterator& operator++() noexcept
        {
            EXPU_L1_ITER_VERIFY(_ptr != _data_ptr->last, "Iterator is at the end of the container.");

            ++_ptr;
            return *this;
        }

        constexpr ctg_const_iterator& operator--() noexcept
        {
            EXPU_L1_ITER_VERIFY(_ptr != _data_ptr->first, "Iterator is at the start of the container.");

            --_ptr;
            return *this;
        }

        constexpr ctg_const_iterator& operator+=(const difference_type n) noexcept
        {
            EXPU_L1_ITER_VERIFY(_data_ptr->last - _ptr >= n, "Iterator increment too large, would be pushed past last element.");
            EXPU_L1_ITER_VERIFY(_data_ptr->first - _ptr <= n, "Iterator decrement too large, would be pushed past first element.");

            _ptr += n;
            return *this;
        }

        constexpr ctg_const_iterator& operator-=(const difference_type n) noexcept
        {
            //Todo: Verify performance penalty
            return operator+=(-n);
        }

    private:
        constexpr void _verify_cont_compat(const ctg_const_iterator& rhs) const noexcept
        {
            EXPU_L1_ITER_VERIFY(_data_ptr == rhs._data_ptr, "Comparing iterators from different containers.");
            //Solves 'unused' warning.
            (void)rhs;
        }

    public:
        [[nodiscard]] friend constexpr difference_type operator-(const ctg_const_iterator& lhs, const ctg_const_iterator& rhs)
            noexcept
        {
            lhs._verify_cont_compat(rhs);
            return static_cast<difference_type>(lhs._ptr - rhs._ptr);
        }

        [[nodiscard]] friend constexpr auto operator<=>(const ctg_const_iterator& lhs, const ctg_const_iterator& rhs)
            noexcept
        {
            lhs._verify_cont_compat(rhs);
            return lhs._ptr <=> rhs._ptr;
        }

        [[nodiscard]] friend constexpr auto operator==(const ctg_const_iterator& lhs, const ctg_const_iterator& rhs)
            noexcept
        {
            lhs._verify_cont_compat(rhs);
            return lhs._ptr == rhs._ptr;
        }

    public:
        [[nodiscard]] constexpr _member_ptr_t _unwrapped() const noexcept
        {
            return _ptr;
        }

    protected:
        _member_ptr_t _ptr;
#if EXPU_ITERATOR_DEBUG_LEVEL > 0
        _ctg_range_ptr_t _data_ptr;
#endif 
    };

    template<_ctg_range_pair CtgRangePair>
    constexpr void swap(ctg_const_iterator<CtgRangePair>& lhs, ctg_const_iterator<CtgRangePair>& rhs)
        noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }

    template<_ctg_range_pair CtgRangePair>
    class ctg_iterator : public ctg_const_iterator<CtgRangePair>
    {
    public:
        using iterator_concept  = std::contiguous_iterator_tag;
        using iterator_category = std::random_access_iterator_tag;
        using pointer           = typename CtgRangePair::pointer;
        using value_type        = std::iter_value_t<pointer>;
        using reference         = std::iter_reference_t<pointer>;
        using difference_type   = std::iter_difference_t<pointer>;

    private:
        using _base_t = ctg_const_iterator<CtgRangePair>;

    public:
        using _base_t::_base_t;

    public:
        [[nodiscard]] constexpr reference operator*() const noexcept
        {
            _base_t::_verify_dereferencable();

            return *_base_t::_ptr;
        }

        [[nodiscard]] constexpr pointer operator->() const noexcept
        {
            _base_t::_verify_dereferencable();

            return _base_t::_ptr;
        }

        [[nodiscard]] constexpr reference operator[](const difference_type n) const noexcept
        {
            //Note: Safe, _ptr is non-const qualified
            return const_cast<reference>(_base_t::operator[](n));
        }

    public:
        constexpr ctg_iterator& operator++() noexcept
        {
            _base_t::operator++();
            return *this;
        }

        constexpr ctg_iterator& operator--() noexcept
        {
            _base_t::operator--();
            return *this;
        }

        constexpr ctg_iterator& operator+=(const difference_type n) noexcept
        {
            _base_t::operator+=(n);
            return *this;
        }

        constexpr ctg_iterator& operator-=(const difference_type n) noexcept
        {
            _base_t::operator-=(n);
            return *this;
        }
    };

    template<class Iter>
    concept _expu_ctg_iterator = template_of<Iter, ctg_const_iterator> || template_of<Iter, ctg_iterator>;

    template<_expu_ctg_iterator Iter>
    [[nodiscard]] constexpr Iter operator++(Iter& iter, int)
        noexcept(std::is_nothrow_copy_constructible_v<Iter> && noexcept(noexcept(++iter)))
    {
        const Iter copy(iter);
        ++iter;
        return copy;
    }

    template<_expu_ctg_iterator Iter>
    [[nodiscard]] constexpr Iter operator--(Iter& iter, int)
        noexcept(std::is_nothrow_copy_constructible_v<Iter> && noexcept(noexcept(--iter)))
    {
        const Iter copy(iter);
        --iter;
        return copy;
    }

    template<_expu_ctg_iterator Iter>
    [[nodiscard]] constexpr Iter operator+(Iter iter, std::iter_difference_t<Iter> n)
        noexcept(std::is_nothrow_copy_constructible_v<Iter> && noexcept(noexcept(iter += n)))
    {
        return iter += n;
    }

    template<_expu_ctg_iterator Iter>
    [[nodiscard]] constexpr Iter operator+(std::iter_difference_t<Iter> n, const Iter& iter)
        noexcept(noexcept(iter + n))
    {
        return iter + n;
    }

    template<_expu_ctg_iterator Iter>
    [[nodiscard]] constexpr Iter operator-(Iter iter, std::iter_difference_t<Iter> n)
        noexcept(std::is_nothrow_copy_constructible_v<Iter> && noexcept(noexcept(iter -= n)))
    {
        return iter -= n;
    }

}

#endif // !CONTIGUOUS_CONTAINER_HPP_INCLUDED