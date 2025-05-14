#include "jdis.h"
#include "../hashtable/hashtable.h"
#include "../holdall/holdall.h"
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

hashtable *get_words(const char *filename, int initial_letters_limit) {
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
  char word_buffer[256]; // Buffer to read full word
  char processed_word_buffer[256]; // Buffer for potentially truncated word
  while (fscanf(file, "%254s", word_buffer) == 1) { // Read into word_buffer
    const char *word_to_process;
    if (initial_letters_limit > 0
        && (int) strlen(word_buffer) > initial_letters_limit) {
      strncpy(processed_word_buffer, word_buffer,
          (size_t) initial_letters_limit);
      processed_word_buffer[initial_letters_limit] = '\0';
      word_to_process = processed_word_buffer;
    } else {
      word_to_process = word_buffer;
    }
    if (hashtable_search(ht_opaque_ptr, word_to_process) == nullptr) {
      char *word_copy = strdup(word_to_process);
      if (word_copy == nullptr) {
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

// --- Static helper functions for handle_graph_output ---

typedef struct {
  hashtable **file_hashtables;
  size_t num_files;
  char **filenames_in_order;
} graph_print_context_t;

// fun1 for holdall_apply_context: passes context through, word_ref is what fun2
// processes.
static void *pass_context_identity(void *context, void *word_ref_ptr) {
  (void) word_ref_ptr; // word_ref (ptr) is what fun2 will use, this fun1 passes
                       // context.
  return context;
}

// fun2 for holdall_apply_context: prints a row for the given word.
static int print_row_via_fun2(void *word_ref, void *context_from_fun1) {
  const char *current_word_str = (const char *) word_ref;
  graph_print_context_t *actual_context
    = (graph_print_context_t *) context_from_fun1;
  printf("%s", current_word_str);
  for (size_t j = 0; j < actual_context->num_files; ++j) {
    printf("\t");
    if (actual_context->file_hashtables[j] != nullptr
        && hashtable_search(actual_context->file_hashtables[j],
        current_word_str) != nullptr) {
      printf("x");
    } else {
      printf("-");
    }
  }
  printf("\n");
  return 0; // Continue apply
}

// --- Main function for graph output ---
void handle_graph_output(hashtable **file_hashtables, size_t num_files,
    char **filenames_in_order, int initial_letters_limit) {
  (void) initial_letters_limit; // Suppress unused parameter warning for now.
  holdall *all_unique_words_ha = holdall_empty();
  if (all_unique_words_ha == nullptr) {
    fprintf(stderr,
        "Error: Failed to allocate memory for holdall in graph mode.\n");
    return;
  }
  hashtable *master_word_registry_ht = hashtable_empty(compare_strings,
      hash_string, 0.75);
  if (master_word_registry_ht == nullptr) {
    fprintf(stderr,
        "Error: Failed to allocate memory for master hashtable in graph mode.\n");
    holdall_dispose(&all_unique_words_ha);
    return;
  }
  for (size_t i = 0; i < num_files; ++i) {
    if (file_hashtables[i] == nullptr) {
      continue;
    }
    struct hashtable *current_file_ht_struct
      = (struct hashtable *) file_hashtables[i];
    for (size_t slot = 0; slot < current_file_ht_struct->nslots; ++slot) {
      struct cell *current_cell = current_file_ht_struct->hasharray[slot];
      while (current_cell != nullptr) {
        const char *word_key = (const char *) current_cell->keyref;
        if (hashtable_search(master_word_registry_ht, word_key) == nullptr) {
          if (hashtable_add(master_word_registry_ht, (void *) word_key,
              (void *) 1) == nullptr) {
            fprintf(stderr,
                "Error: Failed to add word to master registry in graph mode.\n");
            goto cleanup_graph_resources;
          }
          if (holdall_put(all_unique_words_ha, (void *) word_key) != 0) {
            fprintf(stderr,
                "Error: Failed to put word into holdall in graph mode.\n");
            goto cleanup_graph_resources;
          }
        }
        current_cell = current_cell->next;
      }
    }
  }
#if defined HOLDALL_EXT && defined WANT_HOLDALL_EXT
  holdall_sort(all_unique_words_ha, compare_strings);
#else
  fprintf(stderr,
      "Warning: holdall_sort not available (WANT_HOLDALL_EXT not defined during compilation). Graph output will not be sorted by word.\n");
#endif
  printf("\t");
  for (size_t i = 0; i < num_files; ++i) {
    printf("%s", filenames_in_order[i]);
    if (i < num_files - 1) {
      printf("\t");
    }
  }
  printf("\n");
  graph_print_context_t actual_print_context = {
    file_hashtables, num_files, filenames_in_order
  };
  holdall_apply_context(all_unique_words_ha, &actual_print_context,
      pass_context_identity, print_row_via_fun2);
cleanup_graph_resources:
  holdall_dispose(&all_unique_words_ha);
  hashtable_dispose(&master_word_registry_ht);
}
