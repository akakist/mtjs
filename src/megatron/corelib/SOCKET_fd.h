#pragma once
#include "CONTAINER.h"
///  wrapper for int for debug

#ifdef DEBUG
struct SOCKET_fd
{
    int container;
    SOCKET_fd(int fd):container(fd) {}
};
inline int operator < (const SOCKET_fd& a, const SOCKET_fd &b)
{
    return CONTAINER(a)< CONTAINER(b);
}
#else
typedef int SOCKET_fd;
#endif

