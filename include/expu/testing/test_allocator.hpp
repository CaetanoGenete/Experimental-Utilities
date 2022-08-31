#ifndef EXPU_TEST_ALLOCATOR_HPP_INCLUDED
#define EXPU_TEST_ALLOCATOR_HPP_INCLUDED

#include <memory>

namespace expu {

    enum class test_alloc_props
    {
        POCCA,
        POCMA,
        POCS,
        always_comp_false,
    };

    template<test_alloc_props Property, test_alloc_props ... Properties>
    constexpr bool _contains = ((Property == Properties) && ...);

    template<class Type, test_alloc_props ... Properties>
    class test_allocator : public std::allocator<Type>
    {
    private:
        using _base_type = std::allocator<Type>;

    private:
        using _alloc_traits = std::allocator_traits<_base_type>;

    public:
        using pointer            = typename _alloc_traits::pointer;
        using const_pointer      = typename _alloc_traits::const_pointer;
        using void_pointer       = typename _alloc_traits::void_pointer;
        using const_void_pointer = typename _alloc_traits::const_void_pointer;
        using value_type         = typename _alloc_traits::value_type;
        using size_type          = typename _alloc_traits::size_type;
        using difference_type    = typename _alloc_traits::difference_type;

    private:
        template<test_alloc_props Property>
        static constexpr bool _has_prop = _contains<Property, Properties...>;

    public:
        using is_always_equal                        = std::bool_constant<!_has_prop<test_alloc_props::always_comp_false>>;
        using propagate_on_container_copy_assignment = std::bool_constant<_has_prop<test_alloc_props::POCCA>>;
        using propagate_on_container_move_assignment = std::bool_constant<_has_prop<test_alloc_props::POCMA>>;
        using propagate_on_container_swap            = std::bool_constant<_has_prop<test_alloc_props::POCS>>;

    public:
        using _base_type::_base_type;
    };

    template<class Type, test_alloc_props ... Properties>
    requires(_contains<test_alloc_props::always_comp_false, Properties...>)
    constexpr bool operator==(const test_allocator<Type, Properties...>& lhs, const test_allocator<Type, Properties...>& rhs)
    {
        //Only compare equal if they are the exact same instance
        return &lhs == &rhs;
    }

    template<class Type, test_alloc_props ... Properties>
    requires(_contains<test_alloc_props::always_comp_false, Properties...>)
    constexpr bool operator!=(const test_allocator<Type, Properties...>& lhs, const test_allocator<Type, Properties...>& rhs)
    {
        //Compare not_equal if they are not the exact same instance
        return &lhs != &rhs;
    }
}

#endif // !EXPU_TEST_ALLOCATOR_HPP_INCLUDED