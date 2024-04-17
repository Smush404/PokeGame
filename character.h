#ifndef CHARACTER_H
# define CHARACTER_H

# include <cstdint>
#include <vector>
#include <string>

# include "pair.h"

#define DIJKSTRA_PATH_MAX (INT_MAX / 2)
#define NO_NPCS 50
#define NUM_POKEMON 6

typedef enum __attribute__ ((__packed__)) movement_type {
  move_hiker,
  move_rival,
  move_pace,
  move_wander,
  move_sentry,
  move_explore,
  move_swim,
  move_pc,
  num_movement_types
} movement_type_t;

typedef enum __attribute__ ((__packed__)) character_type {
  char_pc,
  char_hiker,
  char_rival,
  char_swimmer,
  char_other,
  num_character_types
} character_type_t;

extern const char *char_type_name[num_character_types];

class character {
 public:
  virtual ~character() {}
  pair_t pos;
  char symbol;
  int next_turn;
  int seq_num;
};

class poke{
  public:
    int hp, max_hp, attack, defense, speed, sp_attack, sp_defence;
    int id, exp, level, iv, is_shiny, is_fainted;
    int moveset[2];
    std::string name;
};

class inv_item{
  public:
  std::string name;
  double heathling;
  int revivable;
  float catch_chance;
};

class npc : public character {
 public:
  character_type_t ctype;
  movement_type_t mtype;
  int defeated;
  poke pl[6];
  int pindex = 0;
  pair_t dir;
};

class pc : public character {
  public:
    int pindex = 0;
    std::vector<inv_item> inv_pokeball;
    std::vector<inv_item> inv_revive;
    std::vector<inv_item> inv_potion;
    poke pokelist[NUM_POKEMON];
};

/* character is defined in poke327.h to allow an instance of character
 * in world without including character.h in poke327.h                 */

int32_t cmp_char_turns(const void *key, const void *with);
void delete_character(void *v);

extern void (*move_func[num_movement_types])(character *, pair_t);

int pc_move(char);
bool is_pc(character *c);

#endif
