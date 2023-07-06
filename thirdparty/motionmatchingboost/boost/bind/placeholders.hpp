#ifndef BOOST_BIND_PLACEHOLDERS_HPP_INCLUDED
#define BOOST_BIND_PLACEHOLDERS_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//
//  bind/placeholders.hpp - _N definitions
//
//  Copyright (c) 2002 Peter Dimov and Multi Media Ltd.
//  Copyright 2015 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
//  See http://www.boost.org/libs/bind/bind.html for documentation.
//

#include <boost/bind/arg.hpp>
#include <boost/config.hpp>

namespace motionmatchingboost
{

namespace placeholders
{

#if defined(BOOST_BORLANDC) || defined(__GNUC__) && (__GNUC__ < 4)

inline motionmatchingboost::arg<1> _1() { return motionmatchingboost::arg<1>(); }
inline motionmatchingboost::arg<2> _2() { return motionmatchingboost::arg<2>(); }
inline motionmatchingboost::arg<3> _3() { return motionmatchingboost::arg<3>(); }
inline motionmatchingboost::arg<4> _4() { return motionmatchingboost::arg<4>(); }
inline motionmatchingboost::arg<5> _5() { return motionmatchingboost::arg<5>(); }
inline motionmatchingboost::arg<6> _6() { return motionmatchingboost::arg<6>(); }
inline motionmatchingboost::arg<7> _7() { return motionmatchingboost::arg<7>(); }
inline motionmatchingboost::arg<8> _8() { return motionmatchingboost::arg<8>(); }
inline motionmatchingboost::arg<9> _9() { return motionmatchingboost::arg<9>(); }

#elif !defined(BOOST_NO_CXX17_INLINE_VARIABLES)

BOOST_INLINE_CONSTEXPR motionmatchingboost::arg<1> _1;
BOOST_INLINE_CONSTEXPR motionmatchingboost::arg<2> _2;
BOOST_INLINE_CONSTEXPR motionmatchingboost::arg<3> _3;
BOOST_INLINE_CONSTEXPR motionmatchingboost::arg<4> _4;
BOOST_INLINE_CONSTEXPR motionmatchingboost::arg<5> _5;
BOOST_INLINE_CONSTEXPR motionmatchingboost::arg<6> _6;
BOOST_INLINE_CONSTEXPR motionmatchingboost::arg<7> _7;
BOOST_INLINE_CONSTEXPR motionmatchingboost::arg<8> _8;
BOOST_INLINE_CONSTEXPR motionmatchingboost::arg<9> _9;

#else

BOOST_STATIC_CONSTEXPR motionmatchingboost::arg<1> _1;
BOOST_STATIC_CONSTEXPR motionmatchingboost::arg<2> _2;
BOOST_STATIC_CONSTEXPR motionmatchingboost::arg<3> _3;
BOOST_STATIC_CONSTEXPR motionmatchingboost::arg<4> _4;
BOOST_STATIC_CONSTEXPR motionmatchingboost::arg<5> _5;
BOOST_STATIC_CONSTEXPR motionmatchingboost::arg<6> _6;
BOOST_STATIC_CONSTEXPR motionmatchingboost::arg<7> _7;
BOOST_STATIC_CONSTEXPR motionmatchingboost::arg<8> _8;
BOOST_STATIC_CONSTEXPR motionmatchingboost::arg<9> _9;

#endif

} // namespace placeholders

} // namespace motionmatchingboost

#endif // #ifndef BOOST_BIND_PLACEHOLDERS_HPP_INCLUDED
