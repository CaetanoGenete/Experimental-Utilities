#ifndef SMM_TESTS_DISABLED_WRAPPER_HPP_INCLUDED
#define SMM_TESTS_DISABLED_WRAPPER_HPP_INCLUDED

#include "meta_sort.hpp"

namespace smm_tests {

    enum class special_member : uint8_t 
    {
        move_ctor, move_asgn,
        copy_ctor, copy_asgn
    };

    template<typename MemberType, special_member...>
    struct _disabled_wrapper_helper {
    public:
        using type = MemberType;

    public:
        constexpr _disabled_wrapper_helper(MemberType type) : _type(type) {}

    public:
        [[nodiscard]] constexpr MemberType& unwrapped() { return _type; }
        [[nodiscard]] constexpr const MemberType& unwrapped() const { return _type; }

    private:
        MemberType _type;
    };

#define SMM_TESTS_COPY_CTOR_default 
#define SMM_TESTS_COPY_CTOR_delete special_member::copy_ctor,
#define SMM_TESTS_MOVE_CTOR_default
#define SMM_TESTS_MOVE_CTOR_delete special_member::move_ctor,
#define SMM_TESTS_COPY_ASGN_default
#define SMM_TESTS_COPY_ASGN_delete special_member::copy_asgn,
#define SMM_TESTS_MOVE_ASGN_default
#define SMM_TESTS_MOVE_ASGN_delete special_member::move_asgn,

#define SMM_TESTS_DISABLED_WRAPPER_HELPER(copy_ctor, move_ctor, copy_asgn, move_asgn)                       \
    template<typename MemberType, special_member ... Disables>                                              \
    struct _disabled_wrapper_helper<                                                                        \
        MemberType,                                                                                         \
        SMM_TESTS_COPY_CTOR_##copy_ctor                                                                     \
        SMM_TESTS_MOVE_CTOR_##move_ctor                                                                     \
        SMM_TESTS_COPY_ASGN_##copy_asgn                                                                     \
        SMM_TESTS_MOVE_ASGN_##move_asgn                                                                     \
        Disables...>                                                                                        \
    {                                                                                                       \
    private:                                                                                                \
        using _parent_t = _disabled_wrapper_helper<MemberType, Disables...>;                                \
                                                                                                            \
    public:                                                                                                 \
        using type = MemberType;                                                                            \
                                                                                                            \
    public:                                                                                                 \
        constexpr _disabled_wrapper_helper(const _parent_t& parent) : _parent(parent) {}                    \
                                                                                                            \
        constexpr _disabled_wrapper_helper(const _disabled_wrapper_helper&)                = copy_ctor;     \
        constexpr _disabled_wrapper_helper(_disabled_wrapper_helper&&) noexcept            = move_ctor;     \
        constexpr _disabled_wrapper_helper& operator=(const _disabled_wrapper_helper&)     = copy_asgn;     \
        constexpr _disabled_wrapper_helper& operator=(_disabled_wrapper_helper&&) noexcept = move_asgn;     \
                                                                                                            \
    public:                                                                                                 \
        [[nodicard]] constexpr MemberType& unwrapped() noexcept             { return _parent.unwrapped(); } \
        [[nodicard]] constexpr const MemberType& unwrapped() const noexcept { return _parent.unwrapped(); } \
                                                                                                            \
    private:                                                                                                \
        _parent_t _parent;                                                                                  \
    }

    SMM_TESTS_DISABLED_WRAPPER_HELPER(delete , default, default, default);
    SMM_TESTS_DISABLED_WRAPPER_HELPER(default, delete , default, default);
    SMM_TESTS_DISABLED_WRAPPER_HELPER(default, default, delete , default);
    SMM_TESTS_DISABLED_WRAPPER_HELPER(default, default, default, delete );
    //Note: Required due to complication in overload resolution
    SMM_TESTS_DISABLED_WRAPPER_HELPER(default, delete , default, delete );

#undef SMM_TESTS_DISABLED_WRAPPER_HELPER

#undef SMM_TESTS_COPY_CTOR_default 
#undef SMM_TESTS_COPY_CTOR_delete
#undef SMM_TESTS_MOVE_CTOR_default
#undef SMM_TESTS_MOVE_CTOR_delete
#undef SMM_TESTS_COPY_ASGN_default
#undef SMM_TESTS_COPY_ASGN_delete
#undef SMM_TESTS_MOVE_ASGN_default
#undef SMM_TESTS_MOVE_ASGN_delete

    template<typename MemberType, special_member ... Disables>
    struct _disabled_wrapper
    {
    private:
        using _unsorted_seq = expu::int_seq<uint8_t, static_cast<uint8_t>(Disables)...>;
        using _sorted_seq   = sorted_int_seq<_unsorted_seq>;

        template<typename>
        struct _to_disabled_wrapper;

        template<uint8_t ... Seq>
        struct _to_disabled_wrapper<expu::int_seq<uint8_t, Seq...>> 
        {
            using type = _disabled_wrapper_helper<MemberType, static_cast<special_member>(Seq)...>;
        };

    public:
        using type = typename _to_disabled_wrapper<_sorted_seq>::type;
    };

    template<typename MemberType, special_member ... Disables>
    using disabled_wrapper_t = typename _disabled_wrapper<MemberType, Disables...>::type;


    template<typename Type, special_member query>
    constexpr bool is_disabled = false;

    template<typename UnwrappedType, special_member ... Disables, special_member query>
    constexpr bool is_disabled<_disabled_wrapper_helper<UnwrappedType, Disables...>, query> = ((query == Disables) || ...);
}

//Hack: Terrible practice but necessary evil, may be undefine behaviour.
template<typename MemberType, smm_tests::special_member ... Disables>
struct std::is_trivially_copyable<smm_tests::_disabled_wrapper_helper<MemberType, Disables...>> :
    std::bool_constant<std::is_trivially_copyable_v<MemberType>> {};

//Hack: Terrible, may be undefine behaviour
template<typename MemberType, smm_tests::special_member ... Disables>
constexpr bool std::is_trivially_copyable_v<smm_tests::_disabled_wrapper_helper<MemberType, Disables...>> = 
    std::is_trivially_copyable_v<MemberType>;

#endif //!SMM_TESTS_DISABLED_WRAPPER_HPP_INCLUDED