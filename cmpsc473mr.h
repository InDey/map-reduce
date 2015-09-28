#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <time.h>

#define out1 "output1.txt"
#define out2 "output2.txt"
#define out3 "output3.txt"
#define pstart(i,s,n) (((i*s)/n)+1)
#define pend(i,s,n) ((((i+1)*s)/n))
#define NUMPUNCT 9

typedef struct __buffer_item {
    char *wd;                // store a word in buffer.
    int i;                      // word count, depending whice buffer is using it
} b_item;

typedef struct __item_node {
    b_item *item;
    struct __item_node *next;
} b_node;

typedef struct __threadArgs {
    int p;                      // portion
    int count;                  // keeps track if buffer is full
    b_node *rBuf;                // ptr to buffer for read/add sharing
    b_node *aBuf;                // ptr to buffer for add/reduce sharing
} t_args; 

b_node *addNode(b_node *head, b_item *item) {

    if(item->wd == NULL) {
        fprintf(stderr, "GIVEN AN NULL STRING! BAD BAD BAD.\n");
        return head;
    }
    if(head == NULL) {
        head = malloc(sizeof(b_node));
        head->item = NULL;
        head->next = head;
    }
    if(head->item == NULL) {
        head->item = malloc(sizeof(b_node));
        head->item->wd = NULL;
        head->item->i = item->i;
    }
    if(head->item->wd == NULL) {
        head->item->wd = strdup(item->wd);
    }else {
        b_node *root = head;
        while(root->next != head){
            root = root->next;
        }
        root->next = malloc(sizeof(b_node));
        root->next->item = malloc(sizeof(b_item));
        root->next->item->wd = strdup(item->wd);
        root->next->item->i = item->i;
        root->next->next = head;
    }
    return head;
}

// will return the deleted node which will then be added to the add/reduce buffer.
b_item *deleteNode(b_node *head) {
    b_item *item = malloc(sizeof(b_item));
    if(head == NULL || head->item == NULL) {
        fprintf(stderr, "BAD HEAD NODE GIVEN, LIST NOT FOUND\n");
        getchar();
        free(item);
        item = NULL;
        return item;
    }
    b_node *root = head;
    while(root->next->next != head){
        root = root->next;
    }
    // tail of list found will extract data and delete node
    item->wd = strdup(root->next->item->wd);
    item->i = root->next->item->i;
    if(root->next == head) {
        free(head->item->wd);
        head->item->wd = NULL;
        head->item->i = 0;
        free(head->item);
        head->item = NULL;
        return item;
    }
    root->next = head;
    return item;
}

// traverses list to find a matching word. if not add it to end of list.
b_node *searchNode(b_node *head, b_item *item) {
    b_node *root = head;
    if(root == NULL) {
        head = addNode(head, item);
        return head;
    }
    do {
        if(strcmp(item->wd, root->item->wd) == 0) {
            root->item->i = root->item->i + item->i;
            return head;
        }
        root = root->next;
    }while(root != head);
    head = addNode(head, item);
    return head;
}

// remove punctuation
char *format(char *wd) {
    char *word = strdup(wd);
    char *ch = strdup(wd);
    char *punct = ".,?!':;()";   
    int k = 0;
    int size = strlen(wd); 
    
    // check each character of the word against each element in punct
    for(int j=0; j<size && word[j] != '\0'; j++, k++) {
        for(int i=0; i<NUMPUNCT; i++) {
            if(ch[k] == punct[i]) {
                k++;
                i = -1;
            } 
            if(ch[k] == '\0') {
                word[j] = ch[k];
                return word;
            }
        }
        // convert to lower case and asign to w
        word[j] = tolower(ch[k]);
    }
    return word;
}

void Pthread_mutex_init(pthread_mutex_t *mutex) {
    if( pthread_mutex_init(mutex, NULL) != 0){
        fprintf(stderr, "FAILED TO INITIALIZE THREAD LOCK, PROGRAM WILL NOW EXIT\n");
        exit(1);
    }
}

// Prototyping
void *readThread(void *args);
void *addThread(void *args);
void *reduceThread(void *args);
