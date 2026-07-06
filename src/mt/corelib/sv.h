#pragma once
#define SV_FMT "%.*s"
#define SV_ARG(sv) static_cast<int>((sv).size()), (sv).data()
