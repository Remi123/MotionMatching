// Copyright Daniel Wallin, David Abrahams 2005.
// Copyright Cromwell D. Enage 2017.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PARAMETER_AUX_ARG_LIST_HPP
#define BOOST_PARAMETER_AUX_ARG_LIST_HPP

namespace motionmatchingboost { namespace parameter { namespace aux {

    //
    // Structures used to build the tuple of actual arguments.  The tuple is a
    // nested cons-style list of arg_list specializations terminated by an
    // empty_arg_list.
    //
    // Each specialization of arg_list is derived from its successor in the
    // list type.  This feature is used along with using declarations to build
    // member function overload sets that can match against keywords.
    //

    // MPL sequence support
    struct arg_list_tag;

    template <typename T>
    struct get_reference
    {
        typedef typename T::reference type;
    };
}}} // namespace motionmatchingboost::parameter::aux

#include <boost/parameter/config.hpp>

#if defined(BOOST_PARAMETER_HAS_PERFECT_FORWARDING)

namespace motionmatchingboost { namespace parameter { namespace aux {

    struct value_type_is_void
    {
    };

    struct value_type_is_not_void
    {
    };
}}} // namespace motionmatchingboost::parameter::aux

#endif

#include <boost/parameter/aux_/void.hpp>
#include <boost/parameter/aux_/yesno.hpp>
#include <boost/parameter/aux_/result_of0.hpp>
#include <boost/parameter/aux_/default.hpp>

#if defined(BOOST_PARAMETER_HAS_PERFECT_FORWARDING)
#include <utility>

#if defined(BOOST_PARAMETER_CAN_USE_MP11)
#include <boost/mp11/integral.hpp>
#include <boost/mp11/list.hpp>
#include <boost/mp11/utility.hpp>
#include <type_traits>
#endif

namespace motionmatchingboost { namespace parameter { namespace aux {

    // Terminates arg_list<> and represents an empty list.  Since this is just
    // the terminating case, you might want to look at arg_list first to get a
    // feel for what's really happening here.
    struct empty_arg_list
    {
        struct tagged_arg
        {
            typedef ::motionmatchingboost::parameter::void_ value_type;
        };

        // Variadic constructor also serves as default constructor.
        template <typename ...Args>
        inline BOOST_CONSTEXPR empty_arg_list(Args&&...)
        {
        }

        // A metafunction class that, given a keyword and a default type,
        // returns the appropriate result type for a keyword lookup given
        // that default.
        struct binding
        {
            template <typename KW, typename Default, typename Reference>
            struct apply
            {
                typedef Default type;
            };

#if defined(BOOST_PARAMETER_CAN_USE_MP11)
            template <typename KW, typename Default, typename Reference>
            using fn = Default;
#endif
        };

        // Terminator for has_key, indicating that the keyword is unique.
        template <typename KW>
        static ::motionmatchingboost::parameter::aux::no_tag has_key(KW*);

        // If either of these operators are called, it means there is no
        // argument in the list that matches the supplied keyword.  Just
        // return the default value.
        template <typename K, typename Default>
        inline BOOST_CONSTEXPR Default&
            operator[](::motionmatchingboost::parameter::aux::default_<K,Default> x) const
        {
            return x.value;
        }

        template <typename K, typename Default>
        inline BOOST_CONSTEXPR Default&&
            operator[](::motionmatchingboost::parameter::aux::default_r_<K,Default> x) const
        {
            return ::std::forward<Default>(x.value);
        }

        // If this operator is called, it means there is no argument in the
        // list that matches the supplied keyword.  Just evaluate and return
        // the default value.
        template <typename K, typename F>
        inline BOOST_CONSTEXPR
        typename ::motionmatchingboost::parameter::aux::result_of0<F>::type
            operator[](BOOST_PARAMETER_lazy_default_fallback<K,F> x) const
        {
            return x.compute_default();
        }

        // No argument corresponding to ParameterRequirements::key_type
        // was found if we match this overload, so unless that parameter
        // has a default, we indicate that the actual arguments don't
        // match the function's requirements.
        template <typename ParameterRequirements, typename ArgPack>
        static typename ParameterRequirements::has_default
            satisfies(ParameterRequirements*, ArgPack*);

        // MPL sequence support
        typedef ::motionmatchingboost::parameter::aux::empty_arg_list type; // convenience
        // For dispatching to sequence intrinsics
        typedef ::motionmatchingboost::parameter::aux::arg_list_tag tag;
    };
}}} // namespace motionmatchingboost::parameter::aux

#include <boost/parameter/aux_/preprocessor/nullptr.hpp>
#include <boost/parameter/aux_/yesno.hpp>
#include <boost/parameter/aux_/is_maybe.hpp>
#include <boost/parameter/aux_/tagged_argument_fwd.hpp>
#include <boost/parameter/aux_/parameter_requirements.hpp>
#include <boost/parameter/aux_/augment_predicate.hpp>
#include <boost/parameter/keyword_fwd.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/apply_wrap.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/core/enable_if.hpp>

namespace motionmatchingboost { namespace parameter { namespace aux {

    // A tuple of tagged arguments, terminated with empty_arg_list.  Every
    // TaggedArg is an instance of tagged_argument<> or
    // tagged_argument_rref<>.
    template <
        typename TaggedArg
      , typename Next = ::motionmatchingboost::parameter::aux::empty_arg_list
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
      , typename EmitsErrors = ::motionmatchingboost::mp11::mp_true
#else
      , typename EmitsErrors = ::motionmatchingboost::mpl::true_
#endif
    >
    class arg_list : public Next
    {
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
        using _holds_maybe = typename ::motionmatchingboost::parameter::aux
        ::is_maybe<typename TaggedArg::value_type>::type;
#else
        typedef typename ::motionmatchingboost::parameter::aux
        ::is_maybe<typename TaggedArg::value_type>::type _holds_maybe;
#endif

        TaggedArg arg;      // Stores the argument

     public:
        typedef TaggedArg tagged_arg;
        typedef ::motionmatchingboost::parameter::aux::arg_list<TaggedArg,Next> self;
        typedef typename TaggedArg::key_type key_type;

#if defined(BOOST_PARAMETER_CAN_USE_MP11)
        using reference = typename ::motionmatchingboost::mp11::mp_if<
            _holds_maybe
          , ::motionmatchingboost::parameter::aux
            ::get_reference<typename TaggedArg::value_type>
          , ::motionmatchingboost::parameter::aux::get_reference<TaggedArg>
        >::type;

        using value_type = ::motionmatchingboost::mp11
        ::mp_if<_holds_maybe,reference,typename TaggedArg::value_type>;
#else   // !defined(BOOST_PARAMETER_CAN_USE_MP11)
        typedef typename ::motionmatchingboost::mpl::eval_if<
            _holds_maybe
          , ::motionmatchingboost::parameter::aux
            ::get_reference<typename TaggedArg::value_type>
          , ::motionmatchingboost::parameter::aux::get_reference<TaggedArg>
        >::type reference;

        typedef typename ::motionmatchingboost::mpl::if_<
            _holds_maybe
          , reference
          , typename TaggedArg::value_type
        >::type value_type;
#endif  // BOOST_PARAMETER_CAN_USE_MP11

        // Create a new list by prepending arg to a copy of tail.  Used when
        // incrementally building this structure with the comma operator.
        inline BOOST_CONSTEXPR arg_list(
            TaggedArg const& head
          , Next const& tail
        ) : Next(tail), arg(head)
        {
        }

        // Store the arguments in successive nodes of this list.
        // Use tag dispatching to determine whether to forward all arguments
        // to the Next constructor, or store the first argument and forward
        // the rest. -- Cromwell D. Enage
        template <typename A0>
        inline BOOST_CONSTEXPR arg_list(
            ::motionmatchingboost::parameter::aux::value_type_is_not_void
          , A0&& a0
        ) : Next(
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
                ::motionmatchingboost::mp11::mp_if<
                    ::std::is_same<
#else
                typename ::motionmatchingboost::mpl::if_<
                    ::motionmatchingboost::is_same<
#endif
                        typename Next::tagged_arg::value_type
                      , ::motionmatchingboost::parameter::void_
                    >
                  , ::motionmatchingboost::parameter::aux::value_type_is_void
                  , ::motionmatchingboost::parameter::aux::value_type_is_not_void
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
                >()
#else
                >::type()
#endif
            )
          , arg(::std::forward<A0>(a0))
        {
        }

        template <typename ...Args>
        inline BOOST_CONSTEXPR arg_list(
            ::motionmatchingboost::parameter::aux::value_type_is_void
          , Args&&... args
        ) : Next(
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
                ::motionmatchingboost::mp11::mp_if<
                    ::std::is_same<
#else
                typename ::motionmatchingboost::mpl::if_<
                    ::motionmatchingboost::is_same<
#endif
                        typename Next::tagged_arg::value_type
                      , ::motionmatchingboost::parameter::void_
                    >
                  , ::motionmatchingboost::parameter::aux::value_type_is_void
                  , ::motionmatchingboost::parameter::aux::value_type_is_not_void
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
                >()
#else
                >::type()
#endif
              , ::std::forward<Args>(args)...
            )
          , arg(::motionmatchingboost::parameter::aux::void_reference())
        {
        }

        template <typename A0, typename A1, typename ...Args>
        inline BOOST_CONSTEXPR arg_list(
            ::motionmatchingboost::parameter::aux::value_type_is_not_void
          , A0&& a0
          , A1&& a1
          , Args&&... args
        ) : Next(
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
                ::motionmatchingboost::mp11::mp_if<
                    ::std::is_same<
#else
                typename ::motionmatchingboost::mpl::if_<
                    ::motionmatchingboost::is_same<
#endif
                        typename Next::tagged_arg::value_type
                      , ::motionmatchingboost::parameter::void_
                    >
                  , ::motionmatchingboost::parameter::aux::value_type_is_void
                  , ::motionmatchingboost::parameter::aux::value_type_is_not_void
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
                >()
#else
                >::type()
#endif
              , ::std::forward<A1>(a1)
              , ::std::forward<Args>(args)...
            )
          , arg(::std::forward<A0>(a0))
        {
        }

        // A metafunction class that, given a keyword and a default type,
        // returns the appropriate result type for a keyword lookup given
        // that default.
        struct binding
        {
            typedef typename Next::binding next_binding;

            template <typename KW, typename Default, typename Reference>
            struct apply
            {
                typedef typename ::motionmatchingboost::mpl::eval_if<
                    ::motionmatchingboost::is_same<KW,key_type>
                  , ::motionmatchingboost::mpl::if_<Reference,reference,value_type>
                  , ::motionmatchingboost::mpl
                    ::apply_wrap3<next_binding,KW,Default,Reference>
                >::type type;
            };

#if defined(BOOST_PARAMETER_CAN_USE_MP11)
            template <typename KW, typename Default, typename Reference>
            using fn = ::motionmatchingboost::mp11::mp_if<
                ::std::is_same<KW,key_type>
              , ::motionmatchingboost::mp11::mp_if<Reference,reference,value_type>
              , ::motionmatchingboost::mp11::mp_apply_q<
                    next_binding
                  , ::motionmatchingboost::mp11::mp_list<KW,Default,Reference>
                >
            >;
#endif
        };

        // Overload for key_type, so the assert below will fire
        // if the same keyword is used again.
        static ::motionmatchingboost::parameter::aux::yes_tag has_key(key_type*);
        using Next::has_key;

     private:
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
        using _has_unique_key = ::motionmatchingboost::mp11::mp_bool<
#else
        typedef ::motionmatchingboost::mpl::bool_<
#endif
            sizeof(
                Next::has_key(
                    static_cast<key_type*>(BOOST_PARAMETER_AUX_PP_NULLPTR)
                )
            ) == sizeof(::motionmatchingboost::parameter::aux::no_tag)
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
        >;
#else
        > _has_unique_key;
#endif

#if defined(BOOST_PARAMETER_CAN_USE_MP11)
        static_assert(
            !(EmitsErrors::value) || (_has_unique_key::value)
          , "duplicate keyword"
        );
#else
        BOOST_MPL_ASSERT_MSG(
            !(EmitsErrors::value) || (_has_unique_key::value)
          , duplicate_keyword
          , (key_type)
        );
#endif

        //
        // Begin implementation of indexing operators
        // for looking up specific arguments by name.
        //

        // Helpers that handle the case when TaggedArg is empty<T>.
        template <typename D>
        inline BOOST_CONSTEXPR reference
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
            get_default(D const&, ::motionmatchingboost::mp11::mp_false) const
#else
            get_default(D const&, ::motionmatchingboost::mpl::false_) const
#endif
        {
            return this->arg.get_value();
        }

        template <typename D>
        inline BOOST_CONSTEXPR reference
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
            get_default(D const& d, ::motionmatchingboost::mp11::mp_true) const
#else
            get_default(D const& d, ::motionmatchingboost::mpl::true_) const
#endif
        {
            return (
                this->arg.get_value()
              ? this->arg.get_value().get()
              : this->arg.get_value().construct(d.value)
            );
        }

     public:
        inline BOOST_CONSTEXPR reference
            operator[](::motionmatchingboost::parameter::keyword<key_type> const&) const
        {
#if !defined(BOOST_NO_CXX14_CONSTEXPR)
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
            static_assert(!_holds_maybe::value, "must not hold maybe");
#elif !( \
        BOOST_WORKAROUND(BOOST_GCC, >= 40700) && \
        BOOST_WORKAROUND(BOOST_GCC, < 40900) \
    ) && !BOOST_WORKAROUND(BOOST_GCC, >= 50000) && \
    !BOOST_WORKAROUND(BOOST_MSVC, < 1910)
            BOOST_MPL_ASSERT_NOT((_holds_maybe));
#endif
#endif
            return this->arg.get_value();
        }

        template <typename Default>
        inline BOOST_CONSTEXPR reference
            operator[](
                ::motionmatchingboost::parameter::aux::default_<key_type,Default> const& d
            ) const
        {
            return this->get_default(d, _holds_maybe());
        }

        template <typename Default>
        inline BOOST_CONSTEXPR reference
            operator[](
                ::motionmatchingboost::parameter::aux::default_r_<key_type,Default> const& d
            ) const
        {
            return this->get_default(d, _holds_maybe());
        }

        template <typename Default>
        inline BOOST_CONSTEXPR reference
            operator[](
                BOOST_PARAMETER_lazy_default_fallback<key_type,Default> const&
            ) const
        {
#if !defined(BOOST_NO_CXX14_CONSTEXPR)
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
            static_assert(!_holds_maybe::value, "must not hold maybe");
#elif !( \
        BOOST_WORKAROUND(BOOST_GCC, >= 40700) && \
        BOOST_WORKAROUND(BOOST_GCC, < 40900) \
    ) && !BOOST_WORKAROUND(BOOST_GCC, >= 50000) && \
    !BOOST_WORKAROUND(BOOST_MSVC, < 1910)
            BOOST_MPL_ASSERT_NOT((_holds_maybe));
#endif
#endif
            return this->arg.get_value();
        }

        // Builds an overload set including operator[]s defined
        // in base classes.
        using Next::operator[];

        //
        // End of indexing support
        //

        // For parameter_requirements matching this node's key_type, return
        // a bool constant wrapper indicating whether the requirements are
        // satisfied by TaggedArg.  Used only for compile-time computation
        // and never really called, so a declaration is enough.
        template <typename HasDefault, typename Predicate, typename ArgPack>
        static typename ::motionmatchingboost::lazy_enable_if<
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
            ::motionmatchingboost::mp11::mp_if<
                EmitsErrors
              , ::motionmatchingboost::mp11::mp_true
              , _has_unique_key
            >
          , ::motionmatchingboost::parameter::aux::augment_predicate_mp11<
#else
            typename ::motionmatchingboost::mpl::if_<
                EmitsErrors
              , ::motionmatchingboost::mpl::true_
              , _has_unique_key
            >::type
          , ::motionmatchingboost::parameter::aux::augment_predicate<
#endif
                Predicate
              , reference
              , key_type
              , value_type
              , ArgPack
            >
        >::type
            satisfies(
                ::motionmatchingboost::parameter::aux::parameter_requirements<
                    key_type
                  , Predicate
                  , HasDefault
                >*
              , ArgPack*
            );

        // Builds an overload set including satisfies functions defined
        // in base classes.
        using Next::satisfies;

        // Comma operator to compose argument list without using parameters<>.
        // Useful for argument lists with undetermined length.
        template <typename KW, typename T2>
        inline BOOST_CONSTEXPR ::motionmatchingboost::parameter::aux::arg_list<
            ::motionmatchingboost::parameter::aux::tagged_argument<KW,T2>
          , self
        >
            operator,(
                ::motionmatchingboost::parameter::aux::tagged_argument<KW,T2> const& x
            ) const
        {
            return ::motionmatchingboost::parameter::aux::arg_list<
                ::motionmatchingboost::parameter::aux::tagged_argument<KW,T2>
              , self
            >(x, *this);
        }

        template <typename KW, typename T2>
        inline BOOST_CONSTEXPR ::motionmatchingboost::parameter::aux::arg_list<
            ::motionmatchingboost::parameter::aux::tagged_argument_rref<KW,T2>
          , self
        >
            operator,(
                ::motionmatchingboost::parameter::aux::tagged_argument_rref<KW,T2> const& x
            ) const
        {
            return ::motionmatchingboost::parameter::aux::arg_list<
                ::motionmatchingboost::parameter::aux::tagged_argument_rref<KW,T2>
              , self
            >(x, *this);
        }

        // MPL sequence support
        typedef self type;        // Convenience for users
        typedef Next tail_type;   // For the benefit of iterators
        // For dispatching to sequence intrinsics
        typedef ::motionmatchingboost::parameter::aux::arg_list_tag tag;
    };
}}} // namespace motionmatchingboost::parameter::aux

#else   // !defined(BOOST_PARAMETER_HAS_PERFECT_FORWARDING)

#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/facilities/intercept.hpp>

namespace motionmatchingboost { namespace parameter { namespace aux {

    // Terminates arg_list<> and represents an empty list.  Since this is just
    // the terminating case, you might want to look at arg_list first to get a
    // feel for what's really happening here.
    struct empty_arg_list
    {
        inline BOOST_CONSTEXPR empty_arg_list()
        {
        }

        // Constructor taking BOOST_PARAMETER_COMPOSE_MAX_ARITY empty_arg_list
        // arguments; this makes initialization.
        inline BOOST_CONSTEXPR empty_arg_list(
            BOOST_PP_ENUM_PARAMS(
                BOOST_PARAMETER_COMPOSE_MAX_ARITY
              , ::motionmatchingboost::parameter::void_ BOOST_PP_INTERCEPT
            )
        )
        {
        }

        // A metafunction class that, given a keyword and a default type,
        // returns the appropriate result type for a keyword lookup given
        // that default.
        struct binding
        {
            template <typename KW, typename Default, typename Reference>
            struct apply
            {
                typedef Default type;
            };
        };

        // Terminator for has_key, indicating that the keyword is unique.
        template <typename KW>
        static ::motionmatchingboost::parameter::aux::no_tag has_key(KW*);

#if BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x564))
        // The overload set technique doesn't work with these older compilers,
        // so they need some explicit handholding.

        // A metafunction class that, given a keyword, returns the type of the
        // base sublist whose get() function can produce the value for that key.
        struct key_owner
        {
            template <typename KW>
            struct apply
            {
                typedef ::motionmatchingboost::parameter::aux::empty_arg_list type;
            };
        };
#endif  // Borland workarounds needed

        // If either of these operators are called, it means there is no
        // argument in the list that matches the supplied keyword.  Just
        // return the default value.
        template <typename K, typename Default>
        inline BOOST_CONSTEXPR Default&
            operator[](::motionmatchingboost::parameter::aux::default_<K,Default> x) const
        {
            return x.value;
        }

        // If this operator is called, it means there is no argument in the
        // list that matches the supplied keyword.  Just evaluate and return
        // the default value.
        template <typename K, typename F>
        inline BOOST_CONSTEXPR
        typename ::motionmatchingboost::parameter::aux::result_of0<F>::type
            operator[](BOOST_PARAMETER_lazy_default_fallback<K,F> x) const
        {
            return x.compute_default();
        }

        // No argument corresponding to ParameterRequirements::key_type
        // was found if we match this overload, so unless that parameter
        // has a default, we indicate that the actual arguments don't
        // match the function's requirements.
        template <typename ParameterRequirements, typename ArgPack>
        static typename ParameterRequirements::has_default
            satisfies(ParameterRequirements*, ArgPack*);

        // MPL sequence support
        typedef ::motionmatchingboost::parameter::aux::empty_arg_list type; // convenience
        // For dispatching to sequence intrinsics
        typedef ::motionmatchingboost::parameter::aux::arg_list_tag tag;
    };
}}} // namespace motionmatchingboost::parameter::aux

#include <boost/parameter/aux_/yesno.hpp>
#include <boost/parameter/aux_/is_maybe.hpp>
#include <boost/parameter/aux_/tagged_argument_fwd.hpp>
#include <boost/parameter/aux_/parameter_requirements.hpp>
#include <boost/parameter/aux_/augment_predicate.hpp>
#include <boost/parameter/keyword_fwd.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/apply_wrap.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_shifted_params.hpp>

#if !defined(BOOST_NO_SFINAE) && !BOOST_WORKAROUND(BOOST_MSVC, < 1800)
#include <boost/core/enable_if.hpp>
#endif

#if !BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x564))
#include <boost/parameter/aux_/preprocessor/nullptr.hpp>
#endif

namespace motionmatchingboost { namespace parameter { namespace aux {

    // A tuple of tagged arguments, terminated with empty_arg_list.  Every
    // TaggedArg is an instance of tagged_argument<>.
    template <
        typename TaggedArg
      , typename Next = ::motionmatchingboost::parameter::aux::empty_arg_list
      , typename EmitsErrors = ::motionmatchingboost::mpl::true_
    >
    class arg_list : public Next
    {
        typedef typename ::motionmatchingboost::parameter::aux
        ::is_maybe<typename TaggedArg::value_type>::type _holds_maybe;

        TaggedArg arg;      // Stores the argument

     public:
        typedef TaggedArg tagged_arg;
        typedef ::motionmatchingboost::parameter::aux::arg_list<TaggedArg,Next> self;
        typedef typename TaggedArg::key_type key_type;

        typedef typename ::motionmatchingboost::mpl::eval_if<
            _holds_maybe
          , ::motionmatchingboost::parameter::aux
            ::get_reference<typename TaggedArg::value_type>
          , ::motionmatchingboost::parameter::aux::get_reference<TaggedArg>
        >::type reference;

        typedef typename ::motionmatchingboost::mpl::if_<
            _holds_maybe
          , reference
          , typename TaggedArg::value_type
        >::type value_type;

        // Create a new list by prepending arg to a copy of tail.  Used when
        // incrementally building this structure with the comma operator.
        inline BOOST_CONSTEXPR arg_list(
            TaggedArg const& head
          , Next const& tail
        ) : Next(tail), arg(head)
        {
        }

        // Store the arguments in successive nodes of this list.
        template <
            // typename A0, typename A1, ...
            BOOST_PP_ENUM_PARAMS(
                BOOST_PARAMETER_COMPOSE_MAX_ARITY
              , typename A
            )
        >
        inline BOOST_CONSTEXPR arg_list(
            // A0& a0, A1& a1, ...
            BOOST_PP_ENUM_BINARY_PARAMS(
                BOOST_PARAMETER_COMPOSE_MAX_ARITY
              , A
              , & a
            )
        ) : Next(
                // a1, a2, ...
                BOOST_PP_ENUM_SHIFTED_PARAMS(
                    BOOST_PARAMETER_COMPOSE_MAX_ARITY
                  , a
                )
              , ::motionmatchingboost::parameter::aux::void_reference()
            )
          , arg(a0)
        {
        }

        // A metafunction class that, given a keyword and a default type,
        // returns the appropriate result type for a keyword lookup given
        // that default.
        struct binding
        {
            typedef typename Next::binding next_binding;

            template <typename KW, typename Default, typename Reference>
            struct apply
            {
                typedef typename ::motionmatchingboost::mpl::eval_if<
                    ::motionmatchingboost::is_same<KW,key_type>
                  , ::motionmatchingboost::mpl::if_<Reference,reference,value_type>
                  , ::motionmatchingboost::mpl::apply_wrap3<
                        next_binding
                      , KW
                      , Default
                      , Reference
                    >
                >::type type;
            };
        };

#if !BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x564))
        // Overload for key_type, so the assert below will fire
        // if the same keyword is used again.
        static ::motionmatchingboost::parameter::aux::yes_tag has_key(key_type*);
        using Next::has_key;

     private:
#if defined(BOOST_NO_SFINAE) || BOOST_WORKAROUND(BOOST_MSVC, < 1800)
        BOOST_MPL_ASSERT_MSG(
            sizeof(
                Next::has_key(
                    static_cast<key_type*>(BOOST_PARAMETER_AUX_PP_NULLPTR)
                )
            ) == sizeof(::motionmatchingboost::parameter::aux::no_tag)
          , duplicate_keyword
          , (key_type)
        );
#else
        typedef ::motionmatchingboost::mpl::bool_<
            sizeof(
                Next::has_key(
                    static_cast<key_type*>(BOOST_PARAMETER_AUX_PP_NULLPTR)
                )
            ) == sizeof(::motionmatchingboost::parameter::aux::no_tag)
        > _has_unique_key;

        BOOST_MPL_ASSERT_MSG(
            !(EmitsErrors::value) || (_has_unique_key::value)
          , duplicate_keyword
          , (key_type)
        );
#endif  // SFINAE/MSVC workarounds needed
#endif  // Borland workarounds not needed

     private:
        //
        // Begin implementation of indexing operators
        // for looking up specific arguments by name.
        //

        // Helpers that handle the case when TaggedArg is empty<T>.
        template <typename D>
        inline BOOST_CONSTEXPR reference
            get_default(D const&, ::motionmatchingboost::mpl::false_) const
        {
            return this->arg.get_value();
        }

        template <typename D>
        inline BOOST_CONSTEXPR reference
            get_default(D const& d, ::motionmatchingboost::mpl::true_) const
        {
            return (
                this->arg.get_value()
              ? this->arg.get_value().get()
              : this->arg.get_value().construct(d.value)
            );
        }

     public:
#if BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x564))
        // These older compilers don't support the overload set creation
        // idiom well, so we need to do all the return type calculation
        // for the compiler and dispatch through an outer function template.

        // A metafunction class that, given a keyword, returns the base
        // sublist whose get() function can produce the value for that key.
        struct key_owner
        {
            typedef typename Next::key_owner next_key_owner;

            template <typename KW>
            struct apply
            {
                typedef typename ::motionmatchingboost::mpl::eval_if<
                    ::motionmatchingboost::is_same<KW,key_type>
                  , ::motionmatchingboost::mpl::identity<
                        ::motionmatchingboost::parameter::aux::arg_list<TaggedArg,Next>
                    >
                  , ::motionmatchingboost::mpl::apply_wrap1<next_key_owner,KW>
                >::type type;
            };
        };

        // Outer indexing operators that dispatch to the right node's
        // get() function.
        template <typename KW>
        inline BOOST_CONSTEXPR typename ::motionmatchingboost::mpl::apply_wrap3<
            binding
          , KW
          , ::motionmatchingboost::parameter::void_
          , ::motionmatchingboost::mpl::true_
        >::type
            operator[](::motionmatchingboost::parameter::keyword<KW> const& x) const
        {
            typename ::motionmatchingboost::mpl::apply_wrap1<key_owner,KW>::type const&
                sublist = *this;
            return sublist.get(x);
        }

        template <typename KW, typename Default>
        inline BOOST_CONSTEXPR typename ::motionmatchingboost::mpl::apply_wrap3<
            binding
          , KW
          , Default&
          , ::motionmatchingboost::mpl::true_
        >::type
            operator[](
                ::motionmatchingboost::parameter::aux::default_<KW,Default> const& x
            ) const
        {
            typename ::motionmatchingboost::mpl::apply_wrap1<key_owner,KW>::type const&
                sublist = *this;
            return sublist.get(x);
        }

        template <typename KW, typename F>
        inline BOOST_CONSTEXPR typename ::motionmatchingboost::mpl::apply_wrap3<
            binding
          , KW
          , typename ::motionmatchingboost::parameter::aux::result_of0<F>::type
          , ::motionmatchingboost::mpl::true_
        >::type
            operator[](
                BOOST_PARAMETER_lazy_default_fallback<KW,F> const& x
            ) const
        {
            typename ::motionmatchingboost::mpl::apply_wrap1<key_owner,KW>::type const&
                sublist = *this;
            return sublist.get(x);
        }

        // These just return the stored value; when empty_arg_list is reached,
        // indicating no matching argument was passed, the default is
        // returned, or if no default_ or lazy_default was passed, compilation
        // fails.
        inline BOOST_CONSTEXPR reference
            get(::motionmatchingboost::parameter::keyword<key_type> const&) const
        {
            BOOST_MPL_ASSERT_NOT((_holds_maybe));
            return this->arg.get_value();
        }

        template <typename Default>
        inline BOOST_CONSTEXPR reference
            get(
                ::motionmatchingboost::parameter::aux::default_<key_type,Default> const& d
            ) const
        {
            return this->get_default(d, _holds_maybe());
        }

        template <typename Default>
        inline BOOST_CONSTEXPR reference
            get(
                BOOST_PARAMETER_lazy_default_fallback<key_type,Default> const&
            ) const
        {
            return this->arg.get_value();
        }
#else   // !BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x564))
        inline BOOST_CONSTEXPR reference
            operator[](::motionmatchingboost::parameter::keyword<key_type> const&) const
        {
            BOOST_MPL_ASSERT_NOT((_holds_maybe));
            return this->arg.get_value();
        }

        template <typename Default>
        inline BOOST_CONSTEXPR reference
            operator[](
                ::motionmatchingboost::parameter::aux::default_<key_type,Default> const& d
            ) const
        {
            return this->get_default(d, _holds_maybe());
        }

        template <typename Default>
        inline BOOST_CONSTEXPR reference
            operator[](
                BOOST_PARAMETER_lazy_default_fallback<key_type,Default> const&
            ) const
        {
            BOOST_MPL_ASSERT_NOT((_holds_maybe));
            return this->arg.get_value();
        }

        // Builds an overload set including operator[]s defined
        // in base classes.
        using Next::operator[];

        //
        // End of indexing support
        //

        // For parameter_requirements matching this node's key_type, return
        // a bool constant wrapper indicating whether the requirements are
        // satisfied by TaggedArg.  Used only for compile-time computation
        // and never really called, so a declaration is enough.
        template <typename HasDefault, typename Predicate, typename ArgPack>
        static typename
#if !defined(BOOST_NO_SFINAE) && !BOOST_WORKAROUND(BOOST_MSVC, < 1800)
        ::motionmatchingboost::lazy_enable_if<
            typename ::motionmatchingboost::mpl::if_<
                EmitsErrors
              , ::motionmatchingboost::mpl::true_
              , _has_unique_key
            >::type,
#endif
            ::motionmatchingboost::parameter::aux::augment_predicate<
                Predicate
              , reference
              , key_type
              , value_type
              , ArgPack
#if !defined(BOOST_NO_SFINAE) && !BOOST_WORKAROUND(BOOST_MSVC, < 1800)
           >
#endif
        >::type
            satisfies(
                ::motionmatchingboost::parameter::aux::parameter_requirements<
                    key_type
                  , Predicate
                  , HasDefault
                >*
              , ArgPack*
            );

        // Builds an overload set including satisfies functions defined
        // in base classes.
        using Next::satisfies;
#endif  // Borland workarounds needed

        // Comma operator to compose argument list without using parameters<>.
        // Useful for argument lists with undetermined length.
        template <typename KW, typename T2>
        inline BOOST_CONSTEXPR ::motionmatchingboost::parameter::aux::arg_list<
            ::motionmatchingboost::parameter::aux::tagged_argument<KW,T2>
          , self
        >
            operator,(
                ::motionmatchingboost::parameter::aux::tagged_argument<KW,T2> const& x
            ) const
        {
            return ::motionmatchingboost::parameter::aux::arg_list<
                ::motionmatchingboost::parameter::aux::tagged_argument<KW,T2>
              , self
            >(x, *this);
        }

        // MPL sequence support
        typedef self type;        // Convenience for users
        typedef Next tail_type;   // For the benefit of iterators
        // For dispatching to sequence intrinsics
        typedef ::motionmatchingboost::parameter::aux::arg_list_tag tag;
    };
}}} // namespace motionmatchingboost::parameter::aux

#endif  // BOOST_PARAMETER_HAS_PERFECT_FORWARDING

#if defined(BOOST_PARAMETER_CAN_USE_MP11)

namespace motionmatchingboost { namespace parameter { namespace aux {

    template <typename ...ArgTuples>
    struct arg_list_cons;

    template <>
    struct arg_list_cons<>
    {
        using type = ::motionmatchingboost::parameter::aux::empty_arg_list;
    };

    template <typename ArgTuple0, typename ...Tuples>
    struct arg_list_cons<ArgTuple0,Tuples...>
    {
        using type = ::motionmatchingboost::parameter::aux::arg_list<
            typename ArgTuple0::tagged_arg
          , typename ::motionmatchingboost::parameter::aux::arg_list_cons<Tuples...>::type
          , typename ArgTuple0::emits_errors
        >;
    };

    template <
        typename Keyword
      , typename TaggedArg
      , typename EmitsErrors = ::motionmatchingboost::mp11::mp_true
    >
    struct flat_like_arg_tuple
    {
        using tagged_arg = TaggedArg;
        using emits_errors = EmitsErrors;
    };

    template <typename ...ArgTuples>
    class flat_like_arg_list
      : public ::motionmatchingboost::parameter::aux::arg_list_cons<ArgTuples...>::type
    {
        using _base_type = typename ::motionmatchingboost::parameter::aux
        ::arg_list_cons<ArgTuples...>::type;

     public:
        inline BOOST_CONSTEXPR flat_like_arg_list(
            typename _base_type::tagged_arg const& head
          , typename _base_type::tail_type const& tail
        ) : _base_type(head, tail)
        {
        }

        template <typename ...Args>
        inline BOOST_CONSTEXPR flat_like_arg_list(Args&&... args)
          : _base_type(::std::forward<Args>(args)...)
        {
        }

        using _base_type::operator[];
        using _base_type::satisfies;

        // Comma operator to compose argument list without using parameters<>.
        // Useful for argument lists with undetermined length.
        template <typename TaggedArg>
        inline BOOST_CONSTEXPR ::motionmatchingboost::parameter::aux::flat_like_arg_list<
            ::motionmatchingboost::parameter::aux::flat_like_arg_tuple<
                typename TaggedArg::base_type::key_type
              , typename TaggedArg::base_type
            >
          , ArgTuples...
        >
            operator,(TaggedArg const& x) const
        {
            return ::motionmatchingboost::parameter::aux::flat_like_arg_list<
                ::motionmatchingboost::parameter::aux::flat_like_arg_tuple<
                    typename TaggedArg::base_type::key_type
                  , typename TaggedArg::base_type
                >
              , ArgTuples...
            >(
                static_cast<typename TaggedArg::base_type const&>(x)
              , static_cast<_base_type const&>(*this)
            );
        }
    };

    template <>
    class flat_like_arg_list<>
      : public ::motionmatchingboost::parameter::aux::empty_arg_list
    {
        using _base_type = ::motionmatchingboost::parameter::aux::empty_arg_list;

     public:
        template <typename ...Args>
        inline BOOST_CONSTEXPR flat_like_arg_list(Args&&... args)
          : _base_type(::std::forward<Args>(args)...)
        {
        }

        using _base_type::operator[];
        using _base_type::satisfies;

        // Comma operator to compose argument list without using parameters<>.
        // Useful for argument lists with undetermined length.
        template <typename TaggedArg>
        inline BOOST_CONSTEXPR ::motionmatchingboost::parameter::aux::flat_like_arg_list<
            ::motionmatchingboost::parameter::aux::flat_like_arg_tuple<
                typename TaggedArg::base_type::key_type
              , typename TaggedArg::base_type
            >
        >
            operator,(TaggedArg const& x) const
        {
            return ::motionmatchingboost::parameter::aux::flat_like_arg_list<
                ::motionmatchingboost::parameter::aux::flat_like_arg_tuple<
                    typename TaggedArg::base_type::key_type
                  , typename TaggedArg::base_type
                >
            >(
                static_cast<typename TaggedArg::base_type const&>(x)
              , static_cast<_base_type const&>(*this)
            );
        }
    };
}}} // namespace motionmatchingboost::parameter::aux

#endif  // BOOST_PARAMETER_CAN_USE_MP11

#include <boost/mpl/iterator_tags.hpp>

namespace motionmatchingboost { namespace parameter { namespace aux {

    // MPL sequence support
    template <typename ArgumentPack>
    struct arg_list_iterator
    {
        typedef ::motionmatchingboost::mpl::forward_iterator_tag category;

        // The incremented iterator
        typedef ::motionmatchingboost::parameter::aux
        ::arg_list_iterator<typename ArgumentPack::tail_type> next;

        // dereferencing yields the key type
        typedef typename ArgumentPack::key_type type;
    };

    template <>
    struct arg_list_iterator< ::motionmatchingboost::parameter::aux::empty_arg_list>
    {
    };
}}} // namespace motionmatchingboost::parameter::aux

#include <boost/mpl/begin_end_fwd.hpp>

// MPL sequence support
namespace motionmatchingboost { namespace mpl {

    template <>
    struct begin_impl< ::motionmatchingboost::parameter::aux::arg_list_tag>
    {
        template <typename S>
        struct apply
        {
            typedef ::motionmatchingboost::parameter::aux::arg_list_iterator<S> type;
        };
    };

    template <>
    struct end_impl< ::motionmatchingboost::parameter::aux::arg_list_tag>
    {
        template <typename>
        struct apply
        {
            typedef ::motionmatchingboost::parameter::aux::arg_list_iterator<
                ::motionmatchingboost::parameter::aux::empty_arg_list
            > type;
        };
    };
}} // namespace motionmatchingboost::mpl

#include <boost/parameter/value_type.hpp>
#include <boost/mpl/has_key_fwd.hpp>
#include <boost/type_traits/is_void.hpp>

namespace motionmatchingboost { namespace mpl {

    template <>
    struct has_key_impl< ::motionmatchingboost::parameter::aux::arg_list_tag>
    {
        template <typename ArgList, typename Keyword>
        struct apply
        {
            typedef typename ::motionmatchingboost::mpl::if_<
                ::motionmatchingboost::is_void<
                    typename ::motionmatchingboost::parameter
                    ::value_type<ArgList,Keyword,void>::type
                >
              , ::motionmatchingboost::mpl::false_
              , ::motionmatchingboost::mpl::true_
            >::type type;
        };
    };
}} // namespace motionmatchingboost::mpl

#include <boost/mpl/count_fwd.hpp>
#include <boost/mpl/int.hpp>

namespace motionmatchingboost { namespace mpl {

    template <>
    struct count_impl< ::motionmatchingboost::parameter::aux::arg_list_tag>
    {
        template <typename ArgList, typename Keyword>
        struct apply
        {
            typedef typename ::motionmatchingboost::mpl::if_<
                ::motionmatchingboost::is_void<
                    typename ::motionmatchingboost::parameter
                    ::value_type<ArgList,Keyword,void>::type
                >
              , ::motionmatchingboost::mpl::int_<0>
              , ::motionmatchingboost::mpl::int_<1>
            >::type type;
        };
    };
}} // namespace motionmatchingboost::mpl

#include <boost/mpl/key_type_fwd.hpp>
#include <boost/mpl/identity.hpp>

namespace motionmatchingboost { namespace mpl {

    template <>
    struct key_type_impl< ::motionmatchingboost::parameter::aux::arg_list_tag>
    {
        template <typename ArgList, typename Keyword>
        struct apply
        {
            typedef typename ::motionmatchingboost::mpl::eval_if<
                ::motionmatchingboost::is_void<
                    typename ::motionmatchingboost::parameter
                    ::value_type<ArgList,Keyword,void>::type
                >
              , void
              , ::motionmatchingboost::mpl::identity<Keyword>
            >::type type;
        };
    };
}} // namespace motionmatchingboost::mpl

#include <boost/mpl/value_type_fwd.hpp>

namespace motionmatchingboost { namespace mpl {

    template <>
    struct value_type_impl< ::motionmatchingboost::parameter::aux::arg_list_tag>
      : ::motionmatchingboost::mpl::key_type_impl< ::motionmatchingboost::parameter::aux::arg_list_tag>
    {
    };
}} // namespace motionmatchingboost::mpl

#include <boost/mpl/at_fwd.hpp>

namespace motionmatchingboost { namespace mpl {

    template <>
    struct at_impl< ::motionmatchingboost::parameter::aux::arg_list_tag>
      : ::motionmatchingboost::mpl::key_type_impl< ::motionmatchingboost::parameter::aux::arg_list_tag>
    {
    };
}} // namespace motionmatchingboost::mpl

#include <boost/mpl/order_fwd.hpp>
#include <boost/mpl/void.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/distance.hpp>

namespace motionmatchingboost { namespace mpl {

    template <>
    struct order_impl< ::motionmatchingboost::parameter::aux::arg_list_tag>
    {
        template <typename ArgList, typename Keyword>
        struct apply
        {
            typedef typename ::motionmatchingboost::mpl::find<ArgList,Keyword>::type Itr;
            typedef typename ::motionmatchingboost::mpl::eval_if<
                ::motionmatchingboost::is_void<
                    typename ::motionmatchingboost::parameter
                    ::value_type<ArgList,Keyword,void>::type
                >
              , ::motionmatchingboost::mpl::identity< ::motionmatchingboost::mpl::void_>
              , ::motionmatchingboost::mpl::distance<
                    Itr
                  , ::motionmatchingboost::parameter::aux::arg_list_iterator<
                        ::motionmatchingboost::parameter::aux::empty_arg_list
                    >
                >
            >::type type;
        };
    };
}} // namespace motionmatchingboost::mpl

#endif  // include guard

