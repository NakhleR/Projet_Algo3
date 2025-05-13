#ifndef JDIS_H
#define JDIS_H

#include <stddef.h>
#include "../hashtable/hashtable.h"

hashtable *get_words(const char *filename);
float jaccard_distance(hashtable *ht1, hashtable *ht2);
void jdis_free_hashtable_content(hashtable *ht);
void jdis_dispose_hashtable_array(hashtable **htt, size_t count);

void print_usage(void);
void print_help(void);

#endif
