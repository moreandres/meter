#include <stdlib.h>
#include <assert.h>
#include <time.h>

double * init(int n)
{
  assert(n > 0);

  double * res = malloc(n * n * sizeof(double));

  srandom(time(NULL));
  
  int i,j;
  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++)
      res[i * n + j] = 1.0 * random() / RAND_MAX;
  
  return res;
}

int mm(double * a, double * b, double * c, int n)
{
  assert(a != NULL && b != NULL && c != NULL && n > 0);
    
  int i, j, k;

#pragma omp parallel for shared(a,b,c)
  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++)
      for (k = 0; k < n; k++)
        c[i * n + j] += a[i * n + k] * b[k * n + j];

  return 0;
}

int main(int argc, char *argv[])
{
  int size = (argc == 2) ? atoi(argv[1]) : 512;

  double *a = init(size);
  double *b = init(size);
  double *c = calloc(size * size, sizeof(double));

  mm(a, b, c, size);

  return 0;
}
