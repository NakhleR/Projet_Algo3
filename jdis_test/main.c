#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "hashtable.h"
#include "holdall.h"
#include "jdis.h"

#define WORD_LENGTH_MAX 100
#define MUL_RESIZE 2

int rfree(void *ptr);
size_t str_hashfun(const void *s);
void print_header(int size, char *a[]);
void print_help();

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <file1> <file2> ... <fileN>\n", argv[0]);
        return EXIT_FAILURE;
    }
    int r = EXIT_SUCCESS;
    hashtable *ht = hashtable_empty(str_compar, str_hashfun);
    if (ht == NULL)
    {
        fprintf(stderr, "Failed to initialize hashtable\n");
        return EXIT_FAILURE;
    }
    holdall *has = holdall_empty();
    if (has == NULL)
    {
        fprintf(stderr, ("Failed to initialize holdall\n"));
        hashtable_dispose(&ht);
        return EXIT_FAILURE;
    }
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-?") == 0)
            {
                print_help();
                goto dispose;
            }
            else
            {
                fprintf(stderr, "Unknown option: %s\n", argv[i]);
                r = EXIT_FAILURE;
                goto dispose;
            }
        }
        else
        {
            FILE *f = fopen(argv[i], "r");
            if (f == NULL)
            {
                fprintf(stderr, "Error while opening file %s!\n", argv[i]);
                continue;
            }
            char *w = malloc(sizeof(char *) * WORD_LENGTH_MAX);
            if (w == NULL)
            {
                fprintf(stderr, "Memory allocation error\n");
                r = EXIT_FAILURE;
                goto dispose;
            }
            size_t index = 0;
            size_t size = WORD_LENGTH_MAX;
            int c;
            while ((c = fgetc(f)) != EOF)
            {
                if (c == ' ' || c == '\n' || c == '\t')
                {
                    w[index] = '\0';
                    index = 0;
                    struct cell *cptr = hashtable_search(ht, w);
                    if (cptr != NULL)
                    {
                        if (cptr->location != i)
                        {
                            cptr->screen = false;
                        }
                        cptr->count += 1;
                    }
                    else
                    {
                        cptr = malloc(sizeof(cell));
                        if (cptr == NULL)
                        {
                            fprintf(stderr, "Memory allocation error\n");
                            fclose(f);
                            r = EXIT_FAILURE;
                            goto dispose;
                        }
                        cptr->word = strdup(w);
                        cptr->location = i;
                        cptr->count = 1;
                        cptr->screen = true;
                        hashtable_add(ht, cptr->word, cptr);
                        if (holdall_put(has, cptr) != 0)
                        {
                            free(cptr->word);
                            free(cptr);
                            fclose(f);
                            r = EXIT_FAILURE;
                            goto dispose;
                        }
                    }
                }
                else
                {
                    if (index > size)
                    {
                        if (index > SIZE_MAX / MUL_RESIZE)
                        {
                            fprintf(stderr, "Overflow\n");
                            fclose(f);
                            r = EXIT_FAILURE;
                            goto dispose;
                        }
                        size *= MUL_RESIZE;
                        char *w2 = realloc(w, size * sizeof(*w));
                        if (w2 == NULL)
                        {
                            fprintf(stderr, "Memory allocation error\n");
                            fclose(f);
                            r = EXIT_FAILURE;
                            goto dispose;
                        }
                        w = w2;
                    }
                    w[index] = (char)c;
                    index += 1;
                }
            }
            if (feof(f) == 0)
            {
                fprintf(stderr, "Something went wrong in file %s", argv[i]);
                free(w);
                r = EXIT_FAILURE;
                goto dispose;
            }
            if (c == EOF)
            {
                w[index] = '\0';
                struct cell *cptr = hashtable_search(ht, w);
                if (cptr != NULL)
                {
                    if (cptr->location != i)
                    {
                        cptr->screen = false;
                    }
                    cptr->count += 1;
                }
                else
                {
                    cptr = malloc(sizeof(cell));
                    if (cptr == NULL)
                    {
                        fprintf(stderr, "Memory allocation error\n");
                        fclose(f);
                        r = EXIT_FAILURE;
                        goto dispose;
                    }
                    cptr->word = strdup(w);
                    cptr->location = i;
                    cptr->count = 1;
                    cptr->screen = true;
                    hashtable_add(ht, cptr->word, cptr);
                    if (holdall_put(has, cptr) != 0)
                    {
                        free(cptr->word);
                        free(cptr);
                        fclose(f);
                        r = EXIT_FAILURE;
                        goto dispose;
                    }
                }
            }
            free(w);
            fclose(f);
        }
    }
    print_header(argc, argv);
    holdall_apply(has, print_cell);
dispose:
    if (has != NULL)
    {
        holdall_apply(has, rfree);
    }
    hashtable_dispose(&ht);
    holdall_dispose(&has);
    return r;
}

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