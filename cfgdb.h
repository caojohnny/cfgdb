#ifndef CFGDB_LIBRARY_H
#define CFGDB_LIBRARY_H

#include <cjson/cJSON.h>
#include "hashtable.h"

typedef struct {
    void *ctx;
    void *sock;

    char *addr;
    hashtable *json_table;
} cfgdb_server_ctx;

typedef struct {
    void *ctx;
    void *sock;

    char *name;
} cfgdb_client_ctx;

/* INIT METHODS */

int cfgdb_begin_server(char *);

cfgdb_server_ctx *cfgdb_get_server(void);

int cfgdb_add_cfg(cfgdb_server_ctx *, char *, char *);

void cfgdb_destroy_server(void);

cfgdb_client_ctx *cfgdb_new_client(char *, char *);

void cfgdb_destroy_client(cfgdb_client_ctx *);

/* KEYS */

int cfgdb_has(cfgdb_client_ctx *, char *);

/* GETTERS */

cJSON *cfgdb_get(cfgdb_client_ctx *, char *);

short cfgdb_get_short(cfgdb_client_ctx *, char *);

int cfgdb_get_int(cfgdb_client_ctx *, char *);

double cfgdb_get_double(cfgdb_client_ctx *, char *);

float cfgdb_get_float(cfgdb_client_ctx *, char *);

long cfgdb_get_long(cfgdb_client_ctx *, char *);

char cfgdb_get_char(cfgdb_client_ctx *, char *);

char *cfgdb_get_string(cfgdb_client_ctx *, char *);

#endif