//  jdis.h : partie interface d'un module pour calculer la distance de
// Jaccard
//    entre des ensembles de mots provenant de fichiers, et pour générer une
// sortie
//    graphique de présence des mots.
//  Fonctionnement général :
//  - Le module lit des fichiers texte, extrait des mots, et peut calculer
//    les distances de Jaccard entre paires de fichiers ou afficher une matrice
//    de présence des mots.
//  - Les mots peuvent être tronqués à une certaine longueur initiale.
//  - La ponctuation peut être traitée comme un séparateur de mots.

#ifndef JDIS__H
#define JDIS__H

#include "hashtable.h"
#include "holdall.h"
#include <stdbool.h>

//  get_words : lit un fichier et en extrait les mots uniques.
//    Les mots sont stockés dans un holdall. La fonction gère la lecture
//    depuis stdin si filename est "-".
//    Paramètres :
//      filename : le nom du fichier à lire ("-" pour stdin).
//      initial_letters_limit : nombre de lettres initiales à considérer
//                               pour chaque mot (0 = pas de limite).
//      punctuation_as_space : si true, traite la ponctuation comme des
//                               espaces séparateurs.
//    Renvoie : un pointeur vers un holdall contenant les mots uniques (chaînes
//              allouées dynamiquement), ou nullptr en cas d'erreur.
extern holdall *get_words(const char *filename, int initial_letters_limit,
    bool punctuation_as_space);

//  jaccard_distance : calcule la dissimilarité de Jaccard entre deux ensembles
//    de mots.
//    Paramètres :
//      ha1 : holdall contenant les mots du premier ensemble.
//      ha2 : holdall contenant les mots du second ensemble.
//    Renvoie : la dissimilarité de Jaccard (entre 0.0 et 1.0). Renvoie 1.0f si
//              l'un des holdalls est nullptr ou en cas d'erreur d'allocation
// mémoire
//              interne. Renvoie 0.0f si les deux ensembles sont vides.
extern float jaccard_distance(holdall *ha1, holdall *ha2);

//  jdis_free_holdall_content : libère la mémoire des chaînes de caractères
//    (mots) stockées dans un holdall.
//    Suppose que les éléments du holdall sont des char* alloués avec
// strdup/malloc.
//    Paramètres :
//      ha : le holdall dont le contenu doit être libéré.
extern void jdis_free_holdall_content(holdall *ha);

//  jdis_dispose_holdall_array : libère un tableau de holdalls, y compris
//    le contenu de chaque holdall et le tableau lui-même.
//    Paramètres :
//      ha_array : le tableau de pointeurs vers des holdalls.
//      count : le nombre d'éléments dans ha_array.
extern void jdis_dispose_holdall_array(holdall **ha_array, size_t count);

//  handle_graph_output : génère et affiche la sortie graphique indiquant la
//    présence ou l'absence de chaque mot unique dans les fichiers fournis.
//    Paramètres :
//      file_holdalls : tableau de holdalls, chacun contenant les mots d'un
// fichier.
//      num_files : nombre de fichiers (et donc de holdalls).
//      filenames_in_order : tableau des noms de fichiers, dans l'ordre.
//      initial_letters_limit : limite sur le nombre de lettres initiales des
// mots (non utilisé directement ici, mais contextuel).
extern void handle_graph_output(holdall **file_holdalls, size_t num_files,
    char **filenames_in_order, int initial_letters_limit);

//  print_usage : affiche un message bref sur l'utilisation du programme.
extern void print_usage(void);

//  print_help : affiche un message d'aide détaillé pour le programme.
extern void print_help(void);

#endif // JDIS__H
