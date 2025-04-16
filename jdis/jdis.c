// jdis.h
#ifndef JDIS__H
#define JDIS__H

#include <stddef.h>
#include <stdio.h>

typedef struct jdis jdis;
typedef struct cjdis cjdis;

// Structure d'un fichier (cjdis)
struct cjdis
{
    const char *filename; // Nom du fichier
    size_t word_count;    // Nombre de mots uniques
    size_t id;            // Numéro d'ordre dans la liste
    cjdis *next;          // Fichier suivant (liste chaînée)
};

/**
 * Crée une structure jdis vide (table de mots → fichiers).
 */
jdis *jdis_empty(void);

/**
 * Traite un fichier texte : lit les mots et les insère dans la table globale.
 *
 * @param jd Structure jdis principale
 * @param filename Nom du fichier à traiter
 * @param file Fichier déjà ouvert en lecture
 * @param word_len_max Longueur max d'un mot (0 = illimitée)
 * @param treat_punct Si différent de 0, ponctuation traitée comme espace
 * @return 0 si OK, non-zéro sinon
 */
int jdis_process_file(jdis *jd, const char *filename, FILE *file, size_t word_len_max, int treat_punct);

/**
 * Calcule et affiche les distances de Jaccard.
 */
int jdis_calculer_distances(const jdis *jd);

/**
 * Affiche le graphe d'appartenance mots → fichiers.
 */
int jdis_afficher_graphe(const jdis *jd);

/**
 * Libère toute la mémoire de la structure jdis.
 */
void jdis_dispose(jdis **jdptr);

#endif

// jdis.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "jdis.h"
#include "hashtable.h"
#include "holdall.h"

#define STR(s) #s
#define XSTR(s) STR(s)

typedef struct fichier_cell
{
    cjdis *file;
    struct fichier_cell *next;
} fichier_cell;

struct jdis
{
    hashtable *ht;     // mot → fichier_cell *
    holdall *words;    // mots (pointeurs à libérer)
    holdall *files;    // fichiers (cjdis *)
    size_t file_count; // compteur pour id
};

static size_t str_hashfun(const void *ref)
{
    const unsigned char *s = ref;
    size_t h = 0;
    while (*s)
        h = 37 * h + *s++;
    return h;
}

static int rfree(void *ptr)
{
    free(ptr);
    return 0;
}

jdis *jdis_empty(void)
{
    jdis *jd = malloc(sizeof *jd);
    if (!jd)
        return nullptr;
    jd->ht = hashtable_empty((int (*)(const void *, const void *))strcmp, str_hashfun, 1.0);
    jd->words = holdall_empty();
    jd->files = holdall_empty();
    jd->file_count = 0;
    if (!jd->ht || !jd->words || !jd->files)
    {
        jdis_dispose(&jd);
        return nullptr;
    }
    return jd;
}

int jdis_process_file(jdis *jd, const char *filename, FILE *file, size_t word_len_max, int treat_punct)
{
    if (!jd || !filename || !file)
        return 1;

    cjdis *f = malloc(sizeof *f);
    if (!f)
        return 2;
    f->filename = filename;
    f->word_count = 0;
    f->id = jd->file_count++;
    f->next = nullptr;
    if (holdall_put(jd->files, f) != 0)
        return 3;

    char buffer[1024];
    size_t len = 0;
    int c;
    while ((c = fgetc(file)) != EOF)
    {
        if (isspace(c) || (treat_punct && ispunct(c)))
        {
            if (len > 0)
            {
                buffer[len] = '\0';
                if (word_len_max == 0 || len <= word_len_max)
                {
                    char *s = buffer;
                    void *found = hashtable_search(jd->ht, s);
                    fichier_cell *list = found;

                    if (!found)
                    {
                        s = strdup(buffer);
                        if (!s)
                            return 4;
                        list = nullptr;
                        hashtable_add(jd->ht, s, list);
                        holdall_put(jd->words, s);
                    }

                    // Rechercher si ce fichier est déjà lié au mot
                    fichier_cell *fc = list;
                    int present = 0;
                    while (fc)
                    {
                        if (fc->file == f)
                        {
                            present = 1;
                            break;
                        }
                        fc = fc->next;
                    }

                    if (!present)
                    {
                        fichier_cell *new_cell = malloc(sizeof *new_cell);
                        if (!new_cell)
                            return 5;
                        new_cell->file = f;
                        new_cell->next = list;
                        hashtable_add(jd->ht, s, new_cell);
                        f->word_count++;
                    }
                }
                len = 0;
            }
        }
        else if (len < sizeof(buffer) - 1)
        {
            buffer[len++] = (char)c;
        }
    }

    return 0;
}

int jdis_afficher_graphe(const jdis *jd)
{
    if (!jd)
        return 1;

    size_t count = holdall_count(jd->files);
    printf("Mots ");
    for (size_t i = 0; i < count; ++i)
        printf("\tF%zu", i);
    printf("\n");

    holdall_apply_context(jd->words, (void *)jd, (void *(*)(void *, void *))hashtable_search, (int (*)(void *, void *))[](void *mot, void *list_ptr) {
            fichier_cell *list = list_ptr;
            printf("%s", (char *)mot);
            for (size_t i = 0; i < holdall_count(((jdis *)jd)->files); ++i) {
                int found = 0;
                for (fichier_cell *fc = list; fc; fc = fc->next)
                    if (fc->file->id == i)
                        found = 1;
                printf("\t%s", found ? "X" : "");
            }
            printf("\n");
            return 0; });

    return 0;
}

int jdis_calculer_distances(const jdis *jd)
{
    if (!jd)
        return 1;
    size_t n = holdall_count(jd->files);
    cjdis **arr = malloc(n * sizeof *arr);
    size_t i = 0;
    holdall_apply_context(jd->files, &i, (void *(*)(void *, void *))[](void *index, void *elt) {
            ((cjdis **)arr)[*(size_t *)index] = elt;
            ++*(size_t *)index;
            return nullptr; }, (int (*)(void *, void *))[](void *f, void *_) { return 0; });

    for (size_t i = 0; i < n; ++i)
    {
        for (size_t j = i + 1; j < n; ++j)
        {
            size_t inter = 0, uni = arr[i]->word_count + arr[j]->word_count;
            holdall_apply_context(jd->words, (void *)jd, (void *(*)(void *, void *))hashtable_search, (int (*)(void *, void *))[](void *mot, void *list_ptr) {
                    fichier_cell *list = list_ptr;
                    int has_i = 0, has_j = 0;
                    for (; list; list = list->next) {
                        if (list->file->id == i) has_i = 1;
                        else if (list->file->id == j) has_j = 1;
                    }
                    if (has_i && has_j) ++inter;
                    else if (has_i || has_j) ++uni;
                    return 0; });
            double dist = 1.0 - ((double)inter / uni);
            printf("%.4f\t%s\t%s\n", dist, arr[i]->filename, arr[j]->filename);
        }
    }

    free(arr);
    return 0;
}

void jdis_dispose(jdis **jdptr)
{
    if (!jdptr || !*jdptr)
        return;
    jdis *jd = *jdptr;
    if (jd->words)
        holdall_apply(jd->words, rfree);
    holdall_dispose(&jd->words);
    holdall_dispose(&jd->files);
    hashtable_dispose(&jd->ht);
    free(jd);
    *jdptr = nullptr;
}