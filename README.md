# `cfgdb`

A configuration file "database" using ZeroMQ and cJSON to 
parse configuration files.

Supports multiple configuration files by abstracting config
paths into names that must be requested by clients. ZeroMQ
can be used to create peer-to-peer (P2P) or to build
brokers which can distribute and proxy requests.

Uses basic request/response system, no fancy protocols.
Also uses libzmq, not czmq, and the message API, not raw
zmq sockets.

# Building

``` shell
git clone https://github.com/caojohnny/cfgdb.git
cd cfgdb
mkdir build
cd build
cmake ..
make
```

This is a library built with C99. It does nothing on its
own. You may use it by including `cfgdb.h`.

# Demo

Client source:

``` C
#include <stdio.h>
#include "cfgdb.h"

int main() {
    cfgdb_client_ctx *client = cfgdb_new_client("tcp://localhost:5000", "config");
    printf("double=%f\n", cfgdb_get_double(client, "double"));
    cfgdb_destroy_client(client);
}
```

Server source:

``` C
#include "cfgdb.h"
#include <pthread.h>

void *start_server(void *arg) {
    cfgdb_begin_server("tcp://*:5000");
    return NULL;
}

int main() {
    pthread_t *pthread;
    pthread_create(pthread, NULL, start_server, NULL);

    cfgdb_server_ctx *server_ctx;
    do {
        server_ctx = cfgdb_get_server();
    } while (server_ctx == NULL);
    cfgdb_add_cfg(server_ctx, "config", "./config.json");

    pthread_join(*pthread, NULL);
}
```

# Notes

  - Only supports top-level configuration keys
  - Only supports JSON
  
---
  
Literally spent a total of 10 hours on 3 trivial issues:

  - Hashtable BAD_ACCESS due to `malloc` of a pointer
  instead of the value of the pointer
  - Hashtable was not getting rehashed when it got
  resized because I was iterating a fresh `calloc`'d array
  instead of the `old_array`
  - Server main loop was getting a BAD_ACCESS due to a
  cJSON instance being put inside of the response, which
  would get freed. Had to use the AddItemReference instead
  
# Credits

Built with [CLion](https://www.jetbrains.com/clion/)

Uses [ZeroMQ](http://zeromq.org/) as the messaging library
and [cJSON](https://github.com/DaveGamble/cJSON) as the
JSON library.
