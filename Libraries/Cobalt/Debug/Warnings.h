// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
namespace cobalt { namespace debug {

// Disable all warning in the following region. Useful when directly including third party headers which trigger
// warnings with our defined warning levels. Note that __clang__ must come before _MSC_VER here to correctly support
// clang-cl, which defines _MSC_VER but does not support the MSVC style warning pragma statements.
#ifdef __clang__
#define WARNINGS_PUSH_OFF            \
	_Pragma("clang diagnostic push") \
	  _Pragma("clang diagnostic ignored \"-Weverything\"")
#elif defined(_MSC_VER)
#define WARNINGS_PUSH_OFF __pragma(warning(push, 0))
#elif defined(__GNUC__)
#define WARNINGS_PUSH_OFF                                                                     \
	_Pragma("GCC diagnostic push")                                                            \
	  _Pragma("GCC diagnostic ignored \"-Wunused-variable\"")                                 \
	    _Pragma("GCC diagnostic ignored \"-Wunused-parameter\"")                              \
	      _Pragma("GCC diagnostic ignored \"-Wunused-function\"")                             \
	        _Pragma("GCC diagnostic ignored \"-Wunused-but-set-variable\"")                   \
	          _Pragma("GCC diagnostic ignored \"-Wsign-compare\"")                            \
	            _Pragma("GCC diagnostic ignored \"-Wconversion\"")                            \
	              _Pragma("GCC diagnostic ignored \"-Wshadow\"")                              \
	                _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")           \
	                  _Pragma("GCC diagnostic ignored \"-Wformat\"")                          \
	                    _Pragma("GCC diagnostic ignored \"-Wstrict-aliasing\"")               \
	                      _Pragma("GCC diagnostic ignored \"-Wpedantic\"")                    \
	                        _Pragma("GCC diagnostic ignored \"-Wcast-align\"")                \
	                          _Pragma("GCC diagnostic ignored \"-Wdouble-promotion\"")        \
	                            _Pragma("GCC diagnostic ignored \"-Wfloat-equal\"")           \
	                              _Pragma("GCC diagnostic ignored \"-Wredundant-decls\"")     \
	                                _Pragma("GCC diagnostic ignored \"-Wlogical-op\"")        \
	                                  _Pragma("GCC diagnostic ignored \"-Wuseless-cast\"")    \
	                                    _Pragma("GCC diagnostic ignored \"-Wuninitialized\"") \
	                                      _Pragma("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
#else
#define WARNINGS_PUSH_OFF
#endif

// Re-enable warnings which were disabled by a previous call to WARNINGS_PUSH_OFF.
#ifdef __clang__
#define WARNINGS_POP _Pragma("clang diagnostic pop")
#elif defined(_MSC_VER)
#define WARNINGS_POP __pragma(warning(pop))
#elif defined(__GNUC__)
#define WARNINGS_POP _Pragma("GCC diagnostic pop")
#else
#define WARNINGS_POP
#endif

}} // namespace cobalt::debug
