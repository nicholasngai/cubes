#include "hash.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int hash_init(struct hash *hash, size_t size) {
    int ret;

    *hash = (struct hash) {
        .size = size,
    };
    hash->entries = calloc(size, sizeof(*hash->entries));
    if (!hash->entries) {
        ret = -1;
        goto exit;
    }

    ret = 0;

exit:
    return ret;
}

void hash_free(struct hash *hash,
        void entry_callback(const void *key, size_t key_len, void *value,
            void *aux),
        void *aux) {
    for (size_t i = 0; i < hash->size; i++) {
        struct hash_entry *entry = hash->entries[i];
        while (entry) {
            entry_callback(entry->key, entry->key_len, entry->value, aux);
            struct hash_entry *next_entry = entry->next;
            free(entry);
            entry = next_entry;
        }
    }
    free(hash->entries);
}

static uint64_t compute_hash(const void *key_, size_t key_len) {
    /* DJB2 hash. */
    const unsigned char *key = key_;
    uint64_t hash = 5381;
    for (size_t i = 0; i < key_len; i++) {
        hash = (hash << 5) + hash + key[i];
    }
    return hash;
}

static int key_comp(const void *key1, size_t key1_len, const void *key2,
        size_t key2_len) {
    if (key1_len != key2_len) {
        return (key1_len > key2_len) - (key1_len < key2_len);
    }
    return memcmp(key1, key2, key1_len);
}

void *hash_search(struct hash *hash, const void *key, size_t key_len,
        void *value) {
    void *ret;

    /* Find hash entry to insert ourselves into. */
    uint64_t hash_val = compute_hash(key, key_len);
    size_t hash_idx = hash_val % hash->size;
    struct hash_entry **entry_ptr = &hash->entries[hash_idx];
    while (1) {
        struct hash_entry *entry = *entry_ptr;

        /* Exit if reached end of linked list. */
        if (!entry) {
            break;
        }

        int comp = key_comp(entry->key, entry->key_len, key, key_len);

        /* Return existing entry if we reached an entry with key matching
         * ours. */
        if (comp == 0) {
            ret = entry->value;
            goto exit;
        }

        /* Exit if we reached existing entry with key greater than ours. */
        if (comp > 0) {
            break;
        }

        entry_ptr = &entry->next;
    }

    /* Insert value into map. */
    struct hash_entry *new_entry = malloc(sizeof(*new_entry));
    if (!new_entry) {
        ret = NULL;
        goto exit;
    }
    *new_entry = (struct hash_entry) {
        .key = key,
        .key_len = key_len,
        .value = value,
        .next = *entry_ptr,
    };
    *entry_ptr = new_entry;

    ret = new_entry->value;

exit:
    return ret;
}
