#include "jdis/jdis.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Affiche l'aide du programme
static void usage(const char *prog_name)
{
    fprintf(stderr, "Usage: %s fichier1 [fichier2 ...]\n", prog_name);
    fprintf(stderr, "Analyse la similarité des fichiers texte en utilisant la distance de Jaccard.\n");
    fprintf(stderr, "Options :\n");
    fprintf(stderr, "  -h, --help    Affiche cette aide et quitte\n");
    fprintf(stderr, "  -g, --graph   Génère un graphe des distances entre les fichiers\n");
}

int main(int argc, char *argv[])
{
    // Vérifier les arguments
    if (argc < 2)
    {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    // Traiter les options et les fichiers
    int generate_graph = 0;
    int i = 1;

    // Vérifier les options
    while (i < argc && (argv[i][0] == '-'))
    {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            usage(argv[0]);
            return EXIT_SUCCESS;
        }
        else if (strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "--graph") == 0)
        {
            generate_graph = 1;
            i++;
        }
        else
        {
            fprintf(stderr, "Option non reconnue : %s\n", argv[i]);
            usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    // Vérifier qu'il y a au moins un fichier
    if (i >= argc)
    {
        fprintf(stderr, "Erreur : Aucun fichier spécifié\n");
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    // Initialiser le jdis
    jdis *jd = jdis_empty();
    if (jd == NULL)
    {
        fprintf(stderr, "Erreur : Impossible d'initialiser jdis\n");
        return EXIT_FAILURE;
    }

    // Traiter chaque fichier
    int error = 0;
    for (; i < argc; i++)
    {
        FILE *file = fopen(argv[i], "r");
        if (file == NULL)
        {
            fprintf(stderr, "Erreur : Impossible d'ouvrir le fichier %s\n", argv[i]);
            error = 1;
            continue;
        }

        printf("Traitement du fichier : %s\n", argv[i]);
        if (jdis_add_file(jd, file, argv[i]) != 0)
        {
            fprintf(stderr, "Erreur lors de l'analyse du fichier %s\n", argv[i]);
            error = 1;
        }

        fclose(file);
    }

    // Si au moins deux fichiers ont été traités avec succès, calculer les distances
    if (error == 0)
    {
        // Calculer et afficher les distances de Jaccard
        if (jdis_jaccard_distance(jd) != 0)
        {
            fprintf(stderr, "Erreur lors du calcul des distances de Jaccard\n");
            error = 1;
        }

        // Générer le graphe si demandé
        if (generate_graph && jdis_run_graph(jd) != 0)
        {
            fprintf(stderr, "Erreur lors de la génération du graphe\n");
            error = 1;
        }
    }

    // Libérer les ressources
    jdis_dispose(&jd);

    return error ? EXIT_FAILURE : EXIT_SUCCESS;
}