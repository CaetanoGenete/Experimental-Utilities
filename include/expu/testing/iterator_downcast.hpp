#ifndef EXPU_ITERATOR_DOWNCAST_HPP_INCLUDED
#define EXPU_ITERATOR_DOWNCAST_HPP_INCLUDED

#include <iterator>

namespace expu {

    //Todo: Create overloads for random-access-iterator (+, -, +=, ...);
    template<std::forward_iterator Iterator, class NewCategory>
    requires(std::is_base_of_v<NewCategory, typename std::iterator_traits<Iterator>::iterator_category>)
    struct iterator_downcast : public Iterator
    {
    public:
        using iterator_category = NewCategory;

    public:
        using Iterator::Iterator;

    public:
        constexpr iterator_downcast& operator++()
            noexcept(noexcept(Iterator::operator++()))
        {
            Iterator::operator++();
            return *this;
        }

        constexpr iterator_downcast operator++(int)
            noexcept(noexcept(Iterator::operator++()) && std::is_nothrow_copy_constructible_v<iterator_downcast>)
        {
            iterator_downcast copy(*this);
            Iterator::operator++();
            return copy;
        }

        constexpr iterator_downcast& operator--()
            noexcept(noexcept(Iterator::operator--()))
            requires(std::bidirectional_iterator<Iterator>)
        {
            Iterator::operator--();
            return *this;
        }

        constexpr iterator_downcast operator--(int)
            noexcept(noexcept(Iterator::operator--()) && std::is_nothrow_copy_constructible_v<iterator_downcast>)
            requires(std::bidirectional_iterator<Iterator>)
        {
            iterator_downcast copy(*this);
            Iterator::operator--();
            return copy;
        }
    };

    template<std::input_iterator Iterator>
    struct iterator_downcast<Iterator, std::input_iterator_tag> : public Iterator {
    public:
        using iterator_category = std::input_iterator_tag;

    public:
        using Iterator::Iterator;

    public:
        constexpr iterator_downcast(const iterator_downcast& other) :
            Iterator(other), _invalidated(other._invalidated)
        {
            other._invalidated = true;
        }

    public:
        constexpr std::iter_reference_t<Iterator> operator*() const
        {
            if (_invalidated)
                throw std::exception("Iterator has been invalidated!");
            else
                return Iterator::operator*();
        }

        constexpr decltype(auto) operator->() const
        {
            if (_invalidated)
                throw std::exception("Iterator has been invalidated!");
            else
                return Iterator::operator->();
        }

    public:
        constexpr iterator_downcast& operator++()
            noexcept(noexcept(Iterator::operator++()))
        {
            Iterator::operator++();
            return *this;
        }

        constexpr iterator_downcast operator++(int)
            noexcept(noexcept(Iterator::operator++()) && std::is_nothrow_copy_constructible_v<iterator_downcast>)
        {
            iterator_downcast copy(*this);
            Iterator::operator++();
            return copy;
        }

    private:
        mutable bool _invalidated = false;
    };

    template<std::input_iterator Iterator> using input_iterator_cast         = iterator_downcast<Iterator, std::input_iterator_tag>;
    template<std::input_iterator Iterator> using forward_iterator_cast       = iterator_downcast<Iterator, std::forward_iterator_tag>;
    template<std::input_iterator Iterator> using bidirectional_iterator_cast = iterator_downcast<Iterator, std::bidirectional_iterator_tag>;
    template<std::input_iterator Iterator> using random_access_iterator_cast = iterator_downcast<Iterator, std::random_access_iterator_tag>;

}

#endif // !EXPU_ITERATOR_DOWNCAST_HPP_INCLUDED