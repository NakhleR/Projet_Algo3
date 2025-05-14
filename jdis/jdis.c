#include "jdis.h"
#include "hashtable.h"
#include "holdall.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

// Fonction pour comparer deux chaînes
int compare_strings(const void *a, const void *b) {
  return strcoll((const char *) a, (const char *) b);
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

// --- Static helper function for freeing words in holdall ---
static int free_word_in_holdall_callback(void *word_ref) {
  free(word_ref);
  return 0; // Continue apply
}

// Function to free strdup'd keys stored in a holdall
void jdis_free_holdall_content(holdall *ha) {
  if (ha == NULL) {
    return;
  }
  holdall_apply(ha, free_word_in_holdall_callback);
}

// Modified get_words to return holdall* and handle punctuation_as_space, and
// dynamic buffer
holdall *get_words(const char *filename, int initial_letters_limit,
    bool punctuation_as_space) {
  FILE *file;
  bool reading_stdin = (strcmp(filename, "-") == 0);
  if (reading_stdin) {
    file = stdin;
  } else {
    file = fopen(filename, "r");
    if (file == NULL) {
      fprintf(stderr, "Error: unable to open %s\n", filename);
      return NULL;
    }
  }
  holdall *words_ha = holdall_empty();
  if (words_ha == NULL) {
    if (!reading_stdin) {
      fclose(file);
    }
    fprintf(stderr, "Error: Failed to allocate holdall in get_words for %s\n",
        reading_stdin ? "stdin" : filename);
    return NULL;
  }
  hashtable *temp_uniqueness_ht
    = hashtable_empty(compare_strings, hash_string, 0.75);
  if (temp_uniqueness_ht == NULL) {
    if (!reading_stdin) {
      fclose(file);
    }
    holdall_dispose(&words_ha);
    fprintf(stderr,
        "Error: Failed to allocate temp hashtable in get_words for %s\n",
        reading_stdin ? "stdin" : filename);
    return NULL;
  }
  char static_word_buffer[256]; // Used when initial_letters_limit > 0 (and word
                                // fits)
  char *dynamic_word_buffer = NULL; // Used when initial_letters_limit == 0
  size_t dynamic_buffer_capacity = 0;
  char *current_word_assembly_buffer = NULL; // Points to static or dynamic
                                             // buffer
  char processed_word_buffer[256]; // For truncation output if needed
  int char_code;
  int word_idx = 0;
  while ((char_code = fgetc(file)) != EOF) {
    char current_char = (char) char_code;
    bool is_delimiter
      = isspace(current_char)
        || (punctuation_as_space && ispunct(current_char));
    if (!is_delimiter) {
      if (initial_letters_limit == 0) { // Use dynamic buffer
        if (dynamic_word_buffer == NULL) { // First char of a word, allocate
          dynamic_buffer_capacity = 256; // Initial capacity
          dynamic_word_buffer = malloc(dynamic_buffer_capacity);
          if (dynamic_word_buffer == NULL) {
            fprintf(stderr,
                "Error: malloc failed for dynamic_word_buffer in %s\n",
                reading_stdin ? "stdin" : filename);
            goto cleanup_error_dynamic_buffer; // Unified cleanup
          }
        } else if ((size_t) word_idx >= dynamic_buffer_capacity - 1) { // -1 for
                                                                       // NULL
                                                                       // terminator
          dynamic_buffer_capacity *= 2;
          char *temp_realloc = realloc(dynamic_word_buffer,
              dynamic_buffer_capacity);
          if (temp_realloc == NULL) {
            fprintf(stderr,
                "Error: realloc failed for dynamic_word_buffer in %s\n",
                reading_stdin ? "stdin" : filename);
            goto cleanup_error_dynamic_buffer; // Unified cleanup
          }
          dynamic_word_buffer = temp_realloc;
        }
        dynamic_word_buffer[word_idx++] = current_char;
        current_word_assembly_buffer = dynamic_word_buffer;
      } else { // initial_letters_limit > 0. Use static_word_buffer if word part
               // fits, else process.
        if ((size_t) word_idx < sizeof(static_word_buffer) - 1) {
          static_word_buffer[word_idx++] = current_char;
        } else { // Static buffer is full, process current content
          static_word_buffer[word_idx] = '\0'; // Null-terminate current segment
          current_word_assembly_buffer = static_word_buffer;
          // --- Process word (common block) --- START
          const char *word_to_process = current_word_assembly_buffer;
          // Truncation for -i > 0 is applied here
          if ((int) strlen(current_word_assembly_buffer)
              > initial_letters_limit) {
            strncpy(processed_word_buffer, current_word_assembly_buffer,
                (size_t) initial_letters_limit);
            processed_word_buffer[initial_letters_limit] = '\0';
            word_to_process = processed_word_buffer;
            fprintf(stderr,
                "Warning: Word '%s...' truncated to '%s' from file '%s' due to -i %d limit.\n",
                current_word_assembly_buffer, processed_word_buffer,
                reading_stdin ? "stdin" : filename, initial_letters_limit);
          }
          if (hashtable_search(temp_uniqueness_ht, word_to_process) == NULL) {
            char *word_copy = strdup(word_to_process);
            if (word_copy == NULL) {
              goto cleanup_error_dynamic_buffer;
            }
            if (hashtable_add(temp_uniqueness_ht, word_copy,
                (void *) 1) == NULL) {
              free(word_copy);
              goto cleanup_error_dynamic_buffer;
            }
            if (holdall_put(words_ha, word_copy) != 0) {
              hashtable_remove(temp_uniqueness_ht, word_copy);
              free(word_copy);
              goto cleanup_error_dynamic_buffer;
            }
          }
          // --- Process word (common block) --- END
          word_idx = 0; // Reset static_word_buffer index
          static_word_buffer[word_idx++] = current_char; // current_char starts
                                                         // the new segment
        }
        current_word_assembly_buffer = static_word_buffer;
      }
    } else { // Is a delimiter
      if (word_idx > 0) { // We have a word in the buffer
        // Determine which buffer was being used based on initial_letters_limit
        current_word_assembly_buffer = (initial_letters_limit
            == 0) ? dynamic_word_buffer : static_word_buffer;
        current_word_assembly_buffer[word_idx] = '\0';
        // --- Process word (common block) --- START
        const char *word_to_process = current_word_assembly_buffer;
        if (initial_letters_limit > 0
            && (int) strlen(current_word_assembly_buffer)
            > initial_letters_limit) {
          strncpy(processed_word_buffer, current_word_assembly_buffer,
              (size_t) initial_letters_limit);
          processed_word_buffer[initial_letters_limit] = '\0';
          word_to_process = processed_word_buffer;
          fprintf(stderr,
              "Warning: Word '%s...' truncated to '%s' from file '%s' due to -i %d limit.\n",
              current_word_assembly_buffer, processed_word_buffer,
              reading_stdin ? "stdin" : filename, initial_letters_limit);
        }
        // No specific truncation for initial_letters_limit == 0 here, full word
        // is processed.
        if (hashtable_search(temp_uniqueness_ht, word_to_process) == NULL) {
          char *word_copy = strdup(word_to_process);
          if (word_copy == NULL) {
            goto cleanup_error_dynamic_buffer;
          }
          if (hashtable_add(temp_uniqueness_ht, word_copy,
              (void *) 1) == NULL) {
            free(word_copy);
            goto cleanup_error_dynamic_buffer;
          }
          if (holdall_put(words_ha, word_copy) != 0) {
            hashtable_remove(temp_uniqueness_ht, word_copy);
            free(word_copy);
            goto cleanup_error_dynamic_buffer;
          }
        }
        // --- Process word (common block) --- END
        word_idx = 0; // Reset for next word
        if (initial_letters_limit == 0 && dynamic_word_buffer != NULL) {
          // For dynamic buffer, effectively reset it for the next word by only
          // changing word_idx.
          // Content will be overwritten or buffer realloc'd if needed.
        }
      }
    }
  }
  // After EOF, check if there's a pending word in the buffer
  if (word_idx > 0) {
    current_word_assembly_buffer = (initial_letters_limit
        == 0) ? dynamic_word_buffer : static_word_buffer;
    current_word_assembly_buffer[word_idx] = '\0';
    // --- Process word (common block) --- START
    const char *word_to_process = current_word_assembly_buffer;
    if (initial_letters_limit > 0
        && (int) strlen(current_word_assembly_buffer) > initial_letters_limit) {
      strncpy(processed_word_buffer, current_word_assembly_buffer,
          (size_t) initial_letters_limit);
      processed_word_buffer[initial_letters_limit] = '\0';
      word_to_process = processed_word_buffer;
      fprintf(stderr,
          "Warning: Word '%s...' truncated to '%s' from file '%s' due to -i %d limit.\n",
          current_word_assembly_buffer, processed_word_buffer,
          reading_stdin ? "stdin" : filename, initial_letters_limit);
    }
    if (hashtable_search(temp_uniqueness_ht, word_to_process) == NULL) {
      char *word_copy = strdup(word_to_process);
      if (word_copy == NULL) {
        goto cleanup_error_dynamic_buffer;
      }
      if (hashtable_add(temp_uniqueness_ht, word_copy, (void *) 1) == NULL) {
        free(word_copy);
        goto cleanup_error_dynamic_buffer;
      }
      if (holdall_put(words_ha, word_copy) != 0) {
        hashtable_remove(temp_uniqueness_ht, word_copy);
        free(word_copy);
        goto cleanup_error_dynamic_buffer;
      }
    }
    // --- Process word (common block) --- END
  }
  if (!reading_stdin) {
    fclose(file);
  }
  if (dynamic_word_buffer != NULL) {
    free(dynamic_word_buffer);
  }
  hashtable_dispose(&temp_uniqueness_ht);
  return words_ha;
cleanup_error_dynamic_buffer:
  // Unified cleanup path
  if (dynamic_word_buffer != NULL) {
    free(dynamic_word_buffer);
  }
  // Fall through to existing resource cleanup for file, holdall, hashtable
  // cleanup_error: // This label is implicitly part of the above now
  if (!reading_stdin && file != NULL && file != stdin) {
    fclose(file); // Ensure file is valid before closing
  }
  if (words_ha != NULL) { // Ensure words_ha was allocated before freeing
                          // content
    jdis_free_holdall_content(words_ha);
    holdall_dispose(&words_ha);
  }
  if (temp_uniqueness_ht != NULL) {
    hashtable_dispose(&temp_uniqueness_ht);
  }
  fprintf(stderr, "Error during word processing for %s\n",
      reading_stdin ? "stdin" : filename);
  return NULL;
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

// --- Static helper functions for jaccard_distance ---

// For populating the temporary hashtable from ha1
static void *fun1_populate_temp_ht(void *context_temp_ht, void *word_ref) {
  hashtable *temp_ht = (hashtable *) context_temp_ht;
  // Words in ha1 (from get_words) are unique.
  // So, if hashtable_add returns NULL, it's an OOM for a new key.
  return hashtable_add(temp_ht, word_ref, (void *) 1);
}

static int fun2_check_populate_error(void *word_ref,
    void *add_result_from_fun1) {
  (void) word_ref; // word_ref is unused in this checker
  // If add_result_from_fun1 is NULL (error from fun1_populate_temp_ht),
  // return
  // 1 to stop.
  return (add_result_from_fun1 == NULL) ? 1 : 0;
}

// For counting common elements (these can remain as they are efficient)
typedef struct {
  hashtable *ht_to_search;
  size_t *common_count;
} jd_count_common_context_t; // This struct is quite minimal

static void *jd_count_common_pass_ctx(void *ctx, void *ref) {
  (void) ref;
  return ctx;
}

static int jd_count_common_check_word(void *word_ref_from_ha,
    void *ctx_from_fun1) {
  jd_count_common_context_t *context
    = (jd_count_common_context_t *) ctx_from_fun1;
  if (hashtable_search(context->ht_to_search, word_ref_from_ha) != NULL) {
    (*context->common_count)++;
  }
  return 0; // Continue apply
}

float jaccard_distance(holdall *ha1, holdall *ha2) {
  if (ha1 == NULL || ha2 == NULL) {
    // Consider if one is NULL and other is not. If both non-NULL but one
    // is
    // empty,
    // current logic handles it. If passed NULL explicitly, 1.0f is
    // reasonable.
    return 1.0f;
  }
  size_t count1 = holdall_count(ha1);
  size_t count2 = holdall_count(ha2);
  if (count1 == 0 && count2 == 0) {
    return 0.0f;
  }
  // If one is empty and the other is not, common is 0, union_size is count of
  // non-empty.
  // Distance = 1.0 - (0 / count_non_empty) = 1.0. Correct.
  hashtable *temp_ht_from_ha1 = hashtable_empty(compare_strings, hash_string,
      0.75);
  if (temp_ht_from_ha1 == NULL) {
    fprintf(stderr,
        "Error: Failed to create temp hashtable for Jaccard distance.\n");
    return 1.0f; // Cannot compute accurately
  }
  // Populate temp_ht_from_ha1 using ha1
  // The context passed to holdall_apply_context is temp_ht_from_ha1 itself.
  if (holdall_apply_context(ha1, temp_ht_from_ha1, fun1_populate_temp_ht,
      fun2_check_populate_error) != 0) {
    // fun2_check_populate_error returned non-zero, meaning an error during
    // hashtable_add
    hashtable_dispose(&temp_ht_from_ha1);
    fprintf(stderr,
        "Error: Failed to populate temp hashtable from ha1 for Jaccard distance.\n");
    return 1.0f;
  }
  size_t common = 0;
  jd_count_common_context_t common_ctx = {
    temp_ht_from_ha1, &common
  };
  // Iterate through ha2 to count common words
  holdall_apply_context(ha2, &common_ctx, jd_count_common_pass_ctx,
      jd_count_common_check_word);
  // holdall_apply_context for counting common doesn't signal errors in its
  // return for this setup,
  // as jd_count_common_check_word always returns 0.
  hashtable_dispose(&temp_ht_from_ha1);
  size_t union_size = count1 + count2 - common;
  return (union_size
    == 0) ? 0.0f : 1.0f - ((float) common / (float) union_size);
}

// Function to free keys before disposing the hashtable - NOW FOR HOLDALL
// void jdis_free_hashtable_content(hashtable *ht_opaque) { // OLD name
// Already defined above as jdis_free_holdall_content

void jdis_dispose_holdall_array(holdall **ha_array, size_t count) {
  if (ha_array == NULL) {
    return;
  }
  for (size_t i = 0; i < count; ++i) {
    if (ha_array[i] != NULL) {
      jdis_free_holdall_content(ha_array[i]);
      holdall_dispose(&ha_array[i]);
    }
  }
  free(ha_array);
}

// --- Static helper functions for handle_graph_output ---

// Context for populating temporary hashtables from holdalls
typedef struct {
  hashtable *ht;
  int error_flag;
} hgo_populate_temp_ht_context_t;

static void *hgo_populate_temp_ht_pass_ctx(void *ctx, void *ref) {
  (void) ref;
  return ctx;
}

static int hgo_populate_temp_ht_add_word(void *word_ref, void *ctx_from_fun1) {
  hgo_populate_temp_ht_context_t *context
    = (hgo_populate_temp_ht_context_t *) ctx_from_fun1;
  // Assuming word_ref are unique char* from get_words, and ht is fresh.
  if (hashtable_add(context->ht, word_ref, (void *) 1) == NULL) {
    void *search_result = hashtable_search(context->ht, word_ref);
    if (search_result == NULL) { // If add failed for a truly new key (OOM)
      context->error_flag = 1;
      return 1; // Stop apply
    }
  }
  return 0; // Continue apply
}

// Context for collecting all unique words into master registry and master
// holdall
typedef struct {
  hashtable *master_registry_ht;
  holdall *all_unique_words_ha;
  int error_flag;
} hgo_collect_words_context_t;

static void *hgo_collect_words_pass_ctx(void *ctx, void *ref) {
  (void) ref;
  return ctx;
}

static int hgo_collect_words_add_master(void *word_key_ref,
    void *ctx_from_fun1) {
  hgo_collect_words_context_t *ctx
    = (hgo_collect_words_context_t *) ctx_from_fun1;
  const char *word_key = (const char *) word_key_ref;
  if (hashtable_search(ctx->master_registry_ht, word_key) == NULL) {
    // word_key points to string owned by one of the file_holdalls.
    // Master registry and holdall will store this pointer, not a new copy.
    if (hashtable_add(ctx->master_registry_ht, (void *) word_key,
        (void *) 1) == NULL) {
      // OOM adding to master registry
      ctx->error_flag = 1;
      return 1;
    }
    if (holdall_put(ctx->all_unique_words_ha, (void *) word_key) != 0) {
      // OOM adding to master holdall
      ctx->error_flag = 1;
      return 1;
    }
  }
  return 0; // Continue
}

// Renamed graph_print_context for clarity within handle_graph_output
typedef struct {
  hashtable **temp_file_hts_for_lookup; // Stores temporary HTs for each file
  size_t num_files;
  char **filenames_in_order;
} hgo_graph_print_row_context_t;

// fun1 for holdall_apply_context: passes context through, word_ref is what fun2
// processes.
// This can be reused.
static void *pass_context_identity(void *context, void *word_ref_ptr) {
  (void) word_ref_ptr;
  return context;
}

// fun2 for holdall_apply_context: prints a row for the given word.
// Context is now hgo_graph_print_row_context_t*
static int print_row_via_fun2(void *word_ref, void *context_from_fun1) {
  const char *current_word_str = (const char *) word_ref;
  hgo_graph_print_row_context_t *actual_context
    = (hgo_graph_print_row_context_t *) context_from_fun1;
  printf("%s", current_word_str);
  for (size_t j = 0; j < actual_context->num_files; ++j) {
    printf("\t");
    if (actual_context->temp_file_hts_for_lookup[j] != NULL
        && hashtable_search(actual_context->temp_file_hts_for_lookup[j],
        current_word_str) != NULL) {
      printf("x");
    } else {
      printf("-");
    }
  }
  printf("\n");
  return 0; // Continue apply
}

// --- Main function for graph output ---
void handle_graph_output(holdall **file_holdalls, size_t num_files,
    char **filenames_in_order, int initial_letters_limit) {
  (void) initial_letters_limit;
  holdall *all_unique_words_ha = holdall_empty();
  if (all_unique_words_ha == NULL) {
    fprintf(stderr,
        "Error: Failed to allocate memory for holdall in graph mode.\n");
    return;
  }
  hashtable *master_word_registry_ht = hashtable_empty(compare_strings,
      hash_string, 0.75);
  if (master_word_registry_ht == NULL) {
    fprintf(stderr,
        "Error: Failed to allocate memory for master hashtable in graph mode.\n");
    holdall_dispose(&all_unique_words_ha);
    return;
  }
  // Temporary hashtables for each file, for fast lookup during printing
  hashtable **temp_file_hts_for_lookup = calloc(num_files, sizeof(hashtable *));
  if (temp_file_hts_for_lookup == NULL) {
    fprintf(stderr,
        "Error: Failed to allocate array for temp lookup HTs in graph mode.\n");
    goto cleanup_graph_main_resources;
  }
  // Note: calloc initializes to NULL pointers.
  // Populate master registry and all_unique_words_ha
  for (size_t i = 0; i < num_files; ++i) {
    if (file_holdalls[i] == NULL) {
      continue;
    }
    hgo_collect_words_context_t collect_ctx = {
      master_word_registry_ht, all_unique_words_ha, 0
    };
    if (holdall_apply_context(file_holdalls[i], &collect_ctx,
        hgo_collect_words_pass_ctx, hgo_collect_words_add_master) != 0
        || collect_ctx.error_flag) {
      fprintf(stderr,
          "Error: Failed while collecting unique words for graph mode.\n");
      goto cleanup_graph_all_resources; // Includes temp_file_hts
    }
  }
  // Populate temporary lookup hashtables for each file
  for (size_t i = 0; i < num_files; ++i) {
    if (file_holdalls[i] != NULL && holdall_count(file_holdalls[i]) > 0) {
      temp_file_hts_for_lookup[i] = hashtable_empty(compare_strings,
          hash_string, 0.75);
      if (temp_file_hts_for_lookup[i] == NULL) {
        fprintf(stderr, "Error: Failed to create temp lookup HT for file %s.\n",
            filenames_in_order[i]);
        goto cleanup_graph_all_resources;
      }
      hgo_populate_temp_ht_context_t pop_temp_ctx = {
        temp_file_hts_for_lookup[i], 0
      };
      if (holdall_apply_context(file_holdalls[i], &pop_temp_ctx,
          hgo_populate_temp_ht_pass_ctx, hgo_populate_temp_ht_add_word) != 0
          || pop_temp_ctx.error_flag) {
        fprintf(stderr,
            "Error: Failed to populate temp lookup HT for file %s.\n",
            filenames_in_order[i]);
        goto cleanup_graph_all_resources;
      }
    }
    // If file_holdalls[i] is NULL or empty, temp_file_hts_for_lookup[i]
    // remains
    // NULL (from calloc)
  }
#if defined HOLDALL_EXT && defined WANT_HOLDALL_EXT
  holdall_sort(all_unique_words_ha, compare_strings);
#else
  fprintf(stderr,
      "Warning: holdall_sort not available. Graph output will not be sorted by word.\n");
#endif
  printf("\t");
  for (size_t i = 0; i < num_files; ++i) {
    printf("%s", filenames_in_order[i]);
    if (i < num_files - 1) {
      printf("\t");
    }
  }
  printf("\n");
  hgo_graph_print_row_context_t actual_print_context = {
    temp_file_hts_for_lookup, num_files, filenames_in_order
  };
  holdall_apply_context(all_unique_words_ha, &actual_print_context,
      pass_context_identity, print_row_via_fun2);
cleanup_graph_all_resources:
  if (temp_file_hts_for_lookup != NULL) {
    for (size_t i = 0; i < num_files; ++i) {
      if (temp_file_hts_for_lookup[i] != NULL) {
        hashtable_dispose(&temp_file_hts_for_lookup[i]);
      }
    }
    free(temp_file_hts_for_lookup);
  }
cleanup_graph_main_resources:
  holdall_dispose(&all_unique_words_ha); // Elements are pointers to strings in
                                         // file_holdalls, not freed here.
  hashtable_dispose(&master_word_registry_ht); // Keys are pointers, not freed
                                               // by hashtable_dispose.
}
