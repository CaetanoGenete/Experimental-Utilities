#ifndef EXPU_FIXED_ARRAY_HPP_INCLUDED
#define EXPU_FIXED_ARRAY_HPP_INCLUDED

#include <memory>
#include <stdexcept>
#include <iterator>
#include <type_traits>

#include "expu/containers/contiguous_container.hpp"

#include "expu/maths/basic_maths.hpp"

#include "expu/debug.hpp"
#include "expu/mem_utils.hpp"

namespace expu
{
    template<class Pointer, class ConstPointer>
    struct _fixed_array_data
    {
    public:
        using pointer = Pointer;
        using const_pointer = ConstPointer;

    public:
        _fixed_array_data() = default;

        _fixed_array_data(_fixed_array_data&& other) noexcept :
            first(std::exchange(other.first, nullptr)),
            last (std::exchange(other.last, nullptr)) {}

        constexpr void steal(_fixed_array_data&& other) noexcept
        {
            first = std::exchange(other.first, nullptr);
            last  = std::exchange(other.last, nullptr);
        }

    public:
        Pointer first;
        Pointer last;
    };

    template<class SizeType, class Pointer, class ConstPointer>
    struct _fixed_array_bool_data
    {
    public:
        using pointer = Pointer;
        using const_pointer = ConstPointer;

    public:
        _fixed_array_bool_data() = default;

        _fixed_array_bool_data(_fixed_array_bool_data&& other) noexcept :
            first(std::exchange(other.first, nullptr)),
            size (std::exchange(other.size, 0)) {}

        constexpr void steal(_fixed_array_bool_data&& other) noexcept
        {
            first = std::exchange(other.first, nullptr);
            size  = std::exchange(other.size, 0);
        }

    public:
        Pointer first;
        SizeType size;
    };

    template<class Type, class Alloc = std::allocator<Type>>
    class fixed_array
    {
    private:
        using _alloc_traits = std::allocator_traits<Alloc>;
        //Ensure allocator value_type matches the container type
        static_assert(std::is_same_v<Type, typename _alloc_traits::value_type>);

    public: //Essential typedefs (Container requirements)
        using allocator_type  = Alloc;
        using value_type      = Type;

        using pointer         = typename _alloc_traits::pointer;
        using const_pointer   = typename _alloc_traits::const_pointer;
        using difference_type = typename _alloc_traits::difference_type;
        using size_type       = typename _alloc_traits::size_type;

    private:
        static constexpr bool _stores_bool = std::is_same_v<std::decay_t<Type>, bool>;

        using _data_type = std::conditional_t<_stores_bool,
            _fixed_array_bool_data<size_type, pointer, const_pointer>,
            _fixed_array_data<pointer, const_pointer>>;

    public:
        using reference       = std::conditional_t<_stores_bool, _bool_index, Type&>;
        using const_reference = std::conditional_t<_stores_bool, _const_bool_index, const Type&>;

        using iterator       = std::conditional_t<_stores_bool, _ctg_bool_iterator<_data_type>, ctg_iterator<_data_type>>;
        using const_iterator = std::conditional_t<_stores_bool, _ctg_bool_const_iterator<_data_type>, ctg_const_iterator<_data_type>>;

    public:
        constexpr fixed_array(const Alloc& new_alloc)
            //Note: N4868 asserts allocators must be nothrow copy constructible.
            noexcept(std::is_nothrow_default_constructible_v<pointer>) :
            _cpair(one_then_variadic{}, new_alloc)
        {}

        constexpr fixed_array(const fixed_array& other, const Alloc& alloc):
            fixed_array(alloc)
        {
            _unallocated_assign(other._first(), other._last(), other.size());
        }

        constexpr fixed_array(const fixed_array& other) :
            fixed_array(other, _alloc_traits::select_on_container_copy_construction(other._alloc())) {}

        constexpr fixed_array(fixed_array&& other, const Alloc& alloc):
            fixed_array(alloc)
        {
            if constexpr (!_alloc_traits::is_always_equal::value) {
                //On allocators compare false: Individually move, other's data
                //cannot be deallocated using new alloc
                if (_alloc() != other._alloc()) {
                    _unallocated_assign(
                        std::make_move_iterator(other._first()),
                        std::make_move_iterator(other._last()),
                        other.size());

                    return;
                }
            }

            //Steal contents
            _data().steal(std::move(other._data()));
        }

        constexpr fixed_array(fixed_array&& other) noexcept:
            _cpair(one_then_variadic{}, std::move(other._alloc()), std::move(other._data())) {}

        constexpr fixed_array(const size_type n, const Type& elem, const Alloc& alloc = Alloc()):
            fixed_array(alloc)
        {
            size_type alloc_size = _stores_bool ? right_shift_round_up(n, 3) : n;

            pointer new_first = _alloc_traits::allocate(_alloc(), alloc_size);
            try {
                if constexpr (_stores_bool) {
                    //Cast to char* here to ensure that underlying data of bool is correctly set. Todo: Create fill_n function that supports checked_allocator
                    auto as_uint8 = reinterpret_cast<uint8_t*>(std::to_address(new_first));
                    auto new_last = std::fill_n(as_uint8, alloc_size, static_cast<uint8_t>(elem ? 0xFF : 0));

                    //Hack: Temporarily added support for checked allocator.
                    _mark_initialised_if_checked_allocator(_alloc(), new_first, new_last, true);
                    _data().size = n;
                }
                else
                    _last() = uninitialised_fill_n(_alloc(), new_first, alloc_size, elem);
            }
            catch (...) {
                _alloc_traits::deallocate(_alloc(), new_first, alloc_size);
                throw;
            }

            _first() = new_first;
        }

        template<std::forward_iterator FwdIt, std::sentinel_for<FwdIt> Sentinel>
        constexpr fixed_array(FwdIt first, Sentinel last, const Alloc& alloc = Alloc()) :
            fixed_array(alloc)
        {
            if constexpr (_stores_bool) {
                size_type range_size = static_cast<size_type>(std::ranges::distance(first, last));

                _unchecked_replace(_ctg_duplicate_bits(first, last, range_size), nullptr, range_size);
            }
            else
                _unallocated_assign(first, last);
        }

        constexpr ~fixed_array() noexcept
        {
            _clear_dealloc();
        }

    private: //Fixed array destruction helper functions
        constexpr void _unchecked_replace(pointer new_first, pointer new_last, size_type new_size) noexcept
        {
            _first() = new_first;
            _set_sentinel(new_last, new_size);
        }

        constexpr void _replace(pointer new_first, pointer new_last, size_type new_size)
        {
            _clear_dealloc();
            _unchecked_replace(new_first, new_last, new_size);
        }

        constexpr void _clear_dealloc()
        {
            if (_first()) {
                destroy_range(_alloc(), _first(), _last());
                _alloc_traits::deallocate(_alloc(), _first(), _size());

                _unchecked_replace(nullptr, nullptr, 0);
            }
        }

    private: //Helper assign functions
        template<
            std::forward_iterator InputIt,
            std::sentinel_for<InputIt> Sentinel>
        constexpr pointer _ctg_duplicate_bits(const InputIt first, const Sentinel last, const size_type range_size)
        {
            const size_type alloc_size = right_shift_round_up(range_size, 3);

            const pointer new_first = _alloc_traits::allocate(_alloc(), alloc_size);
            try {
                set_bits(_alloc(), std::to_address(new_first), first, last);
            }
            catch (...) {
                _alloc_traits::deallocate(_alloc(), new_first, alloc_size);
                throw;
            }

            return new_first;
        }

        template<
            std::forward_iterator FwdIt,
            std::sentinel_for<FwdIt> Sentinel>
        constexpr void _unallocated_assign(const FwdIt begin, const Sentinel end, const size_type bool_size = 0)
        {
            const auto range_size = static_cast<size_type>(std::ranges::distance(begin, end));

            pointer new_last = _ctg_duplicate(_alloc(), begin, end, _first(), range_size);
            _set_sentinel(new_last, bool_size);
        }

        template<
            std::forward_iterator FwdIt,
            std::sentinel_for<FwdIt> Sentinel>
        constexpr void _alt_alloc_assign(Alloc& alt_alloc, const FwdIt first, const Sentinel last, const size_type bool_size = 0)
        {
            const auto range_size = static_cast<size_type>(std::ranges::distance(first, last));

            if (range_size != _size()) {
                pointer new_first = nullptr;
                pointer new_last  = _ctg_duplicate(alt_alloc, first, last, new_first, range_size);

                _replace(new_first, new_last, bool_size);
            }
            else
                copy(first, last, _first());
        }

    public:
        template<
            std::forward_iterator FwdIt,
            std::sentinel_for<FwdIt> Sentinel>
        constexpr fixed_array& assign(FwdIt first, const Sentinel last)
        {
            if constexpr (_stores_bool)
            {
                size_type range_size = static_cast<size_type>(std::ranges::distance(first, last));

                if (size() != range_size)
                    _replace(_ctg_duplicate_bits(first, last, range_size), nullptr, range_size);
                else
                    set_bits(std::to_address(_first()), first, last);
            }
            else
                _alt_alloc_assign(_alloc(), first, last);

            return *this;
        }

    public:
        constexpr fixed_array& operator=(const fixed_array& other)
        {
            if constexpr (_alloc_traits::propagate_on_container_copy_assignment::value) {
                if constexpr (!_alloc_traits::is_always_equal::value) {
                    if (_alloc() != other._alloc()) {
                        _alt_alloc_assign(other._alloc(), other._first(), other._last(), other.size());
                        _alloc() = other._alloc();
                        return *this;
                    }
                }

                _alloc() = other._alloc();
            }

            return _alt_alloc_assign(_alloc(), other._first(), other._last(), other.size());
        }

        constexpr fixed_array& operator=(fixed_array&& other) noexcept
        {
            if constexpr (!_alloc_traits::propagate_on_container_move_assignment::value) {
                if constexpr (!_alloc_traits::is_always_equal::value) {
                    if (_alloc() != other._alloc()) {
                        _alt_alloc_assign(
                            _alloc(),
                            std::make_move_iterator(other._first()),
                            std::make_move_iterator(other._last()),
                            other.size());

                        return *this;
                    }
                }

                _clear_dealloc();
            }
            else {
                _clear_dealloc();
                _alloc() = std::move(other._alloc());
            }

            _data().steal(std::move(other._data()));
            return *this;
        }

    private:
        constexpr reference _index_operator(const size_type index) const noexcept
        {
            EXPU_L1_ITER_VERIFY(index < size(), "heap_array index out of bounds!");

            if constexpr (_stores_bool)
                return _bool_index(std::to_address(_first()) + (index >> 3), index & 7);
            else
                return _first()[index];
        }

    public:
        constexpr const_reference at(const size_type index) const
        {
            //Note: safe to do, non-const version of expu::fixed_array::at is inheritely const
            return static_cast<const_reference>(const_cast<fixed_array&>(*this).at(index));
        }
        constexpr reference at(const size_type index)
        {
            if (index < _size())
                return this->operator[](index);
            else
                throw std::out_of_range("heap_array index out of bounds!");
        }

        constexpr const_reference operator[](const size_type index) const noexcept { return _index_operator(index); }
        constexpr reference       operator[](const size_type index)       noexcept { return _index_operator(index); }

    private:
        constexpr size_type _size() const noexcept
        {
            if constexpr (_stores_bool)
                return right_shift_round_up(_data().size, 3);
            else
                return static_cast<size_type>(_last() - _first());
        }

    public:
        constexpr size_type size() const noexcept
        {
            if constexpr (_stores_bool)
                return _data().size;
            else
                return _size();
        }

    private: //Helper range getter functions
        [[nodiscard]] constexpr iterator _end() const noexcept
        {
            if constexpr (_stores_bool) {
                //Check to see if at boundary
                uint8_t remainder = size() & 7;
                if (remainder != 0)
                    return iterator(_first() + (size() >> 3), remainder, &_data());
            }

            return iterator(_last(), &_data());
        }

    public: //Range getters
        [[nodiscard]] constexpr iterator begin()              noexcept { return iterator(_first(), &_data()); }
        [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return const_iterator(_first(), &_data()); }
        [[nodiscard]] constexpr const_iterator begin()  const noexcept { return cbegin(); }

        [[nodiscard]] constexpr iterator end()              noexcept { return _end(); }
        [[nodiscard]] constexpr const_iterator cend() const noexcept { return _end(); }
        [[nodiscard]] constexpr const_iterator end()  const noexcept { return cend(); }


    private: //private member getters
        constexpr const allocator_type& _alloc() const noexcept
        {
            return _cpair.first();
        }
        constexpr auto& _alloc() noexcept
        {
            return const_cast<allocator_type&>(static_cast<const fixed_array*>(this)->_alloc());
        }

        constexpr const _data_type& _data() const noexcept
        {
            return _cpair.second();

        }
        constexpr auto& _data() noexcept
        {
            return const_cast<_data_type&>(static_cast<const fixed_array*>(this)->_data());
        }

        constexpr const pointer& _first() const noexcept
        {
            return _cpair.second().first;
        }
        constexpr auto& _first() noexcept
        {
            return const_cast<pointer&>(static_cast<const fixed_array*>(this)->_first());
        }

        constexpr pointer _last() const noexcept requires(_stores_bool)
        {
            return _first() + _size();
        }
        constexpr pointer _last() noexcept requires(_stores_bool)
        {
            return static_cast<const fixed_array*>(this)->_last();
        }

        constexpr const pointer& _last() const noexcept
        {
            return _data().last;
        }
        constexpr auto& _last() noexcept
        {
            return const_cast<pointer&>(static_cast<const fixed_array*>(this)->_last());
        }

    private:
        constexpr void _set_sentinel(pointer new_last, size_type new_size)
        {
            if constexpr (_stores_bool)
                _data().size = new_size;
            else
                _data().last = new_last;
        }

    private:
        compressed_pair<allocator_type, _data_type> _cpair;
    };
}

#endif // !EXPU_FIXED_ARRAY_HPP_INCLUDED