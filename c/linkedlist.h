#include <stddef.h>

#define to_list(x) (list_node_t **)(x)
#define to_list_node(x) (list_node_t *)(x)

typedef struct list_node
{
    struct list_node *next;
} list_node_t;

list_node_t *list_get_tail(list_node_t **head);

list_node_t *list_append(list_node_t **head, list_node_t *item);

list_node_t *list_pop(list_node_t **head);

list_node_t *list_remove(list_node_t **head, list_node_t *item);

list_node_t *list_reverse(list_node_t **head);
