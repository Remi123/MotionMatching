// Copyright Cromwell D. Enage 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PARAMETER_AUX_IS_PLACEHOLDER_HPP
#define BOOST_PARAMETER_AUX_IS_PLACEHOLDER_HPP

#include <boost/parameter/config.hpp>

#if defined(BOOST_PARAMETER_CAN_USE_MP11)
#include <boost/mp11/integral.hpp>
#else
#include <boost/mpl/bool.hpp>
#endif

namespace motionmatchingboost { namespace parameter { namespace aux {

    template <typename T>
    struct is_mpl_placeholder
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
      : ::motionmatchingboost::mp11::mp_false
#else
      : ::motionmatchingboost::mpl::false_
#endif
    {
    };
}}} // namespace motionmatchingboost::parameter::aux

#include <boost/mpl/arg_fwd.hpp>

namespace motionmatchingboost { namespace parameter { namespace aux {

    template <int I>
    struct is_mpl_placeholder< ::motionmatchingboost::mpl::arg<I> >
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
      : ::motionmatchingboost::mp11::mp_true
#else
      : ::motionmatchingboost::mpl::true_
#endif
    {
    };
}}} // namespace motionmatchingboost::parameter::aux

#if defined(BOOST_PARAMETER_CAN_USE_MP11)
#include <boost/mp11/bind.hpp>

namespace motionmatchingboost { namespace parameter { namespace aux {

    template <typename T>
    struct is_mp11_placeholder : ::motionmatchingboost::mp11::mp_false
    {
    };

    template < ::std::size_t I>
    struct is_mp11_placeholder< ::motionmatchingboost::mp11::mp_arg<I> >
      : ::motionmatchingboost::mp11::mp_true
    {
    };
}}} // namespace motionmatchingboost::parameter::aux

#endif  // BOOST_PARAMETER_CAN_USE_MP11
#endif  // include guard

