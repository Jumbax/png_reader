#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linkedlist.h"



list_node_t *list_get_tail(list_node_t **head)
{
    list_node_t *current_node = *head;
    list_node_t *last_node = NULL;

    while (current_node)
    {
        last_node = current_node;
        current_node = current_node->next;
    }
       
    return last_node;
}

list_node_t *list_append(list_node_t **head, list_node_t *item)
{
    list_node_t *tail = list_get_tail(head);
    if (!tail)
    {
        *head = item;
    }
    else
    {
        tail->next = item;
    }

    item->next = NULL;
    return item;
}

list_node_t *list_pop(list_node_t **head)
{
    list_node_t *current_head = *head;
    if (!current_head)
    {
        return NULL;
    }

    *head = (*head)->next;
    current_head->next = NULL;

    return current_head;
}

list_node_t *list_remove(list_node_t **head, list_node_t *item)
{
    list_node_t *current_head = *head;
    if (!current_head)
    {
        return NULL;
    }
    int list_size = 0;
    list_node_t *current_head_for_size = *head;
    while (current_head_for_size)
    {
        current_head_for_size = current_head_for_size->next;
        list_size++;
    }

    for (size_t i = 0; i < list_size; i++)
    {
        if (current_head != item)
        {
            current_head = current_head->next;
            continue;
        }
        else
        {
            if (i == 0)
            {
                list_pop(head);
                break;
            }
            else
            {
                list_node_t *prev_node = *head;
                list_node_t *next_node = *head;
                for (size_t j = 0; j < i - 1; j++)
                {
                    prev_node = prev_node->next;
                }
                next_node = prev_node->next->next;
                prev_node->next = next_node;
                current_head->next = NULL;
                break;
            }
        }
    }

    return current_head;
}

list_node_t *list_reverse(list_node_t **head)
{
    list_node_t *current_head = *head;
    if (!current_head)
    {
        return NULL;
    }

    int list_size = 0;
    while (current_head)
    {
        current_head = current_head->next;
        list_size++;
    }

    list_node_t **items = malloc(sizeof(list_node_t *) * list_size);
    for (size_t i = 0; i < list_size; i++)
    {
        items[i] = list_get_tail(head);
        list_remove(head, items[i]);
    }
    for (size_t i = 0; i < list_size; i++)
    {
        list_append(head, items[i]);
    }

    free(items);
    return *head;
}