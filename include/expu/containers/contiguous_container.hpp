#ifndef CONTIGUOUS_CONTAINER_HPP_INCLUDED
#define CONTIGUOUS_CONTAINER_HPP_INCLUDED

#include <iterator>
#include <type_traits>

#include "expu/debug.hpp"

#include "expu/meta/meta_utils.hpp"

#include "expu/maths/basic_maths.hpp"

#if EXPU_ITERATOR_DEBUG_LEVEL > 0
#define EXPU_L1_ITER_VERIFY(condition, message) \
    EXPU_VERIFY(condition, message)
#else
#define EXPU_L1_ITER_VERIFY(condition, message)
#endif //EXPU_ITERATOR_DEBUG_LEVEL > 0

#define EXPU_ITERATOR_NON_MEMBER_FUNCTIONS(iterator_template)                                                                           \
                                                                                                                                        \
    template<class Type>                                                                                                                \
    [[nodiscard]] constexpr auto operator++(iterator_template<Type>& iter, int)                                                         \
        noexcept(std::is_nothrow_copy_constructible_v<iterator_template<Type>> && noexcept(noexcept(++iter)))                           \
    {                                                                                                                                   \
        const iterator_template<Type> copy(iter);                                                                                       \
        ++iter;                                                                                                                         \
        return copy;                                                                                                                    \
    }                                                                                                                                   \
                                                                                                                                        \
    template<class Type>                                                                                                                \
    [[nodiscard]] constexpr auto operator--(iterator_template<Type>& iter, int)                                                         \
        noexcept(std::is_nothrow_copy_constructible_v<iterator_template<Type>> && noexcept(noexcept(--iter)))                           \
    {                                                                                                                                   \
        const iterator_template<Type> copy(iter);                                                                                       \
        --iter;                                                                                                                         \
        return copy;                                                                                                                    \
    }                                                                                                                                   \
                                                                                                                                        \
    template<class Type>                                                                                                                \
    [[nodiscard]] constexpr auto operator+(iterator_template<Type> iter, std::iter_difference_t<iterator_template<Type>> n)             \
        noexcept(std::is_nothrow_copy_constructible_v<iterator_template<Type>> && noexcept(noexcept(iter += n)))                        \
    {                                                                                                                                   \
        return iter += n;                                                                                                               \
    }                                                                                                                                   \
                                                                                                                                        \
    template<class Type>                                                                                                                \
    [[nodiscard]] constexpr auto operator+(std::iter_difference_t<iterator_template<Type>> n, const iterator_template<Type>& iter)      \
        noexcept(noexcept(iter + n))                                                                                                    \
    {                                                                                                                                   \
        return iter + n;                                                                                                                \
    }                                                                                                                                   \
                                                                                                                                        \
    template<class Type>                                                                                                                \
    [[nodiscard]] constexpr auto operator-(iterator_template<Type> iter, std::iter_difference_t<iterator_template<Type>> n)             \
        noexcept(std::is_nothrow_copy_constructible_v<iterator_template<Type>> && noexcept(noexcept(iter -= n)))                        \
    {                                                                                                                                   \
        return iter -= n;                                                                                                               \
    }

namespace expu {

    template<class Type>
    concept _ctg_sized_range_pair =
        std::contiguous_iterator<typename Type::pointer>                          &&
        std::contiguous_iterator<typename Type::const_pointer>                    &&
        std::convertible_to<typename Type::pointer, typename Type::const_pointer> &&
        std::convertible_to<decltype(Type::first) , typename Type::const_pointer> &&
        std::unsigned_integral<decltype(Type::size)>;

    template<class Type>
    concept _ctg_iter_range_pair =
        std::contiguous_iterator<typename Type::pointer> &&
        std::contiguous_iterator<typename Type::const_pointer> &&
        std::convertible_to<typename Type::pointer, typename Type::const_pointer> &&
        std::convertible_to<decltype(Type::first),  typename Type::const_pointer> &&
        std::convertible_to<decltype(Type::last),   typename Type::const_pointer>;

    template<class Type>
    concept _ctg_range_pair = _ctg_sized_range_pair<Type> || _ctg_iter_range_pair<Type>;

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

        [[nodiscard]] friend constexpr bool operator==(const ctg_const_iterator& lhs, const ctg_const_iterator& rhs)
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

    template<class CtgRangePair>
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

    class _const_bool_index
    {
        template<_ctg_range_pair>
        friend struct _ctg_bool_const_iterator;

    public:
        constexpr _const_bool_index(void* const ptr, uint8_t sub_index) :
            _ptr(static_cast<uint8_t*>(ptr)), _mask(1 << sub_index)
        {
            EXPU_VERIFY_DEBUG(sub_index < 8, "Index is too large!");
        }

    public:
        constexpr operator bool() const noexcept
        {
            return static_cast<bool>(*_ptr & _mask);
        }
    protected:
        uint8_t* _ptr;
        uint8_t _mask;
    };

    class _bool_index : public _const_bool_index
    {

    public:
        using _const_bool_index::_const_bool_index;

    public:
        constexpr _bool_index& operator=(bool value) noexcept
        {
            //Todo: perhaps remove branch
            if (value)
                *_const_bool_index::_ptr |= _const_bool_index::_mask;
            else
                *_const_bool_index::_ptr &= ~_const_bool_index::_mask;

            return *this;
        }

    };


    template<_ctg_range_pair BoolRangePair>
    struct _ctg_bool_const_iterator
    {
    public:
        //using iterator_concept  = std::contiguous_iterator_tag;
        using iterator_category = std::random_access_iterator_tag;
        using pointer           = _const_bool_index*;
        using const_pointer     = const _const_bool_index*;
        using value_type        = bool;
        using reference         = _const_bool_index;
        using difference_type   = std::iter_difference_t<pointer>;

    private:
        using _ctg_range_ptr_type = const BoolRangePair*;

    public:
        constexpr _ctg_bool_const_iterator():
            _index(nullptr, 0) {}

        constexpr _ctg_bool_const_iterator(BoolRangePair::pointer ptr, const BoolRangePair* data) :
            _index(ptr, 0) { (void)data; }

        constexpr _ctg_bool_const_iterator(BoolRangePair::pointer ptr, uint8_t index, const BoolRangePair* data):
            _index(ptr, index) { (void)data; }

    public:
        [[nodiscard]] constexpr reference     operator*()  const noexcept { return _index; }
        [[nodiscard]] constexpr const_pointer operator->() const noexcept { return &_index; }

        [[nodiscard]] constexpr reference operator[](const difference_type n) const noexcept
        {
            return *(*this + n);
        }

    public:
        constexpr _ctg_bool_const_iterator& operator++() noexcept
        {
            if (_index._mask == 0x80) {
                ++_index._ptr;
                _index._mask = 1;
            }
            else
                _index._mask <<= 1;

            return *this;
        }

        constexpr _ctg_bool_const_iterator& operator--() noexcept
        {
            if (_index._mask == 0x1) {
                --_index._ptr;
                _index._mask = 0x80;
            }
            else
                _index._mask >>= 1;

            return *this;
        }

        constexpr _ctg_bool_const_iterator& operator+=(const difference_type n) noexcept
        {
            _index._ptr += (n >> 3);

            uint16_t mask_prod = static_cast<uint16_t>(_index._mask) << (n & 7);

            if (mask_prod < 0x100) {
                _index._mask = mask_prod & 0xFF;
                if (n < 0) --_index._ptr;
            }
            else {
                _index._mask = mask_prod >> 8;
                if (0 < n) ++_index._ptr;
            }

            return *this;
        }

        constexpr _ctg_bool_const_iterator& operator-=(const difference_type n) noexcept
        {
            return this->operator+=(-n);
        }

    private:
        /*
        constexpr void _verify_cont_compat(const ctg_const_iterator& rhs) const noexcept
        {
            EXPU_L1_ITER_VERIFY(_data_ptr == rhs._data_ptr, "Comparing iterators from different containers.");
            //Solves 'unused' warning.
            (void)rhs;
        }
        */

    private: // Circumvent non-transitivity of friendship
        constexpr auto _index_ptr()  const noexcept { return _index._ptr; }
        constexpr auto _index_mask() const noexcept { return _index._mask; }

    public:
        [[nodiscard]] friend constexpr difference_type operator-(const _ctg_bool_const_iterator& lhs, const _ctg_bool_const_iterator& rhs)
            noexcept
        {
            //lhs._verify_cont_compat(rhs);
            return static_cast<difference_type>(((lhs._index_ptr() - rhs._index_ptr()) << 3) + int_log2(lhs._index_mask()) - int_log2(rhs._index_mask()));
        }

        [[nodiscard]] friend constexpr auto operator<=>(const _ctg_bool_const_iterator& lhs, const _ctg_bool_const_iterator& rhs)
            noexcept
        {
            //lhs._verify_cont_compat(rhs);
            if (lhs._index_ptr() == rhs._index_ptr())
                return lhs._index_mask() <=> rhs._index_mask();
            else
                return lhs._index_ptr() <=> rhs._index_ptr();

        }

        [[nodiscard]] friend constexpr bool operator==(const _ctg_bool_const_iterator& lhs, const _ctg_bool_const_iterator& rhs)
            noexcept
        {
            //lhs._verify_cont_compat(rhs);
            return lhs._index_ptr() == rhs._index_ptr() && lhs._index_mask() == rhs._index_mask();
        }

    protected:
        _bool_index _index;
    };

    template<_ctg_range_pair CtgRangePair>
    class _ctg_bool_iterator : public _ctg_bool_const_iterator<CtgRangePair>
    {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using pointer           = _bool_index*;
        using value_type        = bool;
        using reference         = _bool_index;
        using difference_type   = std::iter_difference_t<pointer>;

    private:
        using _base_t = _ctg_bool_const_iterator<CtgRangePair>;

    public:
        using _base_t::_base_t;

    public:
        [[nodiscard]] constexpr reference operator*() const noexcept
        {
            //_base_t::_verify_dereferencable();
            return _base_t::_index;
        }

        [[nodiscard]] constexpr pointer operator->() const noexcept
        {
            //_base_t::_verify_dereferencable();
            return &_base_t::_index;
        }

        [[nodiscard]] constexpr reference operator[](const difference_type n) const noexcept
        {
            return *(*this + n);
        }

    public:
        constexpr _ctg_bool_iterator& operator++() noexcept
        {
            _base_t::operator++();
            return *this;
        }

        constexpr _ctg_bool_iterator& operator--() noexcept
        {
            _base_t::operator--();
            return *this;
        }

        constexpr _ctg_bool_iterator& operator+=(const difference_type n) noexcept
        {
            _base_t::operator+=(n);
            return *this;
        }

        constexpr _ctg_bool_iterator& operator-=(const difference_type n) noexcept
        {
            _base_t::operator-=(n);
            return *this;
        }
    };

    EXPU_ITERATOR_NON_MEMBER_FUNCTIONS(ctg_const_iterator);
    EXPU_ITERATOR_NON_MEMBER_FUNCTIONS(ctg_iterator);
    EXPU_ITERATOR_NON_MEMBER_FUNCTIONS(_ctg_bool_const_iterator);
    EXPU_ITERATOR_NON_MEMBER_FUNCTIONS(_ctg_bool_iterator);

}

#undef EXPU_ITERATOR_NON_MEMBER_FUNCTIONS

#endif // !CONTIGUOUS_CONTAINER_HPP_INCLUDED