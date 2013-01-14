AM_CFLAGS = -pthread -Wall -Werror -std=c99 -D_XOPEN_SOURCE=700 -D_BSD_SOURCE \
             -I../src -O0 -g
AM_LDFLAGS = -lpthread

check_PROGRAMS = kvs_test
kvs_test_SOURCES = kvs_test.c ../src/kv.c

