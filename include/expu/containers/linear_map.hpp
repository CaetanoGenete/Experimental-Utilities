#ifndef EXPU_STATIC_MAP_HPP_INCLUDED
#define EXPU_STATIC_MAP_HPP_INCLUDED

#include <utility> 
#include <array> 
#include <vector>

#include "expu/containers/darray.hpp"
#include "expu/iterators/searching.hpp"

namespace expu {

    template<
        typename KeyType,
        typename MappedType,
        typename Container = std::vector<std::pair<KeyType, MappedType>>,
        typename KeyEqual  = std::equal_to<KeyType>> 
    class linear_map {
    public: //Typedefs
        using key_type               = KeyType;
        using mapped_type            = MappedType;
        using key_equal              = KeyEqual;
        using value_type             = typename Container::value_type;
        using size_type              = typename Container::size_type;
        using different_type         = typename Container::difference_type;
        using iterator               = typename Container::iterator;
        using const_iterator         = typename Container::const_iterator;

        /* Find way to implement these if they exist
        using reverse_iterator       = typename Container::reverse_iterator;
        using const_reverse_iterator = typename Container::const_reverse_iterator;
        */

        static_assert(std::is_same_v<value_type, std::pair<KeyType, MappedType>>, "Container must be of type std::pair");

    public: //Constructors
        template<typename ... Args>
        requires std::is_constructible_v<Container, Args...>
        constexpr linear_map(Args&& ... args) 
            noexcept(std::is_nothrow_constructible_v<Container, Args...>):
            _elements(std::forward<Args>(args)...) {}

        constexpr linear_map(const std::initializer_list<value_type>& init_list):
            _elements(init_list) {}

    public:
        [[nodiscard]] constexpr const_iterator find(const key_type& key) const 
        {
            return find_if(cbegin(), cend(), [&](const value_type& curr) {
                return key_equal{}(curr.first, key);
            });
        }

        //Todo: Find way to avoid code duplication above!
        [[nodiscard]] constexpr iterator find(const key_type& key)
        {
            return find_if(begin(), end(), [&](const value_type& curr) {
                return key_equal{}(curr.first, key);
            });
        }

    public: // Indexing functions
        [[nodiscard]] constexpr const mapped_type& at(const key_type& key) const
        {
            const_iterator loc = find(key);

            if (loc == cend())
                throw std::out_of_range("Key not found!");
            else
                return (*loc).second;
        }

        [[nodiscard]] constexpr mapped_type& at(const key_type& key)
        {
            return const_cast<value_type&>(static_cast<const linear_map&>(*this).at(key));
        }

        [[nodiscard]] constexpr const mapped_type& operator[](const key_type& key) const
        {
            const_iterator loc = find(key);

            return (*loc).second;
        }

        constexpr mapped_type& operator[](const key_type& key)
        {
            const_iterator loc = find(key);

            if (loc == cend())
                //Note: The standard does not require that a SequentialContainer's
                //emplace_back returns anything, unlike emplace 
                return (*_elements.emplace(cend(), key, mapped_type())).second;
            else
                //Note: Safe to do since container is non-const
                return const_cast<mapped_type&>((*loc).second);
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
        [[nodiscard]] constexpr size_type size()     const { return _elements.size(); }
        [[nodiscard]] constexpr size_type max_size() const { return _elements.max_size(); }
        [[nodiscard]] constexpr auto empty()         const { return _elements.empty(); }

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

    template<typename KeyType, typename MappedType, typename Container, typename KeyEqual>
    constexpr void swap(linear_map<KeyType, MappedType, Container, KeyEqual>& lhs,
                        linear_map<KeyType, MappedType, Container, KeyEqual>& rhs)
        noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }
}

#endif // !EXPU_STATIC_MAP_HPP_INCLUDED