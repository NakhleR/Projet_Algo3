#ifndef JDIS_H
#define JDIS_H

#include <stddef.h>
#include "hashtable.h"
#include "holdall.h"

hashtable *get_unique_words(const char *filename);
//float jaccard_distance(hashtable *ht1, hashtable *ht2);
void hashtable__print_values(hashtable *ht);
#endif
