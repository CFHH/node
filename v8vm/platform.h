#pragma once

typedef unsigned char Int8;
typedef unsigned short Int16;
typedef unsigned int Int32;

typedef signed char Uint8;
typedef signed short Uint16;
typedef signed int Uint32;

#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef signed __int64 Int64;
typedef unsigned __int64 Uint64;
#define INT64LIT(x) x##i64
#define UINT64LIT(x) x##ui64
#elif (_LP64 || __LP64__)
typedef signed long Int64;
typedef unsigned long Uint64;
#define INT64LIT(x) x##L
#define UINT64LIT(x) x##UL
#else
typedef signed long long Int64;
typedef unsigned long long Uint64;
#define INT64LIT(x) x##LL
#define UINT64LIT(x) x##ULL
#endif

