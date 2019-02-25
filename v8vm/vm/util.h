#pragma once
#include <stdlib.h>

extern bool v8_initialized;

#ifdef __GNUC__
#define NO_RETURN __attribute__((noreturn))
#else
#define NO_RETURN
#endif

NO_RETURN void Abort();
NO_RETURN void Assert(const char* const (*args)[4]);

#ifdef _WIN32
#define ABORT_NO_BACKTRACE() _exit(134)
#else
#define ABORT_NO_BACKTRACE() abort()
#endif

#define ABORT() Abort()

#ifdef __GNUC__
#define LIKELY(expr) __builtin_expect(!!(expr), 1)
#define UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#define PRETTY_FUNCTION_NAME __PRETTY_FUNCTION__
#else
#define LIKELY(expr) expr
#define UNLIKELY(expr) expr
#define PRETTY_FUNCTION_NAME ""
#endif

#define STRINGIFY(x) STRINGIFY_(x)
#define STRINGIFY_(x) #x

#define CHECK(expr)                                                             \
    do {                                                                        \
        if (UNLIKELY(!(expr))) {                                                \
            static const char* const args[] = { __FILE__, STRINGIFY(__LINE__),  \
                                                #expr, PRETTY_FUNCTION_NAME };  \
            Assert(&args);                                                      \
        }                                                                       \
    } while (0)

#define CHECK_EQ(a, b) CHECK((a) == (b))
#define CHECK_GE(a, b) CHECK((a) >= (b))
#define CHECK_GT(a, b) CHECK((a) > (b))
#define CHECK_LE(a, b) CHECK((a) <= (b))
#define CHECK_LT(a, b) CHECK((a) < (b))
#define CHECK_NE(a, b) CHECK((a) != (b))
#define CHECK_NULL(val) CHECK((val) == nullptr)
#define CHECK_NOT_NULL(val) CHECK((val) != nullptr)
#define CHECK_IMPLIES(a, b) CHECK(!(a) || (b))

template <typename T>
inline T MultiplyWithOverflowCheck(T a, T b) {
    auto ret = a * b;
    if (a != 0)
        CHECK_EQ(b, ret / a);

    return ret;
}

void LowMemoryNotification();

template <typename T>
T* UncheckedRealloc(T* pointer, size_t n)
{
    size_t full_size = MultiplyWithOverflowCheck(sizeof(T), n);
    if (full_size == 0)
    {
        free(pointer);
        return nullptr;
    }
    void* allocated = realloc(pointer, full_size);
    if (UNLIKELY(allocated == nullptr))
    {
        LowMemoryNotification();
        allocated = realloc(pointer, full_size);
    }
    return static_cast<T*>(allocated);
}

template <typename T>
inline T* UncheckedMalloc(size_t n)
{
    if (n == 0) n = 1;
    return UncheckedRealloc<T>(nullptr, n);
}

template <typename T>
inline T* UncheckedCalloc(size_t n)
{
    if (n == 0) n = 1;
    MultiplyWithOverflowCheck(sizeof(T), n);
    return static_cast<T*>(calloc(n, sizeof(T)));
}

template <typename T>
inline T* Realloc(T* pointer, size_t n)
{
    T* ret = UncheckedRealloc(pointer, n);
    CHECK_IMPLIES(n > 0, ret != nullptr);
    return ret;
}

template <typename T>
inline T* Malloc(size_t n) {
    T* ret = UncheckedMalloc<T>(n);
    CHECK_IMPLIES(n > 0, ret != nullptr);
    return ret;
}

template <typename T>
inline T* Calloc(size_t n)
{
    T* ret = UncheckedCalloc<T>(n);
    CHECK_IMPLIES(n > 0, ret != nullptr);
    return ret;
}

inline char* Malloc(size_t n) { return Malloc<char>(n); }
inline char* Calloc(size_t n) { return Calloc<char>(n); }
inline char* UncheckedMalloc(size_t n) { return UncheckedMalloc<char>(n); }
inline char* UncheckedCalloc(size_t n) { return UncheckedCalloc<char>(n); }
