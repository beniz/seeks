#ifndef RDLIST_H
#define RDLIST_H

#ifdef __cplusplus
extern "C" {
#endif

  struct rd_list
  {
    struct rd_list *next;
    struct rd_list *prev;
    void *data;
  };

  void rd_list_remove(struct rd_list *list);
  void rd_list_remove_index(struct rd_list *list, int idx);
  struct rd_list * rd_list_index(struct rd_list *list, int idx);
  void * rd_list_index_data(struct rd_list *list, int idx);
  int rd_list_length(struct rd_list *list);
  struct rd_list * rd_list_append(struct rd_list *list, void *data);
  void rd_list_free(struct rd_list *list);

#ifdef __cplusplus
}
#endif

#endif
