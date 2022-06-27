#ifndef SMM_TESTS_COUNTED_ALLOCATOR_HPP_INCLUDED
#define SMM_TESTS_COUNTED_ALLOCATOR_HPP_INCLUDED

#include <memory>

namespace smm_tests {

    template<typename Alloc>
    class counted_allocator : public Alloc {
    private:
        using _base_type_traits = std::allocator_traits<Alloc>;

    public:
        using propagate_on_container_copy_assignment = _base_type_traits::propagate_on_container_copy_assignment;
        using propagate_on_container_move_assignment = _base_type_traits::propagate_on_container_move_assignment;
        using propagate_on_container_swap            = _base_type_traits::propagate_on_container_swap;
        using is_always_equal                        = _base_type_traits::is_always_equal;

    public:
        using Alloc::Alloc;

        constexpr counted_allocator(): 
            _allocations(0), 
            _deallocations(0),
            Alloc() {}

        constexpr counted_allocator(const Alloc& base):
            _allocations(0),
            _deallocations(0),
            Alloc(base) {}

        constexpr ~counted_allocator() noexcept 
        {
            _allocations = 0;
            _deallocations = 0;
        }

    public:
        [[nodiscard]] constexpr _base_type_traits::pointer allocate(
            _base_type_traits::size_type n, _base_type_traits::const_void_pointer hint = nullptr) 
        {
            ++_allocations;
            return _base_type_traits::allocate(*this, n, hint);
        }

        constexpr void deallocate(_base_type_traits::pointer ptr, _base_type_traits::size_type n)
        {
            _base_type_traits::deallocate(*this, ptr, n);
            ++_deallocations;
        }

    public:
        [[nodiscard]] constexpr counted_allocator select_on_container_copy_construction() const 
        {
            return _base_type_traits::select_on_container_copy_construction(*this);
        }

    public:
        [[nodiscard]] constexpr size_t allocations() const noexcept 
        {
            return _allocations;
        }

        [[nodiscard]] constexpr size_t deallocations() const noexcept
        {
            return _deallocations;
        }

    private:
        size_t _allocations;
        size_t _deallocations;
    };

}

#endif // !SMM_TESTS_COUNTED_ALLOCATOR_HPP_INCLUDED
