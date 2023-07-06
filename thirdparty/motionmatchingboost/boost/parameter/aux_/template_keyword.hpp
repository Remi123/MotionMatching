// Copyright Daniel Wallin 2006.
// Copyright Cromwell D. Enage 2017.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PARAMETER_TEMPLATE_KEYWORD_060203_HPP
#define BOOST_PARAMETER_TEMPLATE_KEYWORD_060203_HPP

namespace motionmatchingboost { namespace parameter { namespace aux {

    struct template_keyword_base
    {
    };
}}} // namespace motionmatchingboost::parameter::aux

#include <boost/parameter/config.hpp>

#if defined(BOOST_PARAMETER_CAN_USE_MP11)
#include <type_traits>

namespace motionmatchingboost { namespace parameter { namespace aux {

    template <typename T>
    using is_template_keyword = ::std::is_base_of<
        ::motionmatchingboost::parameter::aux::template_keyword_base
      , typename ::std::remove_const<
            typename ::std::remove_reference<T>::type
        >::type
    >;
}}} // namespace motionmatchingboost::parameter::aux

#else   // !defined(BOOST_PARAMETER_CAN_USE_MP11)
#include <boost/mpl/bool.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/remove_const.hpp>

#if defined(BOOST_PARAMETER_HAS_PERFECT_FORWARDING)
#include <boost/type_traits/is_base_of.hpp>
#include <boost/type_traits/remove_reference.hpp>
#else
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/is_lvalue_reference.hpp>
#endif

namespace motionmatchingboost { namespace parameter { namespace aux {

#if !defined(BOOST_PARAMETER_HAS_PERFECT_FORWARDING)
    template <typename T>
    struct is_template_keyword_aux
      : ::motionmatchingboost::mpl::if_<
            ::motionmatchingboost::is_convertible<
                T*
              , ::motionmatchingboost::parameter::aux::template_keyword_base const*
            >
          , ::motionmatchingboost::mpl::true_
          , ::motionmatchingboost::mpl::false_
        >::type
    {
    };
#endif  // BOOST_PARAMETER_HAS_PERFECT_FORWARDING

    template <typename T>
    struct is_template_keyword
      : ::motionmatchingboost::mpl::if_<
#if defined(BOOST_PARAMETER_HAS_PERFECT_FORWARDING)
            // Cannot use is_convertible<> to check if T is derived from
            // template_keyword_base. -- Cromwell D. Enage
            ::motionmatchingboost::is_base_of<
                ::motionmatchingboost::parameter::aux::template_keyword_base
              , typename ::motionmatchingboost::remove_const<
                    typename ::motionmatchingboost::remove_reference<T>::type
                >::type
            >
          , ::motionmatchingboost::mpl::true_
          , ::motionmatchingboost::mpl::false_
#else
            ::motionmatchingboost::is_lvalue_reference<T>
          , ::motionmatchingboost::mpl::false_
          , ::motionmatchingboost::parameter::aux::is_template_keyword_aux<T>
#endif  // BOOST_PARAMETER_HAS_PERFECT_FORWARDING
        >::type
    {
    };
}}} // namespace motionmatchingboost::parameter::aux

#endif  // BOOST_PARAMETER_CAN_USE_MP11
#endif  // include guard

