#include "cmpsc473mr.h"

/* Jack Pokras
   CMPSC 473
   Project 3
*/

// global variables
int n;        // # thread pairs in mapper stage
int b;        // max buffer 
pthread_mutex_t *tLock;
char *file;
long fsize;
long psize;

int main(int argc, char *argv[]) {
// initialize variables
    struct timeval sTime, eTime;
    gettimeofday(&sTime,NULL);
    n = atoi(argv[2]);
    b = atoi(argv[3]);
    t_args ta[n];                               // args to send into thread pairs
    pthread_t mread[n], madd[n], reduce;       // all threads to be used
    tLock = calloc(n, sizeof(pthread_mutex_t));
    b_node **aBuf[n];
    file = argv[1];
    FILE *fp = fopen(file,"r");
    fseek(fp,0,SEEK_END);
    fsize = ftell(fp);

    // split file size portions evenly (if possible)
    if(fsize%n != 0) {
        psize = (fsize - (fsize%n))/n;
    } else {
        psize = fsize/n;
    }
    fclose(fp);

// mapper stage
    for(int i=0; i<n; i++) {
        ta[i].p = i;
        ta[i].count = 0;
        ta[i].rBuf = NULL;
        ta[i].aBuf = NULL;
        aBuf[i] = &(ta[i].aBuf);
        Pthread_mutex_init(&(tLock[i]));
        // create reader then adder threads
        if(pthread_create(&(mread[i]), NULL, readThread, &(ta[i])) != 0){
            i--;
            fprintf(stderr, "FAILED TO CREATE THREAD %d, ATTEMPTING TO RECREATE\n", i);
        }
        if(pthread_create(&(madd[i]), NULL, addThread, &(ta[i])) != 0){
            fprintf(stderr, "FAILED TO CREATE THREAD %d, ATTEMPTING TO RECREATE\n", i);
            i--;
        }
    }

// wait for all mapper stage read threads to finish
    for(int i=0; i<n; i++) {
        pthread_join(mread[i], NULL);
    }
// reducer stage
    if(pthread_create(&reduce, NULL, reduceThread, &aBuf) != 0) {
        fprintf(stderr, "FAILED TO CREATE REDUCE THREAD, EXITING PROGRAM\n");
        exit(1);
    }
    pthread_join(reduce, NULL);
    gettimeofday(&eTime, NULL);
    int timeDiff = (eTime.tv_sec - sTime.tv_sec)*1000 + (eTime.tv_usec - sTime.tv_usec)/1000;
    printf("timer: %d\n", timeDiff);
    return 0;
}



// threads
void *readThread(void *context) {
// initialize veriables
    t_args *args = context;
    FILE *fp = fopen(file,"r");
    long start = ((args->p==0) ? 0 : pstart(args->p, fsize, n));
    long end = ((args->p==n) ? fsize : start + psize);
    b_item *item = malloc(sizeof(b_item));
    item->wd = malloc(sizeof(char[50]));
    item->i = 1;
    fseek(fp, start, SEEK_SET);
    
    // check if the seeked position is in the middle of a word
    // if it is toss it because the previous thread will read it fully. 
    char *c = malloc(sizeof(char[50]));
    if(args->p != 0) {
        if(fgetc(fp) != 32) {
            fseek(fp, -2, SEEK_CUR);
            if(fgetc(fp) != 32) {
                fscanf(fp, "%s", c);
            }
        }
    }
    free(c);
    c = NULL;

// read loop till portion is done
    while(ftell(fp) <= end && ftell(fp) < fsize ) {
        while(args->count >= b);
        pthread_mutex_lock(&tLock[args->p]);
        // Critical Section

        if((fscanf(fp, "%s", item->wd) == EOF)) {
            break;
        }
        item->wd = format(item->wd);

        args->rBuf = addNode(args->rBuf, item);
        args->count++;

        pthread_mutex_unlock(&tLock[args->p]);
    }
    free(item->wd);
    item->wd = NULL;
    free(item);
    item = NULL;
    return NULL;
}



// add thread takes the read buffer and adds up all of the portion's unique words.
void *addThread(void *context) {
    t_args *args = context;
    b_item *item;
    while(1) {
        while(args->count <= 0);
        pthread_mutex_lock(&tLock[args->p]);
    // Critical Section

        item = deleteNode(args->rBuf);
        args->count--;
        args->aBuf = searchNode(args->aBuf, item);

        pthread_mutex_unlock(&tLock[args->p]);

    }    
    return NULL;
}

void *reduceThread(void *context){
    FILE *fout;
    b_node ***buf = context;
    b_node *root[n];
    for(int i=0; i<n; i++){
        root[i] = *buf[i];
    }
    b_node *final = NULL;
    b_item *temp; 
    for(int i=0; i<n; i++) {
        while(root[i]->item != NULL) {
            temp = deleteNode(root[i]);
            final = searchNode(final, temp);
        }
    }
    fout = fopen("output.txt", "w");
    b_node *cur = final;
    do{
        fprintf(fout, "%s %d\n", cur->item->wd, cur->item->i);
        cur = cur->next;
    }while(cur != final);
    fclose(fout);
    return NULL;
}
        
