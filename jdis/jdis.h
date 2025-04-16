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