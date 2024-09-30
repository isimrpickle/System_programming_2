#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct queuejob {
    char jobID[6]; 
    char* job;
    int file_descriptor;
};

struct node {
    struct queuejob job; // the struct queuejob from jobCommander
    struct node* next_node; // the pointer that'll show to the next node of the queue
};

struct Queue {
    struct node* rear; // πίσω μέρος ουράς
    struct node* front; // μπροστινό μέρος ουράς
    int size;
};

struct Queue* create_queue() { // creating the queue.
    struct Queue* temp = (struct Queue*)malloc(sizeof(struct Queue)); // queue variable that will be used to allocate the necessary space and initialize the queue.
    temp->front = temp->rear = NULL; // at the initialization both the front and rear are zero
    temp->size = 0; // initializing size
    return temp;
}

struct node* create_node() { // returns a new node
    struct node* temp = (struct node*)malloc(sizeof(struct node)); // initializing an empty node
    temp->job.jobID[0] = '\0';
    temp->job.job = NULL;
    temp->job.file_descriptor = -1;
    temp->next_node = NULL;
    return temp;
}

void enqueue(struct Queue* queue, struct queuejob* job) {
    struct node* new_node = create_node();
    if (new_node == NULL) {
        printf("Error: Failed to allocate memory for new node\n");
        exit(-1);
    }

    new_node->job = *job;

    if (queue->rear == NULL) { // if the queue is empty/we are adding the 1st element
        queue->front = queue->rear = new_node; // it's only one element so the pointer doesn't have anywhere to point
    } else {
        queue->rear->next_node = new_node; // updating the last element before the current enqueuing
        queue->rear = new_node; // current rear is new_node (the next_node now is null, it will be changed in the next enqueuing....)
    }
    queue->size++;
}

// dequeuing will return the job of the node
struct queuejob dequeue(struct Queue* queue) {
    if (queue->front == NULL) {
        printf("Queue is empty\n");
        struct queuejob empty_job = {"", NULL, -1};//////
        return empty_job;
    }
    struct node* temp_node = queue->front;
    struct queuejob data = temp_node->job;
    queue->front = queue->front->next_node; // checking the value of next node to see if we are dequeuing the only element of the queue
    if (queue->front == NULL) { // if we are dequeuing the last element we need to set rear=front
        queue->rear = NULL;
    }
    free(temp_node);
    queue->size--; // decrementing the size
    return data; // return the data of the node that got dequeued
}

// in this function, we'll reach each node and free it individually. Then we will free the struct Queue as a whole
void delete_queue(struct Queue* queue) {
    struct node* temp_node; // we will need to create a new node in order to reach every node in the queue.
    while (queue->front != NULL) {
        temp_node = queue->front;
        queue->front = queue->front->next_node; // setting the front node of the queue as the next node, so we fully iterate it
        free(temp_node); // freeing the specific node
    }
    free(queue); // after we have freed each node separately, we free the struct Queue.
}

void print_queue(struct Queue* queue) {
    struct node* temp_node;
    int i = 0;
    if (queue->front == NULL) {
        printf("The queue is empty\n");
        return;
    }
    for (temp_node = queue->front; temp_node != NULL; temp_node = temp_node->next_node) {
        i++;
        printf("\nThe %d node has jobID: %s, job: %s, filedescriptor: %d\n",
               i, temp_node->job.jobID, temp_node->job.job, temp_node->job.file_descriptor);
        fflush(stdout);
    }
}


int stop_job(struct Queue* queue, char* ID) {
    struct node* temp_node = queue->front;
    struct node* prev_node = NULL;

    while (temp_node != NULL) {
        if (strcmp(temp_node->job.jobID, ID) == 0) {
            if (prev_node == NULL) {
                // The node to remove is the front node
                queue->front = temp_node->next_node;
                if (queue->front == NULL) {
                    // The queue is now empty
                    queue->rear = NULL;
                }
            } else {
                // The node to remove is in the middle or end
                prev_node->next_node = temp_node->next_node;
                if (temp_node->next_node == NULL) {
                    // The node to remove is the rear node
                    queue->rear = prev_node;
                }
            }
            if (temp_node->job.job != NULL) {
                free(temp_node->job.job); // Free the dynamically allocated job string
            }
            free(temp_node); // Free the node itself
            queue->size--; // Decrement the size of the queue
            return 1;
        }
        prev_node = temp_node;
        temp_node = temp_node->next_node;
    }
    return 0; // Job with the given ID not found
}


char* poll_queue(struct Queue* queue){
    struct node* temp_node;
    int i=0;
    char* jobstring=malloc(35*sizeof(char));
    char* stringToBePrinted=malloc((queue->size*35)*sizeof(char)+80);
    strcat(stringToBePrinted,"\n-----jobID output start------\n");
    printf("the queue size is %d \n",queue->size);
    for (temp_node = queue->front; temp_node != NULL; temp_node = temp_node->next_node) {
        i++;
        sprintf(jobstring,"JOB<%s\n",temp_node->job.jobID);
        strcat(stringToBePrinted,jobstring);
        strcat(stringToBePrinted,"\n");
    }
    char* jobend="\n-----jobID output end------\n";
    strcat(stringToBePrinted,jobend);
    return stringToBePrinted;
}

