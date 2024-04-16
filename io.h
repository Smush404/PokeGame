#ifndef IO_H
# define IO_H

#include "character.h"

class database;
class character;
typedef int16_t pair_t[2];

void io_init_terminal(database *d);
void io_reset_terminal(void);

void io_display(void);
poke npc_pokemon_event(npc *c);
void pokemon_start();
void add_npclist(npc *np);
void io_handle_input(pair_t dest);
void io_queue_message(const char *format, ...);
void io_battle(character *aggressor, character *defender);

#endif
