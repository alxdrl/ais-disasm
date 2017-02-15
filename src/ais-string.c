#include <unistd.h>

#ifndef HAVE_STRLCPY

#include "ais-string.h"

/* strlcpy
*
* Copy string src to buffer dest (of buffer size dest_size). At most
* dest_size-1 characters will be copied. Always NUL terminates
* (unless dest_size == 0). This function does NOT allocate memory.
* Unlike strncpy, this function doesn't pad dest (so it's often faster).
* Returns size of attempted result, strlen(src),
* so if retval >= dest_size, truncation occurred.
*/

size_t strlcpy (char *dest, const char *src, size_t dest_size)
{
register char *d = dest;
register const char *s = src;
register size_t n = dest_size;

if (dest == NULL) return (0);
if (src == NULL) return (0);

/* Copy as many bytes as will fit */
if (n != 0 && --n != 0) {
do {
register char c = *s++;

*d++ = c;
if (c == 0)
break;
} while (--n != 0);
}

/* If not enough room in dest, add NUL and traverse rest of src */
if (n == 0) {
if (dest_size != 0)
*d = 0;
while (*s++) ;
}

return (s - src - 1); /* count does not include NUL */
} /* strlcpy */

#endif
