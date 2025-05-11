#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include "jdis.h"

#define WORD_LEN_MAX 31

int main(int argc, char *argv[]) {
  if (argc >= 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1],
      "-?") == 0)) {
    print_help();
    return EXIT_SUCCESS;
  }
  if (argc >= 2 && strcmp(argv[1], "--usage") == 0) {
    print_usage();
    return EXIT_SUCCESS;
  }
  if (argc < 3) {
    printf("jdis: Missing operand.\n");
    printf("Try 'jdis --help' for more information.\n");
    return EXIT_FAILURE;
  }
  // Charge les mots uniques des deux fichiers
  hashtable *ht1 = get_unique_words(argv[1]);
  hashtable *ht2 = get_unique_words(argv[2]);
  if (ht1 == nullptr || ht2 == nullptr) {
    hashtable_dispose(&ht1);
    hashtable_dispose(&ht2);
    return 1;
  }
  // Calcul de la distance : reste à définir
  //float distance = jaccard_distance(ht1, ht2);
  //exemple pour tester :
  double distance = 0.2;
  printf("%.4f\t%s\t%s\n", distance, argv[1], argv[2]);
  // Libère la mémoire
  hashtable_dispose(&ht1);
  hashtable_dispose(&ht2);
  return EXIT_SUCCESS;
}
