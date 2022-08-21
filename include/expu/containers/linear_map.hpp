#ifndef EXPU_STATIC_MAP_HPP_INCLUDED
#define EXPU_STATIC_MAP_HPP_INCLUDED

#include <utility> 
#include <vector>

#include "expu/containers/darray.hpp"

namespace expu {

    template<
        class KeyType,
        class MappedType,
        class Container = std::vector<std::pair<KeyType, MappedType>>,
        class KeyEqual  = std::equal_to<KeyType>> 
    class linear_map {
    public: //Typedefs
        using key_type       = KeyType;
        using mapped_type    = MappedType;
        using key_equal      = KeyEqual;
        using value_type     = typename Container::value_type;
        using size_type      = typename Container::size_type;
        using different_type = typename Container::difference_type;
        using iterator       = typename Container::iterator;
        using const_iterator = typename Container::const_iterator;

        static_assert(std::is_same_v<value_type, std::pair<KeyType, MappedType>>, "Container must be of type std::pair");

    public: //Constructors
        template<class ... Args>
        requires std::is_constructible_v<Container, Args...>
        constexpr linear_map(Args&& ... args) 
            noexcept(std::is_nothrow_constructible_v<Container, Args...>):
            _elements(std::forward<Args>(args)...) {}

    public:
        [[nodiscard]] constexpr const_iterator find(const key_type& key) const 
        {
            //Note: Safe to do since find is a const function
            return const_cast<linear_map&>(*this).find(key);
        }

        [[nodiscard]] constexpr iterator find(const key_type& key)
        {
            return std::ranges::find(*this, key, &value_type::first);
        }

    public: // Indexing functions
        [[nodiscard]] constexpr const mapped_type& at(const key_type& key) const
        {
            const const_iterator loc = find(key);

            if (loc == cend())
                throw std::out_of_range("Key not found!");
            else
                return loc->second;
        }

        [[nodiscard]] constexpr mapped_type& at(const key_type& key)
        {
            return const_cast<value_type&>(static_cast<const linear_map&>(*this).at(key));
        }

        [[nodiscard]] constexpr const mapped_type& operator[](const key_type& key) const
        {
            const const_iterator loc = find(key);
            //Todo: Add debug check
            return loc->second;
        }

        constexpr mapped_type& operator[](const key_type& key)
        {
            const const_iterator loc = find(key);

            if (loc == cend())
                //Note: The standard does not require that a SequentialContainer's
                //emplace_back return anything, unlike emplace 
                return _elements.emplace(cend(), key, mapped_type())->second;
            else
                //Note: Safe to do since container is non-const
                return const_cast<mapped_type&>(loc->second);
        }

    public: //Erasion functions
        constexpr void erase(const key_type& key)
        {
            const const_iterator loc = find(key);

            if (loc != cend())
                _elements.erase(loc);
        }

    public: //Comparison operators
        [[nodiscard]] constexpr auto operator!=(const linear_map& other) const
        {
            return _elements != other._elements;
        }

        [[nodiscard]] constexpr auto operator==(const linear_map& other) const
        {
            return _elements == other._elements;
        }

    public:
        constexpr void swap(linear_map& other) 
            noexcept(std::is_nothrow_swappable_v<Container>)
        {
            _elements.swap(other._elements);
        }

    public: //Size getters
        [[nodiscard]] constexpr auto size()     const { return static_cast<size_type>(_elements.size()); }
        [[nodiscard]] constexpr auto max_size() const { return static_cast<size_type>(_elements.max_size()); }
        [[nodiscard]] constexpr auto empty()    const { return _elements.empty(); }

    public: //Iterator getters
        [[nodiscard]] constexpr iterator begin() noexcept { return _elements.begin(); }
        [[nodiscard]] constexpr iterator end()   noexcept { return _elements.end();   }

        [[nodiscard]] constexpr const_iterator begin()  const noexcept { return _elements.cbegin(); }
        [[nodiscard]] constexpr const_iterator end()    const noexcept { return _elements.cend();   }
        [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return _elements.cbegin(); }
        [[nodiscard]] constexpr const_iterator cend()   const noexcept { return _elements.cend();   }

    private:
        Container _elements;
    };

    template<template_of<linear_map> LinearMap>
    constexpr void swap(LinearMap& lhs, LinearMap& rhs)
        noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }
}

#endif // !EXPU_STATIC_MAP_HPP_INCLUDED