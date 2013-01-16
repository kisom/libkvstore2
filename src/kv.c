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
#include <errno.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "kv.h"


const size_t      KVSTORE_DEFAULT_MAX_KEYLEN = 4096;
const size_t      KVSTORE_DEFAULT_MAX_VALLEN = 4096;


struct _kvstore_kv {
        char                    *key;
        size_t                   key_len;
        char                    *val;
        size_t                   val_len;
        TAILQ_ENTRY(_kvstore_kv) entries;
};
TAILQ_HEAD(_tq_kvstore_kv, _kvstore_kv);

struct _kvstore {
        struct _tq_kvstore_kv   *queue;
        sem_t                   *sem;
        size_t                   refs;
        size_t                   max_keylen;
        size_t                   max_vallen;
        struct timeval           timeo;
};


static int      _keymatch(char *, char *);
static int      _lock_kvstore(kvstore);
static int      _unlock_kvstore(kvstore);
static int      _kvstore_update(kvstore, struct _kvstore_kv *, char *);


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
                retval = sem_trywait(kvs->sem);
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
int
_keymatch(char *key1, char *key2)
{
        size_t   i;

        i = 0;
        if (NULL == key1) {
                return 0;
        } else if (NULL == key2) {
                return 0;
        }

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


kvstore
kvstore_new(void)
{
        kvstore kvs;

        kvs = (kvstore)malloc(sizeof(struct _kvstore));
        if (NULL == kvs)
                return NULL;
        else
                memset(kvs, 0x0, sizeof(struct _kvstore));
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
        } else {
                memset(kvs->queue, 0x0, sizeof(struct _tq_kvstore_kv));
                TAILQ_INIT(kvs->queue);
        }

        kvs->timeo.tv_sec = 0;
        kvs->timeo.tv_usec = 10000;
        kvs->max_keylen = KVSTORE_DEFAULT_MAX_KEYLEN;
        kvs->max_vallen = KVSTORE_DEFAULT_MAX_VALLEN;
        _unlock_kvstore(kvs);

        return kvs;
}


int
kvstore_discard(kvstore kvs)
{
        struct _kvstore_kv      *kv;
        int                      retval;

        if (NULL == kvs)
                return 0;
        if (NULL == kvs->sem) {
                free(kvs);
                return 0;
        }
        while (1) {
                retval = _lock_kvstore(kvs) ;
                switch (retval) {
                case 0:
                        retval = 1;
                        break;
                case -1:
                        switch (errno) {
                        case EINVAL:
                                retval = 1;
                                break;
                        case EBADF:
                                retval = 1;
                                break;
                        default:
                                retval = 0;
                                printf("errno: %d\n", errno);
                                continue;
                        }
                }
                if (retval)
                        break;
        }
        kvs->refs--;

        if (kvs->refs) {
                return _unlock_kvstore(kvs);
        }

        while (NULL != (kv = TAILQ_FIRST(kvs->queue))) {
                free(kv->key);
                free(kv->val);
                TAILQ_REMOVE(kvs->queue, kv, entries);
                free(kv);
        }
        free(kvs->queue);
        while (1) {
                retval = _unlock_kvstore(kvs);
                switch (retval) {
                case 0:
                        retval = 1;
                        break;
                case -1:
                        switch (errno) {
                        case EINVAL:
                                retval = 1;
                                break;
                        case EBADF:
                                retval = 1;
                                break;
                        default:
                                retval = 0;
                                continue;
                        }
                }
                if (retval)
                        break;
        }
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



int
kvstore_config(kvstore kvs, KVSTORE_CONFIG_OPT opt, void *val)
{
        if (NULL == kvs)
                return -1;
        switch (opt) {
        case KVSTORE_MAX_KEYLEN:
                kvs->max_keylen = *(size_t *)val;
                break;
        case KVSTORE_MAX_VALLEN:
                kvs->max_vallen = *(size_t *)val;
                break;
        default:
                break;
        }

        return 0;
}


int
kvstore_set(kvstore kvs, char *key, char *val)
{
        struct _kvstore_kv      *kv;
        size_t                   klen;
        size_t                   vlen;

        if (NULL == kvs)
                return -1;

        TAILQ_FOREACH(kv, kvs->queue, entries) {
                if (0 == _keymatch(kv->key, key))
                        return _kvstore_update(kvs, kv, val);
        }

        kv = NULL;
        kv = (struct _kvstore_kv *)malloc(sizeof(struct _kvstore_kv));
        if (NULL == kv)
                return -1;

        klen = strnlen(key, kvs->max_keylen + 1);
        if (((kvs->max_keylen + 1) == klen) || (0 == klen))
                return -1;

        vlen = strnlen(val, kvs->max_vallen + 1);
        if (((kvs->max_vallen + 1) == vlen) || (0 == vlen))
                return -1;

        kv->key = (char *)malloc((klen + 1) * sizeof(char));
        kv->val = (char *)malloc((vlen + 1) * sizeof(char));
        if ((NULL == kv->key) || (NULL == kv->val)) {
                free(kv->key);
                free(kv->val);
                free(kv);
                return -1;
        }

        kv->key_len = klen;
        kv->val_len = vlen;
        strncpy(kv->key, key, klen);
        strncpy(kv->val, val, vlen);

        TAILQ_INSERT_HEAD(kvs->queue, kv, entries);
        return 0;
}


int
_kvstore_update(kvstore kvs, struct _kvstore_kv *kv, char *val)
{
        size_t   vlen;
        char    *update_val;

        printf("\n");
        printf("[-] new val: '%s'\n", val);
        printf("[-] kv->val: '%s'\tkv->val_len: %u\n",
                kv->val, (unsigned int)kv->val_len);
        vlen = strnlen(val, kvs->max_vallen + 1);
        if (((kvs->max_vallen + 1) == vlen) || (0 == vlen))
                return -1;

        update_val = (char *)malloc((vlen + 1) * sizeof(char));
        if (NULL == update_val)
                return -1;

        free(kv->val);
        kv->val = NULL;
        kv->val_len = vlen;
        kv->val = update_val;
        return 0;
}


char *
kvstore_get(kvstore kvs, char *key)
{
        struct _kvstore_kv      *kv;
        int                      match = 0;

        TAILQ_FOREACH(kv, kvs->queue, entries) {
                if (_keymatch(kv->key, key)) {
                        match = 1;
                        break;
                }
        }

        if (!match)
                return NULL;
        return kv->val;
}


int
kvstore_del(kvstore kvs, char *key)
{
        struct _kvstore_kv      *kv;

        TAILQ_FOREACH(kv, kvs->queue, entries) {
                if (_keymatch(kv->key, key)) {
                        free(kv->key);
                        free(kv->val);
                        TAILQ_REMOVE(kvs->queue, kv, entries);
                        free(kv);
                        return 0;
                }
        }
        return -1;
}
