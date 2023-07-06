// Copyright Daniel Wallin 2006.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PARAMETER_VALUE_TYPE_060921_HPP
#define BOOST_PARAMETER_VALUE_TYPE_060921_HPP

#include <boost/parameter/aux_/void.hpp>
#include <boost/parameter/config.hpp>

#if defined(BOOST_PARAMETER_CAN_USE_MP11)
#include <boost/mp11/integral.hpp>
#include <boost/mp11/list.hpp>
#include <boost/mp11/utility.hpp>
#include <type_traits>
#else
#include <boost/mpl/bool.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/apply_wrap.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>
#endif

namespace motionmatchingboost { namespace parameter { 

    // A metafunction that, given an argument pack, returns the value type
    // of the parameter identified by the given keyword.  If no such parameter
    // has been specified, returns Default

    template <typename Parameters, typename Keyword, typename Default>
    struct value_type0
    {
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
        using type = ::motionmatchingboost::mp11::mp_apply_q<
            typename Parameters::binding
          , ::motionmatchingboost::mp11::mp_list<Keyword,Default,::motionmatchingboost::mp11::mp_false>
        >;

        static_assert(
            ::motionmatchingboost::mp11::mp_if<
                ::std::is_same<Default,::motionmatchingboost::parameter::void_>
              , ::motionmatchingboost::mp11::mp_if<
                    ::std::is_same<type,::motionmatchingboost::parameter::void_>
                  , ::motionmatchingboost::mp11::mp_false
                  , ::motionmatchingboost::mp11::mp_true
                >
              , ::motionmatchingboost::mp11::mp_true
            >::value
          , "required parameters must not result in void_ type"
        );
#else   // !defined(BOOST_PARAMETER_CAN_USE_MP11)
        typedef typename ::motionmatchingboost::mpl::apply_wrap3<
            typename Parameters::binding
          , Keyword
          , Default
          , ::motionmatchingboost::mpl::false_
        >::type type;

        BOOST_MPL_ASSERT((
            typename ::motionmatchingboost::mpl::eval_if<
                ::motionmatchingboost::is_same<Default,::motionmatchingboost::parameter::void_>
              , ::motionmatchingboost::mpl::if_<
                    ::motionmatchingboost::is_same<type,::motionmatchingboost::parameter::void_>
                  , ::motionmatchingboost::mpl::false_
                  , ::motionmatchingboost::mpl::true_
                >
              , ::motionmatchingboost::mpl::true_
            >::type
        ));
#endif  // BOOST_PARAMETER_CAN_USE_MP11
    };

#if defined(BOOST_PARAMETER_CAN_USE_MP11)
    template <typename Placeholder, typename Keyword, typename Default>
    struct value_type1
    {
        using type = ::motionmatchingboost::mp11::mp_apply_q<
            Placeholder
          , ::motionmatchingboost::mp11::mp_list<Keyword,Default,::motionmatchingboost::mp11::mp_false>
        >;

        static_assert(
            ::motionmatchingboost::mp11::mp_if<
                ::std::is_same<Default,::motionmatchingboost::parameter::void_>
              , ::motionmatchingboost::mp11::mp_if<
                    ::std::is_same<type,::motionmatchingboost::parameter::void_>
                  , ::motionmatchingboost::mp11::mp_false
                  , ::motionmatchingboost::mp11::mp_true
                >
              , ::motionmatchingboost::mp11::mp_true
            >::value
          , "required parameters must not result in void_ type"
        );
    };
#endif  // BOOST_PARAMETER_CAN_USE_MP11
}} // namespace motionmatchingboost::parameter

#include <boost/parameter/aux_/is_placeholder.hpp>

namespace motionmatchingboost { namespace parameter { 

    template <
        typename Parameters
      , typename Keyword
      , typename Default = ::motionmatchingboost::parameter::void_
    >
    struct value_type
#if !defined(BOOST_PARAMETER_CAN_USE_MP11)
      : ::motionmatchingboost::mpl::eval_if<
            ::motionmatchingboost::parameter::aux::is_mpl_placeholder<Parameters>
          , ::motionmatchingboost::mpl::identity<int>
          , ::motionmatchingboost::parameter::value_type0<Parameters,Keyword,Default>
        >
#endif
    {
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
        using type = typename ::motionmatchingboost::mp11::mp_if<
            ::motionmatchingboost::parameter::aux::is_mpl_placeholder<Parameters>
          , ::motionmatchingboost::mp11::mp_identity<int>
          , ::motionmatchingboost::mp11::mp_if<
                ::motionmatchingboost::parameter::aux::is_mp11_placeholder<Parameters>
              , ::motionmatchingboost::parameter::value_type1<Parameters,Keyword,Default>
              , ::motionmatchingboost::parameter::value_type0<Parameters,Keyword,Default>
            >
        >::type;
#endif
    };
}} // namespace motionmatchingboost::parameter

#include <boost/parameter/aux_/result_of0.hpp>

namespace motionmatchingboost { namespace parameter { 

    // A metafunction that, given an argument pack, returns the value type
    // of the parameter identified by the given keyword.  If no such parameter
    // has been specified, returns the type returned by invoking DefaultFn
    template <typename Parameters, typename Keyword, typename DefaultFn>
    struct lazy_value_type
    {
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
        using type = ::motionmatchingboost::mp11::mp_apply_q<
            typename Parameters::binding
          , ::motionmatchingboost::mp11::mp_list<
                Keyword
              , typename ::motionmatchingboost::parameter::aux::result_of0<DefaultFn>::type
              , ::motionmatchingboost::mp11::mp_false
            >
        >;
#else
        typedef typename ::motionmatchingboost::mpl::apply_wrap3<
            typename Parameters::binding
          , Keyword
          , typename ::motionmatchingboost::parameter::aux::result_of0<DefaultFn>::type
          , ::motionmatchingboost::mpl::false_
        >::type type;
#endif  // BOOST_PARAMETER_CAN_USE_MP11
    };
}} // namespace motionmatchingboost::parameter

#endif  // include guard

