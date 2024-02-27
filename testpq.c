#include <stdio.h>
#include <limits.h>

typedef struct item
{
    char symbol;
    int next_turn;
    int seq_num;
} item_t;

typedef struct PQ
{
    item_t items[100];
    int size;
} PQ_t;

int peek(PQ_t *pqr);

// 1 for same - 0 for not
// int itemcomp(item_t i1, item_t i2)
// {
//     if(i1.symbol == i2.symbol &&
//         i1.next_turn == i2.next_turn &&
//         i1.seq_num == i2.seq_num){
//             return 1;
//     }

//     return 0;
// }

void PQ_init(PQ_t *pqr){
    pqr->size = 0;
}

void enque(PQ_t *pqr, char symbol, int next_turn, int seq_num)
{
    int size = pqr->size;

    pqr->items[size].next_turn = next_turn;
    pqr->items[size].symbol = symbol;
    pqr->items[size].seq_num = seq_num;

    pqr->size = size++;
}

void deque(PQ_t *pqr)
{
    int ind = peek(pqr);

    for (int i = ind; i < pqr->size; i++)
    {
        pqr->items[i] = pqr->items[i + 1];
    }

    pqr->size--;

}

int peek(PQ_t *pqr)
{
    int highestpir = INT_MAX;
    item_t tmp = pqr->items[0];
    int ind = 0;

    for (int i = 0; i < pqr->size; i++)
    {

        /*
            next_turn is less
            if tie the lowest seq_num
        */

        if (pqr->items[i].next_turn < highestpir)
        { // strictly less then on next_turn

            tmp = pqr->items[i];
            highestpir = pqr->items[i].next_turn;
            ind = i;
        }

        else if (pqr->items[i].next_turn == highestpir &&
                 pqr->items[i].seq_num < tmp.seq_num)
        { // if equal check the seq_num

            tmp = pqr->items[i];
            highestpir = pqr->items[i].next_turn;
            ind = i;
        }
    }

    return ind;
}

void print_pq(PQ_t pqr){
    for(int i = 0; i < pqr.size; i++){
        item_t tmp = pqr.items[i];
        printf("%c %d %d\n", tmp.symbol, tmp.next_turn, tmp.seq_num);
    }
}

int main(int argc, char *argv[])
{
    PQ_t pql;
    
    PQ_init(&pql);

    enque(&pql, '@', 10, 50);
    enque(&pql, '!', 15, 5);
    enque(&pql, '&', 10, 5);
    enque(&pql, '&', 10, 2);

    item_t tmp = pql.items[peek(&pql)];
    printf("%c %d %d\n", tmp.symbol, tmp.next_turn, tmp.seq_num);

    return 0;
}