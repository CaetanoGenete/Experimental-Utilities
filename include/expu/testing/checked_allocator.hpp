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

    class _checked_allocator_base {
    protected:
        using _init_memory_container = fixed_array<bool>;

        //Todo: initalised is only being used for size and index operator, consider providing these functions here
        struct _memory 
        {
            const _checked_allocator_base* owner;
            _init_memory_container initialised;

            _memory(const _checked_allocator_base* owner, size_t size):
                owner(owner), initialised(size, false) {}
        };

        using _map_type = std::map<const void*, _memory>;

    protected:
        _checked_allocator_base() noexcept:
            _allocated_memory(std::make_shared<_map_type>()) {}

        _checked_allocator_base(const _checked_allocator_base& other) noexcept :
            _allocated_memory(other._allocated_memory) {}

        _checked_allocator_base(_checked_allocator_base&& other) noexcept :
            _allocated_memory(std::move(other._allocated_memory)) {}

        ~_checked_allocator_base() {
            _check_cleared();
        }

    protected:
        virtual bool _comp_equal(const _checked_allocator_base* other) noexcept = 0;

    protected:
        void _check_cleared() const noexcept
        {
            //Ensure this is not a moved_from object
            if (_allocated_memory)
                EXPU_VERIFY(!(_allocated_memory->size() && _allocated_memory.use_count() == 1), "Not all memory allocated has been deallocated!");
        }

    protected: //Memory search helper functions
        [[nodiscard]] _map_type::iterator _mem_first_common(const void* const xp) const
        {
            _map_type::iterator loc = _allocated_memory->lower_bound(xp);

            if (loc != _allocated_memory->end())
                if (loc->first == xp)
                    return loc;

            if (_allocated_memory->size())
                return --loc;
            else
                return loc;
        }

        [[nodiscard]] _map_type::const_iterator _mem_first(const void* const xp) const { return _mem_first_common(xp); }
        [[nodiscard]] _map_type::iterator       _mem_first(const void* const xp)       { return _mem_first_common(xp); }

    protected: //initialised memory helper functions
        template<class Type>
        [[nodiscard]] constexpr bool _is_all_initialised_to(const _init_memory_container& initialised, size_t at, bool value) const
        {
            for (size_t byte = 0; byte < sizeof(Type); ++byte)
                if (initialised[at + byte] != value)
                    return false;

            return true;
        }

        template<class Type>
        constexpr void _unchecked_mark_initialised(_init_memory_container& initialised, size_t at, bool value)
        {
            for (size_t byte = 0; byte < sizeof(Type); ++byte)
                initialised[at + byte] = value;
        }

        template<class Type>
        constexpr void _check_alignment(size_t at) const
        {
            if ((at % sizeof(Type)) != 0)
                throw std::exception("pointer location does not match alignment!");
        }

    protected:
        std::shared_ptr<_map_type> _allocated_memory;
    };

    template<class Allocator, bool _throw_on_trivial>
    class _checked_allocator : public Allocator, public _checked_allocator_base
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

    public: //Constructors and destructor
        using _checked_allocator_base::_allocated_memory;

        template<class ... Args>
        requires(std::is_constructible_v<Allocator, Args...>)
        _checked_allocator(Args&& ... args) :
            Allocator(std::forward<Args>(args)...), 
            _checked_allocator_base() {}

        template<class OtherAllocator>
        _checked_allocator(const _checked_allocator<OtherAllocator, _throw_on_trivial>& other):
            Allocator(other),
            _checked_allocator_base(other) {}

        _checked_allocator(const _checked_allocator& other) noexcept:
            Allocator(other), 
            _checked_allocator_base(other) {}

        _checked_allocator(_checked_allocator&& other) noexcept:
            Allocator(std::move(other)), 
            _checked_allocator_base(std::move(other)) {}

        _checked_allocator select_on_container_copy_construction() const
        {
            return _alloc_traits::select_on_container_copy_construction(*this);
        }

    public: //Assignment operators
        _checked_allocator& operator=(const _checked_allocator& other) noexcept
        {
            Allocator::operator=(other);

            //If allocators don't compare equal, then memory cannot be deallocated, hence require that everything is
            //deallocated first.
            if constexpr (!is_always_equal::value) {
                if (*this != other)
                    _check_cleared();
            }

            return *this;
        }

        _checked_allocator& operator=(_checked_allocator&& other) noexcept
        {
            Allocator::operator=(std::move(other));

            if constexpr (!is_always_equal::value) {
                if (*this != other)
                    _check_cleared();
            }

            _allocated_memory = std::move(other._allocated_memory);
            return *this;
        }

    public:
        bool _comp_equal(const _checked_allocator_base* other) noexcept override 
        {
            if (typeid(*other) == typeid(*this))
                return static_cast<const _checked_allocator&>(*other) == *this;
            else
                return false;
        }


    private: //Size helper functions
        [[nodiscard]] constexpr size_t _byte_size(const size_type n) const noexcept
        {
            return n * sizeof(value_type);
        }

        [[nodiscard]] constexpr size_t _get_offset(const volatile void* const first, const volatile void* const last) const noexcept
        {
            using char_ptr_type = const volatile char* const;
            return static_cast<size_t>(static_cast<char_ptr_type>(last) - static_cast<char_ptr_type>(first));
        }

    public: //Allocate and deallocate
        [[nodiscard]] pointer allocate(const size_type n, const_void_pointer hint = nullptr)
        {
            pointer result = _alloc_traits::allocate(*this, n, hint);
            //Add memory block 
            _allocated_memory->try_emplace(std::to_address(result), this, _byte_size(n));
            return result;
        }

        void deallocate(const pointer pointer, const size_type n) noexcept
        {
            _alloc_traits::deallocate(*this, pointer, n);

            auto loc = _allocated_memory->find(std::to_address(pointer));

            EXPU_VERIFY(loc != _allocated_memory->end(), "Trying to deallocated memory which has not been allocated!");
            EXPU_VERIFY(_comp_equal(loc->second.owner),  "Cannot deallocate memory, this allocator does not compare equal to allocator which allocated this segment.");

            _init_memory_container& initialised = loc->second.initialised;

            EXPU_VERIFY(initialised.size() == _byte_size(n), "Partially deallocating memory! Prefer full deallocation where possible!");

            if constexpr (!std::is_trivially_destructible_v<value_type> || _throw_on_trivial) {
                for (bool initialised_element : initialised)
                    EXPU_VERIFY(!initialised_element, "Trying to deallocate memory wherein objects have not been destroyed!");
            }

            _allocated_memory->erase(loc);
        }

    public: //Construction and destruction functions
        template<class Type, class ... Args>
        void construct(Type* const xp, Args&& ... args)
        {
            _alloc_traits::construct(*this, xp, std::forward<Args>(args)...);

            const _map_type::iterator loc = _mem_first(xp);
            if (loc != _allocated_memory->end()) {
                _init_memory_container& initialised = loc->second.initialised;

                const size_t at = _get_offset(loc->first, xp);
                _check_alignment<Type>(at);

                if (at < initialised.size()) {
                    if constexpr (!std::is_trivially_destructible_v<value_type> || _throw_on_trivial) {
                        if (!_is_all_initialised_to<Type>(initialised, at, false))
                            throw std::exception("Trying to construct atop an already constructed object! Use assignment here!");
                    }

                    _unchecked_mark_initialised<Type>(initialised, at, true);
                    return;
                }
            }

            throw std::exception("Object is not within memory allocated by this allocator!");
        }

        template<class Type>
        void destroy(Type* const xp)
        {
            _alloc_traits::destroy(*this, xp);

            const _map_type::iterator loc = _mem_first(xp);
            if (loc != _allocated_memory->end()) {
                _init_memory_container& initialised = loc->second.initialised;

                const size_t at = _get_offset(loc->first, xp);
                _check_alignment<Type>(at);

                if (at < initialised.size()) {
                    //Constructed object is inside this allocated range
                    if constexpr (!std::is_trivially_copyable_v<value_type> || _throw_on_trivial) {
                        if (!_is_all_initialised_to<Type>(initialised, at, true))
                            throw std::exception("Trying to destroy an object which hasn't been constructed!");
                    }

                    _unchecked_mark_initialised<Type>(initialised, at, false);
                    return;
                }
            }

            throw std::exception("Object is not within memory allocated by this allocator!");
        }

    public: //Public getters for initialised memory
        [[nodiscard]] bool initialised(const const_pointer first, const const_pointer last) const
        {
            const _map_type::const_iterator loc = _mem_first(std::to_address(first));
            const _init_memory_container& initialised = loc->second.initialised;

                  size_t at_first = _get_offset(loc->first, std::to_address(first));
            const size_t at_end   = _get_offset(loc->first, std::to_address(last));

            if (at_end <= initialised.size()) {
                for (;at_first != at_end; ++at_first)
                    if (!initialised[at_first]) return false;
            }
            else
                throw std::out_of_range("Some or all of the objects being checked are not within memory allocated by allocator.");

            return true;
        }

        [[nodiscard]] bool atleast_one_initiliased_in(const const_pointer first, const const_pointer last) const
        {
            const _map_type::const_iterator loc = _mem_first(std::to_address(first));
            const _init_memory_container& initialised = loc->second.initialised;

                  size_t at_first = _get_offset(loc->first, std::to_address(first));
            const size_t at_end   = _get_offset(loc->first, std::to_address(last));

            if (at_end <= initialised.size()) {
                for (; at_first != at_end; ++at_first)
                    if (initialised[at_first]) return true;
            }
            else
                throw std::out_of_range("Some or all of the objects being checked are not within memory allocated by allocator.");

            return false;
        }

    public:
        template<class Type>
        void _mark_initialised(Type* const first, Type* const last, bool value) 
        {
            const _map_type::iterator loc = _mem_first(first);
            _init_memory_container& initialised = loc->second.initialised;

                  size_t at_first = _get_offset(loc->first, std::to_address(first));
            const size_t at_end   = _get_offset(loc->first, std::to_address(last));

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
    };

#if EXPU_CHECKED_ALLOCATOR_LEVEL > 0
    template<class Allocator, bool _throw_on_trivial = false> using checked_allocator = _checked_allocator<Allocator, _throw_on_trivial>;
#else
    template<class Allocator, bool> using checked_allocator = Allocator;
#endif // EXPU_CHECKED_ALLOCATOR_LEVEL > 0
}

#endif // !EXPU_CHECKED_ALLOCATOR_HPP_INCLUDED