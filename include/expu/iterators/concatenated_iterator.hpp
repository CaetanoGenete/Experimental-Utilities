#ifndef EXPU_CONCATENATED_ITERATOR_HPP_INCLUDED
#define EXPU_CONCATENATED_ITERATOR_HPP_INCLUDED

#include <iterator>
#include <tuple>

#include "expu/meta/typelist_set_operations.hpp" //For access to tuple_subset
#include "expu/maths/basic_maths.hpp" //For access to is_odd

namespace expu {

    template<std::input_iterator Iterator, size_t _iterators_count>
    class _concat_iterator_base
    {
    protected:
        using _iter_traits = std::iterator_traits<Iterator>;

    public:
        static_assert(is_odd(_iterators_count), "There must be an odd number of iterators!");

        using iterator_category = typename _iter_traits::iterator_category;
        using value_type        = typename _iter_traits::value_type;
        using reference         = typename _iter_traits::reference;
        using pointer           = typename _iter_traits::pointer;
        using difference_type   = typename _iter_traits::difference_type;

    public:
        template<std::convertible_to<Iterator> ... Types>
        requires(sizeof...(Types) == _iterators_count)
        constexpr _concat_iterator_base(Types&& ... args) :
            _iterators{ std::forward<Types>(args)... }, _range_index(0) {}

    protected:
        Iterator _iterators[_iterators_count];
        size_t _range_index;
    };

    //Todo: Add noexcept specifiers
    template<class Iterator, size_t _iterators_count>
    class concatenated_iterator : public _concat_iterator_base<Iterator, _iterators_count>
    {
    private:
        using _base_type = _concat_iterator_base<Iterator, _iterators_count>;

    public:
        template<class ... Types>
        requires(sizeof...(Types) == _iterators_count)
        constexpr concatenated_iterator(Types&& ... args) : _base_type(std::forward<Types>(args)...)
        {
            _next_range();
        }

    private:
        constexpr bool _at_end() noexcept 
        {
            return _base_type::_range_index == _iterators_count - 1;
        }

        constexpr void _next_range() {
            //Ensure not at end
            if (!_at_end()) {
                //Until range_size > 0  or at end
                while (*this == _base_type::_iterators[_base_type::_range_index + 1]) {
                    *_base_type::_iterators = _base_type::_iterators[_base_type::_range_index += 2];

                    if (_at_end()) break;
                }
            }
        }

    public: //Referencing functions
        constexpr concatenated_iterator::reference operator*() const { return **_base_type::_iterators; }
        constexpr concatenated_iterator::pointer operator->() const { return _base_type::_iterators->operator->(); }

    public: //Increment functions
        constexpr concatenated_iterator& operator++()
        {
            _base_type::_iterators->operator++();
            _next_range();

            return *this;
        }

        constexpr concatenated_iterator operator++(int)
        {
            concatenated_iterator copy(*this);
            operator++();
            return copy;
        }

    public:
        template<class OtherType>
        constexpr friend auto operator!=(const concatenated_iterator& lhs, const OtherType& rhs) noexcept
        {
            return *lhs._iterators != rhs;
        }

    };

    template<class Iterator, class ... Iterators>
    concatenated_iterator(Iterator&&, Iterators&& ...)->concatenated_iterator<std::decay_t<Iterator>, sizeof...(Iterators) + 1>;

    template<class Iterator, size_t _iterators_count, class OtherType>
    constexpr auto operator==(const concatenated_iterator<Iterator, _iterators_count>& lhs, const OtherType& rhs) noexcept
    {
        return !(lhs != rhs);
    }

    template<class ... Iterators>
    constexpr auto concatenate(Iterators&& ... iters) {
        static constexpr size_t iter_count = sizeof...(Iterators);

        //If numbe of iterators is odd, do nothing.
        if constexpr (is_odd(iter_count))
            return concatenated_iterator(std::forward<Iterators>(iters)...);
        //Otherwise, construct without last iterator
        else
            std::apply(&concatenate, tuple_subset(
                std::make_index_sequence<iter_count - 1>{}, std::make_tuple(std::forward<Iterators>(iters)...)));

    }
}

#endif // !EXPU_CONCATENATED_ITERATOR_HPP_INCLUDED