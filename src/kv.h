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


#ifndef __LIBKVSTORE_KV_H
#define __LIBKVSTORE_KV_H
#include <sys/types.h>
#include <sys/queue.h>
#include <stdlib.h>
#include <unistd.h>

extern const size_t      KVSTORE_DEFAULT_MAX_KEYLEN;
extern const size_t      KVSTORE_DEFAULT_MAX_VALLEN;

typedef enum {
        KVSTORE_MAX_KEYLEN,
        KVSTORE_MAX_VALLEN
} KVSTORE_CONFIG_OPT;

typedef struct _kvstore * kvstore;

kvstore         kvstore_new(void);
int             kvstore_discard(kvstore);
int             kvstore_config(kvstore, KVSTORE_CONFIG_OPT, void *);
int             kvstore_dup(kvstore);
int             kvstore_set(kvstore, char *, char *);

#endif
