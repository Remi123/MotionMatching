// Copyright David Abrahams 2005.
// Copyright Cromwell D. Enage 2017.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PARAMETER_AUX_TAG_DWA2005610_HPP
#define BOOST_PARAMETER_AUX_TAG_DWA2005610_HPP

#include <boost/parameter/aux_/unwrap_cv_reference.hpp>
#include <boost/parameter/aux_/tagged_argument.hpp>
#include <boost/parameter/config.hpp>

#if defined(BOOST_PARAMETER_CAN_USE_MP11) && \
    !BOOST_WORKAROUND(BOOST_MSVC, >= 1910)
// MSVC-14.1+ assigns rvalue references to tagged_argument instances
// instead of tagged_argument_rref instances with this code.
#include <boost/mp11/integral.hpp>
#include <boost/mp11/utility.hpp>
#include <type_traits>

namespace motionmatchingboost { namespace parameter { namespace aux { 

    template <typename Keyword, typename Arg>
    struct tag_if_lvalue_reference
    {
        using type = ::motionmatchingboost::parameter::aux::tagged_argument_list_of_1<
            ::motionmatchingboost::parameter::aux::tagged_argument<
                Keyword
              , typename ::motionmatchingboost::parameter::aux
                ::unwrap_cv_reference<Arg>::type
            >
        >;
    };

    template <typename Keyword, typename Arg>
    struct tag_if_scalar
    {
        using type = ::motionmatchingboost::parameter::aux::tagged_argument_list_of_1<
            ::motionmatchingboost::parameter::aux
            ::tagged_argument<Keyword,typename ::std::add_const<Arg>::type>
        >;
    };

    template <typename Keyword, typename Arg>
    using tag_if_otherwise = ::motionmatchingboost::mp11::mp_if<
        ::std::is_scalar<typename ::std::remove_const<Arg>::type>
      , ::motionmatchingboost::parameter::aux::tag_if_scalar<Keyword,Arg>
      , ::motionmatchingboost::mp11::mp_identity<
            ::motionmatchingboost::parameter::aux::tagged_argument_list_of_1<
                ::motionmatchingboost::parameter::aux::tagged_argument_rref<Keyword,Arg>
            >
        >
    >;

    template <typename Keyword, typename Arg>
    using tag = ::motionmatchingboost::mp11::mp_if<
        ::motionmatchingboost::mp11::mp_if<
            ::std::is_lvalue_reference<Arg>
          , ::motionmatchingboost::mp11::mp_true
          , ::motionmatchingboost::parameter::aux::is_cv_reference_wrapper<Arg>
        >
      , ::motionmatchingboost::parameter::aux::tag_if_lvalue_reference<Keyword,Arg>
      , ::motionmatchingboost::parameter::aux::tag_if_otherwise<Keyword,Arg>
    >;
}}} // namespace motionmatchingboost::parameter::aux_

#elif defined(BOOST_PARAMETER_HAS_PERFECT_FORWARDING)
#include <boost/mpl/bool.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/is_scalar.hpp>
#include <boost/type_traits/is_lvalue_reference.hpp>
#include <boost/type_traits/remove_const.hpp>

namespace motionmatchingboost { namespace parameter { namespace aux { 

    template <typename Keyword, typename ActualArg>
    struct tag
    {
        typedef typename ::motionmatchingboost::parameter::aux
        ::unwrap_cv_reference<ActualArg>::type Arg;
        typedef typename ::motionmatchingboost::add_const<Arg>::type ConstArg;
        typedef typename ::motionmatchingboost::remove_const<Arg>::type MutArg;
        typedef typename ::motionmatchingboost::mpl::eval_if<
            typename ::motionmatchingboost::mpl::if_<
                ::motionmatchingboost::is_lvalue_reference<ActualArg>
              , ::motionmatchingboost::mpl::true_
              , ::motionmatchingboost::parameter::aux::is_cv_reference_wrapper<ActualArg>
            >::type
          , ::motionmatchingboost::mpl::identity<
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
                ::motionmatchingboost::parameter::aux::tagged_argument_list_of_1<
#endif
                    ::motionmatchingboost::parameter::aux::tagged_argument<Keyword,Arg>
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
                >
#endif
            >
          , ::motionmatchingboost::mpl::if_<
                ::motionmatchingboost::is_scalar<MutArg>
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
              , ::motionmatchingboost::parameter::aux::tagged_argument_list_of_1<
                    ::motionmatchingboost::parameter::aux::tagged_argument<Keyword,ConstArg>
                >
              , ::motionmatchingboost::parameter::aux::tagged_argument_list_of_1<
                    ::motionmatchingboost::parameter::aux::tagged_argument_rref<Keyword,Arg>
                >
#else
              , ::motionmatchingboost::parameter::aux::tagged_argument<Keyword,ConstArg>
              , ::motionmatchingboost::parameter::aux::tagged_argument_rref<Keyword,Arg>
#endif
            >
        >::type type;
    };
}}} // namespace motionmatchingboost::parameter::aux_

#else   // !defined(BOOST_PARAMETER_HAS_PERFECT_FORWARDING)

namespace motionmatchingboost { namespace parameter { namespace aux { 

    template <
        typename Keyword
      , typename Arg
#if BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x564))
      , typename = typename ::motionmatchingboost::parameter::aux
        ::is_cv_reference_wrapper<Arg>::type
#endif
    >
    struct tag
    {
        typedef ::motionmatchingboost::parameter::aux::tagged_argument<
            Keyword
          , typename ::motionmatchingboost::parameter::aux::unwrap_cv_reference<Arg>::type
        > type;
    };
}}} // namespace motionmatchingboost::parameter::aux_

#if BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x564))
#include <boost/mpl/bool.hpp>
#include <boost/type_traits/remove_reference.hpp>

namespace motionmatchingboost { namespace parameter { namespace aux { 

    template <typename Keyword, typename Arg>
    struct tag<Keyword,Arg,::motionmatchingboost::mpl::false_>
    {
        typedef ::motionmatchingboost::parameter::aux::tagged_argument<
            Keyword
          , typename ::motionmatchingboost::remove_reference<Arg>::type
        > type;
    };
}}} // namespace motionmatchingboost::parameter::aux_

#endif  // Borland workarounds needed.
#endif  // MP11 or perfect forwarding support
#endif  // include guard

