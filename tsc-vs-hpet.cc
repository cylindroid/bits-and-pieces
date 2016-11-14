#include <cstdint>
#include <cstdio>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <sched.h>

#define DEBUG 0
#define DEBUG_DATA 0
#define MEASUREMENTS 200
#define USECSTEP 10
#define USECSTART 100

uint64_t rdtscp(void) {
    uint32_t lo, hi;
    __asm__ volatile ("rdtscp"
            : /* outputs */ "=a" (lo), "=d" (hi)
            : /* no inputs */
            : /* clobbers */ "%rcx");
    return (uint64_t)lo | (((uint64_t)hi) << 32);
}

typedef uint64_t cycles_t;


/*
   Use linear regression to calculate cycles per microsecond.
http://en.wikipedia.org/wiki/Linear_regression#Parameter_estimation
*/
static double sample_get_cpu_mhz(void)
{
	struct timeval tv1, tv2;
	cycles_t start;
	double sx = 0, sy = 0, sxx = 0, syy = 0, sxy = 0;
	double tx, ty;
	int i;

	/* Regression: y = a + b x */
	long x[MEASUREMENTS];
	cycles_t y[MEASUREMENTS];
	double a; /* system call overhead in cycles */
	double b; /* cycles per microsecond */
	double r_2;

	for (i = 0; i < MEASUREMENTS; ++i) {
		start = rdtscp();

		if (gettimeofday(&tv1, NULL)) {
			fprintf(stderr, "gettimeofday failed.\n");
			return 0;
		}

		do {
			if (gettimeofday(&tv2, NULL)) {
				fprintf(stderr, "gettimeofday failed.\n");
				return 0;
			}
		} while ((tv2.tv_sec - tv1.tv_sec) * 1000000 +
				(tv2.tv_usec - tv1.tv_usec) < USECSTART + i * USECSTEP);

		x[i] = (tv2.tv_sec - tv1.tv_sec) * 1000000 +
			tv2.tv_usec - tv1.tv_usec;
		y[i] = rdtscp() - start;
		if (DEBUG_DATA)
			fprintf(stderr, "x=%ld y=%Ld\n", x[i], (long long)y[i]);
	}

	for (i = 0; i < MEASUREMENTS; ++i) {
		tx = x[i];
		ty = y[i];
		sx += tx;
		sy += ty;
		sxx += tx * tx;
		syy += ty * ty;
		sxy += tx * ty;
	}

	b = (MEASUREMENTS * sxy - sx * sy) / (MEASUREMENTS * sxx - sx * sx);
	a = (sy - b * sx) / MEASUREMENTS;

	if (DEBUG)
		fprintf(stderr, "a = %g\n", a);
	if (DEBUG)
		fprintf(stderr, "b = %g\n", b);
	if (DEBUG)
		fprintf(stderr, "a / b = %g\n", a / b);
	r_2 = (MEASUREMENTS * sxy - sx * sy) * (MEASUREMENTS * sxy - sx * sy) /
		(MEASUREMENTS * sxx - sx * sx) /
		(MEASUREMENTS * syy - sy * sy);

	if (DEBUG)
		fprintf(stderr, "r^2 = %g\n", r_2);
	if (r_2 < 0.9) {
		fprintf(stderr,"Correlation coefficient r^2: %g < 0.9\n", r_2);
		return 0;
	}

	return b;
}

const double mhz = 3311.996;
uint64_t get_ns()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

int main()
{
    double freq = sample_get_cpu_mhz();
    fprintf(stderr, "measured mhz: %.2lf\n", freq);
    uint64_t tsc_begin = rdtscp();
    uint64_t hpet_begin = get_ns();
    sched_yield();
    uint64_t tsc_end = rdtscp();
    uint64_t hpet_end = get_ns();
    double tsc_diff_us = (tsc_end - tsc_begin) / freq;
    double hpet_diff_us = (hpet_end - hpet_begin) / 1000.0;
    printf("Elapsed  TSC time: %.2lf us\n"
           "Elapsed HPET time: %.2lf us\n", tsc_diff_us, hpet_diff_us);
    return 0;
}
