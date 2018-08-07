#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hashtable.h"

const double RESIZE_THRESH = 0.75;

hashtable *hashtable_new(void) {
    static const int INITIAL_CAP = 16;

    hashtable *table = malloc(sizeof(*table));
    if (table == NULL) {
        return NULL;
    }

    table->elements = calloc(INITIAL_CAP, sizeof(*(table->elements)));
    if (table->elements == NULL) {
        free(table);
        return NULL;
    }

    table->size = 0;
    table->capacity = INITIAL_CAP;

    return table;
}

int hash(const char *key) {
    int result = 7;
    for (int i = 0;; i++) {
        char c = *(key + i);
        if (c == '\0') {
            break;
        }

        result = 37 * result + (int) c;
    }

    return result < 0 ? -result : result;
}

int resize(hashtable *table) {
    int orig_size = table->capacity;
    int new_size = 2 * orig_size;
    node **new_array = calloc(new_size, sizeof(*new_array));
    if (new_array == NULL) {
        return 0;
    }

    node **orig_elements = table->elements;
    table->elements = new_array;
    table->capacity = new_size;
    table->size = 0;

    for (int i = 0; i < orig_size; ++i) {
        node *node = *(orig_elements + i);
        while (node != NULL) {
            hashtable_put(table, node->key, node->value);
            node = node->next;
        }
    }

    free(orig_elements);
    return 1;
}

int hashtable_put(hashtable *table, char *key, void *value) {
    int h = hash(key);
    int arrayIdx = h % table->capacity;

    node *entry = NULL;
    node *begin = *(table->elements + arrayIdx);

    // Key does not already exist
    if (begin == NULL) {
        entry = malloc(sizeof(*entry));
        if (entry == NULL) {
            return 0;
        }

        *(table->elements + arrayIdx) = entry;
        table->size++;
    } else {
        entry = begin;

        // Key may/may not exist
        while (strcmp(entry->key, key) != 0) {
            // Reached end of bucket, does not exist
            // create a new node
            if (entry->next == NULL) {
                entry->next = malloc(sizeof(*(entry->next)));
                entry = entry->next;
                if (entry == NULL) {
                    return 0;
                }

                table->size++;
                break;
            }

            entry = entry->next;
        }

        // Otherwise the key will match here
    }

    entry->key = key;
    entry->value = value;
    entry->next = NULL;

    if ((double) table->size / table->capacity >= RESIZE_THRESH) {
        return resize(table);
    }

    return 1;
}

node *hashtable_get(hashtable *table, char *key) {
    int h = hash(key);
    int arrayIdx = h % table->capacity;

    node *entry = *(table->elements + arrayIdx);

    // Key does not exist
    if (entry == NULL) {
        return NULL;
    } else {
        // Search through bucket
        while (strcmp(entry->key, key) != 0) {
            // End of bucket, no key
            if (entry->next == NULL) {
                return NULL;
            }

            entry = entry->next;
        }

        // Entry key matches, return
        return entry;
    }
}

node *hashtable_remove(hashtable *table, char *key) {
    int h = hash(key);
    int arrayIdx = h % table->capacity;

    node *entry = *(table->elements + arrayIdx);

    // Key does not exist
    if (entry == NULL) {
        return NULL;
    } else {
        node *prev = NULL;

        // Look through bucket
        while (strcmp(entry->key, key) != 0) {
            // End of bucket, no key
            if (entry->next == NULL) {
                return 0;
            }

            prev = entry;
            entry = entry->next;
        }

        // If not the only key in bucket, link with next
        if (prev != NULL) {
            prev->next = entry->next;
        }
        table->size--;

        return entry;
    }
}

void hashtable_destroy(hashtable *table) {
    // Free individual buckets
    for (int i = 0; i < table->capacity; i++) {
        node *begin = *(table->elements + i);
        while (begin != NULL) {
            free(begin);
            begin = begin->next;
        }
    }

    // Free the element array
    free(table->elements);

    // Free the struct
    free(table);
}
