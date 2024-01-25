#include <stdio.h>

#define Y_HEIGHT 21 //Y axis of map
#define X_HEIGHT 80 //X axis of map

#define WATER '~'
#define ROCK '%'
#define TREE '^'
#define ROAD '#'
#define CENTER 'C'
#define MARKET 'M'
#define LONG_GRASS ':'
#define CLEARING '.'
#define PLAYER '@'


int main(int argc, char *argv[]){
    
    char map[Y_HEIGHT][X_HEIGHT];
    
    for(int y = 0; y < Y_HEIGHT; y++){
        for(int x = 0; x < X_HEIGHT; x++){
            map[y][x] = '?';
        }
    }

    for(int y = 0; y < Y_HEIGHT; y++){
        for(int x = 0; x < X_HEIGHT; x++){
            printf("%c", map[y][x]);
        }
        printf("\n");
    }

    return 0;
}