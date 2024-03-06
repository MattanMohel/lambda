#ifndef _HASHMAP_H_
#define _HASHMAP_H_

#include <stddef.h>
#include "vec.h"

// TODO: make this be determined dyamically later
#define HMAP_SIZE 100

typedef size_t (*hash_fn) (char*);
typedef struct Entry Entry;

typedef struct Entry {
  char* key;
  void* value;
  Entry* next;
} Entry;

typedef struct {
  hash_fn hash;
  Entry** entries;
  size_t cap;
} Hashmap;

Hashmap* hmap_new ();
void hmap_delete  (Hashmap* hmap);
void hmap_delete_entry (Entry* e);
Entry *hmap_entry_new (char* key, void* value);

void *hmap_add  (Hashmap* hmap, char* key, void* value);
void *hmap_rem  (Hashmap* hmap, char* key);
void *hmap_get  (Hashmap* hmap, char* key);
int hmap_find   (Hashmap* hmap, char* key);

size_t fnv1a_hash (char* str);

#endif
