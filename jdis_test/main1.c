
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "hashtable.h"
#include "holdall.h"
#include "jdis.h"

#define WORD_LENGTH_MAX 100
#define MUL_RESIZE 2

int rfree(void *ptr);
size_t str_hashfun(const void *s);
void print_header(int size, char *a[]);
void print_help();
hashtable read_from_file(const char *filename);
int compare_strings(const void *a, const void *b);


int main(int argc, char *argv[]) {

dispose:
    if (has != NULL)
    {
        holdall_apply(has, rfree);
    }
    hashtable_dispose(&ht);
    holdall_dispose(&has);
    return r;
}

// Fonction pour comparer deux chaînes (utilisée par la hashtable)
int compare_strings(const void *a, const void *b) {
    return strcmp((const char *)a, (const char *)b);
}

// Lit un fichier et retourne une hashtable des mots uniques




int rfree(void *ptr)
{
    cell *cptr = (cell *)ptr;
    free(cptr->word);
    free(cptr);
    return 0;
}

size_t str_hashfun(const void *s)
{
    size_t h = 0;
    for (const unsigned char *p = (const unsigned char *)s; *p != '\0'; ++p)
    {
        h = 37 * h + *p;
    }
    return h;
}

void print_header(int size, char *a[])
{
    for (int i = 1; i < size; i++)
    {
        printf("\t%s", a[i]);
    }
    printf("\n");
}

void print_help()
{
    printf("Usage: ./xwc [OPTION] <File1> <File2> ... <FileN>\n");
    printf(
        "\nExclusive Word Counting: Print the number of occurrences of each word that appears in one and only one of the given text files.\n");
    printf(
        "\nA word is, by default, a maximum length sequence of characters that do not belong to the white-space characters set.\n");
    printf(
        "\nResults are displayed in columns on the standard output. Columns are separated by the tab character. Lines are terminated by the end-of-line character. A header line shows the file names: the name of the first file appears in the second column, that of the second in the third, and so on. For the following lines, a word appears in the first column, its number of occurrences in the file in which it appears to the exclusion of all others in the column associated with the file. No tab characters are written on a line after the number of occurrences.\n");
    printf(
        "\nRead the standard input when no file is given or for any file which is \"-\". In such cases, \"\" is displayed in the column associated with the file on the header line.\n");
    printf(
        "\nThe locale specified by the environment affects sort order. Set 'LC_ALL=C' to get the traditional sort order that uses native byte values.\n");
}
