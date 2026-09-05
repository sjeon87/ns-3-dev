#ifndef SI_UNITS_LOGGER_H
#define SI_UNITS_LOGGER_H

#include "si-units-parser.h"
#include <cassert>
#include <iostream>

/// Asserts if condition is false, together with test condition and the message
#define UNITS_ASRT(condition, message)                                 \
    do                                                                 \
    {                                                                  \
        if (!(condition))                                              \
        {                                                              \
            std::cerr << "assertion failed: " << message << std::endl; \
            assert(condition);                                         \
        }                                                              \
    } while (false)


#endif // SI_UNITS_LOGGER_H
