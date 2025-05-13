#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include "hashtable.h" // Only for `hashtable` type, not its internal functions
#include "holdall.h"   // If used directly by main, otherwise can be removed if
                       // only jdis uses it
#include "jdis.h"      // Now includes all jdis provided functions like
                       // get_words, jaccard_distance, and
                       // jdis_dispose_hashtable_array
// #include <stddef.h> // Not strictly needed if using NULL

#define WORD_LEN_MAX 31

// The function hashtable__tab_dispose is now removed, its logic is in
// jdis_dispose_hashtable_array.

int main(int argc, char *argv[]) {
  if (argc == 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1],
      "-?") == 0)) {
    print_help();
    return EXIT_SUCCESS;
  }
  if (argc == 2 && strcmp(argv[1], "--usage") == 0) {
    print_usage();
    return EXIT_SUCCESS;
  }
  if (argc < 3) {
    printf("jdis: Missing operand.\n");
    printf("Try 'jdis --help' for more information.\n");
    return EXIT_FAILURE;
  }
  size_t filenumb = (size_t) argc - 1;
  hashtable **ht_tab = malloc(sizeof(*ht_tab) * filenumb);
  if (ht_tab == NULL) { // Reverted to NULL
    perror("Failed to allocate memory for hashtable array");
    return EXIT_FAILURE;
  }
  for (size_t i = 0; i < filenumb; ++i) {
    ht_tab[i] = get_words(argv[i + 1]);
    if (ht_tab[i] == NULL) { // Reverted to NULL
      printf("An Error occurred while processing file: %s\n", argv[i + 1]);
      // Dispose of already allocated hashtables and the array itself
      jdis_dispose_hashtable_array(ht_tab, i); // Pass 'i' as count of
                                               // initialized tables
      return EXIT_FAILURE;
    }
  }
  for (size_t j = 1; j < filenumb; ++j) {
    for (size_t k = j + 1; k <= filenumb; ++k) {
      float d = jaccard_distance(ht_tab[j - 1], ht_tab[k - 1]);
      printf("(%ld::%ld)\n", j, k);
      printf("%4f\t%s\t%s\n", d, argv[j], argv[k]);
    }
  }
  jdis_dispose_hashtable_array(ht_tab, filenumb);
  return EXIT_SUCCESS;
}
