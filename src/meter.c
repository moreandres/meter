#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <argp.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>
#include <gsl/gsl_statistics.h>
#include <unistd.h>
/* TBD: get from config.h */
const char *argp_program_version = "facts 0.1";
const char *argp_program_bug_address = "<more.andres@gmail.com>";

static char doc[] = "facts -- generate performance reports";
static char args_doc[] = "PROGRAM FIRST INC LAST";

static struct argp_option options[] = {};

#define ARGS 4

struct args
{
	char *args[ARGS];
};

static error_t parse_opt (int key, char *arg, struct argp_state *state)
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
	
	strftime(buffer, 32, "%Y%m%d-%H%M%S",
		 localtime(&time));

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

	int ret = system(buffer);
	if (ret)
		ret = ret;

	// WEXITSTATUS(ret);

	return wtime() - start;
}

int main(int argc, char **argv)
{
	struct args args;
	argp_parse (&argp, argc, argv, 0, 0, &args);

	stamp = timestamp();
	start = wtime();
	memset(items, '\n', sizeof(struct item) * ITEMS);

	print("%s", date());
	print("Timestamp %s\n", stamp);

	print("Running system benchmark...\n");

	// sys("mpirun -np4 `which hpcc`");

	char *program = args.args[0];
	print("PROGRAM = %s\n", program);
	print("RANGE = [ %s, %s, %s ]\n",
	      args.args[1], args.args[2], args.args[3]);

	sys("mkdir %s", stamp);

#define ITERS 4

	double data[ITERS];

	int i;
	for (i = 0; i < ITERS; i++) {
		double t = sys("%s 2>&1 > %s/%s-%02d.log",
			       program, stamp, stamp, i);
		data[i] = t;
		print("iter %02d %f\n", i, t);
	}

	double mean, variance, largest, smallest;

	mean = gsl_stats_mean(data, 1, 5);
	variance = gsl_stats_variance(data, 1, 5);
	largest  = gsl_stats_max(data, 1, 5);
	smallest = gsl_stats_min(data, 1, 5);

	print("The sample mean is %g\n", mean);
	print("The estimated variance is %g\n", variance);
	print("The largest value is %g\n", largest);
	print("The smallest value is %g\n", smallest);
	
	int first, last, increment;

	first = atoi(args.args[1]);
	last = atoi(args.args[2]);
	increment = atoi(args.args[3]);

	int j;
	for (j = first; j < last; j += increment) {
		double t = sys("%s %d 2>&1 > %s/%s-size-%02d.log",
			       program, j, stamp, stamp, j);
		print("size %02d %f\n", j, t);
	}

	long cores = sysconf(_SC_NPROCESSORS_ONLN);

	print("cores %ld\n", cores);

	int k;
	for (k = 1; k <= cores; k++) {
		double t = sys("OMP_NUM_THREADS=%d %s %d 2>&1 > %s/%s-cores-%02d.log",
			       k, program, first, stamp, stamp, j);
		print("cores %02d %f\n", k, t);
	}

	print("serial: x%\n");
	print("parallel: x%\n");

	print("amdalah: x%\n");
	print("gustafson: x%\n");

	print("%s", date());

	exit (0);
}
