/* sunrise_tool.c - print drik sunrise JD (UT) for a list of dates.
 *
 * Reads "YYYY-MM-DD" lines on stdin, writes "YYYY-MM-DD,<jd_ut>" on stdout.
 * Used to feed the project's real sunrise into the Python Surya Siddhanta
 * experiments without reimplementing it.
 *
 * Build (from the repo root, after `make`):
 *   cc -O2 -std=c99 -Ilib/moshier -Isrc \
 *      -o validation/suryasiddhanta/sunrise_tool \
 *      validation/suryasiddhanta/sunrise_tool.c \
 *      build/moshier/[*].o build/astro.o build/date_utils.o -lm
 */
#include <stdio.h>
#include "astro.h"
#include "date_utils.h"
#include "types.h"

int main(void)
{
    astro_init(NULL);
    Location loc = DEFAULT_LOCATION;

    char line[64];
    while (fgets(line, sizeof line, stdin)) {
        int y, m, d;
        if (sscanf(line, "%d-%d-%d", &y, &m, &d) != 3) continue;
        double jd = gregorian_to_jd(y, m, d);
        double sr = sunrise_jd(jd, &loc);
        printf("%04d-%02d-%02d,%.9f\n", y, m, d, sr);
    }

    astro_close();
    return 0;
}
