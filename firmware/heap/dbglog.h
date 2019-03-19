// ----------------------------------------------------------------------------
// dbglog.h - A set of macros that cleans up code that needs to produce debug
//            or log information.
//
// See copyright notice in LICENSE.TXT
// ----------------------------------------------------------------------------
//
// There are macros to handle the following decreasing levels of detail:
//
// 6 = TRACE
// 5 = DEBUG
// 4 = CRITICAL
// 3 = ERROR
// 2 = WARNING
// 1 = INFO
// 0 = FORCE - The printf is always compiled in and is called only when
//              the first parameter to the macro is non-0
//
// ----------------------------------------------------------------------------
// 
// The following #define should be set up before this file is included so
// that we can be sure that the correct macros are defined.
//
// #define DBGLOG_LEVEL x
// ----------------------------------------------------------------------------

#ifndef DBGLOG_LEVEL
# error "DBGLOG_LEVEL is not defined!"
#endif

// ----------------------------------------------------------------------------
//
// FIXME: Currently the macros are defined at compile time, which means that
//        the debug level is fixed. It will be possible in later versions to
//        set up run time control of debug info at the expense of speed and
//        code size
// ----------------------------------------------------------------------------

#undef DBGLOG_TRACE
#undef DBGLOG_DEBUG
#undef DBGLOG_CRITICAL
#undef DBGLOG_ERROR
#undef DBGLOG_WARNING
#undef DBGLOG_INFO
#undef DBGLOG_FORCE

// ----------------------------------------------------------------------------

#if DBGLOG_LEVEL >= 6
#  define DBGLOG_TRACE( format, ... ) printf( format, ## __VA_ARGS__ )
#else
#  define DBGLOG_TRACE( format, ... )
#endif

#if DBGLOG_LEVEL >= 5
#  define DBGLOG_DEBUG( format, ... ) printf( format, ## __VA_ARGS__ )
#else
#  define DBGLOG_DEBUG( format, ... )
#endif

#if DBGLOG_LEVEL >= 4
#  define DBGLOG_CRITICAL( format, ... ) printf( format, ## __VA_ARGS__ )
#else
#  define DBGLOG_CRITICAL( format, ... )
#endif

#if DBGLOG_LEVEL >= 3
#  define DBGLOG_ERROR( format, ... ) printf( format, ## __VA_ARGS__ )
#else
#  define DBGLOG_ERROR( format, ... )
#endif

#if DBGLOG_LEVEL >= 2
#  define DBGLOG_WARNING( format, ... ) printf( format, ## __VA_ARGS__ )
#else
#  define DBGLOG_WARNING( format, ... )
#endif

#if DBGLOG_LEVEL >= 1
#  define DBGLOG_INFO( format, ... ) printf( format, ## __VA_ARGS__ )
#else
#  define DBGLOG_INFO( format, ... )
#endif

#define DBGLOG_FORCE( force, format, ... ) {if(force) {printf( format, ## __VA_ARGS__  );}}

// ----------------------------------------------------------------------------
