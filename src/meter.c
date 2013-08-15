#include <argp.h>
#include <gsl/gsl_statistics.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "config.h"

const char *argp_program_version = PACKAGE_STRING;
const char *argp_program_bug_address = PACKAGE_BUGREPORT;

static char doc[] = PACKAGE_NAME " - performance report generator";
static char args_doc[] = "PROGRAM FIRST INC LAST";

static struct argp_option options[] = { };

#define ARGS 4

struct args {
	char *args[ARGS];
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	struct args *args = state->input;

	switch (key) {
	case ARGP_KEY_ARG:
		if (state->arg_num >= ARGS)
			argp_usage(state);
		args->args[state->arg_num] = arg;
		break;
	case ARGP_KEY_END:
		if (state->arg_num < ARGS)
			argp_usage(state);
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}
	return EXIT_SUCCESS;
}

static struct argp argp =
    { options, parse_opt, args_doc, doc, NULL, NULL, NULL };

// "program.component.key" => value
struct item {
	char *key;
	char *value;
};

#define ITEMS 32
struct item items[ITEMS];

char *cfg(char *key)
{
	int i;

	for (i = 0; i < ITEMS; i++) {
		if (!strncasecmp(key, items[i].key, strlen(key))) {
			return items[i].value;
		}
	}

	return getenv(key);
}

#define RED "\e[31m"
#define GREEN "\e[32m"
#define YELLOW "\e[33m"
#define BLUE "\e[34m"
#define RESET "\e[0m"

char *stamp = NULL;

char *date(void)
{
	time_t t = time(NULL);
	return ctime(&t);
}

char *timestamp(void)
{
	char *buffer = calloc(32, sizeof(char));

	struct timeval tv;
	time_t time;

	gettimeofday(&tv, NULL);
	time = tv.tv_sec;

	strftime(buffer, 32, "%Y%m%d-%H%M%S", localtime(&time));

	return buffer;
}

double wtime(void)
{
	double sec;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	sec = tv.tv_sec + tv.tv_usec / 1000000.0;

	return sec;
}

double start;

void print(char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	printf(BLUE "[%fs] " RESET, wtime() - start);
	vprintf(fmt, args);
	printf(RESET);
	va_end(args);
}

double sys(char *fmt, ...)
{
	char buffer[1024];

	va_list args;

	va_start(args, fmt);
	vsprintf(buffer, fmt, args);
	va_end(args);

	double start = wtime();

	print(buffer);
	printf("\n");

	int ret = system(buffer);
	if (ret)
		ret = ret;

	// WEXITSTATUS(ret);

	return wtime() - start;
}

int main(int argc, char **argv)
{
	struct args args;
	argp_parse(&argp, argc, argv, 0, 0, &args);

	stamp = timestamp();
	start = wtime();
	memset(items, '\n', sizeof(struct item) * ITEMS);

	print("Starting at %s", date());
	print("Timestamp %s\n", stamp);

	print("Running system benchmark...\n");

	// /usr/share/doc/hpcc/examples/_hpccinf.txt
	// "program.hpcc.output_file_name = HPL.out"
	// "program.hpcc.ns = 8"
	// "program.hpcc.number_of_nbs = 1"
	// "program.hpcc.pmap_process_mapping = 0"

	// sys("mpirun `which hpcc` -np %d", 4); // 4m28.456s
	{
#define ARRAY_SIZE(x) ( sizeof(x) / sizeof(*x) )

	  char *benchmarks[] = { "StarDGEMM_Gflops",
				 "PTRANS_GBs",
				 "StarRandomAccess_GUPs",
				 "StarSTREAM_Triad",
				 "StarFFT_Gflops" };
	  int size = ARRAY_SIZE(benchmarks);
	  int i = 0;
	  for (i = 0; i < size; i++) {
	    sys("grep %s hpccoutf.txt | cut -f2 -d=",
		benchmarks[i]);
	  }
	}

	char *program = args.args[0];
	print("PROGRAM = %s\n", program);
	print("RANGE = [ %s, %s, %s ]\n",
	      args.args[1], args.args[2], args.args[3]);

	sys("make --silent --no-print-directory clean; make --silent --no-print-directory");

	char dir[128];
	sprintf(dir, ".%s/%s", PACKAGE_NAME, stamp);

	sys("mkdir --parents %s", dir);

#define ITERS 8

	double data[ITERS];

	int i;
	for (i = 0; i < ITERS; i++) {
		double t = sys("./%s 2>&1 > .%s/%s/%s-%02d.log",
			       program, PACKAGE_NAME, stamp, stamp, i);
		data[i] = t;
		print("Run %02d took %f secs\n", i, t);
	}

	double mean, variance, largest, smallest;

	mean = gsl_stats_mean(data, 1, 5);
	variance = gsl_stats_variance(data, 1, 5);
	largest = gsl_stats_max(data, 1, 5);
	smallest = gsl_stats_min(data, 1, 5);

	print("The sample mean is %f secs\n", mean);
	print("The estimated variance is %f\n", variance);
	print("The largest value is %f secs\n", largest);
	print("The smallest value is %f secs\n", smallest);

	int first, last, increment;

	first = atoi(args.args[1]);
	last = atoi(args.args[2]);
	increment = atoi(args.args[3]);

	double size[32];
	int l = 0;

	int j;
	for (j = first; j < last; j += increment) {
		double t = sys("%s %d 2>&1 > .%s/%s/%s-size-%02d.log",
			       program, j, PACKAGE_NAME, stamp, stamp, j);
		size[l++] = t;
		print("Using size %02d took %f secs\n", j, t);
	}

	long cores = sysconf(_SC_NPROCESSORS_ONLN);

	print("Detected %ld cores\n", cores);

	int k;
	for (k = 1; k <= cores; k++) {
		double t =
		    sys
		    ("OMP_NUM_THREADS=%d %s %d 2>&1 > .%s/%s/%s-cores-%02d.log",
		     k, program, first, PACKAGE_NAME, stamp, stamp, j);
		print("Using %d cores took %f secs\n", k, t);
	}

	print("Serial fraction: x%\n");
	print("Parallel fraction: x%\n");

	print("Ideal Speedup Amdalah is x%\n");
	print("Ideal Speedup Gustafson is x%\n");

	print("Generating result histogram\n");
	print("Generating scaling charts\n");
	print("Generating speedup charts\n");

	print("Ending at %s", date());

	exit(0);
}
