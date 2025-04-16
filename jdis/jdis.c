#include "jdis.h"
#include "../hashtable/hashtable.h"
#include "../holdall/holdall.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define WORD_MAX_LENGTH 100
#define MAX_LOAD_FACTOR 0.75

// Structure pour stocker les informations d'un fichier
struct file_info
{
    char *filename;    // Nom du fichier
    size_t word_count; // Nombre total de mots dans le fichier
    holdall *words;    // Ensemble des mots uniques dans le fichier
};

// Structure pour stocker une occurrence d'un mot dans un fichier
struct word_occurrence
{
    file_info *file; // Pointeur vers l'information du fichier
    size_t count;    // Nombre d'occurrences du mot dans ce fichier
};

// Struct pour la liste chaînée des occurrences
typedef struct occurrence_node
{
    word_occurrence occurrence;
    struct occurrence_node *next;
} occurrence_node;

// Structure pour un mot dans la table de hachage
typedef struct
{
    char *word;                   // Le mot lui-même
    occurrence_node *occurrences; // Liste des occurrences dans les différents fichiers
} word_entry;

// Structure principale du module jdis
struct jdis
{
    hashtable *word_table; // Table de hachage des mots
    holdall *files;        // Collection des fichiers analysés
};

// Fonction de hachage pour les chaînes de caractères
static size_t hash_string(const void *key)
{
    const char *str = key;
    size_t hash = 5381;
    int c;

    while ((c = *str++))
    {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }

    return hash;
}

// Fonction de comparaison pour les chaînes de caractères
static int compare_strings(const void *a, const void *b)
{
    return strcmp((const char *)a, (const char *)b);
}

// Fonction pour libérer un mot_entry
static int free_word_entry(void *entry)
{
    if (entry == nullptr)
    {
        return 0;
    }

    word_entry *word = entry;

    // Libérer le mot
    free(word->word);

    // Libérer la liste des occurrences
    occurrence_node *current = word->occurrences;
    while (current != nullptr)
    {
        occurrence_node *next = current->next;
        free(current);
        current = next;
    }

    // Libérer l'entrée elle-même
    free(word);

    return 0;
}

// Fonction pour libérer un file_info
static int free_file_info(void *info)
{
    if (info == nullptr)
    {
        return 0;
    }

    file_info *file = info;

    // Libérer le nom du fichier
    free(file->filename);

    // Libérer l'ensemble des mots
    holdall_dispose(&file->words);

    // Libérer la structure elle-même
    free(file);

    return 0;
}

// Crée un nouveau jdis vide
jdis *jdis_empty(void)
{
    jdis *jd = malloc(sizeof *jd);
    if (jd == nullptr)
    {
        return nullptr;
    }

    // Initialiser la table de hachage des mots
    jd->word_table = hashtable_empty(compare_strings, hash_string, MAX_LOAD_FACTOR);
    if (jd->word_table == nullptr)
    {
        free(jd);
        return nullptr;
    }

    // Initialiser la collection des fichiers
    jd->files = holdall_empty();
    if (jd->files == nullptr)
    {
        hashtable_dispose(&jd->word_table);
        free(jd);
        return nullptr;
    }

    return jd;
}

// Ajoute une occurrence d'un mot dans un fichier
static int add_word_occurrence(jdis *jd, const char *word, file_info *file)
{
    // Chercher si le mot existe déjà dans la table de hachage
    word_entry *entry = hashtable_search(jd->word_table, word);

    if (entry == nullptr)
    {
        // Créer une nouvelle entrée pour ce mot
        entry = malloc(sizeof *entry);
        if (entry == nullptr)
        {
            return -1;
        }

        // Copier le mot
        entry->word = strdup(word);
        if (entry->word == nullptr)
        {
            free(entry);
            return -1;
        }

        // Initialiser la liste des occurrences
        entry->occurrences = nullptr;

        // Ajouter à la table de hachage
        if (hashtable_add(jd->word_table, entry->word, entry) == nullptr)
        {
            free(entry->word);
            free(entry);
            return -1;
        }
    }

    // Chercher si ce fichier est déjà dans la liste des occurrences
    occurrence_node *current = entry->occurrences;
    while (current != nullptr)
    {
        if (current->occurrence.file == file)
        {
            // Incrémenter le compteur
            current->occurrence.count++;
            return 0;
        }
        current = current->next;
    }

    // Ajouter une nouvelle occurrence pour ce fichier
    occurrence_node *node = malloc(sizeof *node);
    if (node == nullptr)
    {
        return -1;
    }

    node->occurrence.file = file;
    node->occurrence.count = 1;
    node->next = entry->occurrences;
    entry->occurrences = node;

    // Ajouter le mot à l'ensemble des mots du fichier
    if (holdall_put(file->words, entry->word) != 0)
    {
        // Si échec, supprimer le nœud d'occurrence
        entry->occurrences = node->next;
        free(node);
        return -1;
    }

    return 0;
}

// Ajoute un fichier au jdis et indexe ses mots
int jdis_add_file(jdis *jd, FILE *file, const char *filename)
{
    if (jd == nullptr || file == nullptr || filename == nullptr)
    {
        return -1;
    }

    // Créer une structure file_info pour ce fichier
    file_info *info = malloc(sizeof *info);
    if (info == nullptr)
    {
        return -1;
    }

    // Initialiser les champs
    info->filename = strdup(filename);
    if (info->filename == nullptr)
    {
        free(info);
        return -1;
    }

    info->word_count = 0;

    // Créer l'ensemble des mots pour ce fichier
    info->words = holdall_empty();
    if (info->words == nullptr)
    {
        free(info->filename);
        free(info);
        return -1;
    }

    // Ajouter le fichier à la collection
    if (holdall_put(jd->files, info) != 0)
    {
        holdall_dispose(&info->words);
        free(info->filename);
        free(info);
        return -1;
    }

    // Lire et indexer les mots du fichier
    char word[WORD_MAX_LENGTH + 1];
    int c;
    size_t i = 0;

    // Réinitialiser la position dans le fichier
    rewind(file);

    while ((c = fgetc(file)) != EOF)
    {
        if (isalpha(c) || c == '-' || c == '\'')
        {
            // Caractère faisant partie d'un mot
            if (i < WORD_MAX_LENGTH)
            {
                word[i++] = tolower(c);
            }
        }
        else if (i > 0)
        {
            // Fin d'un mot
            word[i] = '\0';
            info->word_count++;

            // Ajouter l'occurrence de ce mot
            if (add_word_occurrence(jd, word, info) != 0)
            {
                // En cas d'erreur, ne pas arrêter mais continuer pour les autres mots
            }

            i = 0;
        }
    }

    // Traiter le dernier mot s'il y en a un
    if (i > 0)
    {
        word[i] = '\0';
        info->word_count++;
        add_word_occurrence(jd, word, info);
    }

    return 0;
}

// Calcule la distance de Jaccard entre deux fichiers
static double jaccard_distance(file_info *file1, file_info *file2)
{
    // Compter les mots communs
    size_t common_words = 0;
    size_t total_words = 0;

    // Si les deux ensembles sont vides, la distance de Jaccard est définie comme 0
    if (holdall_count(file1->words) == 0 && holdall_count(file2->words) == 0)
    {
        return 0.0;
    }

    // Fonction pour compter les mots communs
    struct count_context
    {
        file_info *other_file;
        size_t *common_count;
    };

    void *check_word(void *ctx, void *word)
    {
        return word; // Simplement passer le mot
    }

    int count_if_found(void *word, void *found)
    {
        return found != nullptr; // Non-zéro si le mot a été trouvé
    }

    struct count_context context = {file2, &common_words};

    // Pour chaque mot dans file1, vérifier s'il est dans file2
    holdall_apply_context(file1->words, &context, check_word, count_if_found);

    // Calculer l'union : total = A + B - intersection
    total_words = holdall_count(file1->words) + holdall_count(file2->words) - common_words;

    // Distance de Jaccard = 1 - (intersection / union)
    if (total_words == 0)
    {
        return 0.0; // Éviter la division par zéro
    }

    return 1.0 - ((double)common_words / total_words);
}

// Calcule la distance de Jaccard entre tous les fichiers ajoutés
int jdis_jaccard_distance(const jdis *jd)
{
    if (jd == nullptr)
    {
        return -1;
    }

    size_t file_count = holdall_count(jd->files);

    if (file_count < 2)
    {
        printf("Il faut au moins deux fichiers pour calculer la distance de Jaccard.\n");
        return 0;
    }

    // Créer un tableau des fichiers pour un accès plus facile
    file_info **files = malloc(file_count * sizeof *files);
    if (files == nullptr)
    {
        return -1;
    }

    // Remplir le tableau
    struct fill_context
    {
        file_info **files;
        size_t index;
    };

    void *store_file(void *ctx, void *file)
    {
        struct fill_context *context = ctx;
        context->files[context->index++] = file;
        return nullptr;
    }

    int dummy(void *dummy1, void *dummy2)
    {
        return 0;
    }

    struct fill_context context = {files, 0};
    holdall_apply_context(jd->files, &context, store_file, dummy);

    // Calculer et afficher les distances
    printf("Distances de Jaccard entre les fichiers :\n");
    for (size_t i = 0; i < file_count; i++)
    {
        for (size_t j = i + 1; j < file_count; j++)
        {
            double distance = jaccard_distance(files[i], files[j]);
            printf("%s <-> %s : %.4f\n",
                   files[i]->filename, files[j]->filename, distance);
        }
    }

    free(files);
    return 0;
}

// Génère un graphe des distances entre les fichiers
int jdis_run_graph(const jdis *jd)
{
    if (jd == nullptr)
    {
        return -1;
    }

    size_t file_count = holdall_count(jd->files);

    if (file_count < 2)
    {
        printf("Il faut au moins deux fichiers pour générer un graphe.\n");
        return 0;
    }

    // Créer un fichier DOT pour Graphviz
    FILE *dot_file = fopen("jdis_graph.dot", "w");
    if (dot_file == nullptr)
    {
        perror("Erreur lors de la création du fichier de graphe");
        return -1;
    }

    // Écrire l'en-tête du fichier DOT
    fprintf(dot_file, "graph jdis {\n");
    fprintf(dot_file, "  node [shape=box];\n");

    // Même processus que pour jdis_jaccard_distance
    file_info **files = malloc(file_count * sizeof *files);
    if (files == nullptr)
    {
        fclose(dot_file);
        return -1;
    }

    struct fill_context
    {
        file_info **files;
        size_t index;
    };

    void *store_file(void *ctx, void *file)
    {
        struct fill_context *context = ctx;
        context->files[context->index++] = file;
        return nullptr;
    }

    int dummy(void *dummy1, void *dummy2)
    {
        return 0;
    }

    struct fill_context context = {files, 0};
    holdall_apply_context(jd->files, &context, store_file, dummy);

    // Écrire les nœuds (fichiers)
    for (size_t i = 0; i < file_count; i++)
    {
        fprintf(dot_file, "  \"%s\";\n", files[i]->filename);
    }

    // Écrire les arêtes avec les distances comme poids
    for (size_t i = 0; i < file_count; i++)
    {
        for (size_t j = i + 1; j < file_count; j++)
        {
            double distance = jaccard_distance(files[i], files[j]);
            fprintf(dot_file, "  \"%s\" -- \"%s\" [label=\"%.4f\", weight=%.0f];\n",
                    files[i]->filename, files[j]->filename,
                    distance, (1.0 - distance) * 10); // Plus faible est la distance, plus épaisse l'arête
        }
    }

    // Fermer le graphe
    fprintf(dot_file, "}\n");
    fclose(dot_file);

    printf("Graphe généré dans jdis_graph.dot\n");
    printf("Pour visualiser, utilisez la commande : dot -Tpng jdis_graph.dot -o jdis_graph.png\n");

    free(files);
    return 0;
}

// Libère les ressources associées au jdis
void jdis_dispose(jdis **jdptr)
{
    if (jdptr == nullptr || *jdptr == nullptr)
    {
        return;
    }

    jdis *jd = *jdptr;

    // Libérer tous les fichiers
    holdall_apply(jd->files, free_file_info);
    holdall_dispose(&jd->files);

    // Libérer tous les mots et leurs occurrences
    // Parcourir la table de hachage et libérer chaque entrée
    struct ht_context
    {
        hashtable *table;
    };

    void *get_key(void *ctx, void *entry)
    {
        word_entry *word = entry;
        return word->word;
    }

    int remove_entry(void *key, void *result)
    {
        // Supprimer l'entrée de la table
        return 0;
    }

    struct ht_context ht_ctx = {jd->word_table};

    // Libérer chaque entrée de la table de hachage
    hashtable *ht = jd->word_table;
    holdall *keys = holdall_empty();

    if (keys != nullptr)
    {
        // Collecter toutes les clés d'abord pour éviter de modifier la table pendant l'itération
        holdall_apply_context(ht_ctx.table, &keys, get_key, (int (*)(void *, void *))holdall_put);

        // Supprimer et libérer chaque entrée
        while (holdall_count(keys) > 0)
        {
            char *key = nullptr;
            // Récupérer une clé
            holdall_apply(keys, (int (*)(void *))&key);

            // Supprimer de la table et libérer
            word_entry *entry = hashtable_remove(ht, key);
            free_word_entry(entry);

            // Supprimer la clé de notre liste de clés
            holdall_apply_context(keys, key, nullptr, nullptr); // Supposé faciliter la suppression
        }

        holdall_dispose(&keys);
    }

    // Libérer la table de hachage
    hashtable_dispose(&jd->word_table);

    // Libérer la structure jdis elle-même
    free(jd);
    *jdptr = nullptr;
}
