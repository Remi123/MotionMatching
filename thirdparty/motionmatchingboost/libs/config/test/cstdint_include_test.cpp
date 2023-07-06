//  Copyright John Maddock 2009.
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define __STDC_CONSTANT_MACROS
#include <boost/cstdint.hpp> // must be the only #include!

int main()
{
   motionmatchingboost::int8_t i8 = INT8_C(0);
   (void)i8;
   motionmatchingboost::uint8_t ui8 = UINT8_C(0);
   (void)ui8;
   motionmatchingboost::int16_t i16 = INT16_C(0);
   (void)i16;
   motionmatchingboost::uint16_t ui16 = UINT16_C(0);
   (void)ui16;
   motionmatchingboost::int32_t i32 = INT32_C(0);
   (void)i32;
   motionmatchingboost::uint32_t ui32 = UINT32_C(0);
   (void)ui32;
#ifndef BOOST_NO_INT64_T
   motionmatchingboost::int64_t i64 = 0;
   (void)i64;
   motionmatchingboost::uint64_t ui64 = 0;
   (void)ui64;
#endif
   motionmatchingboost::int_least8_t i8least = INT8_C(0);
   (void)i8least;
   motionmatchingboost::uint_least8_t ui8least = UINT8_C(0);
   (void)ui8least;
   motionmatchingboost::int_least16_t i16least = INT16_C(0);
   (void)i16least;
   motionmatchingboost::uint_least16_t ui16least = UINT16_C(0);
   (void)ui16least;
   motionmatchingboost::int_least32_t i32least = INT32_C(0);
   (void)i32least;
   motionmatchingboost::uint_least32_t ui32least = UINT32_C(0);
   (void)ui32least;
#ifndef BOOST_NO_INT64_T
   motionmatchingboost::int_least64_t i64least = 0;
   (void)i64least;
   motionmatchingboost::uint_least64_t ui64least = 0;
   (void)ui64least;
#endif
   motionmatchingboost::int_fast8_t i8fast = INT8_C(0);
   (void)i8fast;
   motionmatchingboost::uint_fast8_t ui8fast = UINT8_C(0);
   (void)ui8fast;
   motionmatchingboost::int_fast16_t i16fast = INT16_C(0);
   (void)i16fast;
   motionmatchingboost::uint_fast16_t ui16fast = UINT16_C(0);
   (void)ui16fast;
   motionmatchingboost::int_fast32_t i32fast = INT32_C(0);
   (void)i32fast;
   motionmatchingboost::uint_fast32_t ui32fast = UINT32_C(0);
   (void)ui32fast;
#ifndef BOOST_NO_INT64_T
   motionmatchingboost::int_fast64_t i64fast = 0;
   (void)i64fast;
   motionmatchingboost::uint_fast64_t ui64fast = 0;
   (void)ui64fast;
#endif
   motionmatchingboost::intmax_t im = 0;
   (void)im;
   motionmatchingboost::uintmax_t uim = 0;
   (void)uim;
}
