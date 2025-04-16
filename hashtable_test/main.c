#include <stdio.h>
#include <stdlib.h>
#include "hashtable.h"
#include "holdall.h"

int main(void)
{
    printf("Testing hashtable implementation...\n");

    // Create a hashtable
    hashtable *ht = hashtable_empty(NULL, NULL, NULL);
    if (ht == NULL)
    {
        fprintf(stderr, "Error: Could not create hashtable\n");
        return EXIT_FAILURE;
    }

    printf("Successfully created an empty hashtable\n");

    // Clean up
    hashtable_dispose(&ht);

    return EXIT_SUCCESS;
}