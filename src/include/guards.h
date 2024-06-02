#include <assert.h>

#define ASSERT_INT(x, s) if (x != true) { printf(#s ": %d\n", x); return x; }
#define GUARD_CONTINUE(x) if (x != true) { continue; }
#define ASSERT_EQ_INT(x, y) if (x != y) { printf(#x " == " #y " failed. " #x "=%u, " #y "=%u\n", x, y); assert(x == y); }