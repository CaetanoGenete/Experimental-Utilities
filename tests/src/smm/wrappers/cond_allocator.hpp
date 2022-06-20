#ifndef SMM_TESTS_COND_ALLOCATOR_HPP_INCLUDED
#define SMM_TESTS_COND_ALLOCATOR_HPP_INCLUDED

namespace smm_tests {

    template<typename Alloc, bool POCCA, bool POCMA, bool POCS, bool AE>
    struct conditional_allocator : 
        public Alloc 
    {
    private:


    public:
    };

}

#endif // !SMM_TESTS_COND_ALLOCATOR_HPP_INCLUDED