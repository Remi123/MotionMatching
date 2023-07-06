#ifndef BOOST_SYSTEM_DETAIL_SYSTEM_CATEGORY_MESSAGE_WIN32_HPP_INCLUDED
#define BOOST_SYSTEM_DETAIL_SYSTEM_CATEGORY_MESSAGE_WIN32_HPP_INCLUDED

// Copyright Beman Dawes 2002, 2006
// Copyright (c) Microsoft Corporation 2014
// Copyright 2018, 2020 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See library home page at http://www.boost.org/libs/system

#include <boost/system/detail/snprintf.hpp>
#include <boost/winapi/error_handling.hpp>
#include <boost/winapi/character_code_conversion.hpp>
#include <boost/winapi/local_memory.hpp>
#include <boost/config.hpp>
#include <cstdio>

//

namespace motionmatchingboost
{

namespace system
{

namespace detail
{

inline char const * unknown_message_win32( int ev, char * buffer, std::size_t len )
{
    detail::snprintf( buffer, len, "Unknown error (%d)", ev );
    return buffer;
}

inline motionmatchingboost::winapi::UINT_ message_cp_win32()
{
#if defined(BOOST_SYSTEM_USE_UTF8)

    return motionmatchingboost::winapi::CP_UTF8_;

#else

    return motionmatchingboost::winapi::CP_ACP_;

#endif
}

inline char const * system_category_message_win32( int ev, char * buffer, std::size_t len ) BOOST_NOEXCEPT
{
    if( len == 0 )
    {
        return buffer;
    }

    if( len == 1 )
    {
        buffer[0] = 0;
        return buffer;
    }

    motionmatchingboost::winapi::UINT_ const code_page = message_cp_win32();

    int r = 0;

#if !defined(BOOST_NO_ANSI_APIS)

    if( code_page == motionmatchingboost::winapi::CP_ACP_ )
    {
        using namespace motionmatchingboost::winapi;

        DWORD_ retval = motionmatchingboost::winapi::FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM_ | FORMAT_MESSAGE_IGNORE_INSERTS_,
            NULL,
            ev,
            MAKELANGID_( LANG_NEUTRAL_, SUBLANG_DEFAULT_ ), // Default language
            buffer,
            static_cast<DWORD_>( len ),
            NULL
        );

        r = static_cast<int>( retval );
    }
    else

#endif

    {
        using namespace motionmatchingboost::winapi;

        wchar_t * lpMsgBuf = 0;

        DWORD_ retval = motionmatchingboost::winapi::FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER_ | FORMAT_MESSAGE_FROM_SYSTEM_ | FORMAT_MESSAGE_IGNORE_INSERTS_,
            NULL,
            ev,
            MAKELANGID_( LANG_NEUTRAL_, SUBLANG_DEFAULT_ ), // Default language
            (LPWSTR_) &lpMsgBuf,
            0,
            NULL
        );

        if( retval != 0 )
        {
            r = motionmatchingboost::winapi::WideCharToMultiByte( code_page, 0, lpMsgBuf, -1, buffer, static_cast<int>( len ), NULL, NULL );
            motionmatchingboost::winapi::LocalFree( lpMsgBuf );
            if ( r != 0 ) --r; // exclude null terminator
        }
    }

    if( r == 0 )
    {
        return unknown_message_win32( ev, buffer, len );
    }

    while( r > 0 && ( buffer[ r-1 ] == '\n' || buffer[ r-1 ] == '\r' ) )
    {
        buffer[ --r ] = 0;
    }

    if( r > 0 && buffer[ r-1 ] == '.' )
    {
        buffer[ --r ] = 0;
    }

    return buffer;
}

struct local_free
{
    void * p_;

    ~local_free()
    {
        motionmatchingboost::winapi::LocalFree( p_ );
    }
};

inline std::string unknown_message_win32( int ev )
{
    char buffer[ 38 ];
    return unknown_message_win32( ev, buffer, sizeof( buffer ) );
}

inline std::string system_category_message_win32( int ev )
{
    using namespace motionmatchingboost::winapi;

    wchar_t * lpMsgBuf = 0;

    DWORD_ retval = motionmatchingboost::winapi::FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER_ | FORMAT_MESSAGE_FROM_SYSTEM_ | FORMAT_MESSAGE_IGNORE_INSERTS_,
        NULL,
        ev,
        MAKELANGID_( LANG_NEUTRAL_, SUBLANG_DEFAULT_ ), // Default language
        (LPWSTR_) &lpMsgBuf,
        0,
        NULL
    );

    if( retval == 0 )
    {
        return unknown_message_win32( ev );
    }

    local_free lf_ = { lpMsgBuf };
    (void)lf_;

    UINT_ const code_page = message_cp_win32();

    int r = motionmatchingboost::winapi::WideCharToMultiByte( code_page, 0, lpMsgBuf, -1, 0, 0, NULL, NULL );

    if( r == 0 )
    {
        return unknown_message_win32( ev );
    }

    std::string buffer( r, char() );

    r = motionmatchingboost::winapi::WideCharToMultiByte( code_page, 0, lpMsgBuf, -1, &buffer[0], r, NULL, NULL );

    if( r == 0 )
    {
        return unknown_message_win32( ev );
    }

    --r; // exclude null terminator

    while( r > 0 && ( buffer[ r-1 ] == '\n' || buffer[ r-1 ] == '\r' ) )
    {
        --r;
    }

    if( r > 0 && buffer[ r-1 ] == '.' )
    {
        --r;
    }

    buffer.resize( r );

    return buffer;
}

} // namespace detail

} // namespace system

} // namespace motionmatchingboost

#endif // #ifndef BOOST_SYSTEM_DETAIL_SYSTEM_CATEGORY_MESSAGE_WIN32_HPP_INCLUDED
