#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#define Y_HEIGHT 21 //Y axis of map
#define X_HEIGHT 80 //X axis of map

//all the differnect types of ground

//grounds
#define WATER '~' //0
#define ROCK '%' //1
#define TREE '^' //2
#define LONG_GRASS ':' //3
#define CLEARING '.' //auto fills

//paths/interactables
#define ROAD '#'
#define CENTER 'C'
#define MARKET 'M'

//#define PLAYER '@' 

char map[Y_HEIGHT][X_HEIGHT]; //is the section of map being generated
int exitxy[4][2]; //is the xy of each exit going North, East, South, West

void startGround();

void fill(int i, int x, int y, int w, int z){ //fills in the ground
    char filler;
    
    switch (i) //swtich to find the filling type of ground
    {
    case 0:
        filler = WATER;
        break;
    case 1:
        filler = ROCK;
        break;
    case 2:
        filler = TREE;
        break;
    case 3:
        filler = LONG_GRASS;
        break;
    default:
        break;
    }

    //if(x < w && y < z){//Q1
        for (int l = x; l <= w; l++){
            for (int h = y; h <= z; h++){
                map[l][h] = filler;
            }
        }
    //}

    //if(x > w && y > z){//Q3
        for (int l = w; l <= x; l++){
            for (int h = z; h <= y; h++){
                map[l][h] = filler;
            }
        }
    //}

    //if(x > w && y < z){//Q4
        for (int l = w; l <= x; l++){
            for (int h = y; h <= z; h++){
                map[l][h] = filler;
            }
        }
    //}

    //if(x < w && y > z){//Q2
        for (int l = x; l <= w; l++){
            for (int h = z; h <= y; h++){
                map[l][h] = filler;
            }
        }
    //}

    if(x == w && y == z) {startGround();} //retry
}

void startGround(){
    int x,y; // the starting points for the ground rectangle
    int w,z; // ending points 

    for(int i; i < 4; i++){
        x = rand() % X_HEIGHT;
        y = rand() % Y_HEIGHT; 
        w = rand() % X_HEIGHT;
        z = rand() % Y_HEIGHT;

        printf("\ni = %d\n", i);
        printf("%d\n", x - w);
        printf("%d\n", y - z);

        fill(i, x, y, w, z);
    }
}

void border(){//fills the borders with rocks
    for(int i = 0; i < X_HEIGHT; i++){map[0][i] = ROCK;}//top
    for(int i = 1; i < Y_HEIGHT - 1; i++){//middle
        map[i][0] = ROCK;
        map[i][79] = ROCK;
    }
    for(int i = 0; i < X_HEIGHT; i++){map[20][i] = ROCK;}//bottom
}

void leave(){//makes the exits and stores them
    int x,y;

    x = rand() % X_HEIGHT; //N
    y = 0;

    map[y][x] = ROAD;
    exitxy[0][0] = x;
    exitxy[0][1] = y;

    x = rand() % X_HEIGHT; //S
    y = Y_HEIGHT - 1;

    map[y][x] = ROAD;
    exitxy[1][0] = x;
    exitxy[1][1] = y;

    x = 0; //W
    y = rand() % Y_HEIGHT;

    map[y][x] = ROAD;
    exitxy[2][0] = x;
    exitxy[2][1] = y;

    x = X_HEIGHT - 1;//E
    y = rand() % Y_HEIGHT;

    map[y][x] = ROAD;
    exitxy[3][0] = x;
    exitxy[3][1] = y;

    // for(int y = 0; y < 4; y++){
    //     for(int x = 0; x < 2; x++){
    //         printf("%c", exitxy[y][x]);
    //     }
    //     printf("\n");
    // }
}

void path(){//amkes path from exit to exit
    
}

void printmap(){ //prints the map
    for(int y = 0; y < Y_HEIGHT; y++){
        for(int x = 0; x < X_HEIGHT; x++){
            printf("%c", map[y][x]);
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]){
    srand(time(NULL));

    for(int y = 0; y < Y_HEIGHT; y++){ //fills the map with clearing as a foundation
        for(int x = 0; x < X_HEIGHT; x++){
            map[y][x] = CLEARING; 
        }
    }

    for(int i = 0; i <= 10; i++){
        startGround(); //fills the map with the ground envirments 
    }
    border();

    leave();

    path();

    printmap();

    return 0;
}