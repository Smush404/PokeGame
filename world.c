#include <stdio.h>
#include <stdlib.h>

#include "poke327.h"

int main(int argc, char *argv[]){
    map_t n;

    new_map(&n);
    print_map(&n);
    return 0;
}