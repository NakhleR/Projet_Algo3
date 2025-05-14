#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <stdbool.h>
#include <errno.h>
#include "hashtable.h"
#include "holdall.h"
#include "jdis.h"

#define WORD_LEN_MAX 31

int main(int argc, char *argv[]) {
  setlocale(LC_ALL, ""); // Set locale for sorting, as mentioned in help
  bool graph_mode = false;
  int initial_letters_limit = 0;
  bool punctuation_as_space = false; // Added for -p option
  int opt_args_count = 0; // Number of argv slots taken by options like -g, -i,
                          // VALUE
  // --- Argument Parsing ---
  // Start checking from argv[1]
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "-g") == 0) {
      graph_mode = true;
      opt_args_count++;
    } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-?") == 0) {
      print_help();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "--usage") == 0) {
      print_usage();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "-i") == 0) {
      opt_args_count++; // for -i itself
      if (i + 1 < argc) {
        char *endptr;
        long val = strtol(argv[i + 1], &endptr, 10);
        if (endptr == argv[i + 1] || *endptr != '\0' || errno == ERANGE
            || val < 0) {
          fprintf(stderr,
              "jdis: Invalid value for -i: '%s'. Must be a non-negative integer.\n",
              argv[i + 1]);
          return EXIT_FAILURE;
        }
        initial_letters_limit = (int) val;
        i++; // Consume the VALUE argument
        opt_args_count++; // for VALUE
      } else {
        fprintf(stderr, "jdis: Option -i requires a value.\n");
        return EXIT_FAILURE;
      }
    } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i],
        "--punctuation-like-space") == 0) {
      punctuation_as_space = true;
      opt_args_count++;
    } else {
      // Check if it looks like an option but is unrecognized
      if (argv[i][0] == '-') {
        fprintf(stderr, "jdis: unrecognized option '%s'\n", argv[i]);
        fprintf(stderr, "Try 'jdis --help' for more information.\n");
        return EXIT_FAILURE;
      }
      // Assuming first non-option is a filename. Stop option parsing.
      break;
    }
  }
  int first_file_idx = 1 + opt_args_count;
  // Calculate num_actual_files. It must be non-negative.
  // If argc < first_file_idx, then there are no file arguments.
  size_t num_actual_files = 0;
  if (argc >= first_file_idx) {
    num_actual_files = (size_t) (argc - first_file_idx);
  }
  if (num_actual_files < 2 && graph_mode == false) { // Jaccard needs at least 2
                                                     // files
    fprintf(stderr,
        "jdis: At least two files are required for Jaccard distance.\n");
    fprintf(stderr, "Try 'jdis --help' for more information.\n");
    return EXIT_FAILURE;
  }
  if (num_actual_files < 1 && graph_mode == true) { // Graph mode needs at least
                                                    // 1 file
    fprintf(stderr, "jdis: At least one file is required for graph mode.\n");
    fprintf(stderr, "Try 'jdis --help' for more information.\n");
    return EXIT_FAILURE;
  }
  if (num_actual_files == 0 && graph_mode == false) { // General case if
                                                      // previous checks
    // don't catch
    fprintf(stderr, "jdis: Missing operands (filenames).\n");
    fprintf(stderr, "Try 'jdis --help' for more information.\n");
    return EXIT_FAILURE;
  }
  // --- File Processing ---
  holdall **ht_tab = malloc(sizeof(*ht_tab) * num_actual_files);
  if (ht_tab == NULL) {
    fprintf(stderr, "Failed to allocate memory for hashtable array\n");
    return EXIT_FAILURE;
  }
  // Initialize all pointers to NULL for safe disposal in case of early exit
  for (size_t i = 0; i < num_actual_files; ++i) {
    ht_tab[i] = NULL;
  }
  char **actual_filenames = &argv[first_file_idx];
  for (size_t i = 0; i < num_actual_files; ++i) {
    ht_tab[i] = get_words(actual_filenames[i], initial_letters_limit,
        punctuation_as_space);
    if (ht_tab[i] == NULL) {
      fprintf(stderr, "An Error occurred while processing file: %s\n",
          actual_filenames[i]);
      jdis_dispose_holdall_array(ht_tab, num_actual_files);
      return EXIT_FAILURE;
    }
  }
  // --- Output Generation ---
  if (graph_mode == true) {
    handle_graph_output(ht_tab, num_actual_files, actual_filenames,
        initial_letters_limit);
  } else {
    // Jaccard distance calculation (requires at least 2 files, checked above)
    for (size_t j = 0; j < num_actual_files; ++j) {
      for (size_t k = j + 1; k < num_actual_files; ++k) {
        float d = jaccard_distance(ht_tab[j], ht_tab[k]);
        printf("%.4f\t%s\t%s\n", d, actual_filenames[j], actual_filenames[k]);
      }
    }
  }
  jdis_dispose_holdall_array(ht_tab, num_actual_files);
  return EXIT_SUCCESS;
}
