
//  (C) Copyright Steve Cleary, Beman Dawes, Howard Hinnant & John Maddock 2000.
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#ifndef BOOST_TT_HAS_NOTHROW_DESTRUCTOR_HPP_INCLUDED
#define BOOST_TT_HAS_NOTHROW_DESTRUCTOR_HPP_INCLUDED

#include <boost/type_traits/has_trivial_destructor.hpp>

#if !defined(BOOST_NO_CXX11_NOEXCEPT) && !defined(__SUNPRO_CC) && !(defined(BOOST_MSVC) && (_MSC_FULL_VER < 190023506))

#include <boost/type_traits/declval.hpp>
#include <boost/type_traits/is_destructible.hpp>
#include <boost/type_traits/is_complete.hpp>
#include <boost/static_assert.hpp>

namespace motionmatchingboost{

   namespace detail{

      template <class T, bool b>
      struct has_nothrow_destructor_imp : public motionmatchingboost::integral_constant<bool, false>{};
      template <class T>
      struct has_nothrow_destructor_imp<T, true> : public motionmatchingboost::integral_constant<bool, noexcept(motionmatchingboost::declval<T*&>()->~T())>{};

   }

   template <class T> struct has_nothrow_destructor : public detail::has_nothrow_destructor_imp<T, motionmatchingboost::is_destructible<T>::value>
   {
      BOOST_STATIC_ASSERT_MSG(motionmatchingboost::is_complete<T>::value, "Arguments to has_nothrow_destructor must be complete types");
   };
   template <class T, std::size_t N> struct has_nothrow_destructor<T[N]> : public has_nothrow_destructor<T>
   {
      BOOST_STATIC_ASSERT_MSG(motionmatchingboost::is_complete<T>::value, "Arguments to has_nothrow_destructor must be complete types");
   };
   template <class T> struct has_nothrow_destructor<T&> : public integral_constant<bool, false>{};
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) 
   template <class T> struct has_nothrow_destructor<T&&> : public integral_constant<bool, false>{};
#endif
   template <> struct has_nothrow_destructor<void> : public false_type {};
}
#else

namespace motionmatchingboost {

template <class T> struct has_nothrow_destructor : public ::motionmatchingboost::has_trivial_destructor<T> {};

} // namespace motionmatchingboost

#endif

#endif // BOOST_TT_HAS_NOTHROW_DESTRUCTOR_HPP_INCLUDED
