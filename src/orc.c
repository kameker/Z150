#include "orc.h"

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