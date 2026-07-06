#pragma once

/// macro used with typed int|string typed

#ifdef DEBUG
#define CONTAINER(a) a.container
#else
#define CONTAINER(a) a
#endif
