#include "List.h"

#include <stdlib.h>

void List_Init(List* list) {
    list->first = NULL;
    list->last = NULL;
}

void List_Deinit(List* list) {
    ListNode* iter = list->first;
    ListNode* rmNode;
    void* rmData;

    while (iter != NULL) {
        rmData = iter->data;
        rmNode = iter;

        iter = iter->next;

        free(rmData);
        free(rmNode);
    }

    List_Init(list);
}

void List_Insert(List* list, void* data) {
    ListNode* new_node = (ListNode*)malloc(sizeof(ListNode));

    *new_node = (ListNode) {
        .data = data,
        .next = NULL,
        .prev = list->last
    };

    if (list->first == NULL) {
        list->first = new_node;
    } else {
        list->last->next = new_node;
    }

    list->last = new_node;
}

void List_Remove(List* list, ListNode* rmNode) {
    void* rmData = rmNode->data;

    if (rmNode == list->first) {
        // Remove first node
        if (list->first == list->last) {
            // List has only one node
            list->first = NULL;
            list->last = NULL;
        } else {
            // List has more than one node
            list->first = rmNode->next;
            list->first->prev = NULL;
        }
    } else {
        // Remove 2nd or after node
        if (rmNode == list->last) {
            // Remove last node
            list->last = rmNode->prev;
        } else {
            // Remove middle node
            rmNode->next->prev = rmNode->prev;
        }
        rmNode->prev->next = rmNode->next;
    }

    free(rmData);
    free(rmNode);
}
