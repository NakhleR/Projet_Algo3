#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include "jdis.h"

#define WORD_LEN_MAX 31

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");

    // Options
    int opt_g = 0;
    int opt_i = 0;
    int opt_p = 0;

    // Fichiers restants à traiter
    int first_file_arg = 1;

    // Lecture des options
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-g") == 0)
        {
            opt_g = 1;
        }
        else if (strcmp(argv[i], "-i") == 0)
        {
            opt_i = 1;
        }
        else if (strcmp(argv[i], "-p") == 0)
        {
            opt_p = 1;
        }
        else
        {
            first_file_arg = i;
            break;
        }
    }

    if (first_file_arg >= argc)
    {
        fprintf(stderr, "Usage: %s [-g] [-i] [-p] file1.txt [file2.txt ...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    jdis *jd = jdis_empty();
    if (jd == nullptr)
    {
        fprintf(stderr, "*** Error: could not allocate jdis\n");
        return EXIT_FAILURE;
    }

    int status = EXIT_SUCCESS;

    // Lecture et traitement de chaque fichier
    for (int i = first_file_arg; i < argc; ++i)
    {
        const char *filename = argv[i];
        FILE *f = fopen(filename, "r");
        if (f == nullptr)
        {
            fprintf(stderr, "*** Warning: Cannot open file: %s\n", filename);
            status = EXIT_FAILURE;
            continue;
        }

        if (jdis_process_file(jd, filename, f, opt_i ? 0 : WORD_LEN_MAX, opt_p) != 0)
        {
            fprintf(stderr, "*** Warning: Could not process file: %s\n", filename);
            status = EXIT_FAILURE;
        }

        fclose(f);
    }

    if (opt_g)
    {
        jdis_afficher_graphe(jd);
    }
    else
    {
        jdis_calculer_distances(jd);
    }

    jdis_dispose(&jd);
    return status;
}
