#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include "jdis.h"

#define WORD_LEN_MAX 31

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
    return EXIT_FAILURE;
  }

  hashtable__print_values(ht1);
  hashtable__print_values(ht2);
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
