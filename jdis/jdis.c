#include "jdis.h"
#include "hashtable.h"
#include "holdall.h"

typedef struct jdis jdis;
typedef struct cjdis cjdis;

struct cjdis
{
    const char *filename;
    hashtable *has;
    holdall *hd;
    size_t word_count;
    cjdis *next;
};

struct jdis
{
    cjdis *head;
    cjdis *tail;
    size_t file_count;
};

size_t str_hashfun(const void *s)
{
    size_t h = 0;
    for (const unsigned char *p = (const unsigned char *)s; *p != '\0'; ++p)
    {
        h = 37 * h + *p;
    }
    return h;
}

static int rfree(void *ptr)
{
    free(ptr);
    return 0;
}

jdis *jdis_empty()
{
    jdis *jd = malloc(sizeof *jd);
    if (jd == nullptr)
    {
        return nullptr;
    }
    jd->head = nullptr;
    jd->tail = nullptr;
    jd->file_count = 0;
    return jd;
}

int jdis_add_file(jdis *jd, const char *filename)
{
    if (jd == nullptr || filename == nullptr)
    {

        return 1;
    }

    cjdis *cjd = malloc(sizeof *cjd);
    if (cjd == nullptr)
    {
        return 2;
    }

    cjd->filename = filename;
    cjd->has = hashtable_empty((int (*)(const void *, const void *))strcmp,
                               str_hashfun, 1.0);
    if (cjd->has == nullptr)
    {
        free(cjd);
        return 3;
    }

    cjd->hd = holdall_empty();
    if (cjd->hd == nullptr)
    {
        hashtable_dispose(&cjd->has);
        free(cjd);
        return 4;
    }

    cjd->word_count = 0;
    cjd->next = nullptr;

    if (jd->head == nullptr)
    {
        jd->head = cjd;
        jd->tail = cjd;
    }
    else
    {
        jd->tail->next = cjd;
        jd->tail = cjd;
    }

    return 0;
}

int jdis_jaccard_distance(const jdis *jd)
{
    return 0;
}

int jdis_run_graph(const jdis *jd)
{
    return 0;
}

void jdis_dispose(jdis **jdptr)
{
    if (*jdptr == nullptr)
        return;

    jdis *jd = *jdptr;
    cjdis *cjd = jd->head;
    while (cjd != nullptr)
    {
        cjdis *next = cjd->next;

        if (cjd->hd != nullptr)
            holdall_apply(cjd->hd, rfree);
        holdall_dispose(&cjd->hd);

        if (cjd->has != nullptr)
            hashtable_dispose(&cjd->has);

        free(cjd);
        cjd = next;
    }

    free(jd);
    *jdptr = nullptr;
}

double jdis_distance(const cjdis *cjd1, const cjdis *cjd2)
{
    return 0.0;
}
