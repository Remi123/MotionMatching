// Copyright Daniel Wallin 2006.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PARAMETER_AUX_NAME_HPP
#define BOOST_PARAMETER_AUX_NAME_HPP

namespace motionmatchingboost { namespace parameter { namespace aux {

    struct name_tag_base
    {
    };

    template <typename Tag>
    struct name_tag
    {
    };
}}} // namespace motionmatchingboost::parameter::aux

#include <boost/mpl/bool.hpp>

namespace motionmatchingboost { namespace parameter { namespace aux {

    template <typename T>
    struct is_name_tag : ::motionmatchingboost::mpl::false_
    {
    };
}}} // namespace motionmatchingboost::parameter::aux

#include <boost/parameter/value_type.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/config.hpp>
#include <boost/config/workaround.hpp>

#if !defined(BOOST_NO_SFINAE) && \
    !BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x592))
#include <boost/parameter/aux_/lambda_tag.hpp>
#include <boost/mpl/lambda.hpp>
#include <boost/mpl/bind.hpp>
#include <boost/mpl/quote.hpp>
#include <boost/core/enable_if.hpp>

namespace motionmatchingboost { namespace mpl {

    template <typename T>
    struct lambda<
        T
      , typename ::motionmatchingboost::enable_if<
            ::motionmatchingboost::parameter::aux::is_name_tag<T>
          , ::motionmatchingboost::parameter::aux::lambda_tag
        >::type
    >
    {
        typedef ::motionmatchingboost::mpl::true_ is_le;
        typedef ::motionmatchingboost::mpl::bind3<
            ::motionmatchingboost::mpl::quote3< ::motionmatchingboost::parameter::value_type>
          , ::motionmatchingboost::mpl::arg<2>
          , T
          , void
        > result_;
        typedef result_ type;
    };
}} // namespace motionmatchingboost::mpl

#endif  // SFINAE enabled, not Borland.

#include <boost/parameter/aux_/void.hpp>

#define BOOST_PARAMETER_TAG_PLACEHOLDER_TYPE(tag)                            \
    ::motionmatchingboost::parameter::value_type<                                          \
        ::motionmatchingboost::mpl::_2,tag,::motionmatchingboost::parameter::void_                       \
    >
/**/

#if defined(BOOST_PARAMETER_CAN_USE_MP11)
#define BOOST_PARAMETER_TAG_MP11_PLACEHOLDER_VALUE(name, tag)                \
    template <typename ArgumentPack>                                         \
    using name = typename ::motionmatchingboost::parameter                                 \
    ::value_type<ArgumentPack,tag,::motionmatchingboost::parameter::void_>::type
/**/

#include <boost/parameter/binding.hpp>

#define BOOST_PARAMETER_TAG_MP11_PLACEHOLDER_BINDING(name, tag)              \
    template <typename ArgumentPack>                                         \
    using name = typename ::motionmatchingboost::parameter                                 \
    ::binding<ArgumentPack,tag,::motionmatchingboost::parameter::void_>::type
/**/

#endif  // BOOST_PARAMETER_CAN_USE_MP11
#endif  // include guard

