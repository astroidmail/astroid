# pragma once

# if defined (__clang__)

# if __has_attribute(clang::fallthrough)
# define FALLTHROUGH [[clang::fallthrough]]
# else
# define FALLTHROUGH
# endif

# elif defined (__GNUC__)

# if __has_attribute(fallthrough)
# define FALLTHROUGH __attribute__ ((fallthrough))
# else
# define FALLTHROUGH
# endif

# endif

