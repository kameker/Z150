typedef struct
{
    double v1;
    double v2;
    double v3;
    double v4;
} last4rounds;

void update_l4r(last4rounds l4r, double new_v);


#include <stdio.h>
#include <stdlib.h>

typedef struct Noda {
    double value;
    struct Noda* next;
} noda_n;

typedef struct {
    int size;
    int count;
    noda_n* head;
    noda_n* tail;
} queue_n;

void qn_add(queue_n* quen, double value);
void new_queue_n(queue_n* quen, int size);
void new_noda_n(noda_n** nodn, double value);

