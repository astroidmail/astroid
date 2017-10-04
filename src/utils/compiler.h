# pragma once

# if defined (__clang__)

# ifndef __has_attribute
#   define __has_attribute(x) 0
# endif
# ifndef __has_cpp_attribute
#   define __has_cpp_attribute(x) 0
# endif
# ifndef __has_builtin
#   define __has_builtin(x) 0
# endif
# ifndef __has_feature
#   define __has_feature(x) 0
# endif
# ifndef __has_extension
#   define __has_extension(x) 0
# endif
# ifndef __has_warning
#   define __has_warning(x) 0
# endif
# if __has_cpp_attribute(clang::fallthrough)
#   define FALLTHROUGH [[clang::fallthrough]]
# else
#   define FALLTHROUGH
# endif

# elif defined (__GNUC__)

# if __has_attribute(fallthrough)
# define FALLTHROUGH __attribute__ ((fallthrough))
# else
# define FALLTHROUGH
# endif

# endif

