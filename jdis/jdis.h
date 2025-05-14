#ifndef JDIS_H
#define JDIS_H

#include "hashtable.h"
#include "holdall.h"

holdall *get_words(const char *filename, int initial_letters_limit);
float jaccard_distance(holdall *ha1, holdall *ha2);
void jdis_free_holdall_content(holdall *ha);
void jdis_dispose_holdall_array(holdall **ha_array, size_t count);

void handle_graph_output(holdall **file_holdalls, size_t num_files,
    char **filenames_in_order, int initial_letters_limit);

void print_usage(void);
void print_help(void);

#endif
