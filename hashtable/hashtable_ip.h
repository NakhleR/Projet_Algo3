//  hashtable_ip.h : précisions sur l'implantation du module hashtable.

//  Le nombre de compartiments varie au gré des besoins mais est toujours une
//    puissance de 2. La fonction de hachage est la combinaison de la fonction
//    de pré-hachage passée en paramètre et d'un modulo par le nombre de
//    compartiments.

//  Lorsqu'ils ne sont pas constants, les couts sont exprimés en fonction du
//    nombre de couples (clé, valeur) présents dans la table.
//  Les expressions en temps sont faites sous l'hypothèse probabiliste usuelle
//    de l'uniformité de la fonction de hachage.

//  hashtable_empty : temps constant ; espace constant.
//  hashtable_dispose : temps linéaire ; espace constant.
//  hashtable_add : temps amorti constant ; espace constant.
//  hashtable_remove : temps constant ; espace constant.
//  hashtable_search : temps constant ; espace constant.

#define HASHTABLE_EXT

//  hashtable_get_stats : temps linéaire ; espace constant.
//  hashtable_fprint_stats : temps au plus linéaire ; espace constant.
