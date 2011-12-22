#include <stdlib.h>

#include "rd_list.h"

void
rd_list_remove(struct rd_list *list)
{
    list->next->prev = list->prev;
    list->prev->next = list->next;
    free(list);
}

void
rd_list_remove_index(struct rd_list *list, int idx)
{
    struct rd_list *cur = rd_list_index(list, idx);
    if (cur) {
        rd_list_remove(list);
    }
}

struct rd_list *
rd_list_index(struct rd_list *list, int idx)
{
    struct rd_list *cur;
    int ii = 1;
    if (list) {
        if (idx == 0) {
            return list;
        }
        for (cur = list->next; cur != list; cur = cur->next, ++ii) {
            if (ii == idx) {
                return cur;
            }
        }
    }
    return NULL;
}

void *
rd_list_index_data(struct rd_list *list, int idx)
{
    struct rd_list *cur = rd_list_index(list, idx);
    return cur ? cur->data : NULL;
}

int
rd_list_length(struct rd_list *list)
{
    struct rd_list *cur;
    int len = 0;
    if (list) {
        for (cur = list->next; cur != list; cur = cur->next) {
            len++;
        }
        len++;
    }
    return len;
}

struct rd_list *
rd_list_append(struct rd_list *list, void *data)
{
    struct rd_list *new_list = malloc(sizeof(*new_list));
    new_list->next = new_list;
    new_list->prev = new_list;
    new_list->data = data;
    if (list) {
        struct rd_list *last = list->prev;
        last->next = new_list;
        new_list->prev = last;
        new_list->next = list;
        list->prev = new_list;
        return list;
    }
    return new_list;
}

void
rd_list_free(struct rd_list *list)
{
    struct rd_list *cur;
    struct rd_list *next;
    if (list) {
        for (cur = list->next; cur != list; cur = next) {
            next = cur->next;
            free(cur);
        }
        free(list);
    }
}
