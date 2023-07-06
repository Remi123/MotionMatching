//  (C) Copyright 2009-2011 Frederic Bron.
//
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#ifndef BOOST_TT_HAS_GREATER_HPP_INCLUDED
#define BOOST_TT_HAS_GREATER_HPP_INCLUDED

#define BOOST_TT_TRAIT_NAME has_greater
#define BOOST_TT_TRAIT_OP >
#define BOOST_TT_FORBIDDEN_IF\
   (\
      /* Lhs==pointer and Rhs==fundamental */\
      (\
         ::motionmatchingboost::is_pointer< Lhs_noref >::value && \
         ::motionmatchingboost::is_fundamental< Rhs_nocv >::value\
      ) || \
      /* Rhs==pointer and Lhs==fundamental */\
      (\
         ::motionmatchingboost::is_pointer< Rhs_noref >::value && \
         ::motionmatchingboost::is_fundamental< Lhs_nocv >::value\
      ) || \
      /* Lhs==pointer and Rhs==pointer and Lhs!=base(Rhs) and Rhs!=base(Lhs) and Lhs!=void* and Rhs!=void* */\
      (\
         ::motionmatchingboost::is_pointer< Lhs_noref >::value && \
         ::motionmatchingboost::is_pointer< Rhs_noref >::value && \
         (! \
            ( \
               ::motionmatchingboost::is_base_of< Lhs_noptr, Rhs_noptr >::value || \
               ::motionmatchingboost::is_base_of< Rhs_noptr, Lhs_noptr >::value || \
               ::motionmatchingboost::is_same< Lhs_noptr, Rhs_noptr >::value || \
               ::motionmatchingboost::is_void< Lhs_noptr >::value || \
               ::motionmatchingboost::is_void< Rhs_noptr >::value\
            )\
         )\
      ) || \
      (\
         ::motionmatchingboost::type_traits_detail::is_likely_stateless_lambda<Lhs_noref>::value\
      )\
   )


#include <boost/type_traits/detail/has_binary_operator.hpp>

#undef BOOST_TT_TRAIT_NAME
#undef BOOST_TT_TRAIT_OP
#undef BOOST_TT_FORBIDDEN_IF

#endif
