#include "jdis.h"
#include "../hashtable/hashtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
typedef struct cell cell;

struct hashtable {
  int (*compar)(const void *, const void *);
  size_t (*hashfun)(const void *);
  double lfmax;
  struct cell **hasharray;
  size_t nslots;
  size_t nentries;
};

struct cell { // This definition is for jdis.c internal use
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

hashtable *get_words(const char *filename) {
  FILE *file = fopen(filename, "r");
  if (file == nullptr) {
    fprintf(stderr, "Error : unable to open %s\n", filename);
    return nullptr;
  }
  // ht_internal is treated as an opaque type for library calls,
  // but cast to local struct for inspection/manipulation if needed by jdis.c
  // logic.
  struct hashtable *ht_internal_struct_ptr
    = (struct hashtable *) hashtable_empty(compare_strings, hash_string, 0.75);
  // Use the opaque pointer for library API calls.
  hashtable *ht_opaque_ptr = (hashtable *) ht_internal_struct_ptr;
  if (ht_opaque_ptr == nullptr) {
    fclose(file);
    return nullptr;
  }
  char word[256];
  while (fscanf(file, "%255s", word) == 1) {
    // Check if the string content of 'word' is already a key in the hashtable.
    // We use 'word' (the buffer) for searching.
    // hashtable_search expects a keyref that is comparable by compare_strings.
    if (hashtable_search(ht_opaque_ptr, word) == nullptr) {
      // Word not found, so strdup it and add it.
      char *word_copy = strdup(word);
      if (word_copy == nullptr) { // strdup failed
        fclose(file);
        // Free keys already added to this hashtable *for this file*.
        jdis_free_hashtable_content(ht_opaque_ptr);
        hashtable_dispose(&ht_opaque_ptr);
        return nullptr;
      }
      // Add the newly duplicated string to the hashtable.
      // The valref (void*)1 is just a placeholder.
      hashtable_add(ht_opaque_ptr, word_copy, (void *) 1);
    } else {
      // Word (string content) is already in the hashtable (from a previous
      // strdup).
      // Do not strdup again, do not add again. The current word_copy is not
      // created.
    }
  }
  fclose(file);
  return ht_opaque_ptr;
}

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
float jaccard_distance(hashtable *ht1_opaque, hashtable *ht2_opaque) {
  struct hashtable *ht1 = (struct hashtable *) ht1_opaque;
  struct hashtable *ht2 = (struct hashtable *) ht2_opaque;
  if (ht1 == nullptr || ht2 == nullptr) {
    return 1.0f;
  }
  size_t common = 0;
  size_t total = 0;
  // Iterate through ht1 using its assumed internal structure
  for (size_t i = 0; i < ht1->nslots; ++i) {
    struct cell *curr = ht1->hasharray[i];
    while (curr != nullptr) {
      total += 1;
      // Search for key from ht1 (curr->keyref) in ht2 using library function
      if (hashtable_search(ht2_opaque, curr->keyref) != nullptr) {
        common += 1;
      }
      curr = curr->next;
    }
  }
  // Iterate through ht2 using its assumed internal structure to count elements
  // not in ht1
  for (size_t i = 0; i < ht2->nslots; ++i) {
    struct cell *curr = ht2->hasharray[i];
    while (curr != nullptr) {
      // Search for key from ht2 (curr->keyref) in ht1 using library function
      if (hashtable_search(ht1_opaque, curr->keyref) == nullptr) {
        total++; // Count elements in ht2 that are not in ht1
      }
      curr = curr->next;
    }
  }
  return (total == 0) ? 0.0f : 1.0f - ((float) common / (float) total);
}

// Function to free keys before disposing the hashtable
void jdis_free_hashtable_content(hashtable *ht_opaque) {
  if (ht_opaque == nullptr) {
    return;
  }
  struct hashtable *ht_internal = (struct hashtable *) ht_opaque;
  for (size_t i = 0; i < ht_internal->nslots; ++i) {
    struct cell *current_cell = ht_internal->hasharray[i];
    while (current_cell != nullptr) {
      free((void *) current_cell->keyref); // Free the strdup'd key
      current_cell = current_cell->next;
    }
  }
}

void jdis_dispose_hashtable_array(hashtable **htt, size_t count) {
  if (htt == nullptr) {
    return;
  }
  for (size_t i = 0; i < count; ++i) {
    if (htt[i] != nullptr) {
      jdis_free_hashtable_content(htt[i]);
      hashtable_dispose(&htt[i]);
    }
  }
  free(htt);
}
