#include "jdis.h"
#include "hashtable.h"
#include "holdall.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

//  compare_strings_for_qsort : fonction de comparaison pour qsort (utilisée via
//    holdall_sort). Compare deux chaînes de caractères pointées indirectement
//    par a et b (qui sont des pointeurs vers des char*).
//    Utilise strcoll pour une comparaison sensible à la locale.
int compare_strings_for_qsort(const void *a, const void *b) {
  const char *s1 = *(const char **) a;
  const char *s2 = *(const char **) b;
  return strcoll(s1, s2);
}

//  compare_strings_for_hashtable : fonction de comparaison pour la table de
//    hachage. Compare deux chaînes de caractères pointées par a et b.
//    Utilise strcoll pour une comparaison sensible à la locale.
int compare_strings_for_hashtable(const void *a, const void *b) {
  return strcoll((const char *) a, (const char *) b);
}

//  hash_string : fonction de hachage pour une chaîne de caractères (algorithme
//    djb2).
//    Prend une clé (un pointeur vers une chaîne de caractères) et renvoie sa
//    valeur de hachage de type size_t.
size_t hash_string(const void *key) {
  const char *str = (const char *) key;
  size_t hash = 5381;
  int c;
  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + (size_t) c;
  }
  return hash;
}

//  free_holdall_word : fonction utilitaire pour holdall_apply. Libère la
// mémoire
//    allouée pour une chaîne de caractères (mot) pointée par word_ref.
//    Renvoie toujours 0 pour indiquer à holdall_apply de continuer.
static int free_holdall_word(void *word_ref) {
  free(word_ref);
  return 0;
}

void jdis_free_holdall_content(holdall *ha) {
  if (ha == NULL) {
    return;
  }
  holdall_apply(ha, free_holdall_word);
}

//  process_and_add_words : traite un mot, le tronque si nécessaire selon
//    initial_letters_limit, et l'ajoute au fourretout words_ha s'il n'est
//    pas déjà présent dans la table de hachage temp_uniqueness_ht (assurant
//    l'unicité). Met à jour temp_uniqueness_ht.
//    Renvoie 0 en cas de succès, -1 en cas d'erreur d'allocation.
static int process_and_add_words(
    const char *word_to_process_original,
    int initial_letters_limit,
    holdall *words_ha,
    hashtable *temp_uniqueness_ht,
    char *processed_word_buffer_for_truncation,
    const char *filename_for_log) {
  const char *word_to_add = word_to_process_original;
  if (initial_letters_limit > 0
      && (int) strlen(word_to_process_original) > initial_letters_limit) {
    strncpy(processed_word_buffer_for_truncation, word_to_process_original,
        (size_t) initial_letters_limit);
    processed_word_buffer_for_truncation[initial_letters_limit] = '\0';
    word_to_add = processed_word_buffer_for_truncation;
    fprintf(stderr,
        "Warning: Word '%s...' truncated to '%s' from file '%s' due to -i %d limit.\n",
        word_to_process_original,
        processed_word_buffer_for_truncation,
        filename_for_log,
        initial_letters_limit);
  }
  if (hashtable_search(temp_uniqueness_ht, word_to_add) == NULL) {
    char *word_copy = strdup(word_to_add);
    if (word_copy == NULL) {
      fprintf(stderr, "Error: strdup failed for word '%s' in file '%s'\n",
          word_to_add, filename_for_log);
      return -1;
    }
    if (hashtable_add(temp_uniqueness_ht, word_copy, (void *) 1) == NULL) {
      fprintf(stderr,
          "Error: hashtable_add failed for word '%s' in file '%s'\n",
          word_copy, filename_for_log);
      free(word_copy);
      return -1;
    }
    if (holdall_put(words_ha, word_copy) != 0) {
      fprintf(stderr, "Error: holdall_put failed for word '%s' in file '%s'\n",
          word_copy, filename_for_log);
      hashtable_remove(temp_uniqueness_ht, word_copy);
      free(word_copy);
      return -1;
    }
  }
  return 0;
}

holdall *get_words(const char *filename, int initial_letters_limit,
    bool punctuation_as_space) {
  FILE *file = NULL;
  file = fopen(filename, "r");
  if (file == NULL) {
    fprintf(stderr, "Error: unable to open file '%s'\n", filename);
    return NULL;
  }
  holdall *words_ha = holdall_empty();
  if (words_ha == NULL) {
    fclose(file);
    fprintf(stderr, "Error: Failed to allocate holdall for file '%s'\n",
        filename);
    return NULL;
  }
  hashtable *temp_uniqueness_ht = hashtable_empty(compare_strings_for_hashtable,
      hash_string,
      0.75);
  if (temp_uniqueness_ht == NULL) {
    fclose(file);
    holdall_dispose(&words_ha);
    fprintf(stderr, "Error: Failed to allocate temp hashtable for file '%s'\n",
        filename);
    return NULL;
  }
  char static_word_buffer[256];
  char *dynamic_word_buffer = NULL;
  size_t dynamic_buffer_capacity = 0;
  char *current_word_assembly_buffer = NULL;
  char processed_word_buffer[256];
  int char_code;
  int word_idx = 0;
  while ((char_code = fgetc(file)) != EOF) {
    char current_char = (char) char_code;
    bool is_delimiter = isspace(current_char)
        || (punctuation_as_space && ispunct(current_char));
    if (!is_delimiter) {
      if (initial_letters_limit == 0) {
        if (dynamic_word_buffer == NULL) {
          dynamic_buffer_capacity = 256;
          dynamic_word_buffer = malloc(dynamic_buffer_capacity);
          if (dynamic_word_buffer == NULL) {
            fprintf(stderr,
                "Error: malloc failed for dynamic_word_buffer in file '%s'\n",
                filename);
            goto cleanup_error;
          }
        } else if ((size_t) word_idx >= dynamic_buffer_capacity - 1) {
          dynamic_buffer_capacity *= 2;
          char *temp_realloc = realloc(dynamic_word_buffer,
              dynamic_buffer_capacity);
          if (temp_realloc == NULL) {
            fprintf(stderr,
                "Error: realloc failed for dynamic_word_buffer in file '%s'\n",
                filename);
            goto cleanup_error;
          }
          dynamic_word_buffer = temp_realloc;
        }
        dynamic_word_buffer[word_idx++] = current_char;
        current_word_assembly_buffer = dynamic_word_buffer;
      } else {
        if ((size_t) word_idx < sizeof(static_word_buffer) - 1) {
          static_word_buffer[word_idx++] = current_char;
        } else {
          static_word_buffer[word_idx] = '\0';
          current_word_assembly_buffer = static_word_buffer;
          if (process_and_add_words(current_word_assembly_buffer,
              initial_letters_limit, words_ha,
              temp_uniqueness_ht, processed_word_buffer,
              filename) != 0) {
            goto cleanup_error;
          }
          word_idx = 0;
          static_word_buffer[word_idx++] = current_char;
        }
        current_word_assembly_buffer = static_word_buffer;
      }
    } else {
      if (word_idx > 0) {
        current_word_assembly_buffer = (initial_letters_limit
            == 0) ? dynamic_word_buffer : static_word_buffer;
        current_word_assembly_buffer[word_idx] = '\0';
        if (process_and_add_words(current_word_assembly_buffer,
            initial_letters_limit, words_ha,
            temp_uniqueness_ht, processed_word_buffer,
            filename) != 0) {
          goto cleanup_error;
        }
        word_idx = 0;
      }
    }
  }
  if (word_idx > 0) {
    current_word_assembly_buffer = (initial_letters_limit
        == 0) ? dynamic_word_buffer : static_word_buffer;
    current_word_assembly_buffer[word_idx] = '\0';
    if (process_and_add_words(current_word_assembly_buffer,
        initial_letters_limit, words_ha,
        temp_uniqueness_ht, processed_word_buffer,
        filename) != 0) {
      goto cleanup_error;
    }
  }
  fclose(file);
  if (dynamic_word_buffer != NULL) {
    free(dynamic_word_buffer);
  }
  hashtable_dispose(&temp_uniqueness_ht);
  return words_ha;
cleanup_error:
  if (file != NULL) {
    fclose(file);
  }
  if (dynamic_word_buffer != NULL) {
    free(dynamic_word_buffer);
  }
  if (words_ha != NULL) {
    jdis_free_holdall_content(words_ha);
    holdall_dispose(&words_ha);
  }
  if (temp_uniqueness_ht != NULL) {
    hashtable_dispose(&temp_uniqueness_ht);
  }
  fprintf(stderr, "An error occurred during word processing for file '%s'.\n",
      filename);
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
  printf("Input Control\n");
  printf("  -i VALUE, --initial=VALUE\n");
  printf(
      "        Process words considering only the first VALUE significant initial letters (0 means no limit). Default is 0.\n");
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
  printf(
      "White-space and punctuation characters conform to the standard.\n");
}

//  fun1_populate_temp_ht : fonction pour holdall_apply_context. Ajoute word_ref
//    à la table de hachage pointée par context_temp_ht.
//    Renvoie le résultat de hashtable_add (un pointeur vers la valeur associée
//    à la clé, ou NULL en cas d'erreur ou si la clé existait déjà et que la
//    table ne permet pas les doublons, bien qu'ici les mots soient uniques
//    par construction de ha1).
static void *fun1_populate_temp_ht(void *context_temp_ht, void *word_ref) {
  hashtable *temp_ht = (hashtable *) context_temp_ht;
  return hashtable_add(temp_ht, word_ref, (void *) 1);
}

//  fun2_check_populate_error : fonction pour holdall_apply_context. Vérifie si
//    l'opération d'ajout (dont le résultat est add_result_from_fun1) dans la
//    table de hachage a échoué.
//    Renvoie 1 si add_result_from_fun1 est NULL (erreur), sinon 0.
//    word_ref n'est pas utilisé.
static int fun2_check_populate_error(void *word_ref,
    void *add_result_from_fun1) {
  (void) word_ref;
  return (add_result_from_fun1 == NULL) ? 1 : 0;
}

//  jd_count_common_context_t : structure de contexte utilisée pour compter les
//    éléments communs lors du calcul de la distance de Jaccard.
//    Membres :
//      ht_to_search : pointeur vers la table de hachage (contenant les mots
//                     d'un des ensembles) dans laquelle rechercher.
//      common_count : pointeur vers un compteur (size_t) à incrémenter pour
//                     chaque mot commun trouvé.
typedef struct {
  hashtable *ht_to_search;
  size_t *common_count;
} jd_count_common_context_t;

//  jd_count_common_pass_ctx : fonction pour holdall_apply_context (en tant que
//    fun1). Passe simplement le contexte 'ctx' à la fonction fun2.
//    'ref' n'est pas utilisé.
static void *jd_count_common_pass_ctx(void *ctx, void *ref) {
  (void) ref;
  return ctx;
}

//  jd_count_common_check_word : fonction pour holdall_apply_context (en tant
// que
//    fun2). Recherche word_ref_from_ha dans la table de hachage
//    (context->ht_to_search) et incrémente context->common_count si trouvé.
//    Renvoie toujours 0 pour continuer le parcours.
static int jd_count_common_check_word(void *word_ref_from_ha,
    void *ctx_from_fun1) {
  jd_count_common_context_t *context
    = (jd_count_common_context_t *) ctx_from_fun1;
  if (hashtable_search(context->ht_to_search, word_ref_from_ha) != NULL) {
    (*context->common_count)++;
  }
  return 0;
}

float jaccard_distance(holdall *ha1, holdall *ha2) {
  if (ha1 == NULL || ha2 == NULL) {
    return 1.0f;
  }
  size_t count1 = holdall_count(ha1);
  size_t count2 = holdall_count(ha2);
  if (count1 == 0 && count2 == 0) {
    return 0.0f;
  }
  hashtable *temp_ht_from_ha1 = hashtable_empty(compare_strings_for_hashtable,
      hash_string,
      0.75);
  if (temp_ht_from_ha1 == NULL) {
    fprintf(stderr,
        "Error: Failed to create temp hashtable for Jaccard distance.\n");
    return 1.0f;
  }
  if (holdall_apply_context(ha1, temp_ht_from_ha1, fun1_populate_temp_ht,
      fun2_check_populate_error) != 0) {
    hashtable_dispose(&temp_ht_from_ha1);
    fprintf(stderr,
        "Error: Failed to populate temp hashtable from ha1 for Jaccard distance.\n");
    return 1.0f;
  }
  size_t common = 0;
  jd_count_common_context_t common_ctx = {
    temp_ht_from_ha1, &common
  };
  holdall_apply_context(ha2, &common_ctx, jd_count_common_pass_ctx,
      jd_count_common_check_word);
  hashtable_dispose(&temp_ht_from_ha1);
  size_t union_size = count1 + count2 - common;
  return (union_size
    == 0) ? 0.0f : 1.0f - ((float) common / (float) union_size);
}

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

//  hgo_populate_temp_ht_context_t : structure de contexte utilisée pour peupler
//    les tables de hachage temporaires pour chaque fichier dans le mode
//    graphique.
//    Membres :
//      ht : pointeur vers la table de hachage à peupler.
//      error_flag : indicateur d'erreur (int), mis à 1 en cas d'échec
//                   d'allocation.
typedef struct {
  hashtable *ht;
  int error_flag;
} hgo_populate_temp_ht_context_t;

//  hgo_populate_temp_ht_pass_ctx : fonction pour holdall_apply_context (en tant
//    que fun1). Passe simplement le contexte 'ctx' à la fonction fun2.
//    'ref' n'est pas utilisé.
static void *hgo_populate_temp_ht_pass_ctx(void *ctx, void *ref) {
  (void) ref;
  return ctx;
}

//  hgo_populate_temp_ht_add_word : fonction pour holdall_apply_context (en tant
//    que fun2). Ajoute word_ref à la table de hachage context->ht.
//    Met context->error_flag à 1 et renvoie 1 en cas d'échec d'allocation
//    pour une nouvelle clé (non déjà présente). Sinon, renvoie 0.
static int hgo_populate_temp_ht_add_word(void *word_ref, void *ctx_from_fun1) {
  hgo_populate_temp_ht_context_t *context
    = (hgo_populate_temp_ht_context_t *) ctx_from_fun1;
  if (hashtable_add(context->ht, word_ref, (void *) 1) == NULL) {
    void *search_result = hashtable_search(context->ht, word_ref);
    if (search_result == NULL) {
      context->error_flag = 1;
      return 1;
    }
  }
  return 0;
}

//  hgo_collect_words_context_t : structure de contexte pour collecter tous les
//    mots uniques de tous les fichiers dans une table de hachage principale
//    (master_registry_ht) et un fourretout principal (all_unique_words_ha)
//    pour le mode graphique.
//    Membres :
//      master_registry_ht : pointeur vers la table de hachage principale
//                           enregistrant tous les mots uniques.
//      all_unique_words_ha : pointeur vers le fourretout principal stockant
//                            des pointeurs vers tous les mots uniques (pour
// tri).
//      error_flag : indicateur d'erreur (int), mis à 1 en cas d'échec
//                   d'allocation.
typedef struct {
  hashtable *master_registry_ht;
  holdall *all_unique_words_ha;
  int error_flag;
} hgo_collect_words_context_t;

//  hgo_collect_words_pass_ctx : fonction pour holdall_apply_context (en tant
//    que fun1). Passe simplement le contexte 'ctx' à la fonction fun2.
//    'ref' n'est pas utilisé.
static void *hgo_collect_words_pass_ctx(void *ctx, void *ref) {
  (void) ref;
  return ctx;
}

//  hgo_collect_words_add_master : fonction pour holdall_apply_context (en tant
//    que fun2). Si word_key_ref n'est pas dans context->master_registry_ht,
//    l'ajoute à la fois à context->master_registry_ht et à
//    context->all_unique_words_ha.
//    Met context->error_flag à 1 et renvoie 1 en cas d'échec d'allocation.
//    Sinon, renvoie 0.
static int hgo_collect_words_add_master(void *word_key_ref,
    void *ctx_from_fun1) {
  hgo_collect_words_context_t *ctx
    = (hgo_collect_words_context_t *) ctx_from_fun1;
  const char *word_key = (const char *) word_key_ref;
  if (hashtable_search(ctx->master_registry_ht, word_key) == NULL) {
    if (hashtable_add(ctx->master_registry_ht, (void *) word_key,
        (void *) 1) == NULL) {
      ctx->error_flag = 1;
      return 1;
    }
    if (holdall_put(ctx->all_unique_words_ha, (void *) word_key) != 0) {
      ctx->error_flag = 1;
      return 1;
    }
  }
  return 0;
}

//  hgo_graph_print_row_context_t : structure de contexte pour l'impression des
//    lignes de la sortie graphique. Contient les tables de hachage temporaires
//    de chaque fichier pour une recherche rapide, le nombre de fichiers, et les
//    noms des fichiers dans l'ordre.
//    Membres :
//      temp_file_hts_for_lookup : tableau de pointeurs vers les tables de
//                                 hachage temporaires (une par fichier).
//      num_files : nombre total de fichiers.
//      filenames_in_order : tableau des noms de fichiers.
typedef struct {
  hashtable **temp_file_hts_for_lookup;
  size_t num_files;
  char **filenames_in_order;
} hgo_graph_print_row_context_t;

//  pass_context_identity : fonction pour holdall_apply_context (en tant que
//    fun1). Passe simplement le 'context' fourni à la fonction fun2.
//    'word_ref_ptr' n'est pas utilisé.
static void *pass_context_identity(void *context, void *word_ref_ptr) {
  (void) word_ref_ptr;
  return context;
}

//  print_row_via_fun2 : fonction pour holdall_apply_context (en tant que fun2).
//    Affiche une ligne pour le mot 'word_ref'. La ligne contient le mot, suivi
//    d'une tabulation, puis pour chaque fichier (selon
//    actual_context->filenames_in_order), affiche 'x' si le mot est présent
//    dans la table de hachage temporaire correspondante du fichier, ou '-'
// sinon.
//    Renvoie toujours 0 pour continuer le parcours.
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
  return 0;
}

void handle_graph_output(holdall **file_holdalls, size_t num_files,
    char **filenames_in_order, int initial_letters_limit) {
  (void) initial_letters_limit;
  holdall *all_unique_words_ha = holdall_empty();
  if (all_unique_words_ha == NULL) {
    fprintf(stderr,
        "Error: Failed to allocate memory for holdall in graph mode.\n");
    return;
  }
  hashtable *master_word_registry_ht
    = hashtable_empty(compare_strings_for_hashtable,
      hash_string, 0.75);
  if (master_word_registry_ht == NULL) {
    fprintf(stderr,
        "Error: Failed to allocate memory for master hashtable in graph mode.\n");
    holdall_dispose(&all_unique_words_ha);
    return;
  }
  hashtable **temp_file_hts_for_lookup = calloc(num_files, sizeof(hashtable *));
  if (temp_file_hts_for_lookup == NULL) {
    fprintf(stderr,
        "Error: Failed to allocate array for temp lookup HTs in graph mode.\n");
    goto cleanup_graph_main_resources;
  }
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
      goto cleanup_graph_all_resources;
    }
  }
  for (size_t i = 0; i < num_files; ++i) {
    if (file_holdalls[i] != NULL && holdall_count(file_holdalls[i]) > 0) {
      temp_file_hts_for_lookup[i]
        = hashtable_empty(compare_strings_for_hashtable,
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
  }
#if defined HOLDALL_EXT && defined WANT_HOLDALL_EXT
  holdall_sort(all_unique_words_ha, compare_strings_for_qsort);
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
  holdall_dispose(&all_unique_words_ha);
  hashtable_dispose(&master_word_registry_ht);
}
