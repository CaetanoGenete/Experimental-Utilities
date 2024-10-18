#ifndef SMM_DEBUG_HPP_INCLUDED
#define SMM_DEBUG_HPP_INCLUDED

#include <iostream>

#ifndef EXPU_DEBUG_LEVEL
#define EXPU_DEBUG_LEVEL 0
#endif // !CESS_DEBUG_LEVEL

#ifndef EXPU_ITERATOR_DEBUG_LEVEL
#define EXPU_ITERATOR_DEBUG_LEVEL EXPU_DEBUG_LEVEL
#endif // !SMM_ITERATOR_DEBUG_LEVEL

#define EXPU_VERIFY(condition, message)                                             \
{                                                                                   \
    if (!(condition)) {                                                             \
        std::cerr << __FILE__ << " " << __func__ << " line: " << __LINE__ << "\n";  \
        std::cerr << "'" << #condition << "' FAILED! Message: " << message;         \
        std::abort();                                                               \
    }                                                                               \
}

#ifndef NDEBUG
#define EXPU_VERIFY_DEBUG(condition, message) \
    EXPU_VERIFY(condition, message)
#else
#define EXPU_VERIFY_DEBUG(condition, message)
#endif // !NDEBUG

#endif // !EXPU_DEBUG_HPP_INCLUDED
