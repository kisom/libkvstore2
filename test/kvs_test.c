/*
 * Copyright (c) 2013 Kyle Isom <kyle@tyrfingr.is>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 * ---------------------------------------------------------------------
 */


#include <sys/time.h>
#include <sys/types.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "kv.h"


static void
test_kvstore_new(void)
{
        kvstore kvs;

        kvs = kvstore_new();
        CU_ASSERT_FATAL(NULL != kvs);

        CU_ASSERT(0 == kvstore_discard(kvs));
}


static void
test_kvstore_refcount(void)
{
        kvstore kvs;

        kvs = kvstore_new();
        CU_ASSERT_FATAL(NULL != kvs);
        CU_ASSERT(0 == kvstore_dup(kvs));

        CU_ASSERT(0 == kvstore_discard(kvs));
        CU_ASSERT(0 == kvstore_discard(kvs));
}


static void
test_kvstore_many_refcounts(void)
{
        kvstore  kvs;
        size_t   i;

        CU_ASSERT_FATAL(NULL != (kvs = kvstore_new()));
        for (i = 0; i < 1000000; i++)
                CU_ASSERT(0 == kvstore_dup(kvs));

        for (i = 0; i < 1000000; i++)
                CU_ASSERT(0 == kvstore_discard(kvs));

        CU_ASSERT(0 == kvstore_discard(kvs));
}


static void
test_kvstore_set(void)
{
        kvstore  kvs;
        char     test_key[] = "hello";
        char     test_val[] = "world";

        CU_ASSERT_FATAL(NULL != (kvs = kvstore_new()));
        CU_ASSERT(0 == kvstore_set(kvs, test_key, test_val));

        CU_ASSERT(0 == kvstore_discard(kvs));
}


static void
test_kvstore_update(void)
{
        kvstore  kvs;
        char     test_key[] = "hello";
        char     test_val[] = "world";
        char     test_val2[] = "world!";

        CU_ASSERT_FATAL(NULL != (kvs = kvstore_new()));
        CU_ASSERT(0 == kvstore_set(kvs, test_key, test_val));
        CU_ASSERT(0 == kvstore_set(kvs, test_key, test_val2));

        CU_ASSERT(0 == kvstore_discard(kvs));
}


/*
 * suite set up functions
 */
int
initialise_kvstore_test()
{
        return 0;
}

int
cleanup_kvstore_test()
{
        return 0;
}


void
destroy_test_registry()
{
        CU_cleanup_registry();
        exit(CU_get_error());
}


int
main(int argc, char *argv[])
{
        CU_pSuite kvstore_suite = NULL;
        unsigned int fails = 0;
        printf("starting tests for kvstore...\n");

        if (!CUE_SUCCESS == CU_initialize_registry()) {
                fprintf(stderr, "error initialising CUnit test registry!\n");
                return EXIT_FAILURE;
        }

        /*
         * set up the suite
         */
        kvstore_suite = CU_add_suite("kvstore_tests", initialise_kvstore_test,
                                 cleanup_kvstore_test);

        if (NULL == kvstore_suite)
                destroy_test_registry();

        if (NULL == CU_add_test(kvstore_suite, "lifecycle",
                    test_kvstore_new))
                destroy_test_registry();

        if (NULL == CU_add_test(kvstore_suite, "double refcount check",
                    test_kvstore_refcount))
                destroy_test_registry();

        if (NULL == CU_add_test(kvstore_suite, "many refcounts check",
                    test_kvstore_many_refcounts))
                destroy_test_registry();

        if (NULL == CU_add_test(kvstore_suite, "kvstore set",
                    test_kvstore_set))
                destroy_test_registry();

        if (NULL == CU_add_test(kvstore_suite, "kvstore update",
                    test_kvstore_update))
                destroy_test_registry();

        CU_basic_set_mode(CU_BRM_VERBOSE);
        CU_basic_run_tests();
        fails = CU_get_number_of_tests_failed();

        CU_cleanup_registry();
        return fails;
}
