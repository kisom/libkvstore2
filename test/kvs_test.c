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


static const size_t      MAX_WORD_LEN = 32;
#ifndef KVS_TEST_DICT
#define KVS_TEST_DICT   "/usr/share/dict/words"
#endif


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
        char    *get_val = NULL;

        CU_ASSERT_FATAL(NULL != (kvs = kvstore_new()));
        CU_ASSERT(0 == kvstore_set(kvs, test_key, test_val));

        get_val = kvstore_get(kvs, test_key);
        CU_ASSERT(NULL != get_val);
        CU_ASSERT(0 == strncmp(get_val, test_val, sizeof(test_val)));

        CU_ASSERT(0 == kvstore_discard(kvs));
}


static void
test_kvstore_update(void)
{
        kvstore  kvs;
        char     test_key[] = "hello";
        char     test_val[] = "world";
        char     test_val2[] = "world!";
        char    *get_val = NULL;

        CU_ASSERT_FATAL(NULL != (kvs = kvstore_new()));
        CU_ASSERT(0 == kvstore_set(kvs, test_key, test_val));
        get_val = kvstore_get(kvs, test_key);
        CU_ASSERT(NULL != get_val);
        CU_ASSERT(0 == strncmp(get_val, test_val, sizeof(test_val)));

        CU_ASSERT(0 == kvstore_set(kvs, test_key, test_val2));
        get_val = kvstore_get(kvs, test_key);
        CU_ASSERT(NULL != get_val);
        CU_ASSERT(0 == strncmp(get_val, test_val2, sizeof(test_val2)));

        CU_ASSERT(0 == kvstore_discard(kvs));
}


static void
test_kvstore_del(void)
{
        kvstore  kvs;
        char     test_key[] = "hello";
        char     test_val[] = "world";
        char    *get_val = NULL;

        CU_ASSERT_FATAL(NULL != (kvs = kvstore_new()));
        CU_ASSERT(0 == kvstore_set(kvs, test_key, test_val));

        get_val = kvstore_get(kvs, test_key);
        CU_ASSERT(NULL != get_val);
        CU_ASSERT(0 == strncmp(get_val, test_val, sizeof(test_val)));

        CU_ASSERT(0 == kvstore_del(kvs, test_key));
        get_val = kvstore_get(kvs, test_key);
        CU_ASSERT(NULL == get_val);

        CU_ASSERT(0 == kvstore_discard(kvs));
}


static void
test_kvstore_multikey(void)
{
        kvstore          kvs;
        char             key1[] = "key1";
        char             key2[] = "key2";
        char             key3[] = "key3";
        char             val1[] = "value1";
        char             val1a[] = "ohgodwhatsthis";
        char             val2[] = "value2";
        char             val3[] = "value3";

        CU_ASSERT_FATAL(NULL != (kvs = kvstore_new()));
        CU_ASSERT_FATAL(0 == kvstore_set(kvs, key1, val1));
        CU_ASSERT_FATAL(0 == kvstore_set(kvs, key2, val2));
        CU_ASSERT_FATAL(0 == kvstore_set(kvs, key3, val3));
        CU_ASSERT(0 == strncmp(kvstore_get(kvs, key1), val1, MAX_WORD_LEN));
        CU_ASSERT(0 == strncmp(kvstore_get(kvs, key2), val2, MAX_WORD_LEN));
        CU_ASSERT(0 == strncmp(kvstore_get(kvs, key3), val3, MAX_WORD_LEN));
        CU_ASSERT(0 == kvstore_set(kvs, key1, val1a));
        CU_ASSERT(0 == strncmp(kvstore_get(kvs, key1), val1a, MAX_WORD_LEN));
        CU_ASSERT(3 == kvstore_len(kvs));
        CU_ASSERT(0 == kvstore_discard(kvs));
}


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

        if (NULL == CU_add_test(kvstore_suite, "kvstore delete",
                    test_kvstore_del))
                destroy_test_registry();

        if (NULL == CU_add_test(kvstore_suite, "multikey test",
                    test_kvstore_multikey))
                destroy_test_registry();

        CU_basic_set_mode(CU_BRM_VERBOSE);
        CU_basic_run_tests();
        fails = CU_get_number_of_tests_failed();

        CU_cleanup_registry();
        return fails;
}
