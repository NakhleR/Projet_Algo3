#ifndef JDIS_H
#define JDIS_H

#include <stdio.h>

// Structure pour stocker les informations d'un fichier
typedef struct file_info file_info;

// Structure pour stocker une occurrence d'un mot dans un fichier
typedef struct word_occurrence word_occurrence;

// Structure principale du module jdis
typedef struct jdis jdis;

// Crée un nouveau jdis vide
extern jdis *jdis_empty(void);

// Ajoute un fichier au jdis et indexe ses mots
// Renvoie 0 en cas de succès, une valeur non nulle sinon
extern int jdis_add_file(jdis *jd, FILE *file, const char *filename);

// Calcule la distance de Jaccard entre tous les fichiers ajoutés
// Renvoie 0 en cas de succès, une valeur non nulle sinon
extern int jdis_jaccard_distance(const jdis *jd);

// Génère un graphe des distances entre les fichiers
// Renvoie 0 en cas de succès, une valeur non nulle sinon
extern int jdis_run_graph(const jdis *jd);

// Libère les ressources associées au jdis
extern void jdis_dispose(jdis **jdptr);

#endif // JDIS_H
