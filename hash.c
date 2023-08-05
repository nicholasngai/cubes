#include "hash.h"
#include <search.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct hash_entry {
    const void *key;
    size_t key_len;
    void *value;
};

int hash_init(struct hash *hash, size_t size) {
    int ret;

    *hash = (struct hash) {
        .size = size,
    };
    hash->buckets = malloc(size * sizeof(*hash->buckets));
    if (!hash->buckets) {
        ret = -1;
        goto exit;
    }
    for (size_t i = 0; i < size; i++) {
        hash->buckets[i] = (struct hash_bucket) {
            .tree = NULL,
        };
    }

    ret = 0;
    goto exit;

exit:
    return ret;
}

static int tree_comp(const void *a_, const void *b_) {
    const struct hash_entry *a = a_;
    const struct hash_entry *b = b_;
    if (a->key_len != b->key_len) {
        return (a->key_len > b->key_len) - (a->key_len < b->key_len);
    }
    return memcmp(a->key, b->key, a->key_len);
}

void hash_free(struct hash *hash,
        void entry_callback(const void *key, size_t key_len, void *value,
            void *aux),
        void *aux) {
    for (size_t i = 0; i < hash->size; i++) {
        struct hash_bucket *bucket = &hash->buckets[i];
        while (bucket->tree) {
            struct hash_entry *entry = *((struct hash_entry **) bucket->tree);
            tdelete(entry, &bucket->tree, tree_comp);
            entry_callback(entry->key, entry->key_len, entry->value, aux);
            free(entry);
        }
    }
    free(hash->buckets);
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

void *hash_search(struct hash *hash, const void *key, size_t key_len,
        void *value) {
    void *ret;

    /* Construct hash entry. */
    struct hash_entry *entry = malloc(sizeof(*entry));
    if (!entry) {
        ret = NULL;
        goto exit;
    }
    *entry = (struct hash_entry) {
        .key = key,
        .key_len = key_len,
        .value = value,
    };

    /* Find hash bucket and try to insert. */
    uint64_t hash_val = compute_hash(key, key_len);
    struct hash_bucket *bucket = &hash->buckets[hash_val % hash->size];
    struct hash_entry **inserted = tsearch(entry, &bucket->tree, tree_comp);
    if (!inserted) {
        ret = NULL;
        goto exit_free_entry;
    }
    if (*inserted != entry) {
        /* The value was already in the tree. Return its value. */
        ret = (*inserted)->value;
        free(entry);
        goto exit;
    }

    ret = entry->value;
    goto exit;

exit_free_entry:
    free(entry);
exit:
    return ret;
}
