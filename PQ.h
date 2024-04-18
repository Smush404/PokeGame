#ifndef PQ_H
#define PQ_H

#include <stdlib.h>
#include <climits>
#include <string>
#include "character.h"
  typedef struct item
{
  int seq_num, active_pokemon_index;
  npc *np;
  pc *p;
  poke *pokemon;
  char symbol;
} item_t;

typedef struct PQ
{
  item_t items[2];
  int size;
} PQ_t;

void PQ_init(PQ_t *pqr)
{
  pqr->size = 0;
}

void enque(PQ_t *pqr, int seq_num, npc *input_nc, pc *input_p, poke *input_poke, int active)
{
  int size = pqr->size;

  pqr->items[size].seq_num = seq_num;
  pqr->items[size].active_pokemon_index = active;
  pqr->items[size].np = input_nc;
  pqr->items[size].p = input_p;
  pqr->items[size].pokemon = input_poke;

  if(pqr->items[size].np != nullptr){
    pqr->items[size].symbol = input_nc->symbol;
  }
  else if(pqr->items[size].p != nullptr){
    pqr->items[size].symbol = input_p->symbol;
  }
  else{pqr->items[size].symbol = '\0';}

  pqr->size++;
}

int peek(PQ_t *pqr)
{
  int highestpir = INT_MAX;
  item_t tmp = pqr->items[0];
  int ind = 0;

  for (int i = 0; i < pqr->size; i++)
  {

    if (pqr->items[i].seq_num < highestpir)
    { // strictly less then on next_turn

      tmp = pqr->items[i];
       highestpir = pqr->items[i].seq_num;
      ind = i;
    }
  }

  return ind;
}



void deque(PQ_t *pqr){
  int ind = peek(pqr);
  for (int i = ind; i < pqr->size; i++)
  {
    pqr->items[i] = pqr->items[i + 1];
  }

  pqr->size--;
}


#endif