#ifndef Corburt_Types_h_Include_Guard
#define Corburt_Types_h_Include_Guard
#include <inttypes.h>
// Typedefs
typedef int_fast32_t i32; // 32-bit number
typedef uint_fast32_t u32;
typedef int_fast64_t i64; // 64-bit integer
//typedef long long i64;
//typedef __int64 i64;
typedef double real;
//typedef float real;
#define SPECi32 PRIdFAST32 // i32 specifier
#define SPECi64 PRIdFAST64 // i64 specifier
#endif
