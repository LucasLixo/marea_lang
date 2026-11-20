// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREAMACROS_H)
    #define MAREA_INCLUDE_BASE_MAREAMACROS_H

    #if !defined(MAREA_STRINGIFY_DETAIL)
        #define MAREA_STRINGIFY_DETAIL(x) #x
    #endif

    #if !defined(MAREA_STRINGIFY)
        #define MAREA_STRINGIFY(x) MAREA_STRINGIFY_DETAIL(x)
    #endif

    #if !defined(MAREA_CONCATENATE_DETAIL)
        #define MAREA_CONCATENATE_DETAIL(x, y) x##y
    #endif

    #if !defined(MAREA_CONCATENATE)
        #define MAREA_CONCATENATE(x, y) MAREA_CONCATENATE_DETAIL(x, y)
    #endif

    #if !defined(MAREA_ARRAY_SIZE)
        #define MAREA_ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
    #endif

    #if !defined(MAREA_DECL)
        #if defined(_WIN32)
            #if defined(MAREA_ENABLE_IMPORT)
                #define MAREA_DECL __declspec(dllimport)
            #elif defined(MAREA_ENABLE_EXPORT)
                #define MAREA_DECL __declspec(dllexport)
            #else
                #define MAREA_DECL
            #endif
        #elif defined(__GNUC__) && __GNUC__ >= 4
            #if defined(MAREA_ENABLE_EXPORT)
                #define MAREA_DECL __attribute__((visibility("default")))
            #else
                #define MAREA_DECL
            #endif
        #else
            #define MAREA_DECL
        #endif
    #endif

    #if !defined(MAREA_EXTERN)
        #define MAREA_EXTERN extern
    #endif

    #if !defined(MAREA_EXTERN_DECL)
        #define MAREA_EXTERN_DECL extern MAREA_DECL
    #endif

    #if !defined(MAREA_CXX_STANDARD)
        #if defined(__cplusplus)
            #if defined(_MSVC_LANG)
                #define MAREA_CXX_STANDARD _MSVC_LANG
            #else
                #define MAREA_CXX_STANDARD __cplusplus
            #endif
        #endif
    #endif

    // Needed so a C header stays callable from the C++ tests (see cpp-testing): a function
    // compiled as C has plain, unmangled linkage, which a C++ translation unit can only call
    // through an `extern "C"` declaration. Production code itself is never C++.
    #if !defined(MAREA_EXTERN_C)
        #if defined(MAREA_CXX_STANDARD)
            #define MAREA_EXTERN_C extern "C"
        #else
            #define MAREA_EXTERN_C
        #endif
    #endif

    #if !defined(MAREA_EXTERN_DECL_C)
        #define MAREA_EXTERN_DECL_C MAREA_EXTERN_C MAREA_DECL
    #endif

    #if !defined(MAREA_BEGIN_EXTERN_C)
        #if defined(MAREA_CXX_STANDARD)
            #define MAREA_BEGIN_EXTERN_C \
                MAREA_EXTERN_C           \
                {
        #else
            #define MAREA_BEGIN_EXTERN_C
        #endif
    #endif

    #if !defined(MAREA_END_EXTERN_C)
        #if defined(MAREA_CXX_STANDARD)
            #define MAREA_END_EXTERN_C }
        #else
            #define MAREA_END_EXTERN_C
        #endif
    #endif

    #if !defined(MAREA_CALL)
        #if (defined(_WIN32) && (defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)))
            #define MAREA_CALL __cdecl
        #else
            #define MAREA_CALL
        #endif
    #endif

    #if !defined(MAREA_NODISCARD)
        #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
            #define MAREA_NODISCARD [[nodiscard]]
        #elif (defined(__GNUC__) && (__GNUC__ >= 4)) || defined(__clang__)
            #define MAREA_NODISCARD __attribute__((warn_unused_result))
        #elif defined(_MSC_VER) && (_MSC_VER >= 1700)
            #define MAREA_NODISCARD _Check_return_
        #else
            #define MAREA_NODISCARD
        #endif
    #endif

    #if !defined(__inline__)
        #define __inline__ inline
    #endif

    #if !defined(MAREA_INLINE)
        #if defined(__GNUC__)
            #define MAREA_INLINE __inline__
        #elif defined(_MSC_VER)
            #define MAREA_INLINE __inline
        #else
            #define MAREA_INLINE inline
        #endif
    #endif

    #if !defined(MAREA_FORCE_INLINE)
        #if defined(_MSC_VER)
            #define MAREA_FORCE_INLINE static __forceinline
        #elif ((defined(__GNUC__) && (__GNUC__ >= 4)) || defined(__clang__))
            #define MAREA_FORCE_INLINE __attribute__((always_inline)) static MAREA_INLINE
        #else
            #define MAREA_FORCE_INLINE static MAREA_INLINE
        #endif
    #endif

    // C99 `inline` for a function that must also stay an exported symbol. The header holds the inline
    // definition; exactly one translation unit redeclares it `extern` (no `inline`), which makes the
    // compiler emit the external definition there. A caller may inline the body or call that symbol, and
    // at -O0 it calls, so a Debug build pays nothing extra. Never static and never always_inline (an
    // always_inline copy measured 4x slower than the call at -O0).
    // The export attribute sits on the definition itself so the `extern` redeclaration agrees with it.
    // On Windows it goes only on the translation unit that defines MAREA_DEFINE_INLINE_EXPORTED: cl.exe
    // rejects a dllexport redeclaration of a function already defined without it, MinGW treats a
    // dllexport inline as an external definition in every unit that sees it, and a consumer must never
    // get dllimport on a definition.
    #if !defined(MAREA_INLINE_EXPORTED)
        #if defined(_WIN32) && !defined(MAREA_DEFINE_INLINE_EXPORTED)
            #define MAREA_INLINE_EXPORTED MAREA_INLINE
        #else
            #define MAREA_INLINE_EXPORTED MAREA_DECL MAREA_INLINE
        #endif
    #endif

    #if !defined(MAREA_FORCE_CONST)
        #define MAREA_FORCE_CONST static /* MAREA_INLINE */ const
    #endif

    #if !defined(MAREA_BUILD_DEBUG)
        #if defined(SETTING_BUILD_DEBUG)
            #define MAREA_BUILD_DEBUG 1
        #else
            #define MAREA_BUILD_DEBUG 0
        #endif
    #endif

    #if !defined(MAREA_PACKED_START)
        #if defined(SETTING_USE_STRUCTS_PACKED)
            #if defined(__GNUC__) || defined(__clang__)
                #define MAREA_PACKED_START
            #elif defined(_MSC_VER)
                #define MAREA_PACKED_START __pragma(pack(push, 1))
            #else
                #define MAREA_PACKED_START "MAREA_PACKED_START: SETTING_USE_STRUCTS_PACKED needs GCC, Clang, or MSVC packing support; pre-define MAREA_PACKED_START to override."
            #endif
        #else
            #define MAREA_PACKED_START
        #endif
    #endif

    #if !defined(MAREA_PACKED_END)
        #if defined(SETTING_USE_STRUCTS_PACKED)
            #if defined(__GNUC__) || defined(__clang__)
                #define MAREA_PACKED_END __attribute__((__packed__))
            #elif defined(_MSC_VER)
                #define MAREA_PACKED_END __pragma(pack(pop))
            #else
                #define MAREA_PACKED_END "MAREA_PACKED_END: SETTING_USE_STRUCTS_PACKED needs GCC, Clang, or MSVC packing support; pre-define MAREA_PACKED_END to override."
            #endif
        #else
            #define MAREA_PACKED_END
        #endif
    #endif

    #if defined(_MSC_VER)
        #define MAREA_ASM(symbol)
        #define MAREA_HAS_ASM_LABEL 0
    #elif defined(__clang__) || defined(__GNUC__)
        #define MAREA_ASM(symbol)   __asm__(symbol)
        #define MAREA_HAS_ASM_LABEL 1
    #endif

    #if !defined(MAREA_ASM)
        #define MAREA_ASM(symbol)
    #endif

    #if !defined(MAREA_HAS_ASM_LABEL)
        #define MAREA_HAS_ASM_LABEL 0
    #endif

    #if !defined(MAREA_INLINE_ASM)
        #if defined(_MSC_VER)
            #if defined(_M_X64)
                #define MAREA_INLINE_ASM
            #else
                #define MAREA_INLINE_ASM __asm
            #endif
        #elif defined(__clang__) || defined(__GNUC__)
            #define MAREA_INLINE_ASM __asm__ __volatile__
        #else
            #define MAREA_INLINE_ASM
        #endif
    #endif

    #if !defined(MAREA_DEPRECATED_ATTRIBUTE)
        // INFO: Official support is in gcc 3.1.
        #if defined(__GNUC__) && (__GNUC__ >= 4)
            #define MAREA_DEPRECATED_ATTRIBUTE __attribute__((deprecated))
        #elif defined(_MSC_VER)
            #define MAREA_DEPRECATED_ATTRIBUTE __declspec(deprecated)
        #else
            #define MAREA_DEPRECATED_ATTRIBUTE
        #endif
    #endif

    #if !defined(MAREA_UNUSED)
        #if defined(__GNUC__)
            #define MAREA_UNUSED __attribute__((unused))
        #else
            #define MAREA_UNUSED
        #endif
    #endif

    #if !defined(MAREA_STATIC_ASSERT)
        #if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202311L)
            #define MAREA_STATIC_ASSERT(cond, msg) static_assert(cond, msg " " #cond ".")
        #elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
            #define MAREA_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg " " #cond ".")
        #else
            // INFO: no C11 static_assert here (<assert.h>'s macro is gated the same way) - emulate with an enum that divides by zero when cond is false.
            #define MAREA_STATIC_ASSERT(cond, msg)                                            \
                enum                                                                          \
                {                                                                             \
                    MAREA_CONCATENATE(MAREA_StaticAssertEnum, __LINE__) = 1 / (int)(!!(cond)) \
                };                                                                            \
                typedef int MAREA_CONCATENATE(MAREA_StaticAssertTypedef, __LINE__)[(cond) * 2 - 1]
        #endif
    #endif

    #if !defined(MAREA_BEGIN_IGNORE_PRAGMA_PACK)
        #if defined(__clang__)
            #define MAREA_BEGIN_IGNORE_PRAGMA_PACK \
                _Pragma("clang diagnostic push")   \
                    _Pragma("clang diagnostic ignored \"-Wpragma-pack\"")
        #elif defined(__GNUC__)
            #define MAREA_BEGIN_IGNORE_PRAGMA_PACK \
                _Pragma("GCC diagnostic push")
        #elif defined(_MSC_VER)
            #define MAREA_BEGIN_IGNORE_PRAGMA_PACK \
                __pragma(warning(push))            \
                    __pragma(warning(disable : 4103))
        #else
            #define MAREA_BEGIN_IGNORE_PRAGMA_PACK
        #endif
    #endif

    #if !defined(MAREA_BEGIN_IGNORE_UNUSED_AND_INIT_WARNINGS)
        #if defined(__clang__)
            #define MAREA_BEGIN_IGNORE_UNUSED_AND_INIT_WARNINGS                     \
                _Pragma("clang diagnostic push")                                    \
                    _Pragma("clang diagnostic ignored \"-Wunused-variable\"")       \
                        _Pragma("clang diagnostic ignored \"-Wunused-parameter\"")  \
                            _Pragma("clang diagnostic ignored \"-Wuninitialized\"") \
                                _Pragma("clang diagnostic ignored \"-Winit-self\"")
        #elif defined(__GNUC__)
            #define MAREA_BEGIN_IGNORE_UNUSED_AND_INIT_WARNINGS                   \
                _Pragma("GCC diagnostic push")                                    \
                    _Pragma("GCC diagnostic ignored \"-Wunused-variable\"")       \
                        _Pragma("GCC diagnostic ignored \"-Wunused-parameter\"")  \
                            _Pragma("GCC diagnostic ignored \"-Wuninitialized\"") \
                                _Pragma("GCC diagnostic ignored \"-Winit-self\"")
        #elif defined(_MSC_VER)
            #define MAREA_BEGIN_IGNORE_UNUSED_AND_INIT_WARNINGS \
                __pragma(warning(push))                         \
                    __pragma(warning(disable : 4100))           \
                        __pragma(warning(disable : 4101))       \
                            __pragma(warning(disable : 4700))
        #else
            #define MAREA_BEGIN_IGNORE_UNUSED_AND_INIT_WARNINGS
        #endif
    #endif

    #if !defined(MAREA_END_IGNORE_PRAGMA)
        #if defined(__clang__)
            #define MAREA_END_IGNORE_PRAGMA _Pragma("clang diagnostic pop")
        #elif defined(__GNUC__)
            #define MAREA_END_IGNORE_PRAGMA _Pragma("GCC diagnostic pop")
        #elif defined(_MSC_VER)
            #define MAREA_END_IGNORE_PRAGMA __pragma(warning(pop))
        #else
            #define MAREA_END_IGNORE_PRAGMA
        #endif
    #endif

    #if defined(__GNUC__) && __GNUC__ < 5
        #define MAREA_THREAD_LOCAL __thread
    #elif defined(_MSC_VER)
        #define MAREA_THREAD_LOCAL __declspec(thread)
    #elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
        #define MAREA_THREAD_LOCAL _Thread_local
    #endif

    #if !defined(MAREA_THREAD_LOCAL)
        #if defined(__GNUC__)
            #define MAREA_THREAD_LOCAL __thread
        #else
            #define MAREA_THREAD_LOCAL "MAREA_THREAD_LOCAL: no thread-local storage on this compiler (needs C11, GCC, or MSVC); pre-define MAREA_THREAD_LOCAL to override."
        #endif
    #endif

MAREA_BEGIN_IGNORE_PRAGMA_PACK

    #include <stdalign.h>

MAREA_END_IGNORE_PRAGMA

    #if !defined(MAREA_ALIGNED)
        #if defined(__clang__) || defined(__GNUC__)
            #define MAREA_ALIGNED(x) __attribute__((aligned(x)))
        #elif defined(_MSC_VER)
            #define MAREA_ALIGNED(x) __declspec(align(x))
        #elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
            #define MAREA_ALIGNED(x) _Alignas(x)
        #else
            #define MAREA_ALIGNED(x) "MAREA_ALIGNED: no alignment support on this compiler (needs GCC, Clang, MSVC, or C11); pre-define MAREA_ALIGNED to override."
        #endif
    #endif

    #if !defined(MAREA_RESTRICT)
        #if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
            #define MAREA_RESTRICT restrict
        #elif defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
            #define MAREA_RESTRICT __restrict
        #else
            #define MAREA_RESTRICT
        #endif
    #endif

    #if !defined(MAREA_VECTOR_SIZE)
        #if defined(__clang__) || defined(__GNUC__)
            #define MAREA_VECTOR_SIZE(n)  __attribute__((vector_size(n)))
            #define MAREA_HAS_VECTOR_SIZE 1
        #else
            #define MAREA_VECTOR_SIZE(n)  "MAREA_VECTOR_SIZE: no vector extension on this compiler (needs GCC or Clang); pre-define MAREA_VECTOR_SIZE and MAREA_HAS_VECTOR_SIZE to override."
            #define MAREA_HAS_VECTOR_SIZE 0
        #endif
    #endif

    #if !defined(MAREA_HAS_VECTOR_SIZE)
        #define MAREA_HAS_VECTOR_SIZE 0
    #endif

    #if !defined(MAREA_ALIGNOF)
        #if defined(__clang__) || defined(__GNUC__)
            #define MAREA_ALIGNOF(x) __alignof__(x)
        #elif defined(_MSC_VER)
            #define MAREA_ALIGNOF(x) __alignof(x)
        #elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
            #define MAREA_ALIGNOF(x) _Alignof(x)
        #else
            #define MAREA_ALIGNOF(x) "MAREA_ALIGNOF: no alignof support on this compiler (needs GCC, Clang, MSVC, or C11); pre-define MAREA_ALIGNOF to override."
        #endif
    #endif

    #if !defined(MAREA_IGNORE)
        #define MAREA_IGNORE(expression) ((void)(expression))
    #endif

#endif /* MAREA_INCLUDE_BASE_MAREAMACROS_H */
