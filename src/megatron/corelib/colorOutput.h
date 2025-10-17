#pragma once

#ifndef __MOBILE__
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define ANSI_COLOR_RED2     "\x1b[91m"
#define ANSI_COLOR_GREEN2   "\x1b[92m"
#define ANSI_COLOR_YELLOW2  "\x1b[93m"
#define ANSI_COLOR_BLUE2    "\x1b[94m"
#define ANSI_COLOR_MAGENTA2 "\x1b[95m"
#define ANSI_COLOR_CYAN2    "\x1b[96m"
#else
#define ANSI_COLOR_RED     ""
#define ANSI_COLOR_GREEN   ""
#define ANSI_COLOR_YELLOW  ""
#define ANSI_COLOR_BLUE    ""
#define ANSI_COLOR_MAGENTA ""
#define ANSI_COLOR_CYAN    ""
#define ANSI_COLOR_RESET   ""

#define ANSI_COLOR_RED2     ""
#define ANSI_COLOR_GREEN2   ""
#define ANSI_COLOR_YELLOW2  ""
#define ANSI_COLOR_BLUE2    ""
#define ANSI_COLOR_MAGENTA2 ""
#define ANSI_COLOR_CYAN2    ""
#endif

#define GREEN(a) ANSI_COLOR_GREEN a ANSI_COLOR_RESET "\n"
#define BLUE(a) ANSI_COLOR_BLUE a ANSI_COLOR_RESET "\n"
#define RED(a) ANSI_COLOR_RED a ANSI_COLOR_RESET "\n"
#define YELLOW(a) ANSI_COLOR_YELLOW a ANSI_COLOR_RESET "\n"
#define MAGENTA(a) ANSI_COLOR_MAGENTA a ANSI_COLOR_RESET "\n"
#define CYAN(a) ANSI_COLOR_CYAN a ANSI_COLOR_RESET "\n"


#define GREEN2(a) ANSI_COLOR_GREEN2 a ANSI_COLOR_RESET "\n"
#define BLUE2(a) ANSI_COLOR_BLUE2 a ANSI_COLOR_RESET "\n"
#define RED2(a) ANSI_COLOR_RED2 a ANSI_COLOR_RESET "\n"
#define YELLOW2(a) ANSI_COLOR_YELLOW2 a ANSI_COLOR_RESET "\n"
#define MAGENTA2(a) ANSI_COLOR_MAGENTA2 a ANSI_COLOR_RESET "\n"
#define CYAN2(a) ANSI_COLOR_CYAN2 a ANSI_COLOR_RESET "\n"
