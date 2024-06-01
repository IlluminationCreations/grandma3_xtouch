#include <assert.h>

#define ASSERT_INT(x, s) if (x != true) { printf(#s ": %d\n", x); return x; }
#define GUARD_CONTINUE(x) if (x != true) { continue; }