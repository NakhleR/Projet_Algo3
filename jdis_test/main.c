#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include "jdis.h"

#define WORD_LEN_MAX 31

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s fichier1.txt fichier2.txt\n", argv[0]);
        return 1;
    }

    // Charge les mots uniques des deux fichiers
    hashtable *ht1 = get_unique_words(argv[1]);
    hashtable *ht2 = get_unique_words(argv[2]);

    if (!ht1 || !ht2) {
        hashtable_dispose(&ht1);
        hashtable_dispose(&ht2);
        return 1;
    }

    // Calcule et affiche la distance
    float distance = jaccard_distance(ht1, ht2);
    printf("Distance de Jaccard: %.4f\n", distance);

    // Libère la mémoire
    hashtable_dispose(&ht1);
    hashtable_dispose(&ht2);

    return 0;
}
