// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef INDIRECT_TRAITS_DWA2002131_HPP
# define INDIRECT_TRAITS_DWA2002131_HPP
# include <boost/type_traits/integral_constant.hpp>
# include <boost/type_traits/is_function.hpp>
# include <boost/type_traits/is_reference.hpp>
# include <boost/type_traits/is_pointer.hpp>
# include <boost/type_traits/is_class.hpp>
# include <boost/type_traits/is_const.hpp>
# include <boost/type_traits/is_volatile.hpp>
# include <boost/type_traits/is_member_function_pointer.hpp>
# include <boost/type_traits/is_member_pointer.hpp>
# include <boost/type_traits/remove_cv.hpp>
# include <boost/type_traits/remove_reference.hpp>
# include <boost/type_traits/remove_pointer.hpp>

# include <boost/detail/workaround.hpp>
# include <boost/detail/select_type.hpp>


namespace motionmatchingboost { namespace detail {

namespace indirect_traits {

template <class T>
struct is_reference_to_const : motionmatchingboost::false_type
{
};

template <class T>
struct is_reference_to_const<T const&> : motionmatchingboost::true_type
{
};

#   if defined(BOOST_MSVC) && _MSC_FULL_VER <= 13102140 // vc7.01 alpha workaround
template<class T>
struct is_reference_to_const<T const volatile&> : motionmatchingboost::true_type
{
};
#   endif

template <class T>
struct is_reference_to_function : motionmatchingboost::false_type
{
};

template <class T>
struct is_reference_to_function<T&> : is_function<T>
{
};

template <class T>
struct is_pointer_to_function : motionmatchingboost::false_type
{
};

// There's no such thing as a pointer-to-cv-function, so we don't need
// specializations for those
template <class T>
struct is_pointer_to_function<T*> : is_function<T>
{
};

template <class T>
struct is_reference_to_member_function_pointer_impl : motionmatchingboost::false_type
{
};

template <class T>
struct is_reference_to_member_function_pointer_impl<T&>
    : is_member_function_pointer<typename remove_cv<T>::type>
{
};


template <class T>
struct is_reference_to_member_function_pointer
    : is_reference_to_member_function_pointer_impl<T>
{
};

template <class T>
struct is_reference_to_function_pointer_aux
    : motionmatchingboost::integral_constant<bool,
          is_reference<T>::value &&
          is_pointer_to_function<
              typename remove_cv<
                  typename remove_reference<T>::type
              >::type
          >::value
      >
{
    // There's no such thing as a pointer-to-cv-function, so we don't need specializations for those
};

template <class T>
struct is_reference_to_function_pointer
    : motionmatchingboost::detail::if_true<
          is_reference_to_function<T>::value
      >::template then<
          motionmatchingboost::false_type
        , is_reference_to_function_pointer_aux<T>
      >::type
{
};

template <class T>
struct is_reference_to_non_const
    : motionmatchingboost::integral_constant<bool,
          is_reference<T>::value &&
          !is_reference_to_const<T>::value
      >
{
};

template <class T>
struct is_reference_to_volatile : motionmatchingboost::false_type
{
};

template <class T>
struct is_reference_to_volatile<T volatile&> : motionmatchingboost::true_type
{
};

#   if defined(BOOST_MSVC) && _MSC_FULL_VER <= 13102140 // vc7.01 alpha workaround
template <class T>
struct is_reference_to_volatile<T const volatile&> : motionmatchingboost::true_type
{
};
#   endif


template <class T>
struct is_reference_to_pointer : motionmatchingboost::false_type
{
};

template <class T>
struct is_reference_to_pointer<T*&> : motionmatchingboost::true_type
{
};

template <class T>
struct is_reference_to_pointer<T* const&> : motionmatchingboost::true_type
{
};

template <class T>
struct is_reference_to_pointer<T* volatile&> : motionmatchingboost::true_type
{
};

template <class T>
struct is_reference_to_pointer<T* const volatile&> : motionmatchingboost::true_type
{
};

template <class T>
struct is_reference_to_class
    : motionmatchingboost::integral_constant<bool,
          is_reference<T>::value &&
          is_class<
              typename remove_cv<
                  typename remove_reference<T>::type
              >::type
          >::value
      >
{
};

template <class T>
struct is_pointer_to_class
    : motionmatchingboost::integral_constant<bool,
          is_pointer<T>::value &&
          is_class<
              typename remove_cv<
                  typename remove_pointer<T>::type
              >::type
          >::value
      >
{
};


}

using namespace indirect_traits;

}} // namespace motionmatchingboost::python::detail

#endif // INDIRECT_TRAITS_DWA2002131_HPP
