#include "orc.h"
#include <stdlib.h>

void update_l4r(last4rounds l4r, double new_v){
    if (l4r.v1 != 0) {
        l4r.v4 = l4r.v3;
        l4r.v3 = l4r.v2;
        l4r.v2 = l4r.v1;
        l4r.v1 = new_v;
    } else {
        l4r.v1 = new_v;
    }

}

void new_noda_n(noda_n** nodn, double value) {
    *nodn = (noda_n*)malloc(sizeof(noda_n));
    (*nodn)->value = value;
    (*nodn)->next = NULL;
}

void new_queue_n(queue_n* quen, int size) {
    quen->size = size;
    quen->count = 0;
    quen->head = NULL;
    quen->tail = NULL;
}

void qn_add(queue_n* quen, double value) {
    noda_n* new_node;
    new_noda_n(&new_node, value);
    if (quen->head == NULL) {
        quen->head = new_node;
        quen->tail = new_node;
        quen->count = 1;
        return;
    }

    if (quen->count >= quen->size) {
        noda_n* temp = quen->head;
        quen->head = quen->head->next;
        free(temp);
        quen->count--;
    }

    quen->tail->next = new_node;
    quen->tail = new_node;
    quen->count++;
}