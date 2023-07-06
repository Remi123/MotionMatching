//  (C) Copyright 2009-2011 Frederic Bron.
//
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#ifndef BOOST_TT_HAS_POST_INCREMENT_HPP_INCLUDED
#define BOOST_TT_HAS_POST_INCREMENT_HPP_INCLUDED

#include <boost/type_traits/is_array.hpp>

#define BOOST_TT_TRAIT_NAME has_post_increment
#define BOOST_TT_TRAIT_OP ++
#define BOOST_TT_FORBIDDEN_IF\
   (\
      /* bool */\
      ::motionmatchingboost::is_same< bool, Lhs_nocv >::value || \
      /* void* */\
      (\
         ::motionmatchingboost::is_pointer< Lhs_noref >::value && \
         ::motionmatchingboost::is_void< Lhs_noptr >::value\
      ) || \
      /* (fundamental or pointer) and const */\
      (\
         ( \
            ::motionmatchingboost::is_fundamental< Lhs_nocv >::value || \
            ::motionmatchingboost::is_pointer< Lhs_noref >::value\
         ) && \
         ::motionmatchingboost::is_const< Lhs_noref >::value\
      )||\
      /* Arrays */ \
      ::motionmatchingboost::is_array<Lhs_noref>::value\
      )


#include <boost/type_traits/detail/has_postfix_operator.hpp>

#undef BOOST_TT_TRAIT_NAME
#undef BOOST_TT_TRAIT_OP
#undef BOOST_TT_FORBIDDEN_IF

#if defined(BOOST_TT_HAS_ACCURATE_BINARY_OPERATOR_DETECTION)

namespace motionmatchingboost {

   template <class R>
   struct has_post_increment<bool, R> : public false_type {};
   template <>
   struct has_post_increment<bool, motionmatchingboost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_post_increment<bool, void> : public false_type {};

   template <class R>
   struct has_post_increment<bool&, R> : public false_type {};
   template <>
   struct has_post_increment<bool&, motionmatchingboost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_post_increment<bool&, void> : public false_type {};

}

#endif
#endif
