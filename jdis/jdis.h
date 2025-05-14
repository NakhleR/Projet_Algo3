#ifndef JDIS_H
#define JDIS_H

#include <stddef.h>
#include "../hashtable/hashtable.h"

hashtable *get_words(const char *filename, int initial_letters_limit);
float jaccard_distance(hashtable *ht1, hashtable *ht2);
void jdis_free_hashtable_content(hashtable *ht);
void jdis_dispose_hashtable_array(hashtable **htt, size_t count);

void handle_graph_output(hashtable **file_hashtables, size_t num_files,
    char **filenames_in_order, int initial_letters_limit);

void print_usage(void);
void print_help(void);

#endif
