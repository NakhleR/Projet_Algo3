//  holdall.c : partie implantation du module holdall.

#include "holdall.h"

typedef struct choldall choldall;

struct choldall {
  void *ref;
  choldall *next;
};

struct holdall {
  choldall *head;
#if defined HOLDALL_PUT_TAIL
  choldall **tailptr;
#endif
  size_t count;
};

holdall *holdall_empty() {
  holdall *ha = malloc(sizeof *ha);
  if (ha == NULL) {
    return NULL;
  }
  ha->head = NULL;
#if defined HOLDALL_PUT_TAIL
  ha->tailptr = &ha->head;
#endif
  ha->count = 0;
  return ha;
}

void holdall_dispose(holdall **haptr) {
  if (*haptr == NULL) {
    return;
  }
  choldall *p = (*haptr)->head;
  while (p != NULL) {
    choldall *t = p;
    p = p->next;
    free(t);
  }
  free(*haptr);
  *haptr = NULL;
}

int holdall_put(holdall *ha, void *ref) {
  choldall *p = malloc(sizeof *p);
  if (p == NULL) {
    return -1;
  }
  p->ref = ref;
#if defined HOLDALL_PUT_TAIL
  p->next = NULL;
  *ha->tailptr = p;
  ha->tailptr = &p->next;
#else
  p->next = ha->head;
  ha->head = p;
#endif
  ha->count += 1;
  return 0;
}

size_t holdall_count(holdall *ha) {
  return ha->count;
}

int holdall_apply(holdall *ha,
    int (*fun) (void *)) {
  for (const choldall *p = ha->head; p != NULL; p = p->next) {
    int r = fun(p->ref);
    if (r != 0) {
      return r;
    }
  }
  return 0;
}

int holdall_apply_context(holdall *ha,
    void *context, void *(*fun1) (void *context, void *ptr),
    int (*fun2) (void *ptr, void *resultfun1)) {
  for (const choldall *p = ha->head; p != NULL; p = p->next) {
    int r = fun2(p->ref, fun1(context, p->ref));
    if (r != 0) {
      return r;
    }
  }
  return 0;
}

int holdall_apply_context2(holdall *ha,
    void *context1, void *(*fun1) (void *context1, void *ptr),
    void *context2, int (*fun2) (void *context2, void *ptr, void *resultfun1)) {
  for (const choldall *p = ha->head; p != NULL; p = p->next) {
    int r = fun2(context2, p->ref, fun1(context1, p->ref));
    if (r != 0) {
      return r;
    }
  }
  return 0;
}

#if defined HOLDALL_EXT && defined WANT_HOLDALL_EXT

void **holdall_to_array(holdall *ha) {
  if (ha == NULL || ha->count == 0) {
    return NULL;
  }
  void **array = (void **) malloc(ha->count * sizeof(void *));
  if (array == NULL) {
    return NULL;
  }
  choldall *p = ha->head;
  size_t i = 0;
  while (p != NULL && i < ha->count) {
    array[i] = p->ref;
    p = p->next;
    i++;
  }
  return array;
}

void holdall_sort(holdall *ha, int (*cmp) (const void *, const void *)) {
  void **array = holdall_to_array(ha);
  qsort(array, ha->count, sizeof(void *), cmp);
  choldall *p = ha->head;
  for (size_t i = 0; i < ha->count; i++) {
    p->ref = array[i];
    p = p->next;
  }
  free(array);
}

#endif
