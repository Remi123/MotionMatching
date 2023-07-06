// Copyright Daniel Wallin, David Abrahams 2005.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PARAMETER_AUX_YESNO_HPP
#define BOOST_PARAMETER_AUX_YESNO_HPP

namespace motionmatchingboost { namespace parameter { namespace aux {

    // types used with the "sizeof trick" to capture the results of
    // overload resolution at compile-time.
    typedef char yes_tag;
    typedef char (&no_tag)[2];
}}} // namespace motionmatchingboost::parameter::aux

#include <boost/mpl/bool.hpp>

namespace motionmatchingboost { namespace parameter { namespace aux {

    // mpl::true_ and mpl::false_ are not distinguishable by sizeof(),
    // so we pass them through these functions to get a type that is.
    ::motionmatchingboost::parameter::aux::yes_tag to_yesno(::motionmatchingboost::mpl::true_);
    ::motionmatchingboost::parameter::aux::no_tag to_yesno(::motionmatchingboost::mpl::false_);
}}} // namespace motionmatchingboost::parameter::aux

#include <boost/parameter/config.hpp>

#if defined(BOOST_PARAMETER_CAN_USE_MP11)
#include <boost/mp11/integral.hpp>

namespace motionmatchingboost { namespace parameter { namespace aux {

    // mp11::mp_true and mp11::mp_false are not distinguishable by sizeof(),
    // so we pass them through these functions to get a type that is.
    ::motionmatchingboost::parameter::aux::yes_tag to_yesno(::motionmatchingboost::mp11::mp_true);
    ::motionmatchingboost::parameter::aux::no_tag to_yesno(::motionmatchingboost::mp11::mp_false);
}}} // namespace motionmatchingboost::parameter::aux

#endif  // BOOST_PARAMETER_CAN_USE_MP11
#endif  // include guard

