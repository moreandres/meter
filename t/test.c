#include <stdlib.h>
#include <check.h>

#define check(x, msg) ck_assert_msg(x, msg)

int main ()
{
  int failed = 0;

  Suite *s = suite_create("test");
  TCase *tc = tcase_create("test"); 

  suite_add_tcase(s, tc);

  SRunner *r = srunner_create(s);
  srunner_run_all(r, CK_VERBOSE);
  failed = srunner_ntests_failed(r);
  srunner_free(r);

  return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
