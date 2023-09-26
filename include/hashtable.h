#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <cstddef>
#include <cstdint>

struct HNode {
    HNode *next {nullptr};
    uint64_t hcode {0};
};

struct Htab {
    HNode **tab {nullptr};
    size_t mask {0};
    size_t size {0};
};

struct Hmap {
    Htab ht1;
    Htab ht2;
    size_t resizing_pos {0};
};

HNode *hm_lookup(Hmap *hmap, HNode *key, bool(*cmp)(HNode *, HNode*));
void hm_insert(Hmap *hmap, HNode *node);
HNode *hm_pop(Hmap *hmap, HNode *key, bool(*cmp)(HNode *, HNode*));
void hm_destroy(Hmap *hmap);

#endif /* HASHTABLE_H */