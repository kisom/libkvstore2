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


#include <sys/types.h>
#include <sys/time.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "kv.h"


/* static int      _keymatch(char *, char *); */
static int      _lock_kvstore(kvstore);
static int      _unlock_kvstore(kvstore);


struct _kvstore_kv {
        char                    *key;
        size_t                   key_len;
        char                    *val;
        size_t                   val_len;
        TAILQ_ENTRY(_kvs_kv)     entries;
};
TAILQ_HEAD(_tq_kvstore_kv, _kvstore_kv);

struct _kvstore {
        struct _tq_kvstore_kv   *queue;
        sem_t                   *sem;
        size_t                   refs;
        struct timeval           timeo;
};


int
_lock_kvstore(kvstore kvs)
{
        struct timeval   ts;
        int              retval;

        ts.tv_sec = kvs->timeo.tv_sec;
        ts.tv_usec = kvs->timeo.tv_usec;

        retval = sem_trywait(kvs->sem);
        if (-1 == retval) {
                select(0, NULL, NULL, NULL, &ts);
                return sem_trywait(kvs->sem);
        }
        return retval;
}


int
_unlock_kvstore(kvstore kvs)
{
        return sem_post(kvs->sem);
}


/*
 * compare two keys. neither key should be NULL! returns 1 if the
 * keys match, and 0 if they do not.
 */
/*
int
_keymatch(char *key1, char *key2)
{
        size_t   i;

        i = 0;
        while (1) {
                if (0x0 == key1[i]) {
                        if (0x0 == key2[i])
                                return 1;
                        else
                                return 0;
                }

                if (0x0 == key2[i])
                        return 0;
                if (key1[i] != key2[i])
                        return 0;
                i++;
        }
}
 */

kvstore
kvstore_new(void)
{
        kvstore kvs;

        kvs = (kvstore)malloc(sizeof(struct _kvstore));
        if (NULL == kvs)
                return NULL;
        kvs->refs = 1;

        kvs->sem = (sem_t *)malloc(sizeof(sem_t));
        if (sem_init(kvs->sem, 0, 0)) {
                kvstore_discard(kvs);
                return NULL;
        }

        kvs->queue = (struct _tq_kvstore_kv *)malloc(
                sizeof(struct _tq_kvstore_kv));
        if (NULL == kvs->queue) {
                kvstore_discard(kvs);
                return NULL;
        }

        kvs->timeo.tv_sec = 0;
        kvs->timeo.tv_usec = 10000;
        _unlock_kvstore(kvs);

        return kvs;
}


int
kvstore_discard(kvstore kvs)
{
        if (NULL == kvs)
                return 0;
        if (NULL == kvs->sem) {
                free(kvs);
                return 0;
        }
        while (_lock_kvstore(kvs)) ;
        kvs->refs--;

        if (kvs->refs) {
                return _unlock_kvstore(kvs);
        }

        free(kvs->queue);
        while (_unlock_kvstore(kvs)) ;
        sem_close(kvs->sem);
        free(kvs->sem);
        free(kvs);

        return 0;
}


int
kvstore_dup(kvstore kvs)
{
        if (_lock_kvstore(kvs))
                return -1;
        kvs->refs++;
        return _unlock_kvstore(kvs);
}
