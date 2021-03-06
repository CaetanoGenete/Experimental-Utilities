#ifndef EXPU_CONTAINERS_DARRAY_HPP_INCLUDED
#define EXPU_CONTAINERS_DARRAY_HPP_INCLUDED

#include <iterator>
#include <utility>
#include <memory>

#include "expu/debug.hpp"
#include "expu/meta/meta_utils.hpp"
#include "expu/mem_utils.hpp"
#include "expu/iterators/iter_utils.hpp"

#if EXPU_ITERATOR_DEBUG_LEVEL > 0
#define EXPU_L1_ITER_VERIFY(condition, message) \
    EXPU_VERIFY(condition, message)
#else
#define EXPU_L1_ITER_VERIFY(condition, message)
#endif //EXPU_ITERATOR_DEBUG_LEVEL > 0

namespace expu {

    template<
        typename Type,
        typename PtrType,
        typename ConstPtrType,
        typename DiffType,
        typename SizeType
    > struct _darray_data {
    public:
        using value_type      = Type;
        using reference       = Type&;
        using pointer         = PtrType;
        using const_pointer   = ConstPtrType;
        using const_reference = const Type&;
        using difference_type = DiffType;
        using size_Type       = SizeType;

    public:
        PtrType first; //Starting address of container's allocated (if not nullptr) memory.
        PtrType last;  //Address of last element (Type) stored by container.
        PtrType end;   //One past end address of last allocated element of container.

        constexpr void steal(_darray_data&& other) noexcept {
            first = std::exchange(other.first, nullptr);
            last  = std::exchange(other.last , nullptr);
            end   = std::exchange(other.end  , nullptr);
        }
    };

    template<template_of<_darray_data> DArrayData>
    class darray_const_iterator {
    public:
        using iterator_concept  = std::contiguous_iterator_tag;
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = typename DArrayData::value_type;
        using reference         = typename DArrayData::const_reference;
        using pointer           = typename DArrayData::const_pointer;
        using difference_type   = typename DArrayData::difference_type;

    private:
        using _darray_data_ptr_t = const DArrayData*;
        using _member_ptr_t      = typename DArrayData::pointer;

    public:
        constexpr darray_const_iterator() = default;
        constexpr darray_const_iterator(const darray_const_iterator&) = default;
        constexpr darray_const_iterator(darray_const_iterator&&) = default;
        constexpr ~darray_const_iterator() = default;

        //Todo: Check if noexcept if explicit noexcept modifier is needed here.
        constexpr darray_const_iterator(_member_ptr_t ptr, _darray_data_ptr_t data_type)
            noexcept(std::is_nothrow_copy_constructible_v<_member_ptr_t>): 
            _ptr(ptr)
#if EXPU_ITERATOR_DEBUG_LEVEL > 0
            ,_data_ptr(data)
#endif
        {}

    public:
        constexpr darray_const_iterator& operator=(const darray_const_iterator&) = default;
        constexpr darray_const_iterator& operator=(darray_const_iterator&&) = default;

    protected:
        constexpr void _verify_dereferencable() const noexcept 
        {
            EXPU_L1_ITER_VERIFY(_ptr, "Container is empty, invalid derefence.")
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

        [[nodiscard]] constexpr reference operator[](difference_type n) const noexcept
        {
            return *(*this + n);
        }

    public:
        constexpr void swap(darray_const_iterator& other) 
            noexcept(std::is_nothrow_swappable_v<pointer>)
        {
            std::swap(_ptr, other._ptr);
        }

    public:
        constexpr darray_const_iterator& operator++() noexcept 
        {
            EXPU_L1_ITER_VERIFY(_ptr != _data_ptr->last, "Iterator is at the end of the container.");

            ++_ptr;
            return *this;
        }

        constexpr darray_const_iterator& operator--() noexcept
        {
            EXPU_L1_ITER_VERIFY(_ptr != _data_ptr->first, "Iterator is at the start of the container.");

            --_ptr;
            return *this;
        }

        constexpr darray_const_iterator& operator+=(difference_type n) noexcept
        {
            EXPU_L1_ITER_VERIFY(_data_ptr->last - _ptr  >= n, "Iterator increment too large, would be pushed past last element.");
            EXPU_L1_ITER_VERIFY(_data_ptr->first - _ptr <= n, "Iterator decrement too large, would be pushed past first element.");

            _ptr += n;
            return *this;
        }

        constexpr darray_const_iterator& operator-=(difference_type n) noexcept
        {
            //Todo: Verify performance penalty
            return operator+=(-n);
        }

    private:
        constexpr void _verify_cont_compat(const darray_const_iterator& rhs) const noexcept
        {
            EXPU_L1_ITER_VERIFY(_data_ptr == rhs._data_ptr, "Comparing iterators from different containers.");
        }

    public:
        [[nodiscard]] friend constexpr difference_type operator-(const darray_const_iterator& lhs, const darray_const_iterator& rhs)
            noexcept(noexcept(lhs._ptr - rhs._ptr))
        {
            lhs._verify_cont_compat(rhs);

            return static_cast<difference_type>(lhs._ptr - rhs._ptr);
        }

        [[nodiscard]] friend constexpr auto operator<=>(const darray_const_iterator& lhs, const darray_const_iterator& rhs)
            noexcept(noexcept(lhs._ptr <=> rhs._ptr))
        {
            lhs._verify_cont_compat(rhs);

            return lhs._ptr <=> rhs._ptr;
        }

        [[nodiscard]] friend constexpr auto operator==(const darray_const_iterator& lhs, const darray_const_iterator& rhs)
            noexcept(noexcept(lhs._ptr == rhs._ptr))
        {
            lhs._verify_cont_compat(rhs);

            return lhs._ptr == rhs._ptr;
        }

    public:
        [[nodiscard]] constexpr pointer _unwrapped() const noexcept
        {
            //Let c++ convert to the 
            return _ptr;
        }

    protected:
        _member_ptr_t _ptr;
#if EXPU_ITERATOR_DEBUG_LEVEL > 0
        _darray_data_ptr_t _data_ptr;
#endif 
    };

    template<template_of<_darray_data> DArrayData>
    constexpr void swap(darray_const_iterator<DArrayData>& lhs, darray_const_iterator<DArrayData>& rhs) 
        noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }

    template<template_of<_darray_data> DArrayData>
    class darray_iterator : public darray_const_iterator<DArrayData> {
    public:
        using iterator_concept  = std::contiguous_iterator_tag;
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = typename DArrayData::value_type;
        using reference         = typename DArrayData::reference;
        using pointer           = typename DArrayData::pointer;
        using difference_type   = typename DArrayData::difference_type;

    private:
        using _base_t = darray_const_iterator<DArrayData>;

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

        [[nodiscard]] constexpr reference operator[](difference_type n) const noexcept
        {
            //Note: Safe, _ptr is non-const qualified
            return const_cast<reference>(_base_t::operator[](n));
        }

    public:
        constexpr darray_iterator& operator++() noexcept
        {
            _base_t::operator++();
            return *this;
        }

        constexpr darray_iterator& operator--() noexcept
        {
            _base_t::operator--();
            return *this;
        }

        constexpr darray_iterator& operator+=(difference_type n) noexcept
        {
            _base_t::operator+=(n);
            return *this;
        }

        constexpr darray_iterator& operator-=(difference_type n) noexcept
        {
            _base_t::operator-=(n);
            return *this;
        }

    };
    
    //Todo: Find way to define this concept using typedefs from darray
    template<typename Iter>
    concept _iterator_of_darray = template_of<Iter, darray_const_iterator> || 
                                  template_of<Iter, darray_iterator>;

    template<_iterator_of_darray Iter>
    [[nodiscard]] constexpr Iter operator++(Iter& iter, int)
        noexcept(std::is_nothrow_copy_constructible_v<Iter> && noexcept(noexcept(++iter)))
    {
        Iter copy(iter);
        ++iter;
        return copy;
    }

    template<_iterator_of_darray Iter>
    [[nodiscard]] constexpr Iter operator--(Iter& iter, int)
        noexcept(std::is_nothrow_copy_constructible_v<Iter> && noexcept(noexcept(--iter)))
    {
        Iter copy(iter);
        --iter;
        return copy;
    }

    template<_iterator_of_darray Iter>
    [[nodiscard]] constexpr Iter operator+(Iter iter, std::iter_difference_t<Iter> n)
        noexcept(std::is_nothrow_copy_constructible_v<Iter> && noexcept(noexcept(iter += n)))
    {
        return iter += n;
    }

    template<_iterator_of_darray Iter>
    [[nodiscard]] constexpr Iter operator+(std::iter_difference_t<Iter> n, const Iter& iter)
        noexcept(noexcept(iter + n))
    {
        return iter + n;
    }

    template<_iterator_of_darray Iter>
    [[nodiscard]] constexpr Iter operator-(Iter iter, std::iter_difference_t<Iter> n)
        noexcept(std::is_nothrow_copy_constructible_v<Iter> && noexcept(noexcept(iter -= n)))
    {
        return iter -= n;
    }

    template<
        typename Type,
        typename Alloc = std::allocator<Type>
    > class darray {
    private:
        using _alloc_traits = std::allocator_traits<Alloc>;

        //Ensure allocator value_type matches the container type
        static_assert(std::is_same_v<Type, _alloc_traits::value_type>);

    //Essential typedefs (Container requirements)
    public:
        using allocator_type  = Alloc;
        using value_type      = Type;
        using reference       = Type&;
        using const_reference = const Type&;
        using pointer         = typename _alloc_traits::pointer;
        using const_pointer   = typename _alloc_traits::const_pointer;
        using difference_type = typename _alloc_traits::difference_type;
        using size_type       = typename _alloc_traits::size_type;

    private:
        using _data_t      = _darray_data<Type, pointer, const_pointer, difference_type, size_type>;

    //Iterator typedefs
    public:
        using iterator       = darray_iterator<_data_t>;
        using const_iterator = darray_const_iterator<_data_t>;

    //Special constructors (and destructor)
    public:
        constexpr darray() noexcept(std::is_nothrow_default_constructible_v<Alloc>):
            _cpair(zero_then_variadic{})
        {}
        
        constexpr darray(const Alloc& new_alloc)
            //Note: N4868 asserts allocators must be nothrow copy constructible.
            noexcept(std::is_nothrow_default_constructible_v<pointer>):
            _cpair(one_then_variadic{}, new_alloc)
        {}

        constexpr darray(const darray& other, const Alloc& alloc): 
            darray(alloc)
        {
            _unallocated_assign(
                other._data().first,
                other._data().last,
                other.capacity());
        }

        constexpr darray(const darray& other):
            darray(other, _alloc_traits::select_on_container_copy_construction(other._alloc())) {}

        constexpr darray(darray&& other, const Alloc& alloc) 
            noexcept(_alloc_traits::is_always_equal::value &&
                     std::is_nothrow_copy_constructible_v<Alloc>):
            darray(alloc)
        {
            if constexpr (!_alloc_traits::is_always_equal::value) {
                //On allocators compare false: Individually move, others' data
                //cannot be deallocated using new alloc
                if (_alloc() != other._alloc()) {
                    _unallocated_assign(
                        std::make_move_iterator(_data().first),
                        std::make_move_iterator(_data().last),
                        other.capacity());

                    return;
                }
            }

            //Steal contents
            _data().steal(std::move(other._data()));
        }

        constexpr darray(darray&& other) 
            noexcept:
            _cpair(one_then_variadic{},
                   std::move(other._alloc()), 
                   std::exchange(other._data().first, nullptr),
                   std::exchange(other._data().last , nullptr),
                   std::exchange(other._data().end  , nullptr))
        {}

        constexpr ~darray() noexcept
        {
            _clear_dealloc();
        }

    private:
        constexpr void _clear_dealloc()
            noexcept(std::is_nothrow_destructible_v<value_type>)
        {
            if (_data().first) {
                range_destroy_al(
                    _alloc(),
                    std::to_address(_data().first),
                    std::to_address(_data().last));

                _alloc_traits::deallocate(_alloc(), _data().first, capacity());
            }
        }

        constexpr void _replace(pointer new_first, pointer new_last, size_type new_capacity)
            noexcept(noexcept(_clear_dealloc()))
        {
            _clear_dealloc();

            _data().first = new_first;
            _data().last  = new_last;
            _data().end   = new_first + new_capacity;
        }

    private: //Helper assign functions
        template<std::input_iterator InputIt>
        constexpr void _unallocated_assign(InputIt begin, InputIt end, size_type capacity)
        {  
            std::tie(_data().first, _data().last) = _ctg_duplicate(_alloc(), begin, end, capacity);

            _data().end  = _data().first + capacity;
        }
        
        //In order: Allocates new buffer of specified capacity, copies range into new buffer,
        //then overrides internal array. Provides strong guarantee.
        template<std::input_iterator InputIt>
        constexpr void _resize_assign(Alloc& alloc, InputIt begin, InputIt end, size_type new_capacity)
        {
            auto [new_first, new_last] = _ctg_duplicate(alloc, begin, end, new_capacity);

            _replace(new_first, new_last, new_capacity);
        }

    public:
        //Assign range, utilising a different allocator for allocations and construction.
        //Provides strong guarantee on expansion, weak guarantee otherwise.
        //Note: De-allocation and destruction are performed using internal allocator.
        template<std::input_iterator InputIt>
        constexpr darray& _alt_alloc_assign(Alloc& alt_alloc, InputIt begin, InputIt end)
        {
            if constexpr (std::forward_iterator<InputIt>) {
                const auto range_size = std::distance(begin, end);

                if (capacity() < range_size) {
                    _resize_assign(alt_alloc, begin, end, range_size);
                }
                else if (size() < range_size) {
                    for (pointer first = _data().first; first != _data().last; ++begin, ++first)
                        *first = *begin;

                    //Todo: Check if safe to construct using a different allocator
                    _data().last = uninitialised_copy(alt_alloc, begin, end, _data().last);
                }
                else {
                    pointer new_last = copy(begin, end, _data().first);
                    range_destroy_al(_alloc(), new_last, _data().last);

                    _data().last = new_last;
                }

            }
            else {
                pointer first = _data().first;
                for (; first != _data().end && begin != end; ++begin, ++first)
                    *first = *begin;

                for (; begin != end; ++begin)
                    _alt_alloc_u_emplace_back(alt_alloc, *begin);
            }

            return *this;
        }
    public:
        template<std::input_iterator InputIt>
        constexpr darray& assign(InputIt begin, InputIt end) 
        {
            return _alt_alloc_assign(_alloc(), begin, end);
        }

        constexpr darray& operator=(const darray& other) 
        {
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

        constexpr darray& operator=(darray&& other) noexcept 
        {
            if constexpr (!_alloc_traits::propagate_on_container_move_assignment::value) {
                if constexpr (!_alloc_traits::is_always_equal::value) {
                    if (_alloc() != other._alloc())
                        return assign(
                            std::make_move_iterator(other._data().first),
                            std::make_move_iterator(other._data().last));
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
        constexpr void erase(const_iterator begin) 
            noexcept(std::is_nothrow_destructible_v<value_type>)
        {
            range_destroy_al(
                _alloc(), 
                std::to_address(begin._unwrapped()), 
                std::to_address(_data().last));

            _data().last = begin._unwrapped();
        }

    private:
        template<typename ... Args>
        constexpr void _alt_alloc_u_emplace_back(Alloc& alt_alloc, Args&& ... args)
            //Hack: Assuming here that 'construct' is noexcept if constructor is noexcept
            noexcept(std::is_nothrow_constructible_v<value_type, Args...>)
        {
            SMM_VERIFY_DEBUG(_data().last != _data().end, "Vector has no remaining capacity!");

            _alloc_traits::construct(
                alt_alloc,
                std::to_address(_data().last),
                std::forward<Args>(args)...);


            ++_data().last;
        }

    public:
        //Unchecked emplace_back
        template<typename ... Args>
        constexpr void u_emplace_back(Args&& ... args)
            noexcept(std::is_nothrow_constructible_v<value_type, Args...>)
        {
            _alt_alloc_u_emplace_back(_alloc(), std::forward<Args>(args)...);
        }

        constexpr void upush_back(const value_type& other)
            noexcept(std::is_nothrow_copy_constructible_v<value_type>)
        {
            u_emplace_back(other);
        }

        constexpr void upush_back(value_type&& other)
            noexcept(std::is_nothrow_move_constructible_v<value_type>)
        {
            u_emplace_back(std::move(other));
        }

        template<typename ... Args>
        constexpr void emplace_back(Args&& ... args)
        {
            if (_data().last == _data().end)
                _grow_geometric(size() + 1);

            u_emplace_back(std::forward<Args>(args)...);
        }

        constexpr void push_back(const value_type& other)
        {
            emplace_back(other);
        }

        constexpr void push_back(value_type&& other)
        {
            emplace_back(std::move(other));
        }


    public:
        template<std::input_iterator FwdIt>
        constexpr darray& insert(const_iterator at, FwdIt begin, FwdIt end)
        {
            if constexpr (std::forward_iterator<FwdIt>) {
                const auto range_size = std::distance(begin, end);

                if (range_size < capacity() - size()) {

                }
            }
            else {

            }
        }

    private:
        constexpr darray& _unchecked_grow_exactly(size_type new_capacity) 
        {
            if constexpr (std::is_nothrow_move_constructible_v<value_type>) {
                pointer new_first = _alloc_traits::allocate(_alloc(), new_capacity);
                //Note: Below will not throw, hence strong guarantee provided by _resize_assign is redundant
                pointer new_last  = uninitialised_move(_alloc(), _data().first, _data().last, std::to_address(new_first));

                _replace(new_first, new_last, new_capacity);
            }
            else
                _resize_assign(_alloc(), _data().first, _data().end, new_capacity);

            return *this;
        }

        constexpr darray& _grow_geometric(size_type min_capacity) 
        {
            if (max_size() < min_capacity)
                throw std::bad_array_new_length();

            const size_type half_size = size() >> 1;

            if (max_size() - half_size < size())
                _unchecked_grow_exactly(max_size());
            else
                _unchecked_grow_exactly(std::max(min_capacity, size() + half_size));

            return *this;
        }

    public:
        constexpr darray& reserve(size_type size) {
            if (capacity() < size)
                _unchecked_grow_exactly(size);

            return *this;
        }

    //Size getters
    public:
        [[nodiscard]] constexpr size_type size() const noexcept 
        {
            return static_cast<size_type>(_data().last - _data().first);
        }

        [[nodiscard]] constexpr size_type capacity() const noexcept
        {
            return static_cast<size_type>(_data().end - _data().first);
        }

        [[nodiscard]] constexpr size_type max_size() noexcept {
            return _alloc_traits::max_size(_alloc());
        }

    //Range getters
    public:
        [[nodiscard]] constexpr iterator begin()              noexcept { return iterator(_data().first, &_data()); }
        [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return const_iterator(_data().first, &_data()); }
        [[nodiscard]] constexpr const_iterator begin()  const noexcept { return cbegin(); }

        [[nodiscard]] constexpr iterator end()              noexcept { return iterator(_data().last, &_data()); }
        [[nodiscard]] constexpr const_iterator cend() const noexcept { return const_iterator(_data().last, &_data()); }
        [[nodiscard]] constexpr const_iterator end()  const noexcept { return cend(); }

    public:
        [[nodiscard]] constexpr allocator_type get_allocator() const noexcept { return _alloc(); }

    //Private compressed pair access getters
    private:
        [[nodiscard]] constexpr       _data_t& _data()       noexcept { return _cpair.second(); }
        [[nodiscard]] constexpr const _data_t& _data() const noexcept { return _cpair.second(); }

        [[nodiscard]] constexpr       allocator_type& _alloc()       noexcept { return _cpair.first(); }
        [[nodiscard]] constexpr const allocator_type& _alloc() const noexcept { return _cpair.first(); }

    private:
        compressed_pair<allocator_type, _data_t> _cpair;
    };

}

#endif // !EXPU_CONTAINERS_DARRAY_HPP_INCLUDED