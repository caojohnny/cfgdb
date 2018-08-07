#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "cfgdb.h"
#include "cjson/cJSON.h"
#include "zmq.h"

cfgdb_server_ctx *singleton;

char *sock_read(void *sock) {
    zmq_msg_t msg;
    zmq_msg_init(&msg);
    if (zmq_msg_recv(&msg, sock, 0) == -1) {
        return NULL;
    }

    size_t size = zmq_msg_size(&msg);
    char *string = malloc(size + 1);
    memcpy(string, zmq_msg_data(&msg), size);
    *(string + size) = '\0';

    zmq_msg_close(&msg);

    return string;
}

int sock_write(void *sock, char *string) {
    zmq_msg_t msg;
    size_t len = strlen(string);
    zmq_msg_init_size(&msg, len);
    memmove(zmq_msg_data(&msg), string, len);

    int resp = zmq_msg_send(&msg, sock, 0);

    zmq_msg_close(&msg);

    if (resp == -1) {
        return 0;
    }

    return 1;
}

int cfgdb_begin_server(char *addr) {
    if (singleton == NULL) {
        cfgdb_server_ctx *ctx = malloc(sizeof(*ctx));
        if (ctx == NULL) {
            return 0;
        }

        ctx->ctx = zmq_ctx_new();
        if (ctx->ctx == NULL) {
            return 0;
        }

        ctx->sock = zmq_socket(ctx->ctx, ZMQ_REP);
        if (ctx->sock == NULL) {
            zmq_ctx_destroy(ctx->ctx);
            return 0;
        }

        if (zmq_bind(ctx->sock, addr) != 0) {
            zmq_close(ctx->sock);
            zmq_ctx_destroy(ctx->ctx);
            return 0;
        }

        ctx->addr = addr;
        ctx->json_table = hashtable_new();

        singleton = ctx;

        while (1) {
            char *data = sock_read(ctx->sock);
            if (data == NULL) {
                continue;
            }

            cJSON *request = cJSON_Parse(data);

            cJSON *name = cJSON_GetObjectItem(request, "name");
            if (name == NULL) {
                cJSON_Delete(request);
                free(data);
                continue;
            }

            char *cfg_name = name->valuestring;
            node *entry = hashtable_get(ctx->json_table, cfg_name);
            if (entry == NULL) {
                cJSON_Delete(request);
                free(data);
                continue;
            }

            if (*cfg_name == '@') {
                if (strcmp(cfg_name, "@quit") == 0) {
                    cJSON_Delete(request);
                    free(data);

                    zmq_close(ctx->sock);
                    zmq_ctx_destroy(ctx->ctx);
                    free(ctx->addr);
                    hashtable_destroy(ctx->json_table);
                    break;
                }
            }

            cJSON *json_key = cJSON_GetObjectItem(request, "json_key");
            if (json_key == NULL) {
                cJSON_Delete(request);
                free(data);
                continue;
            }

            char *request_key = json_key->valuestring;
            cJSON *cfg = (cJSON *) entry->value;
            cJSON_bool has = cJSON_HasObjectItem(cfg, request_key);
            cJSON *value = NULL;
            if (has == 1) {
                value = cJSON_GetObjectItem(cfg, request_key);
            } else {
                value = cJSON_CreateNull();
            }

            cJSON *response = cJSON_CreateObject();
            if (cJSON_AddBoolToObject(response, "has", has) == NULL) {
                cJSON_Delete(response);
                cJSON_Delete(request);
                free(data);
                continue;
            }

            /* Important: Must be a reference - spent 4 hours
               trying to figure this out only to discover that
               freeing the response will free a portion of my
               config as well... */
            cJSON_AddItemReferenceToObject(response, "value", value);

            char *response_string = cJSON_Print(response);
            sock_write(ctx->sock, response_string);

            free(response_string);
            cJSON_Delete(response);
            cJSON_Delete(request);
            free(data);
        }

        return 1;
    }

    return 0;
}

cfgdb_server_ctx *cfgdb_get_server() {
    return singleton;
}

int cfgdb_add_cfg(cfgdb_server_ctx *ctx, char *key, char *path) {
    /* Read file */
    FILE *cfg = fopen(path, "r");
    if (cfg == NULL) {
        fclose(cfg);
        return 0;
    }

    fseek(cfg, 0L, SEEK_END);
    long len = ftell(cfg);
    rewind(cfg);

    char *data = malloc(len + 1);
    if (data == NULL) {
        fclose(cfg);
        return 0;
    }

    *(data + len) = '\0';
    fread(data, 1, len, cfg);
    fclose(cfg);

    cJSON *json = cJSON_Parse(data);
    if (json == NULL) {
        return 0;
    }

    int result = hashtable_put(ctx->json_table, key, json);
    if (result == 0) {
        cJSON_Delete(json);
        return 0;
    }

    return 1;
}

void cfgdb_destroy_server() {
    if (singleton == NULL) {
        return;
    }

    void *ctx = zmq_ctx_new();
    void *sock = zmq_socket(ctx, ZMQ_REQ);
    zmq_connect(sock, singleton->addr);

    cJSON *stop_cmd = cJSON_CreateObject();
    if (cJSON_AddStringToObject(stop_cmd, "name", "@quit") == NULL) {
        cJSON_Delete(stop_cmd);
        zmq_close(sock);
        zmq_ctx_destroy(ctx);
    }

    char *msg = cJSON_Print(stop_cmd);
    sock_write(sock, msg);

    free(msg);
    cJSON_Delete(stop_cmd);
    zmq_close(sock);
    zmq_ctx_destroy(ctx);

    singleton = NULL;
}

cfgdb_client_ctx *cfgdb_new_client(char *addr, char *name) {
    cfgdb_client_ctx *client_ctx = malloc(sizeof(*client_ctx));
    if (client_ctx == NULL) {
        return NULL;
    }

    client_ctx->ctx = zmq_ctx_new();
    client_ctx->sock = zmq_socket(client_ctx->ctx, ZMQ_REQ);
    zmq_connect(client_ctx->sock, addr);

    client_ctx->name = strdup(name);
    return client_ctx;
}

void cfgdb_destroy_client(cfgdb_client_ctx *client) {
    zmq_close(client->sock);
    zmq_ctx_destroy(client->ctx);
}

/* KEYS */

cJSON *new_req(cfgdb_client_ctx *client, char *key) {
    cJSON *req = cJSON_CreateObject();
    if (cJSON_AddStringToObject(req, "name", client->name) == NULL) {
        cJSON_Delete(req);
        return NULL;
    }

    if (cJSON_AddStringToObject(req, "json_key", key) == NULL) {
        cJSON_Delete(req);
        return NULL;
    }

    char *msg = cJSON_Print(req);
    sock_write(client->sock, msg);

    free(msg);
    cJSON_Delete(req);

    char *data = sock_read(client->sock);
    if (data == NULL) {
        return NULL;
    }

    cJSON *resp = cJSON_Parse(data);
    free(data);

    return resp;
}

int cfgdb_has(cfgdb_client_ctx *client, char *key) {
    cJSON *resp = new_req(client, key);
    cJSON *has = cJSON_GetObjectItem(resp, "has");
    int result = has->type == cJSON_True;

    cJSON_Delete(resp);
    return result;
}

/* GETTERS */

cJSON *cfgdb_get(cfgdb_client_ctx *client, char *key) {
    cJSON *resp = new_req(client, key);
    if (resp == NULL) {
        return NULL;
    }

    cJSON *value = cJSON_GetObjectItem(resp, "value");
    return value;
}

short cfgdb_get_short(cfgdb_client_ctx *client, char *key) {
    cJSON *value = cfgdb_get(client, key);
    if (value == NULL || cJSON_IsNull(value)) {
        cJSON_Delete(value);
        return 0;
    } else {
        short result = (short) value->valueint;
        cJSON_Delete(value);
        return result;
    }
}

int cfgdb_get_int(cfgdb_client_ctx *client, char *key) {
    cJSON *value = cfgdb_get(client, key);
    if (value == NULL || cJSON_IsNull(value)) {
        cJSON_Delete(value);
        return 0;
    } else {
        int result = value->valueint;
        cJSON_Delete(value);
        return result;
    }
}

double cfgdb_get_double(cfgdb_client_ctx *client, char *key) {
    cJSON *value = cfgdb_get(client, key);
    if (value == NULL || cJSON_IsNull(value)) {
        cJSON_Delete(value);
        return 0.0;
    } else {
        double result = value->valuedouble;
        cJSON_Delete(value);
        return result;
    }
}

float cfgdb_get_float(cfgdb_client_ctx *client, char *key) {
    cJSON *value = cfgdb_get(client, key);
    if (value == NULL || cJSON_IsNull(value)) {
        cJSON_Delete(value);
        return 0.0;
    } else {
        float result = (float) value->valuedouble;
        cJSON_Delete(value);
        return result;
    }
}

long cfgdb_get_long(cfgdb_client_ctx *client, char *key) {
    cJSON *value = cfgdb_get(client, key);
    if (value == NULL || cJSON_IsNull(value)) {
        cJSON_Delete(value);
        return 0;
    } else {
        long result = (long) value->valuedouble;
        cJSON_Delete(value);
        return result;
    }
}

char cfgdb_get_char(cfgdb_client_ctx *client, char *key) {
    cJSON *value = cfgdb_get(client, key);
    if (value == NULL || cJSON_IsNull(value)) {
        cJSON_Delete(value);
        return '\0';
    } else {
        char result = (char) value->valueint;
        cJSON_Delete(value);
        return result;
    }
}

char *cfgdb_get_string(cfgdb_client_ctx *client, char *key) {
    cJSON *value = cfgdb_get(client, key);
    if (value == NULL || cJSON_IsNull(value)) {
        cJSON_Delete(value);
        return "";
    } else {
        char *result = strdup(value->valuestring);
        cJSON_Delete(value);
        return result;
    }
}
