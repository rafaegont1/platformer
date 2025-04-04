#ifndef LIST_H
#define LIST_H

typedef struct ListNode ListNode;
typedef struct List List;

struct ListNode {
    void* data;
    ListNode* next;
    ListNode* prev;
};

struct List {
    ListNode* first;
    ListNode* last;
};

void List_Init(List* list);
void List_Deinit(List* list);
void List_Insert(List* list, void* data);
void List_Remove(List* list, ListNode* rmNode);

#endif // LIST_H
