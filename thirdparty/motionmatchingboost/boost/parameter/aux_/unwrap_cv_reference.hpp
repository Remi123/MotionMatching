// Copyright Daniel Wallin, David Abrahams 2005.
// Copyright Cromwell D. Enage 2017.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PARAMETER_AUX_UNWRAP_CV_REFERENCE_HPP
#define BOOST_PARAMETER_AUX_UNWRAP_CV_REFERENCE_HPP

namespace motionmatchingboost {

    template <typename T>
    class reference_wrapper;
} // namespace motionmatchingboost

#include <boost/parameter/aux_/yesno.hpp>

namespace motionmatchingboost { namespace parameter { namespace aux {

    //
    // reference_wrapper support -- if perfect forwarding is unsupported,
    // then when passing arguments positionally by non-const reference,
    // we ask users of named parameter interfaces to use ref(x) to wrap them.
    //

    template <typename U>
    ::motionmatchingboost::parameter::aux::yes_tag
        is_cv_reference_wrapper_check(
            ::motionmatchingboost::reference_wrapper<U> const volatile*
        );

    ::motionmatchingboost::parameter::aux::no_tag is_cv_reference_wrapper_check(...);
}}} // namespace motionmatchingboost::parameter::aux

#include <boost/parameter/config.hpp>

#if !defined(BOOST_NO_CXX11_HDR_FUNCTIONAL)
#include <functional>

namespace motionmatchingboost { namespace parameter { namespace aux {

    // Support for std::ref(x) -- Cromwell D. Enage
    template <typename U>
    ::motionmatchingboost::parameter::aux::yes_tag
        is_cv_reference_wrapper_check(
            ::std::reference_wrapper<U> const volatile*
        );
}}} // namespace motionmatchingboost::parameter::aux
#endif

#include <boost/parameter/aux_/preprocessor/nullptr.hpp>

#if defined(BOOST_PARAMETER_CAN_USE_MP11) && !( \
        BOOST_WORKAROUND(BOOST_MSVC, >= 1900) && \
        BOOST_WORKAROUND(BOOST_MSVC, < 1910) \
    )
#include <boost/mp11/integral.hpp>
#include <boost/mp11/utility.hpp>
#include <type_traits>
#else   // !defined(BOOST_PARAMETER_CAN_USE_MP11) || MSVC-14.0
#include <boost/mpl/bool.hpp>
#include <boost/type_traits/remove_reference.hpp>
#if !BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x564)) && \
    !BOOST_WORKAROUND(BOOST_GCC, < 40000)
#include <boost/mpl/eval_if.hpp>
#endif
#endif  // BOOST_PARAMETER_CAN_USE_MP11 && not MSVC-14.0

namespace motionmatchingboost { namespace parameter { namespace aux {

#if defined(BOOST_PARAMETER_CAN_USE_MP11) && !( \
        BOOST_WORKAROUND(BOOST_MSVC, >= 1900) && \
        BOOST_WORKAROUND(BOOST_MSVC, < 1910) \
    )
    // This metafunction returns mp11::mp_true if T is of type
    // reference_wrapper<U> cv.
    template <typename T>
    using is_cv_reference_wrapper = ::motionmatchingboost::mp11::mp_bool<
        sizeof(
            ::motionmatchingboost::parameter::aux::is_cv_reference_wrapper_check(
                static_cast<
                    typename ::std::remove_reference<T>::type*
                >(BOOST_PARAMETER_AUX_PP_NULLPTR)
            )
        ) == sizeof(::motionmatchingboost::parameter::aux::yes_tag)
    >;

    // Needed for unwrap_cv_reference below. T might be const, so
    // mp_eval_if<> might fail because of deriving from T const on EDG.
    template <typename T>
    using unwrap_cv_reference_impl = typename ::std::remove_reference<T>::type;

    // Produces the unwrapped type to hold a reference to in
    // tagged_argument<>.  Can't use motionmatchingboost::unwrap_reference<> here
    // because it doesn't handle the case where T = reference_wrapper<U> cv.
    template <typename T>
    using unwrap_cv_reference = ::motionmatchingboost::mp11::mp_eval_if<
        ::motionmatchingboost::parameter::aux::is_cv_reference_wrapper<T>
      , ::motionmatchingboost::parameter::aux::unwrap_cv_reference_impl<T>
      , ::std::remove_reference
      , T
    >;
#else  // !defined(BOOST_PARAMETER_CAN_USE_MP11) || MSVC-14.0
    // This metafunction returns mpl::true_ if T is of type
    // reference_wrapper<U> cv.
    template <typename T>
    struct is_cv_reference_wrapper
    {
        BOOST_STATIC_CONSTANT(
            bool, value = (
                sizeof(
                    ::motionmatchingboost::parameter::aux::is_cv_reference_wrapper_check(
                        static_cast<
                            typename ::motionmatchingboost::remove_reference<T>::type*
                        >(BOOST_PARAMETER_AUX_PP_NULLPTR)
                    )
                ) == sizeof(::motionmatchingboost::parameter::aux::yes_tag)
            )
        );

        typedef motionmatchingboost::mpl::bool_<
#if BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x564))
            is_cv_reference_wrapper::
#endif 
        value> type;
    };

#if BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x564)) || \
    BOOST_WORKAROUND(BOOST_GCC, < 40000)
    template <
        typename T
      , typename = typename ::motionmatchingboost::parameter::aux
        ::is_cv_reference_wrapper<T>::type
    >
    struct unwrap_cv_reference : ::motionmatchingboost::remove_reference<T>
    {
    };

    template <typename T>
    struct unwrap_cv_reference<T const,::motionmatchingboost::mpl::false_>
    {
        typedef T const type;
    };

    template <typename T>
    struct unwrap_cv_reference<T,::motionmatchingboost::mpl::true_> : T
    {
    };
#else   // no Borland or GCC 3- workarounds needed
    // Needed for unwrap_cv_reference below. T might be const, so
    // eval_if<> might fail because of deriving from T const on EDG.
    template <typename T>
    struct unwrap_cv_reference_impl : ::motionmatchingboost::remove_reference<T>::type
    {
    };

    // Produces the unwrapped type to hold a reference to in
    // tagged_argument<>.  Can't use motionmatchingboost::unwrap_reference<> here
    // because it doesn't handle the case where T = reference_wrapper<U> cv.
    template <typename T>
    struct unwrap_cv_reference
      : ::motionmatchingboost::mpl::eval_if<
            ::motionmatchingboost::parameter::aux::is_cv_reference_wrapper<T>
          , ::motionmatchingboost::parameter::aux::unwrap_cv_reference_impl<T>
          , ::motionmatchingboost::remove_reference<T>
        >
    {
    };
#endif  // Borland or GCC 3- workarounds needed
#endif  // BOOST_PARAMETER_CAN_USE_MP11 && not MSVC-14.0
}}} // namespace motionmatchingboost::parameter::aux

#endif  // include guard

