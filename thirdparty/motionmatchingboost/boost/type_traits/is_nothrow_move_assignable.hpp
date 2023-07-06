
//  (C) Copyright Steve Cleary, Beman Dawes, Howard Hinnant & John Maddock 2000.
//  (C) Copyright Eric Friedman 2002-2003.
//  (C) Copyright Antony Polukhin 2013.
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#ifndef BOOST_TT_IS_NOTHROW_MOVE_ASSIGNABLE_HPP_INCLUDED
#define BOOST_TT_IS_NOTHROW_MOVE_ASSIGNABLE_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/type_traits/has_trivial_move_assign.hpp>
#include <boost/type_traits/has_nothrow_assign.hpp>
#include <boost/type_traits/is_array.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <boost/type_traits/enable_if.hpp>
#include <boost/type_traits/declval.hpp>
#include <boost/type_traits/is_complete.hpp>
#include <boost/static_assert.hpp>

namespace motionmatchingboost {

#ifdef BOOST_IS_NOTHROW_MOVE_ASSIGN

template <class T>
struct is_nothrow_move_assignable : public integral_constant<bool, BOOST_IS_NOTHROW_MOVE_ASSIGN(T)>
{
   BOOST_STATIC_ASSERT_MSG(motionmatchingboost::is_complete<T>::value, "Arguments to is_nothrow_move_assignable must be complete types");
};
template <class T> struct is_nothrow_move_assignable<T const> : public false_type{};
template <class T> struct is_nothrow_move_assignable<T volatile> : public false_type{};
template <class T> struct is_nothrow_move_assignable<T const volatile> : public false_type{};
template <class T> struct is_nothrow_move_assignable<T&> : public false_type{};
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) 
template <class T> struct is_nothrow_move_assignable<T&&> : public false_type{};
#endif

#elif !defined(BOOST_NO_CXX11_NOEXCEPT) && !defined(BOOST_NO_SFINAE_EXPR) && !BOOST_WORKAROUND(BOOST_GCC_VERSION, < 40700)

namespace detail{

template <class T, class Enable = void>
struct false_or_cpp11_noexcept_move_assignable: public ::motionmatchingboost::false_type {};

template <class T>
struct false_or_cpp11_noexcept_move_assignable <
        T,
        typename ::motionmatchingboost::enable_if_<sizeof(T) && BOOST_NOEXCEPT_EXPR(::motionmatchingboost::declval<T&>() = ::motionmatchingboost::declval<T>())>::type
    > : public ::motionmatchingboost::integral_constant<bool, BOOST_NOEXCEPT_EXPR(::motionmatchingboost::declval<T&>() = ::motionmatchingboost::declval<T>())>
{};

}

template <class T>
struct is_nothrow_move_assignable : public integral_constant<bool, ::motionmatchingboost::detail::false_or_cpp11_noexcept_move_assignable<T>::value>
{
   BOOST_STATIC_ASSERT_MSG(motionmatchingboost::is_complete<T>::value, "Arguments to is_nothrow_move_assignable must be complete types");
};

template <class T> struct is_nothrow_move_assignable<T const> : public ::motionmatchingboost::false_type {};
template <class T> struct is_nothrow_move_assignable<T const volatile> : public ::motionmatchingboost::false_type{};
template <class T> struct is_nothrow_move_assignable<T volatile> : public ::motionmatchingboost::false_type{};
template <class T> struct is_nothrow_move_assignable<T&> : public ::motionmatchingboost::false_type{};
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
template <class T> struct is_nothrow_move_assignable<T&&> : public ::motionmatchingboost::false_type{};
#endif

#else

template <class T>
struct is_nothrow_move_assignable : public integral_constant<bool,
   (::motionmatchingboost::has_trivial_move_assign<T>::value || ::motionmatchingboost::has_nothrow_assign<T>::value) &&  ! ::motionmatchingboost::is_array<T>::value>
{
   BOOST_STATIC_ASSERT_MSG(motionmatchingboost::is_complete<T>::value, "Arguments to is_nothrow_move_assignable must be complete types");
};

#endif


template <> struct is_nothrow_move_assignable<void> : public false_type{};
#ifndef BOOST_NO_CV_VOID_SPECIALIZATIONS
template <> struct is_nothrow_move_assignable<void const> : public false_type{};
template <> struct is_nothrow_move_assignable<void const volatile> : public false_type{};
template <> struct is_nothrow_move_assignable<void volatile> : public false_type{};
#endif

} // namespace motionmatchingboost

#endif // BOOST_TT_IS_NOTHROW_MOVE_ASSIGNABLE_HPP_INCLUDED
