AC_PREREQ([2.68])
AC_INIT([meter], [0.1], [more.andres@gmail.com])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/meter.c])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CC

AC_CHECK_LIB([check], [suite_create])
AC_CHECK_LIB([m], [cos])
AC_CHECK_LIB([gslcblas], [cblas_dgemm])
AC_CHECK_LIB([gsl], [gsl_blas_dgemm])

AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([sys/time.h])

AC_CHECK_FUNCS([gettimeofday])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([strncasecmp])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 t/Makefile])
AC_OUTPUT
