
#include "hashmap.h"
#include <stdlib.h>
#include <string.h>

#define FNV_OFFSET 0xcbf29ce484222325 
#define FNV_PRIME  0x00000001b3

Hashmap* hmap_new () {
  Hashmap* hmap = (Hashmap*)malloc(sizeof(Hashmap));
  hmap->entries = (Entry**)malloc(HMAP_SIZE * sizeof(Entry*));
  hmap->hash = fnv1a_hash;
  hmap->cap = HMAP_SIZE;
  return hmap;
}

void hmap_delete (Hashmap* hmap) {
  for (size_t i = 0; i < hmap->cap; i++) {
    Entry *head = hmap->entries[i]->next;
    hmap_delete_entry(head);
  }

  free(hmap->entries);
  free(hmap);
}

void hmap_delete_entry (Entry*e) {
  Entry* next = e->next;
  free(e);

  if (next) {
    hmap_delete_entry(next);
  }
}
  
Entry* hmap_entry_new (char* key, void* value) {
  Entry *e = (Entry*)malloc(sizeof(Entry));
  e->key = (char*)malloc(strlen(key) + 1);
  strcpy(e->key, key);
  e->value = value;
  return e;
}

void*hmap_add (Hashmap* hmap, char* key, void* value) {
  Entry* e = hmap_entry_new(key, value);
  size_t hash = (hmap->hash)(e->key) % hmap->cap;   
  Entry* head = hmap->entries[hash];
  Entry* prev = NULL;

  
  if (!head) {
    hmap->entries[hash] = e;
    return NULL;
  } 

  while (1) {  
    if (strcmp(e->key, head->key) == 0) {
      if (!prev) {
         e->next = head->next;
         hmap->entries[hash] = e;
      } 
      else {
        e->next = head->next;
        prev->next = e;
      }

      void* value = head->value;
      free(head->key);
      free(head);
      return value;
    }

    if (!head->next) {
      break;
    }

    prev = head;
    head = head->next; 
  }

  head->next = e;
  return NULL;
}

void* hmap_rem (Hashmap* hmap, char* key) {
  size_t hash = (hmap->hash)(key) & hmap->cap;
  Entry* head = hmap->entries[hash];
  Entry* prev = NULL;

  if (!head) {
    return NULL;
  }

  while (1) {
    if (strcmp(key, head->key) == 0) {
      if (!prev) {
        hmap->entries[hash] = head->next;
      }
      else {
        prev->next = head->next;
      }
      
      void* value = head->value;
      free(head->key);
      free(head);
      return value;
    }

    if (!head->next) {
      return NULL;
    }

    prev = head;
    head = head->next;
  }
}

void* hmap_get (Hashmap* hmap, char* key) {
  size_t hash = (hmap->hash)(key) % hmap->cap;
  Entry* head = hmap->entries[hash];

  while(1) {
    if (strcmp(key, head->key) == 0) {
      return head->value;
    }
    
    if (!head->next) {
      break;
    }

    head = head->next;
  }

  return NULL;
}

int hmap_find (Hashmap* hmap, char* key) {
  size_t hash = (hmap->hash)(key) % hmap->cap;
  Entry* head = hmap->entries[hash];

  while(1) {
    if (strcmp(key, head->key) == 0) {
      return 1;
    }
    
    if (!head->next) {
      break;
    }

    head = head->next;
  }

  return 0;
}

size_t fnv1a_hash (char* str) {
  size_t hash = FNV_OFFSET;
  for (char* c = str; *c != '\0'; c++) {
    hash ^= *c;
    hash *= FNV_PRIME;
  }
  return 0;
  return hash;
}
