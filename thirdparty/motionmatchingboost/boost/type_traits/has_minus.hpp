//  (C) Copyright 2009-2011 Frederic Bron.
//
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#ifndef BOOST_TT_HAS_MINUS_HPP_INCLUDED
#define BOOST_TT_HAS_MINUS_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/type_traits/detail/config.hpp>

// cannot include this header without getting warnings of the kind:
// gcc:
//    warning: value computed is not used
//    warning: comparison between signed and unsigned integer expressions
// msvc:
//    warning C4018: '<' : signed/unsigned mismatch
//    warning C4244: '+=' : conversion from 'double' to 'char', possible loss of data
//    warning C4547: '*' : operator before comma has no effect; expected operator with side-effect
//    warning C4800: 'int' : forcing value to bool 'true' or 'false' (performance warning)
//    warning C4804: '<' : unsafe use of type 'bool' in operation
//    warning C4805: '==' : unsafe mix of type 'bool' and type 'char' in operation
// cannot find another implementation -> declared as system header to suppress these warnings.
#if defined(__GNUC__)
#   pragma GCC system_header
#elif defined(BOOST_MSVC)
#   pragma warning ( push )
#   pragma warning ( disable : 4018 4244 4547 4800 4804 4805 4913 4133)
#   if BOOST_WORKAROUND(BOOST_MSVC_FULL_VER, >= 140050000)
#       pragma warning ( disable : 6334)
#   endif
#endif

#if defined(BOOST_TT_HAS_ACCURATE_BINARY_OPERATOR_DETECTION)

#include <boost/type_traits/integral_constant.hpp>
#include <boost/type_traits/make_void.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/is_void.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <utility>

namespace motionmatchingboost
{

   namespace binary_op_detail {

      struct dont_care;

      template <class T, class U, class Ret, class = void>
      struct has_minus_ret_imp : public motionmatchingboost::false_type {};

      template <class T, class U, class Ret>
      struct has_minus_ret_imp<T, U, Ret, typename motionmatchingboost::make_void<decltype(std::declval<typename add_reference<T>::type>() - std::declval<typename add_reference<U>::type>())>::type>
         : public motionmatchingboost::integral_constant<bool, ::motionmatchingboost::is_convertible<decltype(std::declval<typename add_reference<T>::type>() - std::declval<typename add_reference<U>::type>()), Ret>::value> {};

      template <class T, class U, class = void >
      struct has_minus_void_imp : public motionmatchingboost::false_type {};

      template <class T, class U>
      struct has_minus_void_imp<T, U, typename motionmatchingboost::make_void<decltype(std::declval<typename add_reference<T>::type>() - std::declval<typename add_reference<U>::type>())>::type>
         : public motionmatchingboost::integral_constant<bool, ::motionmatchingboost::is_void<decltype(std::declval<typename add_reference<T>::type>() - std::declval<typename add_reference<U>::type>())>::value> {};

      template <class T, class U, class = void>
      struct has_minus_dc_imp : public motionmatchingboost::false_type {};

      template <class T, class U>
      struct has_minus_dc_imp<T, U, typename motionmatchingboost::make_void<decltype(std::declval<typename add_reference<T>::type>() - std::declval<typename add_reference<U>::type>())>::type>
         : public motionmatchingboost::true_type {};

      template <class T, class U, class Ret>
      struct has_minus_ret_filter : public motionmatchingboost::binary_op_detail::has_minus_ret_imp <T, U, Ret> {};
      template <class T, class U>
      struct has_minus_ret_filter<T, U, void> : public motionmatchingboost::binary_op_detail::has_minus_void_imp <T, U> {};
      template <class T, class U>
      struct has_minus_ret_filter<T, U, motionmatchingboost::binary_op_detail::dont_care> : public motionmatchingboost::binary_op_detail::has_minus_dc_imp <T, U> {};

      template <class T, class U, class Ret, bool b>
      struct has_minus_void_ptr_filter : public motionmatchingboost::binary_op_detail::has_minus_ret_filter <T, U, Ret> {};
      template <class T, class U, class Ret>
      struct has_minus_void_ptr_filter<T, U, Ret, true> : public motionmatchingboost::false_type {};

   }

   template <class T, class U = T, class Ret = motionmatchingboost::binary_op_detail::dont_care>
   struct has_minus : 
      public motionmatchingboost::binary_op_detail::has_minus_void_ptr_filter<
         T, U, Ret, 
         motionmatchingboost::is_void<typename remove_pointer<typename remove_reference<T>::type>::type>::value 
         || motionmatchingboost::is_void<typename remove_pointer<typename remove_reference<U>::type>::type>::value> {};


}

#else


#define BOOST_TT_TRAIT_NAME has_minus
#define BOOST_TT_TRAIT_OP -
#define BOOST_TT_FORBIDDEN_IF\
   (\
      /* Lhs==pointer and Rhs==fundamental and Rhs!=integral */\
      (\
         ::motionmatchingboost::is_pointer< Lhs_noref >::value && \
         ::motionmatchingboost::is_fundamental< Rhs_nocv >::value && \
         (!  ::motionmatchingboost::is_integral< Rhs_noref >::value )\
      ) || \
      /* Lhs==void* and (Rhs==fundamental or Rhs==pointer) */\
      (\
         ::motionmatchingboost::is_pointer< Lhs_noref >::value && \
         ::motionmatchingboost::is_void< Lhs_noptr >::value && \
         ( \
            ::motionmatchingboost::is_fundamental< Rhs_nocv >::value || \
            ::motionmatchingboost::is_pointer< Rhs_noref >::value\
          )\
      ) || \
      /* Rhs==void* and (Lhs==fundamental or Lhs==pointer) */\
      (\
         ::motionmatchingboost::is_pointer< Rhs_noref >::value && \
         ::motionmatchingboost::is_void< Rhs_noptr >::value && \
         (\
            ::motionmatchingboost::is_fundamental< Lhs_nocv >::value || \
            ::motionmatchingboost::is_pointer< Lhs_noref >::value\
          )\
      ) ||\
      /* Lhs=fundamental and Rhs=pointer */\
      (\
         ::motionmatchingboost::is_fundamental< Lhs_nocv >::value && \
         ::motionmatchingboost::is_pointer< Rhs_noref >::value\
      ) ||\
      /* two different pointers */\
      (\
         ::motionmatchingboost::is_pointer< Lhs_noref >::value && \
         ::motionmatchingboost::is_pointer< Rhs_noref >::value && \
         (!  ::motionmatchingboost::is_same< Lhs_nocv, Rhs_nocv >::value )\
      )\
      )

#define BOOST_TT_FORBIDDEN_IF_NEW (motionmatchingboost::is_void<typename remove_pointer<typename motionmatchingboost::remove_reference<T>::type>::type>::value || motionmatchingboost::is_void<typename remove_pointer<typename motionmatchingboost::remove_reference<U>::type>::type>::value)

#include <boost/type_traits/detail/has_binary_operator.hpp>

#undef BOOST_TT_TRAIT_NAME
#undef BOOST_TT_TRAIT_OP
#undef BOOST_TT_FORBIDDEN_IF

#endif

#if defined(BOOST_MSVC)
#   pragma warning (pop)
#endif

#endif
