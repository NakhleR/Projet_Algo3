#ifndef JDIS_H
#define JDIS_H

typedef struct jdis jdis;
typedef struct hashtable hashtable;
typedef struct holdall holdall;

extern jdis *jdis_empty();

extern int jdis_add_file(jdis *jd, const char *filename);

extern int jdis_jaccard_distance(const jdis *jd);

extern int jdis_run_graph(const jdis *jd);

extern void jdis_dispose(jdis **jdptr);

#endif
