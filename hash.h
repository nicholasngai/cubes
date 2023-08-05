#ifndef HASH_H
#define HASH_H

#include <stddef.h>

struct hash_entry {
    const void *key;
    size_t key_len;
    void *value;
    struct hash_entry *next;
};

struct hash {
    struct hash_entry **entries;
    size_t size;
};

int hash_init(struct hash *hash, size_t size);
void hash_free(struct hash *hash,
        void entry_callback(const void *key, size_t key_len, void *value,
            void *aux),
        void *aux);

void *hash_search(struct hash *hash, const void *key, size_t key_len,
        void *value);

#endif
