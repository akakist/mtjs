#pragma once
#include <limits>
#include <cstddef>
#include <stdint.h>
#ifndef _MSC_VER
//#include <sys/cdefs.h>
#endif
#include <string>
#include <sys/types.h>
#include "pconfig.h"

#ifdef REAL_FLOAT
typedef float real;
#else
#ifdef REAL_DOUBLE
typedef double real;
#else
#error need define REAL_FLOAT or REAL_DOUBLE
#endif

#endif


