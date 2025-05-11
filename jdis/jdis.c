#include "jdis.h"
#include "../hashtable/hashtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
typedef struct cell cell;
typedef struct hashtable hashtable;
struct hashtable {
    int (*compar)(const void *, const void *);
    size_t (*hashfun)(const void *);
    double lfmax;
    struct cell **hasharray;
    size_t nslots;
    size_t nentries;
};

struct cell {
    const void *keyref;
    const void *valref;
    struct cell *next;
};
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
hashtable *get_words(const char *filename) {
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

  //Pourquoi l'utilisation d'une copie???
  char word[256];
  while (fscanf(file, "%255s", word) == 1) {
    char *word_copy = strdup(word);
    if (word_copy == nullptr) {
      fclose(file);
      hashtable_dispose(&ht);
      return nullptr;
    }
    hashtable_add(ht, word_copy, (void *) 1);
    free(word_copy);
  }
  fclose(file);
  return ht;
}

//OUTILS D'AFFICHAGE
void print_usage(void) {
  printf("Usage: jdis [OPTION]... FILE1 FILE2 [FILE]...\n");
}

void print_help(void) {
  printf("Usage: jdis [OPTION]... FILE1 FILE2 [FILE]...\n");
  printf("\n");
  printf("Computes Jaccard dissimilarities of sets of words in FILEs.\n");
  printf("\n");
  printf(
      "For any pair of FILEs, dissimilarity is displayed first to four decimal places,\n");
  printf(
      "followed by the two FILEs in the pair. A word is, by default, a maximum length\n");
  printf(
      "sequence of characters that do not belong to the white-space characters set.\n");
  printf("\n");
  printf(
      "Read the standard input for any FILE that is '-' on command line. The standard\n");
  printf(
      "input is displayed as a pair of double quotation marks in productions.\n");
  printf("\n");
  printf("Program Information\n");
  printf("  -?, --help\n");
  printf("        Print this help message and exit.\n");
  printf("\n");
  printf("  --usage\n");
  printf("        Print a short usage message and exit.\n");
  printf("\n");
  printf("  --version\n");
  printf("        Print version information.\n");
  printf("\n");
  printf("Processing\n");
  printf("  -h    Same as --words-processing=hash-table.\n");
  printf(
      "        Process the words according to the data structure specified by TYPE. The\n");
  printf(
      "        available values for TYPE are self explanatory: 'avl-binary-tree' and\n");
  printf("        'hash-table'. Default is 'hash-table'.\n");
  printf("\n");
  printf("Input Control\n");
  printf("  -i VALUE, --initial=VALUE\n");
  printf(
      "        Set the maximal number of significant initial letters for words to\n");
  printf("        VALUE. 0 means without limitation. Default is 0.\n");
  printf("\n");
  printf("  -p, --punctuation-like-space\n");
  printf(
      "        Make the punctuation characters play the same role as white-space\n");
  printf("        characters in the meaning of words.\n");
  printf("\n");
  printf("Output Control\n");
  printf("  -g, --graph\n");
  printf(
      "        Suppress normal output. Instead, for each word found in any FILE, jdis\n");
  printf(
      "        list the FILEs in which it does or does not appear. A header line\n");
  printf(
      "        indicates the FILE names: the name of the first FILE appears in the\n");
  printf(
      "        second column, that of the second in the third, and so on. For the\n");
  printf(
      "        subsequent lines, a word appears in the first column, followed by\n");
  printf(
      "        appearance marks: 'x' for yes, '-' for no. The list is lexicographically\n");
  printf(
      "        sorted. The locale specified by the environment affects the sort order.\n");
  printf(
      "        Set 'LC_ALL=C' or 'LC_COLLATE=C' to get the traditional sort order that\n");
  printf("        uses native byte values.\n");
  printf("\n");
  printf("File Selection\n");
  printf("  -P LIST, --path=LIST\n");
  printf(
      "        Specify a list of directories in which to search for any FILE if it is\n");
  printf(
      "        not present in the current directory. In LIST, directory names are\n");
  printf(
      "        separated by colons. The order in which directories are listed is the\n");
  printf("        order followed by jdis in its search.\n");
  printf("\n");
  printf(
      "White-space and punctuation characters conform to the standard. At most 64 FILEs\n");
  printf("are supported.\n");
}

//CALCUL DE LA DISTANCE
float jaccard_distance(hashtable *ht1, hashtable *ht2) {
  if (ht1 == nullptr || ht2 == nullptr) {
    return 1.0f;
  }

  size_t common = 0;
  size_t total = 0;
  for (size_t i = 0; i < (ht1->nslots); ++i) {
    cell *curr = ht1->hasharray[i];
    while (curr != nullptr) {
      total += 1;
      if (hashtable_search(ht2, curr->keyref) != nullptr) {
        common += 1;
      }
      curr = curr->next;
    }
  }
  for (size_t i = 0; i < ht2->nslots; ++i) {
    cell *curr = ht2->hasharray[i];
    while (curr != nullptr) {
      if (hashtable_search(ht1, curr->keyref) == nullptr) {
        total++;
      }
      curr = curr->next;
    }
  }
  return (total == 0) ? 0.0f : 1.0f - ((float) common / (float) total);
}
