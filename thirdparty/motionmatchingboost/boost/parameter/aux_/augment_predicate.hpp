// Copyright Cromwell D. Enage 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PARAMETER_AUGMENT_PREDICATE_HPP
#define BOOST_PARAMETER_AUGMENT_PREDICATE_HPP

#include <boost/parameter/keyword_fwd.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/type_traits/is_lvalue_reference.hpp>
#include <boost/type_traits/is_scalar.hpp>
#include <boost/type_traits/is_same.hpp>

namespace motionmatchingboost { namespace parameter { namespace aux {

    template <typename V, typename R, typename Tag>
    struct augment_predicate_check_consume_ref
      : ::motionmatchingboost::mpl::eval_if<
            ::motionmatchingboost::is_scalar<V>
          , ::motionmatchingboost::mpl::true_
          , ::motionmatchingboost::mpl::eval_if<
                ::motionmatchingboost::is_same<
                    typename Tag::qualifier
                  , ::motionmatchingboost::parameter::consume_reference
                >
              , ::motionmatchingboost::mpl::if_<
                    ::motionmatchingboost::is_lvalue_reference<R>
                  , ::motionmatchingboost::mpl::false_
                  , ::motionmatchingboost::mpl::true_
                >
              , motionmatchingboost::mpl::true_
            >
        >::type
    {
    };
}}} // namespace motionmatchingboost::parameter::aux

#include <boost/type_traits/is_const.hpp>

namespace motionmatchingboost { namespace parameter { namespace aux {

    template <typename V, typename R, typename Tag>
    struct augment_predicate_check_out_ref
      : ::motionmatchingboost::mpl::eval_if<
            ::motionmatchingboost::is_same<
                typename Tag::qualifier
              , ::motionmatchingboost::parameter::out_reference
            >
          , ::motionmatchingboost::mpl::eval_if<
                ::motionmatchingboost::is_lvalue_reference<R>
              , ::motionmatchingboost::mpl::if_<
                    ::motionmatchingboost::is_const<V>
                  , ::motionmatchingboost::mpl::false_
                  , ::motionmatchingboost::mpl::true_
                >
              , ::motionmatchingboost::mpl::false_
            >
          , ::motionmatchingboost::mpl::true_
        >::type
    {
    };
}}} // namespace motionmatchingboost::parameter::aux

#include <boost/parameter/aux_/lambda_tag.hpp>
#include <boost/mpl/apply_wrap.hpp>
#include <boost/mpl/lambda.hpp>

namespace motionmatchingboost { namespace parameter { namespace aux {

    template <
        typename Predicate
      , typename R
      , typename Tag
      , typename T
      , typename Args
    >
    class augment_predicate
    {
        typedef typename ::motionmatchingboost::mpl::lambda<
            Predicate
          , ::motionmatchingboost::parameter::aux::lambda_tag
        >::type _actual_predicate;

     public:
        typedef typename ::motionmatchingboost::mpl::eval_if<
            typename ::motionmatchingboost::mpl::if_<
                ::motionmatchingboost::parameter::aux
                ::augment_predicate_check_consume_ref<T,R,Tag>
              , ::motionmatchingboost::parameter::aux
                ::augment_predicate_check_out_ref<T,R,Tag>
              , ::motionmatchingboost::mpl::false_
            >::type
          , ::motionmatchingboost::mpl::apply_wrap2<_actual_predicate,T,Args>
          , ::motionmatchingboost::mpl::false_
        >::type type;
    };
}}} // namespace motionmatchingboost::parameter::aux

#include <boost/parameter/config.hpp>

#if defined(BOOST_PARAMETER_CAN_USE_MP11)
#include <boost/mp11/integral.hpp>
#include <boost/mp11/utility.hpp>
#include <type_traits>

namespace motionmatchingboost { namespace parameter { namespace aux {

    template <typename V, typename R, typename Tag>
    using augment_predicate_check_consume_ref_mp11 = ::motionmatchingboost::mp11::mp_if<
        ::std::is_scalar<V>
      , ::motionmatchingboost::mp11::mp_true
      , ::motionmatchingboost::mp11::mp_if<
            ::std::is_same<
                typename Tag::qualifier
              , ::motionmatchingboost::parameter::consume_reference
            >
          , ::motionmatchingboost::mp11::mp_if<
                ::std::is_lvalue_reference<R>
              , ::motionmatchingboost::mp11::mp_false
              , ::motionmatchingboost::mp11::mp_true
            >
          , motionmatchingboost::mp11::mp_true
        >
    >;

    template <typename V, typename R, typename Tag>
    using augment_predicate_check_out_ref_mp11 = ::motionmatchingboost::mp11::mp_if<
        ::std::is_same<
            typename Tag::qualifier
          , ::motionmatchingboost::parameter::out_reference
        >
      , ::motionmatchingboost::mp11::mp_if<
            ::std::is_lvalue_reference<R>
          , ::motionmatchingboost::mp11::mp_if<
                ::std::is_const<V>
              , ::motionmatchingboost::mp11::mp_false
              , ::motionmatchingboost::mp11::mp_true
            >
          , ::motionmatchingboost::mp11::mp_false
        >
      , ::motionmatchingboost::mp11::mp_true
    >;
}}} // namespace motionmatchingboost::parameter::aux

#include <boost/mp11/list.hpp>

namespace motionmatchingboost { namespace parameter { namespace aux {

    template <
        typename Predicate
      , typename R
      , typename Tag
      , typename T
      , typename Args
    >
    struct augment_predicate_mp11_impl
    {
        using type = ::motionmatchingboost::mp11::mp_if<
            ::motionmatchingboost::mp11::mp_if<
                ::motionmatchingboost::parameter::aux
                ::augment_predicate_check_consume_ref_mp11<T,R,Tag>
              , ::motionmatchingboost::parameter::aux
                ::augment_predicate_check_out_ref_mp11<T,R,Tag>
              , ::motionmatchingboost::mp11::mp_false
            >
          , ::motionmatchingboost::mp11
            ::mp_apply_q<Predicate,::motionmatchingboost::mp11::mp_list<T,Args> >
          , ::motionmatchingboost::mp11::mp_false
        >;
    };
}}} // namespace motionmatchingboost::parameter::aux

#include <boost/parameter/aux_/has_nested_template_fn.hpp>

namespace motionmatchingboost { namespace parameter { namespace aux {

    template <
        typename Predicate
      , typename R
      , typename Tag
      , typename T
      , typename Args
    >
    using augment_predicate_mp11 = ::motionmatchingboost::mp11::mp_if<
        ::motionmatchingboost::parameter::aux::has_nested_template_fn<Predicate>
      , ::motionmatchingboost::parameter::aux
        ::augment_predicate_mp11_impl<Predicate,R,Tag,T,Args>
      , ::motionmatchingboost::parameter::aux
        ::augment_predicate<Predicate,R,Tag,T,Args>
    >;
}}} // namespace motionmatchingboost::parameter::aux

#endif  // BOOST_PARAMETER_CAN_USE_MP11
#endif  // include guard

