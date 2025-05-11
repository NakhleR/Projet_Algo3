#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include "hashtable.h"
#include "holdall.h"
#include "jdis.h"

#define WORD_LEN_MAX 31
void hashtable__tab_dispose(hashtable **htt, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    hashtable_dispose(&htt[i]);
  }
  free(htt);
}

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
  // Charge les mots uniques de chaque fichier dans une table
  size_t filenumb = (size_t)argc - 1;
  hashtable **ht_tab = malloc(sizeof(*ht_tab) * filenumb);
  for (size_t i = 0; i < filenumb; ++i) {
    ht_tab[i] = get_words(argv[i + 1]);
    if (ht_tab[i] == nullptr) {
      printf("An Error occured while putting the words into the hashtables\n");
      hashtable__tab_dispose(ht_tab, i);
      return EXIT_FAILURE;
    }
  }


  //hashtable *ht1 = get_words(argv[1]);
  //hashtable *ht2 = get_words(argv[2]);

  //if (ht1 == nullptr || ht2 == nullptr) {
    //hashtable_dispose(&ht1);
    //hashtable_dispose(&ht2);
    //return 1;
  //}
  for(size_t j = 1; j < filenumb; ++j) {
    for (size_t k = j + 1; k <= filenumb; ++k) {
      float d = jaccard_distance(ht_tab[j-1], ht_tab[k-1]);
      printf("(%ld::%ld)\n",j ,k);
      printf("%4f\t%s\t%s\n", d, argv[j], argv[k]);
    }
  }
  //float distance = jaccard_distance(ht1, ht2);

  //printf("%.4f\t%s\t%s\n", distance, argv[1], argv[2]);
   //Libère la mémoire
  //hashtable_dispose(&ht1);
  //hashtable_dispose(&ht2);

  hashtable__tab_dispose(ht_tab, filenumb - 1);
  return EXIT_SUCCESS;
}
