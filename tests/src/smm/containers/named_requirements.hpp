#ifndef SMM_TESTS_NAMED_REQUIREMENTS_HPP_INCLUDED
#define SMM_TESTS_NAMED_REQUIREMENTS_HPP_INCLUDED

#include <type_traits>
#include <iterator>
#include <memory>
#include <concepts>

#include "smm/defines.hpp"

namespace smm_tests {

    template<class Type, class Alloc = ::std::allocator<Type>>
    concept erasable = requires(Alloc & alloc, Type * ptr) {
        std::allocator_traits<Alloc>::destroy(alloc, ptr);
    };

    template<class Type, class Alloc = ::std::allocator<Type>>
    concept copy_insertable = requires(Alloc & alloc, Type * ptr, Type val) {
        std::allocator_traits<Alloc>::construct(alloc, ptr, val);
    };

    template<class ContainerType>
    concept container = requires(ContainerType cont, const ContainerType & const_cont) {
        requires erasable<typename ContainerType::value_type>;
        requires std::forward_iterator<typename ContainerType::iterator>;
        requires std::forward_iterator<typename ContainerType::const_iterator>;
        requires std::convertible_to<
            typename ContainerType::iterator,
            typename ContainerType::const_iterator>;
        requires std::signed_integral<typename ContainerType::difference_type>;
        requires std::same_as<typename ContainerType::difference_type, std::iter_difference_t<typename ContainerType::iterator>>;
        requires std::same_as<typename ContainerType::difference_type, std::iter_difference_t<typename ContainerType::const_iterator>>;
        requires std::unsigned_integral<typename ContainerType::size_type>;
        requires sizeof(typename ContainerType::size_type) >= sizeof(typename ContainerType::difference_type);

        requires std::regular<ContainerType>;
        requires std::swappable<ContainerType>;
        { cont.begin() }          -> std::same_as<typename ContainerType::iterator>;
        { cont.end() }            -> std::same_as<typename ContainerType::iterator>;
        { const_cont.begin() }    -> std::same_as<typename ContainerType::const_iterator>;
        { const_cont.end() }      -> std::same_as<typename ContainerType::const_iterator>;
        { cont.cbegin() }         -> std::same_as<decltype(const_cont.begin())>;
        { cont.cend() }           -> std::same_as<decltype(const_cont.end())>;
        { const_cont.size() }     -> std::same_as<typename ContainerType::size_type>;
        { const_cont.max_size() } -> std::same_as<typename ContainerType::size_type>;
        { const_cont.empty() }    -> std::convertible_to<bool>;
    };

    template<class SContainerType>
    concept reversible_container = requires(SContainerType scont, const SContainerType & const_scont) {
        requires container<SContainerType>;
        requires std::same_as<typename SContainerType::reverse_iterator, std::reverse_iterator<typename SContainerType::iterator>>;
        requires std::same_as<typename SContainerType::const_reverse_iterator, std::reverse_iterator<typename SContainerType::const_iterator>>;

        { scont.rbegin() }       -> std::same_as<typename SContainerType::reverse_iterator>;
        { scont.rend() }         -> std::same_as<typename SContainerType::reverse_iterator>;
        { const_scont.rbegin() } -> std::same_as<typename SContainerType::const_reverse_iterator>;
        { const_scont.rend() }   -> std::same_as<typename SContainerType::const_reverse_iterator>;
        { scont.crbegin() }      -> std::same_as<decltype(const_scont.rbegin())>;
        { scont.crend() }        -> std::same_as<decltype(const_scont.rend())>;
    };

    template<class AAContainer, class Allocator>
    concept _allocator_aware_container_helper = requires(const AAContainer & acont, AAContainer && rv_acont, const Allocator & alloc) {
        { acont.get_allocator() } -> std::same_as<Allocator>;
        { AAContainer(alloc) };
        { AAContainer(acont, alloc) };
        { AAContainer(rv_acont, alloc) };
    };

    template<class AAContainer>
    concept allocator_aware_container = requires(AAContainer acont) {
        requires _allocator_aware_container_helper<AAContainer, typename AAContainer::allocator_type>;
        requires container<AAContainer>;
    };

}

#endif // !SMM_TESTS_NAMED_REQUIREMENTS_HPP_INCLUDED
