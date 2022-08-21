#ifndef EXPU_FIXED_ARRAY_HPP_INCLUDED
#define EXPU_FIXED_ARRAY_HPP_INCLUDED

#include <memory>
#include <stdexcept>
#include <iterator>
#include <type_traits>

#include "expu/containers/contiguous_container.hpp"

#include "expu/debug.hpp"
#include "expu/mem_utils.hpp"

namespace expu 
{
    template<class Pointer, class ConstPointer>
    struct _fixed_array_data 
    {
    public:
        using pointer       = Pointer;
        using const_pointer = ConstPointer;

    public:
        Pointer first;
        Pointer last;

        constexpr void steal(_fixed_array_data&& other) noexcept
        {
            first = std::exchange(other.first, nullptr);
            last  = std::exchange(other.last, nullptr);
        }
    };

    template<class Type, class Alloc = std::allocator<Type>>
    class fixed_array 
    {
    private:
        using _alloc_traits = std::allocator_traits<Alloc>;
        //Ensure allocator value_type matches the container type
        static_assert(std::is_same_v<Type, _alloc_traits::value_type>);

    public: //Essential typedefs (Container requirements)
        using allocator_type  = Alloc;
        using value_type      = Type;
        using reference       = Type&;
        using const_reference = const Type&;
        using pointer         = typename _alloc_traits::pointer;
        using const_pointer   = typename _alloc_traits::const_pointer;
        using difference_type = typename _alloc_traits::difference_type;
        using size_type       = typename _alloc_traits::size_type;

    private:
        using _data_type = _fixed_array_data<pointer, const_pointer>;

    public:
        using iterator       = ctg_iterator<_data_type>;
        using const_iterator = ctg_iterator<_data_type>;

    public:
        constexpr fixed_array(const Alloc& new_alloc)
            //Note: N4868 asserts allocators must be nothrow copy constructible.
            noexcept(std::is_nothrow_default_constructible_v<pointer>) :
            _cpair(one_then_variadic{}, new_alloc)
        {}

        constexpr fixed_array(const fixed_array& other, const Alloc& alloc):
            fixed_array(other._data().first, other._data().last, alloc) {}

        constexpr fixed_array(const fixed_array& other) :
            fixed_array(other, _alloc_traits::select_on_container_copy_construction(other._alloc())) {}

        constexpr fixed_array(fixed_array&& other, const Alloc& alloc):
            fixed_array(alloc)
        {
            if constexpr (!_alloc_traits::is_always_equal::value) {
                //On allocators compare false: Individually move, others' data
                //cannot be deallocated using new alloc
                if (_alloc() != other._alloc()) {
                    _unallocated_assign(
                        std::make_move_iterator(other._data().first),
                        std::make_move_iterator(other._data().last));

                    return;
                }
            }

            //Steal contents
            _data().steal(std::move(other._data()));
        }

        constexpr fixed_array(fixed_array&& other) noexcept:
            _cpair(one_then_variadic{},
                std::move(other._alloc()),
                std::exchange(other._data().first, nullptr),
                std::exchange(other._data().last, nullptr))
        {}

        constexpr fixed_array(const size_type n, const Type& elem, const Alloc& alloc = Alloc()):
            fixed_array(alloc)
        {
            _data().first = _alloc_traits::allocate(_alloc(), n);
            try {
                _data().last = uninitialised_fill_n(_alloc(), _data().first, n, elem);
            }
            catch (...) {
                _alloc_traits::deallocate(_alloc(), _data().first, n);
                _data().first = nullptr;
                throw;
            }
        }

        template<std::forward_iterator FwdIt, std::sentinel_for<FwdIt> Sentinel>
        constexpr fixed_array(FwdIt first, Sentinel last, const Alloc& alloc = Alloc()) :
            fixed_array(alloc)
        {
            _unallocated_assign(first, last);
        }

        constexpr ~fixed_array() noexcept 
        {
            _clear_dealloc();
        }

    private: //Helper assign functions
        template<
            std::forward_iterator InputIt,
            std::sentinel_for<InputIt> Sentinel>
        constexpr void _unallocated_assign(const InputIt begin, const Sentinel end)
        {
            _data().last = _ctg_duplicate(_alloc(), begin, end, _data().first, std::ranges::distance(begin, end));
        }

        template<
            std::forward_iterator FwdIt,
            std::sentinel_for<FwdIt> Sentinel>
        constexpr void _alt_alloc_assign(Alloc& alt_alloc, FwdIt first, const Sentinel last)
        {
            const auto range_size = static_cast<size_type>(std::ranges::distance(first, last));

            if (range_size != _size()) {
                pointer new_first = nullptr;
                pointer new_last  = nullptr;

                try {
                    new_last = _ctg_duplicate(alt_alloc, first, last, new_first, range_size);
                }
                catch (...) { throw; /*Provide strong guarantee*/ }

                _clear_dealloc();
                _data().first = new_first;
                _data().last  = new_last;
            }
            else
                copy(first, last, _data().first);
        }

    private:
        constexpr void _clear_dealloc() 
        {
            if (_data().first) {
                destroy_range(
                    _alloc(),
                    std::to_address(_data().first),
                    std::to_address(_data().last));

                _alloc_traits::deallocate(_alloc(), _data().first, _size());

                _data().first = nullptr;
                _data().last  = nullptr;
            }
        }

    public:
        template<
            std::forward_iterator FwdIt,
            std::sentinel_for<FwdIt> Sentinel>
        constexpr fixed_array& assign(FwdIt first, const Sentinel last)
        {
            _alt_alloc_assign(_alloc(), first, last);
            return *this;
        }

    public:
        constexpr fixed_array& operator=(const fixed_array& other) {
            if constexpr (_alloc_traits::propagate_on_container_copy_assignment::value) {
                if constexpr (!_alloc_traits::is_always_equal::value) {
                    if (_alloc() != other._alloc()) {
                        _alt_alloc_assign(other._alloc(), other._data().first, other._data().last);
                        _alloc() = other._alloc();
                        return *this;
                    }
                }

                _alloc() = other._alloc();
            }

            return assign(other._data().first, other._data().end);
        }

        constexpr fixed_array& operator=(fixed_array&& other) noexcept
        {
            if constexpr (!_alloc_traits::propagate_on_container_move_assignment::value) {
                if constexpr (!_alloc_traits::is_always_equal::value) {
                    if (_alloc() != other._alloc())
                        return assign(
                            std::make_move_iterator(other._data().first),
                            std::move_sentinel(other._data().last));
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

    public:
        constexpr const_reference at(const size_type index) const 
        {
            if (index < _size())
                return _data().first[index];
            else
                throw std::out_of_range("heap_array index out of bounds!");
        }

        constexpr reference at(const size_type index) 
        {
            return const_cast<reference>(static_cast<const fixed_array&>(*this).at(index));
        }

        constexpr const_reference operator[](const size_type index) const
        {
            EXPU_L1_ITER_VERIFY(index < _size(), "heap_array index out of bounds!");
            return _data().first[index];
        }

        constexpr reference operator[](const size_type index)
        {
            return const_cast<reference>(static_cast<const fixed_array&>(*this).operator[](index));
        }

    private:
        constexpr size_type _size() const noexcept 
        {
            return static_cast<size_type>(_data().last - _data().first);
        }

    public:
        constexpr size_type size() const noexcept 
        {
            return _size();
        }

    //Range getters
    public:
        [[nodiscard]] constexpr iterator begin()              noexcept { return iterator(_data().first, &_data()); }
        [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return const_iterator(_data().first, &_data()); }
        [[nodiscard]] constexpr const_iterator begin()  const noexcept { return cbegin(); }

        [[nodiscard]] constexpr iterator end()              noexcept { return iterator(_data().last, &_data()); }
        [[nodiscard]] constexpr const_iterator cend() const noexcept { return const_iterator(_data().last, &_data()); }
        [[nodiscard]] constexpr const_iterator end()  const noexcept { return cend(); }


    private:
        constexpr       allocator_type& _alloc()       noexcept { return _cpair.first(); }
        constexpr const allocator_type& _alloc() const noexcept { return _cpair.first(); }

        constexpr       _data_type& _data()       noexcept { return _cpair.second(); }
        constexpr const _data_type& _data() const noexcept { return _cpair.second(); }


    private:
        compressed_pair<allocator_type, _data_type> _cpair;
    };

}

#endif // !EXPU_FIXED_ARRAY_HPP_INCLUDED