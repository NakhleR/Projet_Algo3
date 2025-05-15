#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <stdbool.h>
#include <errno.h>
#include "hashtable.h"
#include "holdall.h"
#include "holdall_ip.h"
#include "jdis.h"

#define MAX_FILES_SUPPORTED 64

int main(int argc, char *argv[]) {
  setlocale(LC_ALL, "");
  bool graph_mode = false;
  int initial_letters_limit = 0;
  bool punctuation_as_space = false;
  int opt_args_count = 0;
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "--graph") == 0) {
      graph_mode = true;
      opt_args_count++;
    } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-?") == 0) {
      print_help();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "--usage") == 0) {
      print_usage();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "-i") == 0) {
      opt_args_count++;
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
        i++;
        opt_args_count++;
      } else {
        fprintf(stderr, "jdis: Option -i requires a value.\n");
        return EXIT_FAILURE;
      }
    } else if (strncmp(argv[i], "--initial=", strlen("--initial=")) == 0) {
      const char *value_str = argv[i] + strlen("--initial=");
      char *endptr;
      long val = strtol(value_str, &endptr, 10);
      if (endptr == value_str || *endptr != '\0' || errno == ERANGE
          || val < 0) {
        fprintf(stderr,
            "jdis: Invalid value for --initial: '%s'. Must be a non-negative integer.\n",
            value_str);
        return EXIT_FAILURE;
      }
      initial_letters_limit = (int) val;
      opt_args_count++;
    } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i],
        "--punctuation-like-space") == 0) {
      punctuation_as_space = true;
      opt_args_count++;
    } else {
      if (argv[i][0] == '-') {
        fprintf(stderr, "jdis: unrecognized option '%s'\n", argv[i]);
        fprintf(stderr, "Try 'jdis --help' for more information.\n");
        return EXIT_FAILURE;
      }
      break;
    }
  }
  int first_file_idx = 1 + opt_args_count;
  size_t num_actual_files = 0;
  if (argc >= first_file_idx) {
    num_actual_files = (size_t) (argc - first_file_idx);
  }
  if (num_actual_files > MAX_FILES_SUPPORTED) {
    fprintf(stderr, "jdis: Too many files. At most %d files are supported.\n",
        MAX_FILES_SUPPORTED);
    fprintf(stderr, "Try 'jdis --help' for more information.\n");
    return EXIT_FAILURE;
  }
  if (num_actual_files < 2 && graph_mode == false) {
    fprintf(stderr,
        "jdis: At least two files are required for Jaccard distance.\n");
    fprintf(stderr, "Try 'jdis --help' for more information.\n");
    return EXIT_FAILURE;
  }
  if (num_actual_files < 2 && graph_mode == true) {
    fprintf(stderr, "jdis: At least one file is required for graph mode.\n");
    fprintf(stderr, "Try 'jdis --help' for more information.\n");
    return EXIT_FAILURE;
  }
  if (num_actual_files == 0 && graph_mode == false) {
    fprintf(stderr, "jdis: Missing operands (filenames).\n");
    fprintf(stderr, "Try 'jdis --help' for more information.\n");
    return EXIT_FAILURE;
  }
  holdall **ht_tab = malloc(sizeof(*ht_tab) * num_actual_files);
  if (ht_tab == nullptr) {
    fprintf(stderr, "Failed to allocate memory for hashtable array\n");
    return EXIT_FAILURE;
  }
  for (size_t i = 0; i < num_actual_files; ++i) {
    ht_tab[i] = nullptr;
  }
  char **actual_filenames = &argv[first_file_idx];
  for (size_t i = 0; i < num_actual_files; ++i) {
    ht_tab[i] = get_words(actual_filenames[i], initial_letters_limit,
        punctuation_as_space);
    if (ht_tab[i] == nullptr) {
      fprintf(stderr, "An Error occurred while processing file: %s\n",
          actual_filenames[i]);
      jdis_dispose_holdall_array(ht_tab, num_actual_files);
      return EXIT_FAILURE;
    }
  }
  if (graph_mode == true) {
    handle_graph_output(ht_tab, num_actual_files, actual_filenames,
        initial_letters_limit);
  } else {
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
