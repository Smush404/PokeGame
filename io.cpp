#include <unistd.h>
#include <ncurses.h>
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

#include "io.h"
#include "character.h"
#include "poke327.h"
#include "parser.h"
#include "PQ.h"

#define TRAINER_LIST_FIELD_WIDTH 46

npc *npclist[50];
int nextnpc;

database *io_db;
typedef struct io_message
{
  /* Will print " --more-- " at end of line when another message follows. *
   * Leave 10 extra spaces for that.                                      */
  char msg[71];
  struct io_message *next;
} io_message_t;

static io_message_t *io_head, *io_tail;

void add_npclist(npc *np)
{
  npclist[nextnpc] = np;
  nextnpc++;
}

void io_init_terminal(database *d)
{
  io_db = d;
  initscr();
  raw();
  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);
  start_color();
  init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
  init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
  init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
  init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
  init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
  init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
  init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
}

void io_reset_terminal(void)
{
  endwin();

  while (io_head)
  {
    io_tail = io_head;
    io_head = io_head->next;
    free(io_tail);
  }
  io_tail = NULL;
}

void io_queue_message(const char *format, ...)
{
  io_message_t *tmp;
  va_list ap;

  if (!(tmp = (io_message_t *)malloc(sizeof(*tmp))))
  {
    perror("malloc");
    exit(1);
  }

  tmp->next = NULL;

  va_start(ap, format);

  vsnprintf(tmp->msg, sizeof(tmp->msg), format, ap);

  va_end(ap);

  if (!io_head)
  {
    io_head = io_tail = tmp;
  }
  else
  {
    io_tail->next = tmp;
    io_tail = tmp;
  }
}

static void io_print_message_queue(uint32_t y, uint32_t x)
{
  while (io_head)
  {
    io_tail = io_head;
    attron(COLOR_PAIR(COLOR_CYAN));
    mvprintw(y, x, "%-80s", io_head->msg);
    attroff(COLOR_PAIR(COLOR_CYAN));
    io_head = io_head->next;
    if (io_head)
    {
      attron(COLOR_PAIR(COLOR_CYAN));
      mvprintw(y, x + 70, "%10s", " --more-- ");
      attroff(COLOR_PAIR(COLOR_CYAN));
      refresh();
      getch();
    }
    free(io_tail);
  }
  io_tail = NULL;
}

/**************************************************************************
 * Compares trainer distances from the PC according to the rival distance *
 * map.  This gives the approximate distance that the PC must travel to   *
 * get to the trainer (doesn't account for crossing buildings).  This is  *
 * not the distance from the NPC to the PC unless the NPC is a rival.     *
 *                                                                        *
 * Not a bug.                                                             *
 **************************************************************************/
static int compare_trainer_distance(const void *v1, const void *v2)
{
  const character *const *c1 = (const character *const *)v1;
  const character *const *c2 = (const character *const *)v2;

  return (world.rival_dist[(*c1)->pos[dim_y]][(*c1)->pos[dim_x]] -
          world.rival_dist[(*c2)->pos[dim_y]][(*c2)->pos[dim_x]]);
}

static character *io_nearest_visible_trainer()
{
  character **c, *n;
  uint32_t x, y, count;

  c = (character **)malloc(world.cur_map->num_trainers * sizeof(*c));

  /* Get a linear list of trainers */
  for (count = 0, y = 1; y < MAP_Y - 1; y++)
  {
    for (x = 1; x < MAP_X - 1; x++)
    {
      if (world.cur_map->cmap[y][x] && world.cur_map->cmap[y][x] !=
                                           &world.pc)
      {
        c[count++] = world.cur_map->cmap[y][x];
      }
    }
  }

  /* Sort it by distance from PC */
  qsort(c, count, sizeof(*c), compare_trainer_distance);

  n = c[0];

  free(c);

  return n;
}

void io_display()
{
  uint32_t y, x;
  character *c;

  clear();
  for (y = 0; y < MAP_Y; y++)
  {
    for (x = 0; x < MAP_X; x++)
    {
      if (world.cur_map->cmap[y][x])
      {
        mvaddch(y + 1, x, world.cur_map->cmap[y][x]->symbol);
      }
      else
      {
        switch (world.cur_map->map[y][x])
        {
        case ter_boulder:
          attron(COLOR_PAIR(COLOR_MAGENTA));
          mvaddch(y + 1, x, BOULDER_SYMBOL);
          attroff(COLOR_PAIR(COLOR_MAGENTA));
          break;
        case ter_mountain:
          attron(COLOR_PAIR(COLOR_MAGENTA));
          mvaddch(y + 1, x, MOUNTAIN_SYMBOL);
          attroff(COLOR_PAIR(COLOR_MAGENTA));
          break;
        case ter_tree:
          attron(COLOR_PAIR(COLOR_GREEN));
          mvaddch(y + 1, x, TREE_SYMBOL);
          attroff(COLOR_PAIR(COLOR_GREEN));
          break;
        case ter_forest:
          attron(COLOR_PAIR(COLOR_GREEN));
          mvaddch(y + 1, x, FOREST_SYMBOL);
          attroff(COLOR_PAIR(COLOR_GREEN));
          break;
        case ter_path:
        case ter_bailey:
          attron(COLOR_PAIR(COLOR_YELLOW));
          mvaddch(y + 1, x, PATH_SYMBOL);
          attroff(COLOR_PAIR(COLOR_YELLOW));
          break;
        case ter_gate:
          attron(COLOR_PAIR(COLOR_YELLOW));
          mvaddch(y + 1, x, GATE_SYMBOL);
          attroff(COLOR_PAIR(COLOR_YELLOW));
          break;
        case ter_mart:
          attron(COLOR_PAIR(COLOR_BLUE));
          mvaddch(y + 1, x, POKEMART_SYMBOL);
          attroff(COLOR_PAIR(COLOR_BLUE));
          break;
        case ter_center:
          attron(COLOR_PAIR(COLOR_RED));
          mvaddch(y + 1, x, POKEMON_CENTER_SYMBOL);
          attroff(COLOR_PAIR(COLOR_RED));
          break;
        case ter_grass:
          attron(COLOR_PAIR(COLOR_GREEN));
          mvaddch(y + 1, x, TALL_GRASS_SYMBOL);
          attroff(COLOR_PAIR(COLOR_GREEN));
          break;
        case ter_clearing:
          attron(COLOR_PAIR(COLOR_GREEN));
          mvaddch(y + 1, x, SHORT_GRASS_SYMBOL);
          attroff(COLOR_PAIR(COLOR_GREEN));
          break;
        case ter_water:
          attron(COLOR_PAIR(COLOR_CYAN));
          mvaddch(y + 1, x, WATER_SYMBOL);
          attroff(COLOR_PAIR(COLOR_CYAN));
          break;
        default:
          /* Use zero as an error symbol, since it stands out somewhat, and it's *
           * not otherwise used.                                                 */
          attron(COLOR_PAIR(COLOR_CYAN));
          mvaddch(y + 1, x, ERROR_SYMBOL);
          attroff(COLOR_PAIR(COLOR_CYAN));
        }
      }
    }
  }

  mvprintw(23, 1, "PC position is (%2d,%2d) on map %d%cx%d%c.",
           world.pc.pos[dim_x],
           world.pc.pos[dim_y],
           abs(world.cur_idx[dim_x] - (WORLD_SIZE / 2)),
           world.cur_idx[dim_x] - (WORLD_SIZE / 2) >= 0 ? 'E' : 'W',
           abs(world.cur_idx[dim_y] - (WORLD_SIZE / 2)),
           world.cur_idx[dim_y] - (WORLD_SIZE / 2) <= 0 ? 'N' : 'S');
  mvprintw(22, 1, "%d known %s.", world.cur_map->num_trainers,
           world.cur_map->num_trainers > 1 ? "trainers" : "trainer");
  mvprintw(22, 30, "Nearest visible trainer: ");
  if ((c = io_nearest_visible_trainer()))
  {
    attron(COLOR_PAIR(COLOR_RED));
    mvprintw(22, 55, "%c at vector %d%cx%d%c.",
             c->symbol,
             abs(c->pos[dim_y] - world.pc.pos[dim_y]),
             ((c->pos[dim_y] - world.pc.pos[dim_y]) <= 0 ? 'N' : 'S'),
             abs(c->pos[dim_x] - world.pc.pos[dim_x]),
             ((c->pos[dim_x] - world.pc.pos[dim_x]) <= 0 ? 'W' : 'E'));
    attroff(COLOR_PAIR(COLOR_RED));
  }
  else
  {
    attron(COLOR_PAIR(COLOR_BLUE));
    mvprintw(22, 55, "NONE.");
    attroff(COLOR_PAIR(COLOR_BLUE));
  }

  io_print_message_queue(0, 0);

  refresh();
}

uint32_t io_teleport_pc(pair_t dest)
{
  /* Just for fun. And debugging.  Mostly debugging. */

  do
  {
    dest[dim_x] = rand_range(1, MAP_X - 2);
    dest[dim_y] = rand_range(1, MAP_Y - 2);
  } while (world.cur_map->cmap[dest[dim_y]][dest[dim_x]] ||
           move_cost[char_pc][world.cur_map->map[dest[dim_y]]
                                                [dest[dim_x]]] ==
               DIJKSTRA_PATH_MAX ||
           world.rival_dist[dest[dim_y]][dest[dim_x]] < 0);

  return 0;
}

static void io_scroll_trainer_list(char (*s)[TRAINER_LIST_FIELD_WIDTH],
                                   uint32_t count)
{
  uint32_t offset;
  uint32_t i;

  offset = 0;

  while (1)
  {
    for (i = 0; i < 13; i++)
    {
      mvprintw(i + 6, 19, " %-40s ", s[i + offset]);
    }
    switch (getch())
    {
    case KEY_UP:
      if (offset)
      {
        offset--;
      }
      break;
    case KEY_DOWN:
      if (offset < (count - 13))
      {
        offset++;
      }
      break;
    case 27:
      return;
    }
  }
}

static void io_list_trainers_display(npc **c, uint32_t count)
{
  uint32_t i;
  char(*s)[TRAINER_LIST_FIELD_WIDTH]; /* pointer to array of 40 char */

  s = (char(*)[TRAINER_LIST_FIELD_WIDTH])malloc(count * sizeof(*s));

  mvprintw(3, 19, " %-40s ", "");
  /* Borrow the first element of our array for this string: */
  snprintf(s[0], TRAINER_LIST_FIELD_WIDTH, "You know of %d trainers:", count);
  mvprintw(4, 19, " %-40s ", *s);
  mvprintw(5, 19, " %-40s ", "");

  for (i = 0; i < count; i++)
  {
    snprintf(s[i], TRAINER_LIST_FIELD_WIDTH, "%16s %c: %2d %s by %2d %s",
             char_type_name[c[i]->ctype],
             c[i]->symbol,
             abs(c[i]->pos[dim_y] - world.pc.pos[dim_y]),
             ((c[i]->pos[dim_y] - world.pc.pos[dim_y]) <= 0 ? "North" : "South"),
             abs(c[i]->pos[dim_x] - world.pc.pos[dim_x]),
             ((c[i]->pos[dim_x] - world.pc.pos[dim_x]) <= 0 ? "West" : "East"));
    if (count <= 13)
    {
      /* Handle the non-scrolling case right here. *
       * Scrolling in another function.            */
      mvprintw(i + 6, 19, " %-40s ", s[i]);
    }
  }

  if (count <= 13)
  {
    mvprintw(count + 6, 19, " %-40s ", "");
    mvprintw(count + 7, 19, " %-40s ", "Hit escape to continue.");
    while (getch() != 27 /* escape */)
      ;
  }
  else
  {
    mvprintw(19, 19, " %-40s ", "");
    mvprintw(20, 19, " %-40s ",
             "Arrows to scroll, escape to continue.");
    io_scroll_trainer_list(s, count);
  }

  free(s);
}

static void io_list_trainers()
{
  npc **c;
  uint32_t x, y, count;

  c = (npc **)malloc(world.cur_map->num_trainers * sizeof(*c));

  /* Get a linear list of trainers */
  for (count = 0, y = 1; y < MAP_Y - 1; y++)
  {
    for (x = 1; x < MAP_X - 1; x++)
    {
      if (world.cur_map->cmap[y][x] && world.cur_map->cmap[y][x] !=
                                           &world.pc)
      {
        c[count++] = dynamic_cast<npc *>(world.cur_map->cmap[y][x]);
      }
    }
  }

  /* Sort it by distance from PC */
  qsort(c, count, sizeof(*c), compare_trainer_distance);

  /* Display it */
  io_list_trainers_display(c, count);
  free(c);

  /* And redraw the map */
  io_display();
}

void io_pokemart()
{
  mvprintw(0, 0, "Welcome to the Pokemart.  Could I interest you in some Pokeballs?");
  refresh();
  getch();
}

void io_pokemon_center()
{
  mvprintw(0, 0, "Welcome to the Pokemon Center.  How can Nurse Joy assist you?");
  refresh();
  getch();
}

int playerturn(char battlename, PQ_t *bpq)
{
  int height = 10;
  int width = 80;
  int startY = (LINES - height) / 2; // Center vertically
  int startX = (COLS - width) / 2;   // Center horizontally

  // Create a subwindow for the box
  WINDOW *popupWin = newwin(height, width, startY, startX);
  box(popupWin, 0, 0); // Create a box around the window

  mvprintw(8, 20, "Battle against a(n) %c", battlename);
  mvprintw(9, 20, "Pick your next move!");
  // mvprintw(9, 20, "emeny hp %d", 10);
  mvprintw(11, 20, "   1) Fight");
  mvprintw(12, 20, "   2) Bag");
  mvprintw(13, 20, "   3) Run");
  mvprintw(14, 20, "   4) Switch");
  //  for (int i = 0; i < nextnpc; i++)
  // {
  //   for (int j = 0; j < npclist[i]->pindex; j++)
  //   {
  //         mvwprintw(popupWin, i +j + 1, 1, "%d} %s - hp: %d, attack: %d",
  //             j + 1,npclist[i]->pl[j].name.c_str(), npclist[i]->pl[j].attack);
  //   }
  // }
  wrefresh(popupWin);

  char i = getch();
  while (1)
  {
    if (i == '1' || i == '2' || i == '3' || i == '4')
    {
      break;
    }
    else
    {
      i = getch();
    }
  }
  delwin(popupWin);
  refresh();

  double damage = 0;
  int has = 0;
  int again = 1;
  WINDOW *extrawindow = newwin(height, width, startY, startX);
  switch (i)
  {
  case '1':
    // if (!world.pc.pokelist[bpq->items[0].active_pokemon_index].is_fainted)
    // { // TODO -- add acceracy
    damage = world.pc.pokelist[bpq->items[0].active_pokemon_index].attack;
    //}

    // if(bpq->items[1].np->pl[bpq->items[1].active_pokemon_index].hp - damage < 0){
    //   differance = bpq->items[1].np->pl[bpq->items[1].active_pokemon_index].hp - damage;
    //   damage += differance;
    //   bpq->items[1].np->pl[bpq->items[1].active_pokemon_index].hp -= damage;
    // }
    // else{
    bpq->items[1].np->pl[bpq->items[1].active_pokemon_index].hp = bpq->items[1].np->pl[bpq->items[1].active_pokemon_index].hp - damage; // crashes when less then 0??????????
    //}
    if (bpq->items[1].np->pl[bpq->items[1].active_pokemon_index].hp <= 0)
    {
      bpq->items[1].np->pl[bpq->items[1].active_pokemon_index].is_fainted = 1;
      for (int i = 0; i < bpq->items[1].np->pindex; i++)
      {
        if (!bpq->items[1].np->pl[i].is_fainted)
        {
          bpq->items[1].active_pokemon_index = i;
          has = 1;
        }
      }

      if (!has)
      {
        // delwin(popupWin);
        return 10;
      }
    }

    box(extrawindow, 0, 0); // Create a box around the window

    mvprintw(8, 20, "Battle against a %c", bpq->items[1].np->pl[bpq->items[1].active_pokemon_index].name.c_str());
    // mvprintw( 9, 20, "Pick your next move!");
    mvprintw(9, 20, "emeny hp: %d", bpq->items[1].np->pl[bpq->items[1].active_pokemon_index].hp);
    mvprintw(10, 20, "damage: %d", (int)damage);
    mvprintw(11, 20, "fainted?: %d", bpq->items[1].np->pl[bpq->items[1].active_pokemon_index].is_fainted);
    mvprintw(12, 20, "press any key to contune");
    wrefresh(extrawindow);
    getch();

    break;
  case '2': // bag

    break;
  case '3': // run
    delwin(popupWin);
    return -1;
    break;

  case '4': // switch
    while (again)
    {
      box(extrawindow, 0, 0); // Create a box around the window

      mvprintw(8, 20, "what pokemon do you want to use?");
      mvprintw(9, 20, "pick one.  Choose wisely.");
      for (int i = 0; i < bpq->items[0].p->pindex; i++)
      {
        mvprintw(10 + i, 20, "%d) %s - hp: %d", i + 1, bpq->items[0].p->pokelist[i].name.c_str(), (int)bpq->items[0].p->pokelist[i].hp);
      }
      wrefresh(extrawindow);

      refresh();
      i = getchar();
      i = i - '0';
      if (!bpq->items[0].p->pokelist[(int)i].is_fainted)
      {
        bpq->items[0].p->pindex = i;
        again = 0;
      }
    }
    break;
  }

  delwin(popupWin);
  delwin(extrawindow);
  return 0;
}

void io_battle(character *aggressor, character *defender)
{
  npc *n = (npc *)((aggressor == &world.pc) ? defender : aggressor);
  int nowin = 1;
  PQ bpq;
  char battlename;
  item_t current;
  PQ_init(&bpq); // init PQ for battle

  switch (aggressor->symbol)
  {
  case '@':
    enque(&bpq, 0, nullptr, (pc *)aggressor, nullptr);
    break;

  case 'h':
  case 's':
  case 'r':
  case 'w':
  case 'e':
    enque(&bpq, 0, (npc *)aggressor, nullptr, nullptr);
    break;

  default:
    enque(&bpq, 0, nullptr, nullptr, (poke *)aggressor);
    break;
  }

  switch (defender->symbol)
  {
  case '@':
    enque(&bpq, 1, nullptr, (pc *)defender, nullptr);
    break;

  case 'h':
  case 's':
  case 'r':
  case 'w':
  case 'e':
    enque(&bpq, 1, (npc *)defender, nullptr, nullptr);
    break;

  default:
    enque(&bpq, 1, nullptr, nullptr, (poke *)defender);
    break;
  }

  if (aggressor->symbol == '@')
  {
    battlename = defender->symbol;
  }
  else if (defender->symbol != '\0')
  {
    battlename = aggressor->symbol;
  }
  else
  {
    battlename = 'P';
  }

  echo();
  curs_set(1);

  int response = 0;

  while (nowin)
  {
    current = bpq.items[peek(&bpq)];

    switch (current.symbol)
    {
    case '@':
      response = playerturn(battlename, &bpq);
      if (response == 10)
      {

        n->defeated = 1;
        if (n->ctype == char_hiker || n->ctype == char_rival)
        {
          n->mtype = move_wander;
        }

        int height = 10;
        int width = 80;
        int startY = (LINES - height) / 2; // Center vertically
        int startX = (COLS - width) / 2;   // Center horizontally

        // Create a subwindow for the box
        WINDOW *popupWin = newwin(height, width, startY, startX);
        box(popupWin, 0, 0); // Create a box around the window

        mvprintw(11, 20, "You Win");
        mvprintw(12, 20, "press any key to contune");

        wrefresh(popupWin);

        getch();

        delwin(popupWin);
        refresh();
        nowin = 0;
      }
      else if (response == -1)
      {
        nowin = 0;
      }
      break;

    default:
      // npcturn();
      nowin = 0;
      break;

      if (bpq.items[0].p != nullptr)
      {
        enque(&bpq, bpq.items[0].seq_num + 2, nullptr, bpq.items[0].p, nullptr);
      }
      else
      {
        enque(&bpq, bpq.items[1].seq_num + 2, bpq.items[1].np, nullptr, nullptr);
      }

      deque(&bpq);
    }

    // if(world.pc.pokelist[0].hp <= 0){
    //   nowin = 0;

    // }
    // else if(bpq.items[0].np->pl[0].hp <= 0){
    //   nowin = 0;
    // }
    // add if blank pokes no heath they winner
  }
  //   mvprintw( 4, 20, "Battle against a(n) %c", battlename);
  //   mvprintw( 9, 20, "pick one.  Choose wisely.");
  //   mvprintw(11, 20, "   1) %s", choice[0]->get_species());
  //   mvprintw(12, 20, "   2) %s", choice[1]->get_species());
  //   mvprintw(13, 20, "   3) %s", choice[2]->get_species());
  //   mvprintw(15, 20, "Enter 1, 2, or 3: ");

  //   refresh();
  //   i = getch();

  // noecho();
  // curs_set(0);

  // io_display();
  // mvprintw(0, 0, "Aww, how'd you get so strong?  You and your pokemon must share a special bond!");
  // refresh();
  // getch();

  noecho();
  curs_set(0);
}

poke npc_pokemon_event(npc *np)
{
  poke p;

  int rand_moves;
  int randdom_index = rand() % 1092;

  pokemon new_pokemon = io_db->pokemonl[randdom_index];
  p.id = new_pokemon.ID;

  // pokemon_moves pm = io_db->pokemon_movesl[p.id];

  moves m;

  for (int i = 0; i < 2; i++)
  {
    rand_moves = rand() % 844;
    m = io_db->movesl[rand_moves];

    p.name = new_pokemon.identifier;

    p.moveset[i] = m.id;
  }

  // pokemon_stats ps = io_db->pokemon_statsl[p.id + 1];
  p.attack = io_db->pokemon_statsl[p.id + 1].base_stat; // ERROR HERE convertion between ps and p

  // ps = io_db->pokemon_statsl[p.id + 2];
  p.defense = io_db->pokemon_statsl[p.id + 2].base_stat;

  // p.exp = new_pokemon.base_experience;
  // ps = io_db->pokemon_statsl[p.id];
  p.hp = io_db->pokemon_statsl[p.id].base_stat;

  // ps = io_db->pokemon_statsl[p.id + 3];
  p.sp_attack = io_db->pokemon_statsl[p.id + 3].base_stat;

  // ps = io_db->pokemon_statsl[p.id + 4];
  p.sp_defence = io_db->pokemon_statsl[p.id + 4].base_stat;

  // ps = io_db->pokemon_statsl[p.id + 5];
  p.speed = io_db->pokemon_statsl[p.id + 5].base_stat;

  p.iv = rand() % 15;

  p.is_shiny = (int)rand() % 8192 == 0;
  p.is_fainted = 0;

  int distance = abs(max(world.cur_idx[dim_y], world.cur_idx[dim_x]));

  if (distance <= 200)
  {
    if (distance < 3)
    {
      p.level = 1;
    }
    p.level = distance / 2;
  }
  else
  {
    p.level = (distance - 200) / 2;
  }

  if (p.level == 0)
  {
    p.level = 1;
  }

  if (p.hp == 0)
  {
    p.hp = 5;
  }

  //     int height = 10;
  //   int width = 80;
  //   int startY = (LINES - height) / 2; // Center vertically
  //   int startX = (COLS - width) / 2;   // Center horizontally

  // WINDOW *popupWin = newwin(height, width, startY, startX);
  //   box(popupWin, 0, 0); // Create a box around the window

  //   // mvprintw(8, 20, "Battle against a(n) %c", battlename);
  //   // mvprintw( 9, 20, "Pick your next move!");
  //   // mvprintw(9, 20, "emeny hp %d", 10);
  //   // mvprintw(11, 20, "   1) Fight");

  //   // mvprintw(12, 20, "   2) Bag");
  //   // mvprintw(13, 20, "   3) Run");
  //   // mvprintw(14, 20, "   4) Switch");
  //   mvwprintw(popupWin,1, 1, "%d} %s - hp: %d, attack: %d, defence: %d",
  //               1,  p.name.c_str(),  p.hp , io_db->pokemon_statsl[p.id + 1], ceil(p.defense));
  //   wrefresh(popupWin);

  //   getch();

  //   delwin(popupWin);

  np->pl[np->pindex] = p;
  np->pindex++;

  return p;
}

void pokemon_start()
{
  poke plist[3];
  moves m;
  poke p;
  pokemon new_pokemon;

  pokemon_stats ps;

  int indexer = 0;

  while (indexer != 3)
  {
    int rand_moves;
    int randdom_index = rand() % 1092;

    new_pokemon = io_db->pokemonl[randdom_index];
    p.id = new_pokemon.ID;

    for (int i = 0; i < 2; i++)
    {
      rand_moves = rand() % 844;
      m = io_db->movesl[rand_moves];

      p.name = new_pokemon.identifier;

      p.moveset[i] = m.id;
    }

    p.name = new_pokemon.identifier;

    p.moveset[0] = m.id;

    ps = io_db->pokemon_statsl[p.id + 1];
    p.attack = ps.base_stat;

    ps = io_db->pokemon_statsl[p.id + 2];
    p.defense = ps.base_stat;

    p.exp = new_pokemon.base_experience;
    ps = io_db->pokemon_statsl[p.id];
    p.hp = ps.base_stat;

    ps = io_db->pokemon_statsl[p.id + 3];
    p.sp_attack = ps.base_stat;

    ps = io_db->pokemon_statsl[p.id + 4];
    p.sp_defence = ps.base_stat;

    ps = io_db->pokemon_statsl[p.id + 5];
    p.speed = ps.base_stat;

    p.iv = rand() % 15;

    p.is_shiny = (int)rand() % 8192 == 0;

    p.level = 1;

    plist[indexer] = p;
    indexer++;
  }

  io_display();
  refresh();

  int height = 10;
  int width = 80;
  int startY = (LINES - height) / 2; // Center vertically
  int startX = (COLS - width) / 2;   // Center horizontally

  // Create a subwindow for the box
  WINDOW *popupWin = newwin(height, width, startY, startX);
  box(popupWin, 0, 0); // Create a box around the window

  // Print Pokémon information in the box
  for (int i = 0; i < indexer; i++)
  {
    mvwprintw(popupWin, i + 1, 1, "%d} %s - Level: %d - Attack: %d - Defense: %d - shiny %d",
              i + 1,
              plist[i].name.c_str(), plist[i].level, static_cast<int>(round(plist[i].attack)), static_cast<int>(round(plist[i].defense)), plist[i].is_shiny);
  }
  // mvwprintw(popupWin, 1, 1, "i: %d", 0);
  // mvwprintw(popupWin, 2, 1, "Name: %s", plist[0].name.c_str());
  // mvwprintw(popupWin, 3, 1, "Level: %d", plist[0].level);
  // mvwprintw(popupWin, 4, 1, "attack: %d", plist[0].attack);
  // mvwprintw(popupWin, 2, 1, "defense: %d", plist[0].defense);

  // Refresh the window to display changes
  wrefresh(popupWin);

  int answer;
  int done = 0;
  // Wait for user input to close the popup
  while (!done)
  {
    answer = getch();
    if (answer == 49 || answer == 50 || answer == 51)
    {
      done = 1;
      delwin(popupWin);
    }
  }

  switch (answer)
  {
  case 49:
    answer = 1;
    break;
  case 50:
    answer = 2;
    break;
  case 51:
    answer = 3;
    break;
  }

  world.pc.pokelist[world.pc.pindex] = plist[answer - 1];
  world.pc.pindex++;

  // io_display();
  // mvprintw(0, 0, "move? %s", (*io_db).movesl[world.pc.pokelist[0].moveset[1]].identifier.c_str());
  // refresh();
  // getch();
}

// void print_player_moves(){
//   io_display();
//   refresh();

//   //     io_display();
//   // mvprintw(0, 0, "here");
//   // refresh();
//   // getch();

//   moves pm0;
//   moves pm1;
//   database db = *io_db;

//   int height = 10;
//     int width = 80;
//     int startY = (LINES - height) / 2;   // Center vertically
//     int startX = (COLS - width) / 2;     // Center horizontally

//     // Create a subwindow for the box
//     WINDOW *popupWin = newwin(height, width, startY, startX);
//     box(popupWin, 0, 0); // Create a box around the window

//     // Print Pokémon information in the box
//     for (int i = 0; i < world.pc.pindex; i++) {
//         pm0 = db.movesl[world.pc.pokelist[i].moveset[0]];
//         pm1 = db.movesl[world.pc.pokelist[i].moveset[1]];
//         mvwprintw(popupWin, i + 1, 1, "%d} %s - move1: %s move2: %s",
//          i + 1,
//          world.pc.pokelist[i].name.c_str(), pm0.identifier.c_str(), pm1.identifier.c_str());
//     }
//     // mvwprintw(popupWin, 1, 1, "i: %d", 0);
//     // mvwprintw(popupWin, 2, 1, "Name: %s", plist[0].name.c_str());
//     // mvwprintw(popupWin, 3, 1, "Level: %d", plist[0].level);
//     // mvwprintw(popupWin, 4, 1, "attack: %d", plist[0].attack);
//     //mvwprintw(popupWin, 2, 1, "defense: %d", plist[0].defense);

//     // Refresh the window to display changes
//     wrefresh(popupWin);

//     int answer;
//     int done = 0;
//     // Wait for user input to close the popup
//     while(!done){
//       answer = getch();
//       if(answer == 27 /*escape*/){
//         done = 1;
//         delwin(popupWin);
//       }
//     }
// }

int hp_div(int base, int iv, int level)
{
  return ceil((((base + iv) * 2) * level) / 100) + level + 10;
}
int other_div(int base, int iv, int level)
{
  return ceil((((base + iv) * 2) * level) / 100) + 5;
}

void pokemon_event()
{
  poke p;

  int rand_moves;
  int randdom_index = rand() % 1092;

  pokemon new_pokemon = io_db->pokemonl[randdom_index];
  p.id = new_pokemon.ID;

  moves m;

  for (int i = 0; i < 2; i++)
  {
    rand_moves = rand() % 844;
    m = io_db->movesl[rand_moves];

    p.name = new_pokemon.identifier;

    p.moveset[i] = m.id;
  }

  pokemon_stats ps = io_db->pokemon_statsl[p.id + 1];
  p.attack = ps.base_stat;

  ps = io_db->pokemon_statsl[p.id + 2];
  p.defense = ps.base_stat;

  p.exp = new_pokemon.base_experience;
  ps = io_db->pokemon_statsl[p.id];
  p.hp = ps.base_stat;

  ps = io_db->pokemon_statsl[p.id + 3];
  p.sp_attack = ps.base_stat;

  ps = io_db->pokemon_statsl[p.id + 4];
  p.sp_defence = ps.base_stat;

  ps = io_db->pokemon_statsl[p.id + 5];
  p.speed = ps.base_stat;

  p.iv = rand() % 15;

  p.is_shiny = (int)rand() % 8192 == 0;

  int distance = abs(max(world.cur_idx[dim_y], world.cur_idx[dim_x]));

  if (distance <= 200)
  {
    if (distance < 3)
    {
      p.level = 1;
    }
    p.level = distance / 2;
  }
  else
  {
    p.level = (distance - 200) / 2;
  }

  if (p.level == 0)
  {
    p.level = 1;
  }

  p.attack = other_div(p.attack, p.iv, p.level);
  p.defense = other_div(p.defense, p.iv, p.level);
  p.speed = other_div(p.speed, p.iv, p.level);
  p.sp_attack = other_div(p.sp_attack, p.iv, p.level);
  p.sp_defence = other_div(p.sp_defence, p.iv, p.level);
  p.hp = hp_div(p.hp, p.iv, p.level);

  world.pc.pokelist[world.pc.pindex] = p;
  world.pc.pindex++;

  std::string su = "name: " + new_pokemon.identifier + " level: " + std::to_string(p.level) + " attack: " + std::to_string(static_cast<int>(std::round(p.attack))) + " defence: " + std::to_string(static_cast<int>(std::round(p.defense))) + " shiny: " + std::to_string(p.is_shiny);
  const char *s = su.c_str();

  io_display();
  mvprintw(0, 0, s);
  refresh();
  getch();
}

uint32_t move_pc_dir(uint32_t input, pair_t dest)
{
  dest[dim_y] = world.pc.pos[dim_y];
  dest[dim_x] = world.pc.pos[dim_x];

  switch (input)
  {
  case 1:
  case 2:
  case 3:
    dest[dim_y]++;
    break;
  case 4:
  case 5:
  case 6:
    break;
  case 7:
  case 8:
  case 9:
    dest[dim_y]--;
    break;
  }
  switch (input)
  {
  case 1:
  case 4:
  case 7:
    dest[dim_x]--;
    break;
  case 2:
  case 5:
  case 8:
    break;
  case 3:
  case 6:
  case 9:
    dest[dim_x]++;
    break;
  case '>':
    if (world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x]] ==
        ter_mart)
    {
      io_pokemart();
    }
    if (world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x]] ==
        ter_center)
    {
      io_pokemon_center();
    }
    break;
  }

  if (world.cur_map->cmap[dest[dim_y]][dest[dim_x]])
  {
    if (dynamic_cast<npc *>(world.cur_map->cmap[dest[dim_y]][dest[dim_x]]) &&
        ((npc *)world.cur_map->cmap[dest[dim_y]][dest[dim_x]])->defeated)
    {
      // Some kind of greeting here would be nice
      return 1;
    }
    else if ((dynamic_cast<npc *>(world.cur_map->cmap[dest[dim_y]][dest[dim_x]])))
    {
      io_battle(&world.pc, world.cur_map->cmap[dest[dim_y]][dest[dim_x]]);
      // Not actually moving, so set dest back to PC position
      dest[dim_x] = world.pc.pos[dim_x];
      dest[dim_y] = world.pc.pos[dim_y];
    }
  }

  if (move_cost[char_pc][world.cur_map->map[dest[dim_y]][dest[dim_x]]] ==
      DIJKSTRA_PATH_MAX)
  {
    return 1;
  }

  if (world.cur_map->map[dest[dim_y]][dest[dim_x]] == ter_grass && rand() % 100 < 10)
  {
    pokemon_event();
  }

  return 0;
}

void io_teleport_world(pair_t dest)
{
  /* mvscanw documentation is unclear about return values.  I believe *
   * that the return value works the same way as scanf, but instead   *
   * of counting on that, we'll initialize x and y to out of bounds   *
   * values and accept their updates only if in range.                */
  int x = INT_MAX, y = INT_MAX;

  world.cur_map->cmap[world.pc.pos[dim_y]][world.pc.pos[dim_x]] = NULL;

  echo();
  curs_set(1);
  do
  {
    mvprintw(0, 0, "Enter x [-200, 200]:           ");
    refresh();
    mvscanw(0, 21, (char *)"%d", &x);
  } while (x < -200 || x > 200);
  do
  {
    mvprintw(0, 0, "Enter y [-200, 200]:          ");
    refresh();
    mvscanw(0, 21, (char *)"%d", &y);
  } while (y < -200 || y > 200);

  refresh();
  noecho();
  curs_set(0);

  x += 200;
  y += 200;

  world.cur_idx[dim_x] = x;
  world.cur_idx[dim_y] = y;

  new_map(1);
  io_teleport_pc(dest);
}

void io_handle_input(pair_t dest)
{
  uint32_t turn_not_consumed;
  int key;
  std::string ss;

  do
  {
    switch (key = getch())
    {
    case '7':
    case 'y':
    case KEY_HOME:
      turn_not_consumed = move_pc_dir(7, dest);
      break;
    case '8':
    case 'k':
    case KEY_UP:
      turn_not_consumed = move_pc_dir(8, dest);
      break;
    case '9':
    case 'u':
    case KEY_PPAGE:
      turn_not_consumed = move_pc_dir(9, dest);
      break;
    case '6':
    case 'l':
    case KEY_RIGHT:
      turn_not_consumed = move_pc_dir(6, dest);
      break;
    case '3':
    case 'n':
    case KEY_NPAGE:
      turn_not_consumed = move_pc_dir(3, dest);
      break;
    case '2':
    case 'j':
    case KEY_DOWN:
      turn_not_consumed = move_pc_dir(2, dest);
      break;
    case '1':
    case 'b':
    case KEY_END:
      turn_not_consumed = move_pc_dir(1, dest);
      break;
    case '4':
    case 'h':
    case KEY_LEFT:
      turn_not_consumed = move_pc_dir(4, dest);
      break;
    case '5':
    case ' ':
    case '.':
    case KEY_B2:
      dest[dim_y] = world.pc.pos[dim_y];
      dest[dim_x] = world.pc.pos[dim_x];
      turn_not_consumed = 0;
      break;
    case '>':
      turn_not_consumed = move_pc_dir('>', dest);
      break;
    case 'Q':
      dest[dim_y] = world.pc.pos[dim_y];
      dest[dim_x] = world.pc.pos[dim_x];
      world.quit = 1;
      turn_not_consumed = 0;
      break;
      break;
    case 't':
      io_list_trainers();
      turn_not_consumed = 1;
      break;
    case 'p':
      /* Teleport the PC to a random place in the map.              */
      io_teleport_pc(dest);
      turn_not_consumed = 0;
      break;
    case 'f':
      /* Fly to any map in the world.                                */
      io_teleport_world(dest);
      turn_not_consumed = 0;
      break;
    case 'm':
      for (int i = 0; i < world.pc.pindex; i++)
      {
        ss = "name: ";
        ss += world.pc.pokelist[i].name;
        ss += " level: " + std::to_string(world.pc.pokelist[i].level);
        ss += " move 1: " + io_db->movesl[world.pc.pokelist[i].moveset[0]].identifier;
        ss += " move 2: " + io_db->movesl[world.pc.pokelist[i].moveset[1]].identifier;
        // ss += " index i: " + std::to_string(nextnpc);
        io_queue_message(ss.c_str());
      }

      //     io_display();
      // mvprintw(0, 0, "real? %d", world.pc.pindex);
      // refresh();
      // getch();
      io_queue_message("done with the poke move list");
      // print_player_moves();
      dest[dim_y] = world.pc.pos[dim_y];
      dest[dim_x] = world.pc.pos[dim_x];
      turn_not_consumed = 0;
      break;
    case 'q':
      /* Demonstrate use of the message queue.  You can use this for *
       * printf()-style debugging (though gio_db is probably a better   *
       * option.  Not that it matters, but using this command will   *
       * waste a turn.  Set turn_not_consumed to 1 and you should be *
       * able to figure out why I did it that way.                   */

      for (int i = 0; i < nextnpc; i++)
      {
        ss = "name: ";
        ss += std::string(1, npclist[i]->symbol);
        ss += " pokemon: " + npclist[i]->pl[0].name;
        // ss += " index i: " + std::to_string(nextnpc);
        io_queue_message(ss.c_str());
      }
      io_queue_message("done with the trainer list");

      dest[dim_y] = world.pc.pos[dim_y];
      dest[dim_x] = world.pc.pos[dim_x];
      turn_not_consumed = 0;
      break;
    default:
      /* Also not in the spec.  It's not always easy to figure out what *
       * key code corresponds with a given keystroke.  Print out any    *
       * unhandled key here.  Not only does it give a visual error      *
       * indicator, but it also gives an integer value that can be used *
       * for that key in this (or other) switch statements.  Printed in *
       * octal, with the leading zero, because ncurses.h lists codes in *
       * octal, thus allowing us to do reverse lookups.  If a key has a *
       * name defined in the header, you can use the name here, else    *
       * you can directly use the octal value.                          */
      mvprintw(0, 0, "Unbound key: %#o ", key);
      turn_not_consumed = 1;
    }
    refresh();
  } while (turn_not_consumed);
}
