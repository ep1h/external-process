/**
 * @file c_cpp_compat.h
 * @brief Macros to facilitate interoperability between C and C++ in varying
 * compiler environments.
 */
#ifndef C_CPP_COMPAT_H_
#define C_CPP_COMPAT_H_

#if defined(_MSC_VER)
#define CC_CDECL_         __cdecl
#define CC_STDCALL_       __stdcall
#define CC_FASTCALL_      __fastcall
#define CC_THISCALL_      __fastcall
#define TYPES_UNALIGNED_  __unaligned
#define TYPES_ALIGNAS_(x) __declspec(align(x))
#define NOINLINE_         __declspec(noinline)

#elif defined(__GNUC__) || defined(__clang__)
#define CC_CDECL_         __attribute__((cdecl))
#define CC_STDCALL_       __attribute__((stdcall))
#define CC_FASTCALL_      __attribute__((fastcall))
#define CC_THISCALL_      __attribute__((fastcall))
#define TYPES_UNALIGNED_  __attribute__((packed))
#define TYPES_ALIGNAS_(x) __attribute__((aligned(x)))
#define NOINLINE_         __attribute__((noinline))
#endif /* _MSC_VER */

// TODO: Use alignas() if __cplusplus >= 201103L
// TODO: Use _Alignas() if __STDC_VERSION__ >= 201112L

#endif /* C_CPP_COMPAT_H_ */
