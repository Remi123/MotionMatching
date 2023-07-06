// Copyright Daniel Wallin, David Abrahams 2010.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PARAMETER_IS_MAYBE_050329_HPP
#define BOOST_PARAMETER_IS_MAYBE_050329_HPP

namespace motionmatchingboost { namespace parameter { namespace aux {

    struct maybe_base
    {
    };
}}} // namespace motionmatchingboost::parameter::aux

#if defined(BOOST_PARAMETER_CAN_USE_MP11)
#include <type_traits>

namespace motionmatchingboost { namespace parameter { namespace aux {

    template <typename T>
    using is_maybe = ::std::is_base_of<
        ::motionmatchingboost::parameter::aux::maybe_base
      , typename ::std::remove_const<T>::type
    >;
}}} // namespace motionmatchingboost::parameter::aux

#else   // !defined(BOOST_PARAMETER_CAN_USE_MP11)
#include <boost/mpl/bool.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/type_traits/remove_const.hpp>

namespace motionmatchingboost { namespace parameter { namespace aux {

    template <typename T>
    struct is_maybe
      : ::motionmatchingboost::mpl::if_<
            ::motionmatchingboost::is_base_of<
                ::motionmatchingboost::parameter::aux::maybe_base
              , typename ::motionmatchingboost::remove_const<T>::type
            >
          , ::motionmatchingboost::mpl::true_
          , ::motionmatchingboost::mpl::false_
        >::type
    {
    };
}}} // namespace motionmatchingboost::parameter::aux

#endif  // BOOST_PARAMETER_CAN_USE_MP11
#endif  // include guard

