#ifndef EXPU_CHECKED_ALLOCATOR_HPP_INCLUDED
#define EXPU_CHECKED_ALLOCATOR_HPP_INCLUDED

#include <memory>      //For access to allocator_traits
#include <exception>   //For access to exception
#include <type_traits> //For access to is_trivially_copyable and is_trivially_destructible
#include <map>

#include "expu/containers/fixed_array.hpp"
#include "expu/debug.hpp"
#include "expu/mem_utils.hpp"

#ifndef EXPU_CHECKED_ALLOCATOR_LEVEL
#define EXPU_CHECKED_ALLOCATOR_LEVEL 1
#endif // !EXPU_CHECKED_ALLOCATOR_LEVEL


namespace expu {

#ifdef EXPU_TESTING_THROW_ON_TRIVIAL
    constexpr bool _checked_allocator_throw_on_trivial = true;
#else
    constexpr bool _checked_allocator_throw_on_trivial = false;
#endif

    template<class Allocator>
    class _checked_allocator : public Allocator
    {
    private:
        using _alloc_traits = std::allocator_traits<Allocator>;

    public:
        using pointer            = typename _alloc_traits::pointer;
        using const_pointer      = typename _alloc_traits::const_pointer;
        using void_pointer       = typename _alloc_traits::void_pointer;
        using const_void_pointer = typename _alloc_traits::const_void_pointer;
        using value_type         = typename _alloc_traits::value_type;
        using size_type          = typename _alloc_traits::size_type;
        using difference_type    = typename _alloc_traits::difference_type;

        using is_always_equal                        = _alloc_traits::is_always_equal;
        using propagate_on_container_copy_assignment = _alloc_traits::propagate_on_container_copy_assignment;
        using propagate_on_container_move_assignment = _alloc_traits::propagate_on_container_move_assignment;
        using propagate_on_container_swap            = _alloc_traits::propagate_on_container_swap;

    private:
        using _init_memory_container = fixed_array<bool>;
        //For type-correctness, the raw pointer is extracted from std::to_address
        using _raw_ptr_type = decltype(std::to_address(std::declval<const_pointer&>()));

        using _map_type = std::map<_raw_ptr_type, _init_memory_container>;

    public:
        template<class ... Args>
        requires(std::is_constructible_v<Allocator, Args...>)
        constexpr _checked_allocator(Args&& ... args) :
            Allocator(std::forward<Args>(args)...), _allocated_memory(), _is_view(false) {}

        constexpr _checked_allocator(const _checked_allocator& other) noexcept:
            Allocator(other), 
            _allocated_memory(other._allocated_memory), 
            _is_view(!other._allocated_memory.empty()) {}

        constexpr _checked_allocator(_checked_allocator&& other) noexcept:
            Allocator(std::move(other)), _allocated_memory(std::move(other._allocated_memory)), _is_view(false)
        {
            other._allocated_memory.clear();
        }

        constexpr ~_checked_allocator() noexcept
        {
            _check_cleared();
        }

    private:
        constexpr void _check_cleared() const noexcept
        {
            EXPU_VERIFY(!_allocated_memory.size() || _is_view, "Not all memory allocated has been deallocated!");
        }

        constexpr void _view_check() const
        {
            if (_is_view) 
                throw std::exception("Copied checked_allocator can only be used to access getters!");
        }

        constexpr void _view_check_no_throw() const noexcept
        {
            EXPU_VERIFY(!_is_view, "Copied checked_allocator can only be used to access getters!");
        }

    public:
        constexpr _checked_allocator& operator=(const _checked_allocator& other) noexcept
        {
            _view_check_no_throw();
            Allocator::operator=(other);

            //If allocators don't compare equal, then memory cannot be deallocated, hence require that everything is
            //deallocated first.
            if constexpr (!is_always_equal::value) {
                if (*this != other)
                    _check_cleared();
            }

            return *this;
        }

        constexpr _checked_allocator& operator=(_checked_allocator&& other) noexcept
        {
            _view_check_no_throw();
            Allocator::operator=(std::move(other));

            if constexpr (!is_always_equal::value) {
                if (*this != other)
                    _check_cleared();
            }

            _allocated_memory = std::move(other._allocated_memory);
            return *this;
        }

    public:
        [[nodiscard]] constexpr pointer allocate(const size_type n, const_void_pointer hint = nullptr)
        {
            _view_check();
            pointer result = _alloc_traits::allocate(*this, n, hint);
            //Add memory block 
            _allocated_memory.try_emplace(std::to_address(result), n, false);
            return result;
        }

        constexpr void deallocate(const pointer pointer, const size_type n) noexcept
        {
            _view_check_no_throw();
            _alloc_traits::deallocate(*this, pointer, n);

            auto loc = _allocated_memory.find(std::to_address(pointer));

            EXPU_VERIFY(loc != _allocated_memory.end(), "Trying to deallocated memory which has not been allocated!");

            _init_memory_container& initialised = loc->second;

            EXPU_VERIFY(initialised.size() == n, "Partially deallocating memory! Prefer full deallocation where possible!");

            if constexpr (!std::is_trivially_destructible_v<value_type> || _checked_allocator_throw_on_trivial) {
                for (bool initialised_element : initialised)
                    EXPU_VERIFY(!initialised_element, "Trying to deallocate memory wherein objects have not been destroyed!");
            }

            _allocated_memory.erase(loc);
        }

    private:
        constexpr _map_type::iterator _mem_first(const void* const xp)
        {
            const auto xp_value_ptr = reinterpret_cast<_raw_ptr_type>(xp);
            auto loc = _allocated_memory.lower_bound(xp_value_ptr);

            if (loc != _allocated_memory.end())
                if (loc->first == xp_value_ptr)
                    return loc;

            if (_allocated_memory.size())
                return --loc;
            else
                return loc;
        }

        constexpr _map_type::const_iterator _mem_first(const void* const xp) const
        {
            //Note: safe to do since _mem_first is intrinsically const
            return const_cast<_checked_allocator&>(*this)._mem_first(xp);
        }

    public:
        template<class Type, class ... Args>
        constexpr void construct(Type* const xp, Args&& ... args)
        {
            _view_check();
            _alloc_traits::construct(*this, xp, std::forward<Args>(args)...);

            const auto loc = _mem_first(xp);
            if (loc != _allocated_memory.end()) {
                _init_memory_container& initialised = loc->second;

                const auto at = static_cast<size_type>(reinterpret_cast<_raw_ptr_type>(xp) - loc->first);
                if (at < initialised.size()) {
                    if constexpr (!std::is_trivially_destructible_v<value_type> || _checked_allocator_throw_on_trivial) {
                        if (initialised[at])
                            throw std::exception("Trying to construct atop an already constructed object! Use assignment here!");
                    }

                    initialised[at] = true;
                    return;
                }
            }

            throw std::exception("Object is not within memory allocated by this allocator!");
        }

        template<class Type>
        constexpr void destroy(Type* const xp)
        {
            _view_check();
            _alloc_traits::destroy(*this, xp);

            const auto loc = _mem_first(xp);
            if (loc != _allocated_memory.end()) {
                _init_memory_container& initialised = loc->second;

                const auto at = static_cast<size_type>(reinterpret_cast<_raw_ptr_type>(xp) - loc->first);
                if (at < initialised.size()) {
                    //Constructed object is inside this allocated range
                    if constexpr (!std::is_trivially_copyable_v<value_type> || _checked_allocator_throw_on_trivial) {
                        if (!initialised[at])
                            throw std::exception("Trying to destroy an object which hasn't been constructed!");
                    }

                    initialised[at] = false;
                    return;
                }
            }

            throw std::exception("Object is not within memory allocated by this allocator!");
        }

        constexpr _checked_allocator select_on_container_copy_construction() const
        {
            _view_check();
            return _alloc_traits::select_on_container_copy_construction(*this);
        }

    public:
        constexpr bool initiliased(const_pointer first, const_pointer last) 
        {
            const auto loc = _mem_first(std::to_address(first));
            const _init_memory_container& initialised = loc->second;

            auto at_first = static_cast<size_type>(std::to_address(first) - reinterpret_cast<_raw_ptr_type>(loc->first));
            auto at_end   = static_cast<size_type>(std::to_address(last) - reinterpret_cast<_raw_ptr_type>(loc->first));

            if (at_end <= initialised.size()) {
                for (;at_first != at_end; ++at_first)
                    if (!initialised[at_first]) return false;
            }
            else
                throw std::out_of_range("Some or all of the objects being checked are not within memory allocated by allocator.");

            return true;
        }

        constexpr bool atleast_one_initiliased_in(const_pointer first, const_pointer last)
        {
            const auto loc = _mem_first(std::to_address(first));
            const _init_memory_container& initialised = loc->second;

            auto at_first = static_cast<size_type>(std::to_address(first) - reinterpret_cast<_raw_ptr_type>(loc->first));
            auto at_end   = static_cast<size_type>(std::to_address(last) - reinterpret_cast<_raw_ptr_type>(loc->first));

            if (at_end <= initialised.size()) {
                for (; at_first != at_end; ++at_first)
                    if (initialised[at_first]) return true;
            }
            else
                throw std::out_of_range("Some or all of the objects being checked are not within memory allocated by allocator.");

            return false;
        }

    public:
        constexpr void _mark_initialised(_raw_ptr_type first, _raw_ptr_type last, bool value) 
        {
            _view_check();

            const auto loc = _mem_first(first);
            _init_memory_container& initialised = loc->second;

            auto at_first = static_cast<size_type>(first - reinterpret_cast<_raw_ptr_type>(loc->first));
            auto at_end   = static_cast<size_type>(last - reinterpret_cast<_raw_ptr_type>(loc->first));

            if (at_end <= initialised.size()) {
                for (; at_first != at_end; ++at_first) {
                    if (initialised[at_first] != value)
                        initialised[at_first] = value;
                    else
                        throw std::out_of_range("Trying to construct/destruct in already constructed/destructed memory!");
                }
            }
            else
                throw std::out_of_range("Some or all of the objects being marked are not within memory allocated by allocator.");
        }

    private:
        _map_type _allocated_memory;
        ///Denotes that checked_allocator is for viewing purposes only. Occurs when copying from non-empty allocator. 
        bool _is_view;
    };

#if EXPU_CHECKED_ALLOCATOR_LEVEL > 0
    template<class Allocator> using checked_allocator = _checked_allocator<Allocator>;
#else
    template<class Allocator> using checked_allocator = Allocator;
#endif // EXPU_CHECKED_ALLOCATOR_LEVEL > 0
}

#endif // !EXPU_CHECKED_ALLOCATOR_HPP_INCLUDED