/* Extract SE delta-T values at yearly resolution for embedding */
#include <stdio.h>
#include <math.h>
extern double swe_deltat_ex(double tjd, int iflag, char *serr);
extern void swe_set_ephe_path(char *path);
extern void swe_close(void);
#define SEFLG_MOSEPH 4

int main(void) {
    swe_set_ephe_path(NULL);
    char serr[256];

    printf("/* SE delta-T values (seconds), yearly from 1900 to 2050 */\n");
    printf("static const double se_dt_table[] = {\n");

    for (int year = 1900; year <= 2050; year++) {
        /* JD for Jan 1.5 of each year (same convention as SE table) */
        double jd = 2451545.0 + (year - 2000) * 365.25;
        double dt = swe_deltat_ex(jd, SEFLG_MOSEPH, serr) * 86400.0;

        if (year % 5 == 0)
            printf("    /* %d */ ", year);
        printf("%7.3f, ", dt);
        if (year % 5 == 4)
            printf("\n");
    }
    printf("\n};\n");
    printf("#define SE_DT_START 1900\n");
    printf("#define SE_DT_END   2050\n");

    swe_close();
    return 0;
}
