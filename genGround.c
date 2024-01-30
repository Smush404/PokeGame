#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>


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

/*
TODO
- make center and market random placements on the path
- make either 
    - seed regons 
    - add a region generator that forces current types of regons to be made at some point
        so that the map is more filled
    - comment what stuff does
*/

char map[Y_HEIGHT][X_HEIGHT]; //is the section of map being generated

int exitxy[4][2]; //is the xy of each exit going 0 - North, 1 - South, 2 - West, 3 - East [waht exit] then [][0] x and [][1] y

bool cdone = false, mdone = false;

void startGround();
void NSshops(int i, int y, int x);
int EWshops(int i, int y, int x);

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
    case 4:
        filler = CLEARING;
    default:
        break;
    }

    if(x < w && y < z){//Q1
        for (int l = x; l <= w; l++){
            for (int h = y; h <= z; h++){
                map[l][h] = filler;
            }
        }
    }

    if(x > w && y > z){//Q3
        for (int l = w; l <= x; l++){
            for (int h = z; h <= y; h++){
                map[l][h] = filler;
            }
        }
    }

    if(x > w && y < z){//Q4
        for (int l = w; l <= x; l++){
            for (int h = y; h <= z; h++){
                map[l][h] = filler;
            }
        }
    }

    if(x < w && y > z){//Q2
        for (int l = x; l <= w; l++){
            for (int h = z; h <= y; h++){
                map[l][h] = filler;
            }
        }
    }
}

void startGround(){
    int x,y; // the starting points for the ground rectangle
    int w,z; // ending points 

    for(int i = 0; i <= 4; i++){
        x = rand() % 100;
        y = rand() % 100; 
        w = rand() % 100;
        z = rand() % 100;

        // printf("\ni = %d\n", i);
        // printf("%d\n", x - w);
        // printf("%d\n", y - z);

        fill(i, x, y, w, z);
    }


    //TODO works for now but inconsant
    for(int i = 0; i < 2; i++){ //two tall grass regons
        x = rand() % 100;
        y = rand() % 100; 
        w = rand() % 100;
        z = rand() % 100;

        fill(3, x, y, w, z);
    }

    x = rand() % 79 ;
    y = rand() % 21; 
    w = rand() % 79;
    z = rand() % 21;

    fill(0, x, y, w, z);
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

    if(x == 0){ x += 1;}
    if(x == X_HEIGHT){ x -= 1;}

    map[y][x] = ROAD;
    exitxy[0][0] = x;
    exitxy[0][1] = y;

    x = rand() % X_HEIGHT; //S
    y = Y_HEIGHT - 1;

    if(x == 0){ x += 1;}
    if(x == X_HEIGHT){ x -= 1;}

    map[y][x] = ROAD;
    exitxy[1][0] = x;
    exitxy[1][1] = y;

    x = 0; //W
    y = rand() % Y_HEIGHT - 1;

    if(y == 0){ y += 1;}
    if(y == Y_HEIGHT){ y -= 1;}

    map[y][x] = ROAD;
    exitxy[2][0] = x;
    exitxy[2][1] = y;

    x = X_HEIGHT - 1;//E
    y = rand() % Y_HEIGHT - 1;

    if(y == 0){ y += 1;}
    if(y == Y_HEIGHT + 1){ y -= 1;}

    map[y][x] = ROAD;
    exitxy[3][0] = x;
    exitxy[3][1] = y;

}

void NtoSPath(){
    for(int i = 0; i < (Y_HEIGHT / 2) + 1; i++){ //N down
        map[i][exitxy[0][0]] = ROAD;
    }

    for(int i = 0; i < (Y_HEIGHT / 2) + 1; i++){//S up
        map[Y_HEIGHT - i][exitxy[1][0]] = ROAD;
    }

    for(int i = 0; i < (X_HEIGHT / 2) + 1; i++){ //west
        map[exitxy[2][1]][i] = ROAD;
    }

    for(int i = 1; i < (X_HEIGHT / 2); i++){ //east
        map[exitxy[3][1]][X_HEIGHT - i] = ROAD;
    }

}

void EtoWPath(){

    if(exitxy[1][0] - exitxy[0][0] > 0){
        for(int i = 0; i <= exitxy[1][0] - exitxy[0][0]; i++){ //left connection
            map[(Y_HEIGHT / 2) + 1][exitxy[0][0] + i] = ROAD;
            if(i == 2){NSshops(i, 0, 0);i++;} 
            else if(i == rand() % 10){NSshops(i, 0, 0);i++;} 
        }
    }
    else{
        for(int i = 0; i <= exitxy[0][0] - exitxy[1][0]; i++){ //right connection
            map[(Y_HEIGHT / 2) + 1][exitxy[1][0] + i] = ROAD;
            if(i == 2){NSshops(i, 1, 0);i++;} 
            else if(i == rand() % 10){NSshops(i, 1, 0);i++;} 
        }
    }

    cdone = false;
    mdone = false;

    if(exitxy[3][1] - exitxy[2][1] > 0){ //east down
        for(int i = 0; i <= exitxy[3][1] - exitxy[2][1]; i++){ //east connection
            map[exitxy[2][1] + i][(X_HEIGHT / 2) + 1] = ROAD;
            if(i == 2){EWshops(i, 2, 1);i++;} 
            if(i == rand() % 10){EWshops(i, 2, 1);i++;}
        }
    }
    else{
        for(int i = 0; i <= exitxy[2][1] - exitxy[3][1]; i++){ //east connection
            map[exitxy[3][1] + i][(X_HEIGHT / 2)] = ROAD;
            if(i == 2){EWshops(i, 3, 1); i++;}
            if(i == (rand() % 5) + 2){EWshops(i, 3, 1); i++;}
        }
    }


}


void path(){//makes path from exit to exit
    NtoSPath();
    EtoWPath();
}

int EWshops(int i, int y, int x){
    if(!cdone){
        map[exitxy[y][x] + i][(X_HEIGHT / 2)] = CENTER;
        map[exitxy[y][x] + i + 1][(X_HEIGHT / 2)] = CENTER;
        map[exitxy[y][x] + i][(X_HEIGHT / 2) + 1] = CENTER;
        map[exitxy[y][x] + i + 1][(X_HEIGHT / 2) + 1] = CENTER;
        cdone = true;
    }
    else if(!mdone &&
                (map[exitxy[y][x] + i][(X_HEIGHT / 2)] != CENTER &&
                map[exitxy[y][x] + i + 1][(X_HEIGHT / 2)] != CENTER &&
                map[exitxy[y][x] + i][(X_HEIGHT / 2) + 1] != CENTER &&
                map[exitxy[y][x] + i + 1][(X_HEIGHT / 2) + 1] != CENTER))
    {
        map[exitxy[y][x] + i][(X_HEIGHT / 2)] = MARKET;
        map[exitxy[y][x] + i + 1][(X_HEIGHT / 2)] = MARKET;
        map[exitxy[y][x] + i][(X_HEIGHT / 2) + 1] = MARKET;
        map[exitxy[y][x] + i + 1][(X_HEIGHT / 2) + 1] = MARKET;
        mdone = true;
    }
    return 0;
}

void NSshops(int i, int y, int x){
    if(!mdone){
        map[Y_HEIGHT/2][exitxy[y][x] + i] = MARKET;
        map[Y_HEIGHT/2][exitxy[y][x] + i + 1] = MARKET;
        map[Y_HEIGHT/2 + 1][exitxy[y][x] + i] = MARKET;
        map[Y_HEIGHT/2 + 1][exitxy[y][x] + i + 1] = MARKET;
        mdone = true;
    }
    else if(!cdone && 
                    (map[Y_HEIGHT/2][exitxy[y][x] + i] != MARKET || 
                    map[Y_HEIGHT/2][exitxy[y][x] + i + 1] != MARKET ||
                    map[Y_HEIGHT/2 + 1][exitxy[y][x] + i] != MARKET ||
                    map[Y_HEIGHT/2 + 1][exitxy[y][x] + i + 1] != MARKET))
    {
        map[Y_HEIGHT/2][exitxy[y][x] + i] = CENTER;
        map[Y_HEIGHT/2][exitxy[y][x] + i + 1] = CENTER;
        map[Y_HEIGHT/2 + 1][exitxy[y][x] + i] = CENTER;
        map[Y_HEIGHT/2 + 1][exitxy[y][x] + i + 1] = CENTER;
        cdone = true;
    }
}

void printmap(){ //prints the map
    for(int y = 0; y < Y_HEIGHT; y++){
        for(int x = 0; x < X_HEIGHT; x++){
            printf("%1c", map[y][x]);
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

    for(int i = 0; i <= 5; i++){
        startGround(); //fills the map with the ground envirments 
    }

    border();

    leave();

    path();

    printmap();

    return 0;
}