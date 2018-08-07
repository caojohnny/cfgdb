#ifndef CFGDB_HASHTABLE_H
#define CFGDB_HASHTABLE_H

typedef struct node {
    char *key;
    void *value;

    struct node *next;
} node;

typedef struct {
    int size;
    int capacity;
    node **elements;
} hashtable;

hashtable *hashtable_new(void);

int hashtable_put(hashtable *table, char *, void *);

node *hashtable_get(hashtable *table, char *);

node *hashtable_remove(hashtable *table, char *);

void hashtable_destroy(hashtable *);

#endif /* CFGDB_HASHTABLE_H */
