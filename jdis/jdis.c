#include "jdis.h"
#include "../hashtable/hashtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Fonction pour comparer deux chaînes
int compare_strings(const void *a, const void *b) {
  return strcmp((const char *) a, (const char *) b);
}

// Fonction de hachage (djb2)
size_t hash_string(const void *key) {
  const char *str = (const char *) key;
  size_t hash = 5381;
  int c;
  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + (size_t) c;
  }
  return hash;
}

// Fonction de callback pour compter les mots
//int count_words(void *key, void *val) {
//(void)val; // Unused
//holdall_put(all_words, key);
//total++;
//if (hashtable_search(ht2, key) != nullptr) {
//common++;
//}
//return 0;
//}

// Fonction pour lire les mots d'un fichier
hashtable *get_unique_words(const char *filename) {
  FILE *file = fopen(filename, "r");
  if (file == nullptr) {
    fprintf(stderr, "Error : unable to open %s\n", filename);
    return nullptr;
  }
  hashtable *ht = hashtable_empty(compare_strings, hash_string, 0.75);
  if (ht == nullptr) {
    fclose(file);
    return nullptr;
  }
  char word[256];
  while (fscanf(file, "%255s", word) == 1) {
    char *word_copy = strdup(word);
    if (word_copy == nullptr) {
      fclose(file);
      hashtable_dispose(&ht);
      return nullptr;
    }
    hashtable_add(ht, word_copy, (void *) 1);
  }
  fclose(file);
  return ht;
}

void hashtable__print_values(hashtable *ht) {
  for (size_t i; i < (ht->nslots); i+=1) {
    cell *cur = (ht->hasharray[i]);
    while(cur != nullptr) {
      const char *val = (const char *)(cur->valref);
      printf("%s\n", val);
      cur = cur->next;
    }
  }
}


// Nouvelle implémentation sans accès direct à la structure interne
//float jaccard_distance(hashtable *ht1, hashtable *ht2) {

//Utilisation de holdall pour stocker tous les mots uniques
//holdall *all_words = holdall_empty();
//if (all_words == nullptr) {
//return 1.0f;
//}

//size_t common = 0;
//size_t total = 0;

//Fonction de callback pour compter les mots
//count_words(void *key, void *val);

//Parcours de la première table
//if (hashtable_apply(ht1, count_words) != 0) {
//holdall_dispose(&all_words);
//return 1.0f;
//}

//Fonction pour compter les mots uniques de la deuxième table
//int count_unique(void *key, void *val) {
//(void)val; // Unused
//if (hashtable_search(ht1, key) == nullptr) {
//total++;
//}
//return 0;
//}

//Parcours de la deuxième table
//if (hashtable_apply(ht2, count_unique) != 0) {
//holdall_dispose(&all_words);
//return 1.0f;
//}

//holdall_dispose(&all_words);
//if (total == 0) return 0.0f;
//return 1.0f - ((float)common / (float)total);
//}
