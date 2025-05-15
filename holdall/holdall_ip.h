//  holdall_ip.h : précisions sur l'implantation du module holdall.

//  L'implantation recoure à une liste dynamique simplement chainée des
//    références. Si la macroconstante HOLDALL_PUT_TAIL est définie, la fonction
//    holdall_put insère les références en queue ; dans le cas contraire, elle
//    les insère en tête.

//  Lorsqu'ils ne sont pas constants, les couts sont exprimés en fonction du
//    nombre nombre d'insertions effectuées avec succès dans le fourretout
//    depuis sa création.

//  holdall_empty : temps constant ; espace constant.
//  holdall_dispose : temps linéaire ; espace constant.
//  holdall_put : temps constant ; espace constant.
//  holdall_count : temps constant ; espace constant.
//  holdall_apply, holdall_apply_context, holdall_apply_context2, sans compter
//    ni le temps ni l'espace nécessaire à l'exécution des fonctions fun, fun1
//    ou fun2 passées en paramètre : temps au plus linéaire ; espace constant.

#define HOLDALL_EXT

//  holdall_sort : temps ... ; espace ...
