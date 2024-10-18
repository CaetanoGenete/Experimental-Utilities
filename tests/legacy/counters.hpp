#ifndef EXPU_TESTS_COUNTERS_HPP_INCLUDED
#define EXPU_TESTS_COUNTERS_HPP_INCLUDED

#include "expu/containers/linear_map.hpp"
#include "expu/meta/meta_utils.hpp"
#include "expu/meta/function_traits.hpp"

#include <memory>
#include <type_traits>
#include <string>
#include <sstream>

namespace expu_tests {

    template<typename ... Args>
    [[nodiscard]] constexpr std::string arg_list_str()
    {
        std::string result = ((expu::type_name<Args>() + ",") + ...);

        if constexpr(sizeof...(Args) > 0)
            return result.substr(0, result.size() - 1);
        else
            return result;
    }

    class calls_counter
    {
    public:
        using mapped_type = size_t;
        using key_type    = std::string;

    public:
        constexpr calls_counter(): _map() {}
    
    public: //Indexing functions
        constexpr mapped_type get(const std::string& arg_list) const
        {
            const auto loc = _map.find(arg_list);

            if (loc == _map.cend())
                return 0;
            else
                return loc->second;
        }

        template<typename ... Args>
        constexpr mapped_type get() const
        {
            return get(arg_list_str<Args...>());
        }

    public: //Erasior functions
        template<typename ... Args>
        constexpr void erase()
        {
            _map.erase(arg_list_str<Args...>());
        }
    
    public: //Counter alteration functions
        template<typename ... Args>
        constexpr calls_counter& add(mapped_type count = 1)
        {
            const std::string key = arg_list_str<Args...>();
            const auto loc = _map.find(key);

            if (loc == _map.end())
                _map[key] = count;
            else
                loc->second += count;

            return *this;
        }

        template<typename ... Args>
        constexpr calls_counter& reset()
        {
            const std::string key = arg_list_str<Args...>();
            const auto loc = _map.find(key);

            if (loc != _map.end())
                loc->second = 0;

            return *this;
        }

    public:
        [[nodiscard]] constexpr auto begin()  const { return _map.cbegin(); }
        [[nodiscard]] constexpr auto cbegin() const { return _map.cbegin(); }

        [[nodiscard]] constexpr auto end()  const { return _map.cend(); }
        [[nodiscard]] constexpr auto cend() const { return _map.cend(); }

    private:
        expu::linear_map<std::string, size_t> _map;
    };

    struct alloc_counters
    {
    public:
        static constexpr size_t do_not_check = std::numeric_limits<size_t>::max();

    public:
        using map_type = calls_counter;

    public:
        size_t copy_ctor_calls  = do_not_check;
        size_t move_ctor_calls  = do_not_check;
        size_t destructor_calls = do_not_check;

        size_t allocations   = do_not_check;
        size_t deallocations = do_not_check;

        map_type calls;

    public:
        //Todo: Consider making non-member
        constexpr size_t ctor_calls() const noexcept 
        {
            size_t total = 0;

            if (copy_ctor_calls != do_not_check)
                total += copy_ctor_calls;
            if (move_ctor_calls != do_not_check)
                total += move_ctor_calls;

            return total;
        }
    };

    template<typename Allocator>
    struct counted_allocator : public Allocator 
    {
    private:
        using _alloc_traits = std::allocator_traits<Allocator>;

    public:
        using map_type = calls_counter;

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

    public:
        template<typename ... Args>
        constexpr counted_allocator(Args&& ... args)
            noexcept(std::is_nothrow_constructible_v<Allocator, Args...> &&
                     std::is_nothrow_default_constructible_v<map_type>):
            Allocator(std::forward<Args>(args)...),
            _data{ 0, 0, 0, 0, 0 } {}

    public:
        [[nodiscard]] constexpr auto allocate(size_type n, const_void_pointer hint = nullptr) 
        {
            auto result = _alloc_traits::allocate(*this, n, hint);
            ++_data.allocations;
            return result;
        }

        constexpr void deallocate(pointer pointer, size_type n)
        {
            _alloc_traits::deallocate(*this, pointer, n);
            ++_data.deallocations;
        }


    public:
        template<typename Type, typename ... Args>
        constexpr void construct(Type* xp, Args&& ... args) 
        {
            _alloc_traits::construct(*this, xp, std::forward<Args>(args)...);

            if constexpr (sizeof...(Args) == 1) {

                if constexpr (expu::calls_move_ctor_v<Type, decltype(args)...>) {
                    ++_data.move_ctor_calls;
                }
                else if constexpr (expu::calls_copy_ctor_v<Type, decltype(args)...>) {
                    ++_data.copy_ctor_calls;
                }

            }

            _data.calls.add<decltype(args)...>();
        }

        template<typename Type>
        constexpr void destroy(Type* xp) 
        {
            _alloc_traits::destroy(*this, xp);
            ++_data.destructor_calls;
        }

    public:
        [[nodiscard]] constexpr counted_allocator select_on_container_copy_construction() const 
        {
            return _alloc_traits::select_on_container_copy_construction(*this);
        }

    public: //Counter getters
        [[nodiscard]] constexpr size_type copy_ctor_calls()  const noexcept { return _data.copy_ctor_calls; }
        [[nodiscard]] constexpr size_type move_ctor_calls()  const noexcept { return _data.move_ctor_calls; }
        [[nodiscard]] constexpr size_type destructor_calls() const noexcept { return _data.destructor_calls; }

        [[nodiscard]] constexpr size_type allocations()   const noexcept { return _data.allocations; }
        [[nodiscard]] constexpr size_type deallocations() const noexcept { return _data.deallocations; }

        template<typename ... Args>
        [[nodiscard]] constexpr size_type calls_count() const
        {
            return _data.calls.get<Args...>();
        }

        [[nodiscard]] constexpr size_type calls_count(const std::string& arg_list) const
        {
            return _data.calls.get(arg_list);
        }

    public: //data getters
        [[nodiscard]] constexpr alloc_counters data() const { return _data; }

    private:
        alloc_counters _data;
    };    


    template<typename Allocator>
    inline constexpr std::string check_counters(
        const counted_allocator<Allocator>& alloc,
        const alloc_counters& change,
        const alloc_counters& expected)
    {
//Helper macro for check_counters function. IMPORTANT: Macro is dependant on function parameter names!
#define EXPU_TESTS_CHECK_DIFFERENCE(variable) \
        if((alloc.variable() - change.variable) != expected.variable && expected.variable != alloc_counters::do_not_check)

        std::stringstream ss;

        //Check specific constructor calls, only verify those that exist
        //within expected.
        for (const auto& pair : expected.calls)
        {
            auto& [key, count] = pair;
            
            if ((alloc.calls_count(key) - change.calls.get(key)) != count) {
                ss << "Unexpected calls to: ";
                ss << typeid(expu::_alloc_value_t<Allocator>).name();
                ss << "(" << key << ").\n";
            }
        }

        EXPU_TESTS_CHECK_DIFFERENCE(copy_ctor_calls)  ss << "Unexpected number of calls to copy constructor.\n";
        EXPU_TESTS_CHECK_DIFFERENCE(move_ctor_calls)  ss << "Unexpected number of calls to move constructor.\n";
        EXPU_TESTS_CHECK_DIFFERENCE(destructor_calls) ss << "Unexpected number of calls to destructor.\n";
        EXPU_TESTS_CHECK_DIFFERENCE(allocations)      ss << "Unexpected number of allocations.\n";
        EXPU_TESTS_CHECK_DIFFERENCE(deallocations)    ss << "Unexpected number of deallocations.\n";

#undef EXPU_TESTS_CHECK_DIFFERENCE

        return ss.str();
    }

    template<typename Allocator>
    inline constexpr std::string check_counters(
        const counted_allocator<Allocator>& alloc,
        const alloc_counters& expected)
    {
        //Note: calls field is automatically defaulted to be an empty map.
        return check_counters(alloc, { 0, 0, 0, 0, 0 }, expected);
    }

}

#endif // !EXPU_TESTS_COUNTERS_HPP_INCLUDED