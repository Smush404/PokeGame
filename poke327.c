#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>
#include <ctype.h>

#include "heap.h"

  /*
    TODO:
    - all npcs but hikers can not fight???
    - player moves to npc start fight 

    BUGS:
      - npcs no longer going to pc
      - p will disapear and then break the game :(
      - pc going through npcs and making two of them.
  */

#define malloc(size) ({          \
  void *_tmp;                    \
  assert((_tmp = malloc(size))); \
  _tmp;                          \
})
typedef enum dim
{
  dim_x,
  dim_y,
  num_dims
} dim_t;

typedef int16_t pair_t[num_dims];
typedef enum __attribute__((__packed__)) terrain_type
{
  ter_boulder,
  ter_tree,
  ter_path,
  ter_mart,
  ter_center,
  ter_grass,
  ter_clearing,
  ter_mountain,
  ter_forest,
  ter_water,
  ter_gate,
  ter_rival,
  ter_hiker,
  ter_exploer,
  ter_wonder,
  ter_sentry,
  ter_pacer,
  num_terrain_types,
  ter_debug
} terrain_type_t;
typedef struct charitor
{
  pair_t pos;
  char symbol;
  terrain_type_t prevous;
  int dead;
} cahr_t;

typedef struct item
{
  char symbol;
  cahr_t c;
  int next_turn;
  int seq_num;
} item_t;

typedef struct PQ
{
  item_t items[100];
  int size;
} PQ_t;
typedef struct path
{
  heap_node_t *hn;
  uint8_t pos[2];
  uint8_t from[2];
  int32_t cost;
} path_t;


#define MAP_X 80
#define MAP_Y 21
#define MIN_TREES 10
#define MIN_BOULDERS 10
#define TREE_PROB 95
#define BOULDER_PROB 95
#define WORLD_SIZE 401
#define MAX_CHAR 50

//colors :)
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

#define MOUNTAIN_SYMBOL '%'
#define BOULDER_SYMBOL '0'
#define TREE_SYMBOL '4'
#define FOREST_SYMBOL '^'
#define GATE_SYMBOL '#'
#define PATH_SYMBOL '#'
#define POKEMART_SYMBOL 'M'
#define POKEMON_CENTER_SYMBOL 'C'
#define TALL_GRASS_SYMBOL ':'
#define SHORT_GRASS_SYMBOL '.'
#define WATER_SYMBOL '~'
#define ERROR_SYMBOL '&'

#define HIKER_SYMBOL 'h'
#define RIVAL_SYMBOL 'r'
#define PACER_SYMBOL 'p'
#define WONDER_SYMBOL 'w'
#define SENTRY_SYMBOL 's'
#define EXPLOER_SYMBOL 'e'


#define DIJKSTRA_PATH_MAX (INT_MAX / 2)

#define mappair(pair) (m->map[pair[dim_y]][pair[dim_x]])
#define mapxy(x, y) (m->map[y][x])
#define heightpair(pair) (m->height[pair[dim_y]][pair[dim_x]])
#define heightxy(x, y) (m->height[y][x])



typedef enum __attribute__((__packed__)) character_type
{
  char_pc,
  char_hiker,
  char_rival,
  char_other,
  char_pacer,
  char_wonder,
  char_exploer,
  char_sentry,
  num_character_types
} character_type_t;

typedef struct pc
{
  pair_t pos;
} pc_t;

typedef struct map
{
  terrain_type_t map[MAP_Y][MAP_X];
  uint8_t height[MAP_Y][MAP_X];
  int8_t n, s, e, w;
  cahr_t cahr[MAX_CHAR];
} map_t;

typedef struct queue_node
{
  int x, y;
  struct queue_node *next;
} queue_node_t;

typedef struct world
{
  map_t *world[WORLD_SIZE][WORLD_SIZE];
  pair_t cur_idx;
  map_t *cur_map;
  /* Place distance maps in world, not map, since *
   * we only need one pair at any given time.     */
  int hiker_dist[MAP_Y][MAP_X];
  int rival_dist[MAP_Y][MAP_X];
  pc_t pc;
} world_t;

/* Even unallocated, a WORLD_SIZE x WORLD_SIZE array of pointers is a very *
 * large thing to put on the stack.  To avoid that, world is a global.     */
world_t world;
int global_count = 0;

/* Just to make the following table fit in 80 columns */
#define IM DIJKSTRA_PATH_MAX
    int32_t move_cost[num_character_types][num_terrain_types] = {
        //  boulder,tree,path,mart,center,grass,clearing,mountain,forest,water,gate
        {IM, IM, 10, 10, 10, 20, 10, IM, IM, IM, 10},
        {IM, IM, 10, 50, 50, 15, 10, 15, 15, IM, IM},
        {IM, IM, 10, 50, 50, 20, 10, IM, IM, IM, IM},
        {IM, IM, IM, IM, IM, IM, IM, IM, IM, 7, IM},
};
#undef IM

static int32_t path_cmp(const void *key, const void *with)
{
  return ((path_t *)key)->cost - ((path_t *)with)->cost;
}

static int32_t edge_penalty(int8_t x, int8_t y)
{
  return (x == 1 || y == 1 || x == MAP_X - 2 || y == MAP_Y - 2) ? 2 : 1;
}

static void dijkstra_path(map_t *m, pair_t from, pair_t to)
{
  static path_t path[MAP_Y][MAP_X], *p;
  static uint32_t initialized = 0;
  heap_t h;
  uint32_t x, y;

  if (!initialized)
  {
    for (y = 0; y < MAP_Y; y++)
    {
      for (x = 0; x < MAP_X; x++)
      {
        path[y][x].pos[dim_y] = y;
        path[y][x].pos[dim_x] = x;
      }
    }
    initialized = 1;
  }

  for (y = 0; y < MAP_Y; y++)
  {
    for (x = 0; x < MAP_X; x++)
    {
      path[y][x].cost = DIJKSTRA_PATH_MAX;
    }
  }

  path[from[dim_y]][from[dim_x]].cost = 0;

  heap_init(&h, path_cmp, NULL);

  for (y = 1; y < MAP_Y - 1; y++)
  {
    for (x = 1; x < MAP_X - 1; x++)
    {
      path[y][x].hn = heap_insert(&h, &path[y][x]);
    }
  }

  while ((p = heap_remove_min(&h)))
  {
    p->hn = NULL;

    if ((p->pos[dim_y] == to[dim_y]) && p->pos[dim_x] == to[dim_x])
    {
      for (x = to[dim_x], y = to[dim_y];
           (x != from[dim_x]) || (y != from[dim_y]);
           p = &path[y][x], x = p->from[dim_x], y = p->from[dim_y])
      {
        /* Don't overwrite the gate */
        if (x != to[dim_x] || y != to[dim_y])
        {
          mapxy(x, y) = ter_path;
          heightxy(x, y) = 0;
        }
      }
      heap_delete(&h);
      return;
    }

    if ((path[p->pos[dim_y] - 1][p->pos[dim_x]].hn) &&
        (path[p->pos[dim_y] - 1][p->pos[dim_x]].cost >
         ((p->cost + heightpair(p->pos)) *
          edge_penalty(p->pos[dim_x], p->pos[dim_y] - 1))))
    {
      path[p->pos[dim_y] - 1][p->pos[dim_x]].cost =
          ((p->cost + heightpair(p->pos)) *
           edge_penalty(p->pos[dim_x], p->pos[dim_y] - 1));
      path[p->pos[dim_y] - 1][p->pos[dim_x]].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y] - 1][p->pos[dim_x]].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1]
                                           [p->pos[dim_x]]
                                               .hn);
    }
    if ((path[p->pos[dim_y]][p->pos[dim_x] - 1].hn) &&
        (path[p->pos[dim_y]][p->pos[dim_x] - 1].cost >
         ((p->cost + heightpair(p->pos)) *
          edge_penalty(p->pos[dim_x] - 1, p->pos[dim_y]))))
    {
      path[p->pos[dim_y]][p->pos[dim_x] - 1].cost =
          ((p->cost + heightpair(p->pos)) *
           edge_penalty(p->pos[dim_x] - 1, p->pos[dim_y]));
      path[p->pos[dim_y]][p->pos[dim_x] - 1].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y]][p->pos[dim_x] - 1].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]]
                                           [p->pos[dim_x] - 1]
                                               .hn);
    }
    if ((path[p->pos[dim_y]][p->pos[dim_x] + 1].hn) &&
        (path[p->pos[dim_y]][p->pos[dim_x] + 1].cost >
         ((p->cost + heightpair(p->pos)) *
          edge_penalty(p->pos[dim_x] + 1, p->pos[dim_y]))))
    {
      path[p->pos[dim_y]][p->pos[dim_x] + 1].cost =
          ((p->cost + heightpair(p->pos)) *
           edge_penalty(p->pos[dim_x] + 1, p->pos[dim_y]));
      path[p->pos[dim_y]][p->pos[dim_x] + 1].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y]][p->pos[dim_x] + 1].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]]
                                           [p->pos[dim_x] + 1]
                                               .hn);
    }
    if ((path[p->pos[dim_y] + 1][p->pos[dim_x]].hn) &&
        (path[p->pos[dim_y] + 1][p->pos[dim_x]].cost >
         ((p->cost + heightpair(p->pos)) *
          edge_penalty(p->pos[dim_x], p->pos[dim_y] + 1))))
    {
      path[p->pos[dim_y] + 1][p->pos[dim_x]].cost =
          ((p->cost + heightpair(p->pos)) *
           edge_penalty(p->pos[dim_x], p->pos[dim_y] + 1));
      path[p->pos[dim_y] + 1][p->pos[dim_x]].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y] + 1][p->pos[dim_x]].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1]
                                           [p->pos[dim_x]]
                                               .hn);
    }
  }
}

static int build_paths(map_t *m)
{
  pair_t from, to;

  /*  printf("%d %d %d %d\n", m->n, m->s, m->e, m->w);*/

  if (m->e != -1 && m->w != -1)
  {
    from[dim_x] = 1;
    to[dim_x] = MAP_X - 2;
    from[dim_y] = m->w;
    to[dim_y] = m->e;

    dijkstra_path(m, from, to);
  }

  if (m->n != -1 && m->s != -1)
  {
    from[dim_y] = 1;
    to[dim_y] = MAP_Y - 2;
    from[dim_x] = m->n;
    to[dim_x] = m->s;

    dijkstra_path(m, from, to);
  }

  if (m->e == -1)
  {
    if (m->s == -1)
    {
      from[dim_x] = 1;
      from[dim_y] = m->w;
      to[dim_x] = m->n;
      to[dim_y] = 1;
    }
    else
    {
      from[dim_x] = 1;
      from[dim_y] = m->w;
      to[dim_x] = m->s;
      to[dim_y] = MAP_Y - 2;
    }

    dijkstra_path(m, from, to);
  }

  if (m->w == -1)
  {
    if (m->s == -1)
    {
      from[dim_x] = MAP_X - 2;
      from[dim_y] = m->e;
      to[dim_x] = m->n;
      to[dim_y] = 1;
    }
    else
    {
      from[dim_x] = MAP_X - 2;
      from[dim_y] = m->e;
      to[dim_x] = m->s;
      to[dim_y] = MAP_Y - 2;
    }

    dijkstra_path(m, from, to);
  }

  if (m->n == -1)
  {
    if (m->e == -1)
    {
      from[dim_x] = 1;
      from[dim_y] = m->w;
      to[dim_x] = m->s;
      to[dim_y] = MAP_Y - 2;
    }
    else
    {
      from[dim_x] = MAP_X - 2;
      from[dim_y] = m->e;
      to[dim_x] = m->s;
      to[dim_y] = MAP_Y - 2;
    }

    dijkstra_path(m, from, to);
  }

  if (m->s == -1)
  {
    if (m->e == -1)
    {
      from[dim_x] = 1;
      from[dim_y] = m->w;
      to[dim_x] = m->n;
      to[dim_y] = 1;
    }
    else
    {
      from[dim_x] = MAP_X - 2;
      from[dim_y] = m->e;
      to[dim_x] = m->n;
      to[dim_y] = 1;
    }

    dijkstra_path(m, from, to);
  }

  return 0;
}

static int gaussian[5][5] = {
    {1, 4, 7, 4, 1},
    {4, 16, 26, 16, 4},
    {7, 26, 41, 26, 7},
    {4, 16, 26, 16, 4},
    {1, 4, 7, 4, 1}};

static int smooth_height(map_t *m)
{
  int32_t i, x, y;
  int32_t s, t, p, q;
  queue_node_t *head, *tail, *tmp;
  /*  FILE *out;*/
  uint8_t height[MAP_Y][MAP_X];

  memset(&height, 0, sizeof(height));

  /* Seed with some values */
  for (i = 1; i < 255; i += 20)
  {
    do
    {
      x = rand() % MAP_X;
      y = rand() % MAP_Y;
    } while (height[y][x]);
    height[y][x] = i;
    if (i == 1)
    {
      head = tail = malloc(sizeof(*tail));
    }
    else
    {
      tail->next = malloc(sizeof(*tail));
      tail = tail->next;
    }
    tail->next = NULL;
    tail->x = x;
    tail->y = y;
  }

  /*
  out = fopen("seeded.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", MAP_X, MAP_Y);
  fwrite(&height, sizeof (height), 1, out);
  fclose(out);
  */

  /* Diffuse the vaules to fill the space */
  while (head)
  {
    x = head->x;
    y = head->y;
    i = height[y][x];

    if (x - 1 >= 0 && y - 1 >= 0 && !height[y - 1][x - 1])
    {
      height[y - 1][x - 1] = i;
      tail->next = malloc(sizeof(*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x - 1;
      tail->y = y - 1;
    }
    if (x - 1 >= 0 && !height[y][x - 1])
    {
      height[y][x - 1] = i;
      tail->next = malloc(sizeof(*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x - 1;
      tail->y = y;
    }
    if (x - 1 >= 0 && y + 1 < MAP_Y && !height[y + 1][x - 1])
    {
      height[y + 1][x - 1] = i;
      tail->next = malloc(sizeof(*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x - 1;
      tail->y = y + 1;
    }
    if (y - 1 >= 0 && !height[y - 1][x])
    {
      height[y - 1][x] = i;
      tail->next = malloc(sizeof(*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x;
      tail->y = y - 1;
    }
    if (y + 1 < MAP_Y && !height[y + 1][x])
    {
      height[y + 1][x] = i;
      tail->next = malloc(sizeof(*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x;
      tail->y = y + 1;
    }
    if (x + 1 < MAP_X && y - 1 >= 0 && !height[y - 1][x + 1])
    {
      height[y - 1][x + 1] = i;
      tail->next = malloc(sizeof(*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x + 1;
      tail->y = y - 1;
    }
    if (x + 1 < MAP_X && !height[y][x + 1])
    {
      height[y][x + 1] = i;
      tail->next = malloc(sizeof(*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x + 1;
      tail->y = y;
    }
    if (x + 1 < MAP_X && y + 1 < MAP_Y && !height[y + 1][x + 1])
    {
      height[y + 1][x + 1] = i;
      tail->next = malloc(sizeof(*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x + 1;
      tail->y = y + 1;
    }

    tmp = head;
    head = head->next;
    free(tmp);
  }

  /* And smooth it a bit with a gaussian convolution */
  for (y = 0; y < MAP_Y; y++)
  {
    for (x = 0; x < MAP_X; x++)
    {
      for (s = t = p = 0; p < 5; p++)
      {
        for (q = 0; q < 5; q++)
        {
          if (y + (p - 2) >= 0 && y + (p - 2) < MAP_Y &&
              x + (q - 2) >= 0 && x + (q - 2) < MAP_X)
          {
            s += gaussian[p][q];
            t += height[y + (p - 2)][x + (q - 2)] * gaussian[p][q];
          }
        }
      }
      m->height[y][x] = t / s;
    }
  }
  /* Let's do it again, until it's smooth like Kenny G. */
  for (y = 0; y < MAP_Y; y++)
  {
    for (x = 0; x < MAP_X; x++)
    {
      for (s = t = p = 0; p < 5; p++)
      {
        for (q = 0; q < 5; q++)
        {
          if (y + (p - 2) >= 0 && y + (p - 2) < MAP_Y &&
              x + (q - 2) >= 0 && x + (q - 2) < MAP_X)
          {
            s += gaussian[p][q];
            t += height[y + (p - 2)][x + (q - 2)] * gaussian[p][q];
          }
        }
      }
      m->height[y][x] = t / s;
    }
  }

  /*
  out = fopen("diffused.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", MAP_X, MAP_Y);
  fwrite(&height, sizeof (height), 1, out);
  fclose(out);

  out = fopen("smoothed.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", MAP_X, MAP_Y);
  fwrite(&m->height, sizeof (m->height), 1, out);
  fclose(out);
  */

  return 0;
}

static void find_building_location(map_t *m, pair_t p)
{
  do
  {
    p[dim_x] = rand() % (MAP_X - 3) + 1;
    p[dim_y] = rand() % (MAP_Y - 3) + 1;

    if ((((mapxy(p[dim_x] - 1, p[dim_y]) == ter_path) &&
          (mapxy(p[dim_x] - 1, p[dim_y] + 1) == ter_path)) ||
         ((mapxy(p[dim_x] + 2, p[dim_y]) == ter_path) &&
          (mapxy(p[dim_x] + 2, p[dim_y] + 1) == ter_path)) ||
         ((mapxy(p[dim_x], p[dim_y] - 1) == ter_path) &&
          (mapxy(p[dim_x] + 1, p[dim_y] - 1) == ter_path)) ||
         ((mapxy(p[dim_x], p[dim_y] + 2) == ter_path) &&
          (mapxy(p[dim_x] + 1, p[dim_y] + 2) == ter_path))) &&
        (((mapxy(p[dim_x], p[dim_y]) != ter_mart) &&
          (mapxy(p[dim_x], p[dim_y]) != ter_center) &&
          (mapxy(p[dim_x] + 1, p[dim_y]) != ter_mart) &&
          (mapxy(p[dim_x] + 1, p[dim_y]) != ter_center) &&
          (mapxy(p[dim_x], p[dim_y] + 1) != ter_mart) &&
          (mapxy(p[dim_x], p[dim_y] + 1) != ter_center) &&
          (mapxy(p[dim_x] + 1, p[dim_y] + 1) != ter_mart) &&
          (mapxy(p[dim_x] + 1, p[dim_y] + 1) != ter_center))) &&
        (((mapxy(p[dim_x], p[dim_y]) != ter_path) &&
          (mapxy(p[dim_x] + 1, p[dim_y]) != ter_path) &&
          (mapxy(p[dim_x], p[dim_y] + 1) != ter_path) &&
          (mapxy(p[dim_x] + 1, p[dim_y] + 1) != ter_path))))
    {
      break;
    }
  } while (1);
}

static int place_pokemart(map_t *m)
{
  pair_t p;

  find_building_location(m, p);

  mapxy(p[dim_x], p[dim_y]) = ter_mart;
  mapxy(p[dim_x] + 1, p[dim_y]) = ter_mart;
  mapxy(p[dim_x], p[dim_y] + 1) = ter_mart;
  mapxy(p[dim_x] + 1, p[dim_y] + 1) = ter_mart;

  return 0;
}

static int place_center(map_t *m)
{
  pair_t p;

  find_building_location(m, p);

  mapxy(p[dim_x], p[dim_y]) = ter_center;
  mapxy(p[dim_x] + 1, p[dim_y]) = ter_center;
  mapxy(p[dim_x], p[dim_y] + 1) = ter_center;
  mapxy(p[dim_x] + 1, p[dim_y] + 1) = ter_center;

  return 0;
}

/* Chooses tree or boulder for border cell.  Choice is biased by dominance *
 * of neighboring cells.                                                   */
static terrain_type_t border_type(map_t *m, int32_t x, int32_t y)
{
  int32_t p, q;
  int32_t r, t;
  int32_t miny, minx, maxy, maxx;

  r = t = 0;

  miny = y - 1 >= 0 ? y - 1 : 0;
  maxy = y + 1 <= MAP_Y ? y + 1 : MAP_Y;
  minx = x - 1 >= 0 ? x - 1 : 0;
  maxx = x + 1 <= MAP_X ? x + 1 : MAP_X;

  for (q = miny; q < maxy; q++)
  {
    for (p = minx; p < maxx; p++)
    {
      if (q != y || p != x)
      {
        if (m->map[q][p] == ter_mountain ||
            m->map[q][p] == ter_boulder)
        {
          r++;
        }
        else if (m->map[q][p] == ter_forest ||
                 m->map[q][p] == ter_tree)
        {
          t++;
        }
      }
    }
  }

  if (t == r)
  {
    return rand() & 1 ? ter_boulder : ter_tree;
  }
  else if (t > r)
  {
    if (rand() % 10)
    {
      return ter_tree;
    }
    else
    {
      return ter_boulder;
    }
  }
  else
  {
    if (rand() % 10)
    {
      return ter_boulder;
    }
    else
    {
      return ter_tree;
    }
  }
}

static int map_terrain(map_t *m, int8_t n, int8_t s, int8_t e, int8_t w)
{
  int32_t i, x, y;
  queue_node_t *head, *tail, *tmp;
  //  FILE *out;
  int num_grass, num_clearing, num_mountain, num_forest, num_water, num_total;
  terrain_type_t type;
  int added_current = 0;

  num_grass = rand() % 4 + 2;
  num_clearing = rand() % 4 + 2;
  num_mountain = rand() % 2 + 1;
  num_forest = rand() % 2 + 1;
  num_water = rand() % 2 + 1;
  num_total = num_grass + num_clearing + num_mountain + num_forest + num_water;

  memset(&m->map, 0, sizeof(m->map));

  /* Seed with some values */
  for (i = 0; i < num_total; i++)
  {
    do
    {
      x = rand() % MAP_X;
      y = rand() % MAP_Y;
    } while (m->map[y][x]);
    if (i == 0)
    {
      type = ter_grass;
    }
    else if (i == num_grass)
    {
      type = ter_clearing;
    }
    else if (i == num_grass + num_clearing)
    {
      type = ter_mountain;
    }
    else if (i == num_grass + num_clearing + num_mountain)
    {
      type = ter_forest;
    }
    else if (i == num_grass + num_clearing + num_mountain + num_forest)
    {
      type = ter_water;
    }
    m->map[y][x] = type;
    if (i == 0)
    {
      head = tail = malloc(sizeof(*tail));
    }
    else
    {
      tail->next = malloc(sizeof(*tail));
      tail = tail->next;
    }
    tail->next = NULL;
    tail->x = x;
    tail->y = y;
  }

  /*
  out = fopen("seeded.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", MAP_X, MAP_Y);
  fwrite(&m->map, sizeof (m->map), 1, out);
  fclose(out);
  */

  /* Diffuse the vaules to fill the space */
  while (head)
  {
    x = head->x;
    y = head->y;
    i = m->map[y][x];

    if (x - 1 >= 0 && !m->map[y][x - 1])
    {
      if ((rand() % 100) < 80)
      {
        m->map[y][x - 1] = i;
        tail->next = malloc(sizeof(*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x - 1;
        tail->y = y;
      }
      else if (!added_current)
      {
        added_current = 1;
        m->map[y][x] = i;
        tail->next = malloc(sizeof(*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
      }
    }

    if (y - 1 >= 0 && !m->map[y - 1][x])
    {
      if ((rand() % 100) < 20)
      {
        m->map[y - 1][x] = i;
        tail->next = malloc(sizeof(*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y - 1;
      }
      else if (!added_current)
      {
        added_current = 1;
        m->map[y][x] = i;
        tail->next = malloc(sizeof(*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
      }
    }

    if (y + 1 < MAP_Y && !m->map[y + 1][x])
    {
      if ((rand() % 100) < 20)
      {
        m->map[y + 1][x] = i;
        tail->next = malloc(sizeof(*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y + 1;
      }
      else if (!added_current)
      {
        added_current = 1;
        m->map[y][x] = i;
        tail->next = malloc(sizeof(*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
      }
    }

    if (x + 1 < MAP_X && !m->map[y][x + 1])
    {
      if ((rand() % 100) < 80)
      {
        m->map[y][x + 1] = i;
        tail->next = malloc(sizeof(*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x + 1;
        tail->y = y;
      }
      else if (!added_current)
      {
        added_current = 1;
        m->map[y][x] = i;
        tail->next = malloc(sizeof(*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
      }
    }

    added_current = 0;
    tmp = head;
    head = head->next;
    free(tmp);
  }

  /*
  out = fopen("diffused.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", MAP_X, MAP_Y);
  fwrite(&m->map, sizeof (m->map), 1, out);
  fclose(out);
  */

  for (y = 0; y < MAP_Y; y++)
  {
    for (x = 0; x < MAP_X; x++)
    {
      if (y == 0 || y == MAP_Y - 1 ||
          x == 0 || x == MAP_X - 1)
      {
        mapxy(x, y) = border_type(m, x, y);
      }
    }
  }

  m->n = n;
  m->s = s;
  m->e = e;
  m->w = w;

  if (n != -1)
  {
    mapxy(n, 0) = ter_gate;
    mapxy(n, 1) = ter_gate;
  }
  if (s != -1)
  {
    mapxy(s, MAP_Y - 1) = ter_gate;
    mapxy(s, MAP_Y - 2) = ter_gate;
  }
  if (w != -1)
  {
    mapxy(0, w) = ter_gate;
    mapxy(1, w) = ter_gate;
  }
  if (e != -1)
  {
    mapxy(MAP_X - 1, e) = ter_gate;
    mapxy(MAP_X - 2, e) = ter_gate;
  }

  return 0;
}

static int place_boulders(map_t *m)
{
  int i;
  int x, y;

  for (i = 0; i < MIN_BOULDERS || rand() % 100 < BOULDER_PROB; i++)
  {
    y = rand() % (MAP_Y - 2) + 1;
    x = rand() % (MAP_X - 2) + 1;
    if (m->map[y][x] != ter_forest &&
        m->map[y][x] != ter_path &&
        m->map[y][x] != ter_gate)
    {
      m->map[y][x] = ter_boulder;
    }
  }

  return 0;
}

static int place_trees(map_t *m)
{
  int i;
  int x, y;

  for (i = 0; i < MIN_TREES || rand() % 100 < TREE_PROB; i++)
  {
    y = rand() % (MAP_Y - 2) + 1;
    x = rand() % (MAP_X - 2) + 1;
    if (m->map[y][x] != ter_mountain &&
        m->map[y][x] != ter_path &&
        m->map[y][x] != ter_water &&
        m->map[y][x] != ter_gate)
    {
      m->map[y][x] = ter_tree;
    }
  }

  return 0;
}

// New map expects cur_idx to refer to the index to be generated.  If that
// map has already been generated then the only thing this does is set
// cur_map.
static int new_map()
{
  int d, p;
  int e, w, n, s;

  if (world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x]])
  {
    world.cur_map = world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x]];
    return 0;
  }

  world.cur_map =
      world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x]] =
          malloc(sizeof(*world.cur_map));

  smooth_height(world.cur_map);

  if (!world.cur_idx[dim_y])
  {
    n = -1;
  }
  else if (world.world[world.cur_idx[dim_y] - 1][world.cur_idx[dim_x]])
  {
    n = world.world[world.cur_idx[dim_y] - 1][world.cur_idx[dim_x]]->s;
  }
  else
  {
    n = 1 + rand() % (MAP_X - 2);
  }
  if (world.cur_idx[dim_y] == WORLD_SIZE - 1)
  {
    s = -1;
  }
  else if (world.world[world.cur_idx[dim_y] + 1][world.cur_idx[dim_x]])
  {
    s = world.world[world.cur_idx[dim_y] + 1][world.cur_idx[dim_x]]->n;
  }
  else
  {
    s = 1 + rand() % (MAP_X - 2);
  }
  if (!world.cur_idx[dim_x])
  {
    w = -1;
  }
  else if (world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x] - 1])
  {
    w = world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x] - 1]->e;
  }
  else
  {
    w = 1 + rand() % (MAP_Y - 2);
  }
  if (world.cur_idx[dim_x] == WORLD_SIZE - 1)
  {
    e = -1;
  }
  else if (world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x] + 1])
  {
    e = world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x] + 1]->w;
  }
  else
  {
    e = 1 + rand() % (MAP_Y - 2);
  }

  map_terrain(world.cur_map, n, s, e, w);

  place_boulders(world.cur_map);
  place_trees(world.cur_map);
  build_paths(world.cur_map);
  d = (abs(world.cur_idx[dim_x] - (WORLD_SIZE / 2)) +
       abs(world.cur_idx[dim_y] - (WORLD_SIZE / 2)));
  p = d > 200 ? 5 : (50 - ((45 * d) / 200));
  //  printf("d=%d, p=%d\n", d, p);
  if ((rand() % 100) < p || !d)
  {
    place_pokemart(world.cur_map);
  }
  if ((rand() % 100) < p || !d)
  {
    place_center(world.cur_map);
  }

  return 0;
}

static void print_map()
{
  int x, y;
  int default_reached = 0;

  printf("\n");

  for (y = 0; y < MAP_Y; y++)
  {
    for (x = 0; x < MAP_X; x++)
    {
      if (world.pc.pos[dim_y] == y &&
          world.pc.pos[dim_x] == x)
      {
        printf(GRN "@" RESET);
      }
      else
      {
        switch (world.cur_map->map[y][x])
        {
        case ter_boulder:
          putchar(BOULDER_SYMBOL);
          break;
        case ter_mountain:
          putchar(MOUNTAIN_SYMBOL);
          break;
        case ter_tree:
          putchar(TREE_SYMBOL);
          break;
        case ter_forest:
          putchar(FOREST_SYMBOL);
          break;
        case ter_path:
          printf(YEL "#" RESET);
          break;
        case ter_gate:
          printf(YEL "#" RESET);
          break;
        case ter_mart:
          printf(CYN "M" RESET);
          break;
        case ter_center:
          printf(CYN "C" RESET);
          break;
        case ter_grass:
          putchar(TALL_GRASS_SYMBOL);
          break;
        case ter_clearing:
          putchar(SHORT_GRASS_SYMBOL);
          break;
        case ter_water:
          printf(BLU "~" RESET);
          break;
        case ter_hiker:
          printf(RED "h" RESET);
          break;
        case ter_rival:
          printf(RED "r" RESET);
          break;
        case ter_pacer:
          printf(RED "p" RESET);
          break;
        case ter_wonder:
          printf(RED "w" RESET);;
          break;
        case ter_sentry:
          printf(RED "s" RESET);
          break;
        case ter_exploer:
          printf(RED "e" RESET);
          break;
        default:
          putchar(ERROR_SYMBOL);
          default_reached = 1;
          break;
        }
      }
    }
    putchar('\n');
  }

  if (default_reached)
  {
    fprintf(stderr, "Default reached in %s\n", __FUNCTION__);
  }
}

// The world is global because of its size, so init_world is parameterless
void init_world()
{
  world.cur_idx[dim_x] = world.cur_idx[dim_y] = WORLD_SIZE / 2;
  new_map();
}

void delete_world()
{
  int x, y;

  for (y = 0; y < WORLD_SIZE; y++)
  {
    for (x = 0; x < WORLD_SIZE; x++)
    {
      if (world.world[y][x])
      {
        free(world.world[y][x]);
        world.world[y][x] = NULL;
      }
    }
  }
}

#define ter_cost(x, y, c) move_cost[c][m->map[y][x]]

static int32_t hiker_cmp(const void *key, const void *with)
{
  return (world.hiker_dist[((path_t *)key)->pos[dim_y]]
                          [((path_t *)key)->pos[dim_x]] -
          world.hiker_dist[((path_t *)with)->pos[dim_y]]
                          [((path_t *)with)->pos[dim_x]]);
}

static int32_t rival_cmp(const void *key, const void *with)
{
  return (world.rival_dist[((path_t *)key)->pos[dim_y]]
                          [((path_t *)key)->pos[dim_x]] -
          world.rival_dist[((path_t *)with)->pos[dim_y]]
                          [((path_t *)with)->pos[dim_x]]);
}

void pathfind(map_t *m)
{
  heap_t h;
  uint32_t x, y;
  static path_t p[MAP_Y][MAP_X], *c;
  static uint32_t initialized = 0;

  if (!initialized)
  {
    initialized = 1;
    for (y = 0; y < MAP_Y; y++)
    {
      for (x = 0; x < MAP_X; x++)
      {
        p[y][x].pos[dim_y] = y;
        p[y][x].pos[dim_x] = x;
      }
    }
  }

  for (y = 0; y < MAP_Y; y++)
  {
    for (x = 0; x < MAP_X; x++)
    {
      world.hiker_dist[y][x] = world.rival_dist[y][x] = DIJKSTRA_PATH_MAX;
    }
  }
  world.hiker_dist[world.pc.pos[dim_y]][world.pc.pos[dim_x]] =
      world.rival_dist[world.pc.pos[dim_y]][world.pc.pos[dim_x]] = 0;

  heap_init(&h, hiker_cmp, NULL);

  for (y = 1; y < MAP_Y - 1; y++)
  {
    for (x = 1; x < MAP_X - 1; x++)
    {
      if (ter_cost(x, y, char_hiker) != DIJKSTRA_PATH_MAX)
      {
        p[y][x].hn = heap_insert(&h, &p[y][x]);
      }
      else
      {
        p[y][x].hn = NULL;
      }
    }
  }

  while ((c = heap_remove_min(&h)))
  {
    c->hn = NULL;
    if ((p[c->pos[dim_y] - 1][c->pos[dim_x] - 1].hn) &&
        (world.hiker_dist[c->pos[dim_y] - 1][c->pos[dim_x] - 1] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
             ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker)))
    {
      world.hiker_dist[c->pos[dim_y] - 1][c->pos[dim_x] - 1] =
          world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
          ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] - 1][c->pos[dim_x] - 1].hn);
    }
    if ((p[c->pos[dim_y] - 1][c->pos[dim_x]].hn) &&
        (world.hiker_dist[c->pos[dim_y] - 1][c->pos[dim_x]] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
             ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker)))
    {
      world.hiker_dist[c->pos[dim_y] - 1][c->pos[dim_x]] =
          world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
          ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] - 1][c->pos[dim_x]].hn);
    }
    if ((p[c->pos[dim_y] - 1][c->pos[dim_x] + 1].hn) &&
        (world.hiker_dist[c->pos[dim_y] - 1][c->pos[dim_x] + 1] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
             ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker)))
    {
      world.hiker_dist[c->pos[dim_y] - 1][c->pos[dim_x] + 1] =
          world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
          ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] - 1][c->pos[dim_x] + 1].hn);
    }
    if ((p[c->pos[dim_y]][c->pos[dim_x] - 1].hn) &&
        (world.hiker_dist[c->pos[dim_y]][c->pos[dim_x] - 1] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
             ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker)))
    {
      world.hiker_dist[c->pos[dim_y]][c->pos[dim_x] - 1] =
          world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
          ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y]][c->pos[dim_x] - 1].hn);
    }
    if ((p[c->pos[dim_y]][c->pos[dim_x] + 1].hn) &&
        (world.hiker_dist[c->pos[dim_y]][c->pos[dim_x] + 1] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
             ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker)))
    {
      world.hiker_dist[c->pos[dim_y]][c->pos[dim_x] + 1] =
          world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
          ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y]][c->pos[dim_x] + 1].hn);
    }
    if ((p[c->pos[dim_y] + 1][c->pos[dim_x] - 1].hn) &&
        (world.hiker_dist[c->pos[dim_y] + 1][c->pos[dim_x] - 1] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
             ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker)))
    {
      world.hiker_dist[c->pos[dim_y] + 1][c->pos[dim_x] - 1] =
          world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
          ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] + 1][c->pos[dim_x] - 1].hn);
    }
    if ((p[c->pos[dim_y] + 1][c->pos[dim_x]].hn) &&
        (world.hiker_dist[c->pos[dim_y] + 1][c->pos[dim_x]] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
             ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker)))
    {
      world.hiker_dist[c->pos[dim_y] + 1][c->pos[dim_x]] =
          world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
          ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] + 1][c->pos[dim_x]].hn);
    }
    if ((p[c->pos[dim_y] + 1][c->pos[dim_x] + 1].hn) &&
        (world.hiker_dist[c->pos[dim_y] + 1][c->pos[dim_x] + 1] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
             ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker)))
    {
      world.hiker_dist[c->pos[dim_y] + 1][c->pos[dim_x] + 1] =
          world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
          ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] + 1][c->pos[dim_x] + 1].hn);
    }
  }
  heap_delete(&h);

  heap_init(&h, rival_cmp, NULL);

  for (y = 1; y < MAP_Y - 1; y++)
  {
    for (x = 1; x < MAP_X - 1; x++)
    {
      if (ter_cost(x, y, char_rival) != DIJKSTRA_PATH_MAX)
      {
        p[y][x].hn = heap_insert(&h, &p[y][x]);
      }
      else
      {
        p[y][x].hn = NULL;
      }
    }
  }

  while ((c = heap_remove_min(&h)))
  {
    c->hn = NULL;
    if ((p[c->pos[dim_y] - 1][c->pos[dim_x] - 1].hn) &&
        (world.rival_dist[c->pos[dim_y] - 1][c->pos[dim_x] - 1] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
             ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival)))
    {
      world.rival_dist[c->pos[dim_y] - 1][c->pos[dim_x] - 1] =
          world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
          ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] - 1][c->pos[dim_x] - 1].hn);
    }
    if ((p[c->pos[dim_y] - 1][c->pos[dim_x]].hn) &&
        (world.rival_dist[c->pos[dim_y] - 1][c->pos[dim_x]] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
             ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival)))
    {
      world.rival_dist[c->pos[dim_y] - 1][c->pos[dim_x]] =
          world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
          ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] - 1][c->pos[dim_x]].hn);
    }
    if ((p[c->pos[dim_y] - 1][c->pos[dim_x] + 1].hn) &&
        (world.rival_dist[c->pos[dim_y] - 1][c->pos[dim_x] + 1] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
             ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival)))
    {
      world.rival_dist[c->pos[dim_y] - 1][c->pos[dim_x] + 1] =
          world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
          ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] - 1][c->pos[dim_x] + 1].hn);
    }
    if ((p[c->pos[dim_y]][c->pos[dim_x] - 1].hn) &&
        (world.rival_dist[c->pos[dim_y]][c->pos[dim_x] - 1] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
             ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival)))
    {
      world.rival_dist[c->pos[dim_y]][c->pos[dim_x] - 1] =
          world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
          ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y]][c->pos[dim_x] - 1].hn);
    }
    if ((p[c->pos[dim_y]][c->pos[dim_x] + 1].hn) &&
        (world.rival_dist[c->pos[dim_y]][c->pos[dim_x] + 1] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
             ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival)))
    {
      world.rival_dist[c->pos[dim_y]][c->pos[dim_x] + 1] =
          world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
          ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y]][c->pos[dim_x] + 1].hn);
    }
    if ((p[c->pos[dim_y] + 1][c->pos[dim_x] - 1].hn) &&
        (world.rival_dist[c->pos[dim_y] + 1][c->pos[dim_x] - 1] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
             ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival)))
    {
      world.rival_dist[c->pos[dim_y] + 1][c->pos[dim_x] - 1] =
          world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
          ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] + 1][c->pos[dim_x] - 1].hn);
    }
    if ((p[c->pos[dim_y] + 1][c->pos[dim_x]].hn) &&
        (world.rival_dist[c->pos[dim_y] + 1][c->pos[dim_x]] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
             ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival)))
    {
      world.rival_dist[c->pos[dim_y] + 1][c->pos[dim_x]] =
          world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
          ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] + 1][c->pos[dim_x]].hn);
    }
    if ((p[c->pos[dim_y] + 1][c->pos[dim_x] + 1].hn) &&
        (world.rival_dist[c->pos[dim_y] + 1][c->pos[dim_x] + 1] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
             ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival)))
    {
      world.rival_dist[c->pos[dim_y] + 1][c->pos[dim_x] + 1] =
          world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
          ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] + 1][c->pos[dim_x] + 1].hn);
    }
  }
  heap_delete(&h);
}

void init_pc()
{
  int x, y;

  do
  {
    x = rand() % (MAP_X - 2) + 1;
    y = rand() % (MAP_Y - 2) + 1;
  } while (world.cur_map->map[y][x] != ter_path);

  world.pc.pos[dim_x] = x;
  world.pc.pos[dim_y] = y;
}

void print_hiker_dist()
{
  int x, y;

  for (y = 0; y < MAP_Y; y++)
  {
    for (x = 0; x < MAP_X; x++)
    {
      if (world.hiker_dist[y][x] == DIJKSTRA_PATH_MAX)
      {
        printf("   ");
      }
      else
      {
        printf(" %02d", world.hiker_dist[y][x] % 100);
      }
    }
    printf("\n");
  }
}

void print_rival_dist()
{
  int x, y;

  for (y = 0; y < MAP_Y; y++)
  {
    for (x = 0; x < MAP_X; x++)
    {
      if (world.rival_dist[y][x] == DIJKSTRA_PATH_MAX ||
          world.rival_dist[y][x] < 0)
      {
        printf("   ");
      }
      else
      {
        printf(" %02d", world.rival_dist[y][x] % 100);
      }
    }
    printf("\n");
  }
}

void place_hiker(map_t *m){
  int x,y;

  y = rand() % (MAP_Y - 2) + 1;
  x = rand() % (MAP_X - 2) + 1;

  if (m->map[y][x] != ter_water &&
      m->map[y][x] != ter_gate &&
      m->map[y][x] != ter_rival &&
      m->map[y][x] != ter_hiker &&
      m->map[y][x] != ter_exploer &&
      m->map[y][x] != ter_wonder &&
      m->map[y][x] != ter_sentry &&
      m->map[y][x] != ter_pacer &&
      y != world.pc.pos[0] &&
      x != world.pc.pos[1] &&
      global_count <= MAX_CHAR)
  {
    cahr_t h;
    h.pos[0] = x;
    h.pos[1] = y;
    h.prevous = m->map[y][x];
    h.symbol = 'h';

     m->map[y][x] = ter_hiker;
    world.cur_map->cahr[global_count] = h;
    global_count++;
  }
}

void place_rival(map_t *m)
{
  int x, y;

  y = rand() % (MAP_Y - 2) + 1;
  x = rand() % (MAP_X - 2) + 1;

  if (m->map[y][x] != ter_water &&
      m->map[y][x] != ter_gate &&
      m->map[y][x] != ter_rival &&
      m->map[y][x] != ter_hiker &&
      m->map[y][x] != ter_exploer &&
      m->map[y][x] != ter_wonder &&
      m->map[y][x] != ter_sentry &&
      m->map[y][x] != ter_pacer &&
      m->map[y][x] != ter_mountain &&
      m->map[y][x] != ter_boulder &&
      m->map[y][x] != ter_tree &&
      y != world.pc.pos[0] &&
      x != world.pc.pos[1] &&
      global_count <= MAX_CHAR)
  {
    cahr_t h;
    h.pos[0] = x;
    h.pos[1] = y;
    h.prevous = m->map[y][x];
    h.symbol = 'r';
    h.dead = 0;

    m->map[y][x] = ter_rival;
    world.cur_map->cahr[global_count] = h;
    global_count++;
  }
}

void place_wonder(map_t *m)
{
  int x, y;

  y = rand() % (MAP_Y - 2) + 1;
  x = rand() % (MAP_X - 2) + 1;

  if (m->map[y][x] != ter_water &&
      m->map[y][x] != ter_gate &&
      m->map[y][x] != ter_rival &&
      m->map[y][x] != ter_rival &&
      m->map[y][x] != ter_hiker &&
      m->map[y][x] != ter_exploer &&
      m->map[y][x] != ter_wonder &&
      m->map[y][x] != ter_sentry &&
      m->map[y][x] != ter_pacer &&
      m->map[y][x] != ter_mountain &&
      m->map[y][x] != ter_boulder &&
      m->map[y][x] != ter_tree &&
      y != world.pc.pos[0] &&
      x != world.pc.pos[1] &&
      global_count <= MAX_CHAR)
  {
    cahr_t h;
    h.pos[0] = x;
    h.pos[1] = y;
    h.prevous = m->map[y][x];
    h.symbol = 'w';
h.dead = 0;
    m->map[y][x] = ter_wonder;
    world.cur_map->cahr[global_count] = h;
    global_count++;
  }
}

void place_sentry(map_t *m)
{
  int x, y;

  y = rand() % (MAP_Y - 2) + 1;
  x = rand() % (MAP_X - 2) + 1;

  if (m->map[y][x] != ter_water &&
      m->map[y][x] != ter_gate &&
      m->map[y][x] != ter_rival &&
      m->map[y][x] != ter_rival &&
      m->map[y][x] != ter_hiker &&
      m->map[y][x] != ter_exploer &&
      m->map[y][x] != ter_wonder &&
      m->map[y][x] != ter_sentry &&
      m->map[y][x] != ter_pacer &&
      m->map[y][x] != ter_mountain &&
      m->map[y][x] != ter_boulder &&
      m->map[y][x] != ter_tree &&

      y != world.pc.pos[0] &&
      x != world.pc.pos[1] &&
      global_count <= MAX_CHAR)
  {
    cahr_t h;
    h.pos[0] = x;
    h.pos[1] = y;
    h.prevous = m->map[y][x];
    h.symbol = 's';
h.dead = 0;
    m->map[y][x] = ter_sentry;
    world.cur_map->cahr[global_count] = h;

    global_count++;
  }
}

void place_exploer(map_t *m)
{
  int x, y;

  y = rand() % (MAP_Y - 2) + 1;
  x = rand() % (MAP_X - 2) + 1;

  if (m->map[y][x] != ter_water &&
      m->map[y][x] != ter_gate &&
      m->map[y][x] != ter_rival &&
      m->map[y][x] != ter_rival &&
      m->map[y][x] != ter_hiker &&
      m->map[y][x] != ter_exploer &&
      m->map[y][x] != ter_wonder &&
      m->map[y][x] != ter_sentry &&
      m->map[y][x] != ter_pacer &&
      m->map[y][x] != ter_mountain &&
      m->map[y][x] != ter_forest &&
      m->map[y][x] != ter_boulder &&
      m->map[y][x] != ter_tree &&
      y != world.pc.pos[0] &&
      x != world.pc.pos[1] &&
      global_count <= MAX_CHAR)
  {
    cahr_t h;
    h.pos[0] = x;
    h.pos[1] = y;
    h.prevous = m->map[y][x];
    h.symbol = 'e';
h.dead = 0;
    m->map[y][x] = ter_exploer;
    world.cur_map->cahr[global_count] = h;
    global_count++;
  }
}

void place_pacer(map_t *m)
{
  int x, y;

  y = rand() % (MAP_Y - 2) + 1;
  x = rand() % (MAP_X - 2) + 1;

  if (m->map[y][x] != ter_water &&
      m->map[y][x] != ter_gate &&
      m->map[y][x] != ter_rival &&
      m->map[y][x] != ter_rival &&
      m->map[y][x] != ter_hiker &&
      m->map[y][x] != ter_exploer &&
      m->map[y][x] != ter_wonder &&
      m->map[y][x] != ter_sentry &&
      m->map[y][x] != ter_pacer &&
      m->map[y][x] != ter_mountain &&
      m->map[y][x] != ter_boulder &&
      m->map[y][x] != ter_tree &&
      y != world.pc.pos[0] &&
      x != world.pc.pos[1] &&
      global_count <= MAX_CHAR)
  {
    cahr_t h;
    h.pos[0] = x;
    h.pos[1] = y;
    h.prevous = m->map[y][x];
    h.symbol = 'p';
h.dead = 0;
    m->map[y][x] = ter_pacer;
    world.cur_map->cahr[global_count] = h;
    global_count++;
  }
}

int peek(PQ_t *pqr);

// 0 for same - 1 for not - if 2 for i2 seq_num less - if 3 for i1 seq_num less
int itemcomp(item_t i1, item_t i2)
{
  if (i1.symbol == i2.symbol &&
      i1.next_turn == i2.next_turn &&
      i1.seq_num == i2.seq_num)
  { // equal
    return 1;
  }

  else if (i1.next_turn == i2.next_turn &&
           i1.seq_num < i2.seq_num)
  { // i2 is less
    return 2;
  }

  else if (i1.next_turn == i2.next_turn &&
           i1.seq_num > i2.seq_num)
  { // i1 is less
    return 3;
  }

  return 1; // not the same atall
}

void PQ_init(PQ_t *pqr)
{
  pqr->size = 0;
}

void enque(PQ_t *pqr, char symbol, int next_turn, int seq_num, cahr_t c)
{
  int size = pqr->size;

  pqr->items[size].next_turn = next_turn;
  pqr->items[size].symbol = symbol;
  pqr->items[size].seq_num = seq_num;
  pqr->items[size].c = c;

  pqr->size++;
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

void place_npc(int num_npc){
  // for(int i = 0; i < num_npc/2; i++){
  //   place_rival(world.cur_map);
  //   place_hiker(world.cur_map);
  // }
  while(global_count < num_npc - 1){
    place_rival(world.cur_map);
    place_hiker(world.cur_map);
    place_exploer(world.cur_map);
    place_pacer(world.cur_map);
    place_sentry(world.cur_map);
    place_wonder(world.cur_map);
  }
}

int no_npc(terrain_type_t ter_type){

  switch (ter_type)
  {
  case ter_pacer:
    printf("\nThere is a pacer in the way\n");
    return 0;
    break;
  case ter_exploer:
    printf("\nThere is a expoler in the way\n");
    return 0;
    break;
  case ter_wonder:
    printf("\nThere is a wander in the way\n");
    return 0;
    break;
  case ter_hiker:
    printf("\nThere is a hiker in the way\n");
    return 0;
    break;
  case ter_rival:

    printf("\nThere is a rival in the way\n");
    return 0;
    break;
  default:
    break;
  }

  return 1;
}

int player_no_npc(terrain_type_t ter_type){

  switch (ter_type)
  {
  case ter_pacer:
    printf("\nThere is a pacer in the way\n");
    return 0;
    break;
  case ter_exploer:
    printf("\nThere is a expoler in the way\n");
    return 0;
    break;
  case ter_wonder:
    printf("\nThere is a wander in the way\n");
    return 0;
    break;
  case ter_hiker:
    printf("\nThere is a hiker in the way\n");
    return 0;
    break;
  case ter_rival:

    printf("\nThere is a rival in the way\n");
    return 0;
    break;
  default:
    break;
  }

  return 1;
}

void checkbattle(int y, int x, cahr_t *person){
    char answer = 'f';

  if(y == world.pc.pos[dim_y] && x == world.pc.pos[dim_x] && person->dead != 1){

    person->dead = 1;

    while (answer != 'q')
    {
      printf("this is a battle press q: ");
      scanf("%c", &answer);
    } 
  }

}


//pass the next y or x to check if npc is there and not dead
void pc_checkbattle(int y, int x){
    char answer = 'f';

  if(world.cur_map->map[y][x] == ter_hiker ||
    world.cur_map->map[y][x] == ter_rival ||
    world.cur_map->map[y][x] == ter_pacer ||
    world.cur_map->map[y][x] == ter_wonder ||
    world.cur_map->map[y][x] == ter_exploer ||
    world.cur_map->map[y][x] == ter_sentry
    ){

      for(int i = 0; i < 50; i++){
        if(world.cur_map->cahr[i].pos[dim_y] == y && world.cur_map->cahr[i].pos[dim_x] == x){
               world.cur_map->cahr[i].dead = 1;
               break;
        }
      }

    while (answer != 'q')
    {
      printf("this is a battle press q: ");
      scanf("%c", &answer);
    } 
  }

}

// int checkplayer(int y, int x){
//   if((y != world.pc.pos[dim_y]) && (x != world.pc.pos[dim_x])){return 1;} //if player and npc can fight they will

//   return 0;
// }

int move_hiker(item_t *person){
  pair_t cheapest;
  item_t cur_person = *person;

  cheapest[dim_y] = cur_person.c.pos[dim_y];
  cheapest[dim_x] = cur_person.c.pos[dim_x];

  int cheap = SHRT_MAX; 

  //printf("c1=%d//", cheap);

  if (world.hiker_dist[cur_person.c.pos[dim_y] + 1][cur_person.c.pos[dim_x]] < cheap 
       && (cur_person.c.pos[dim_y] + 1) != 0 && (cur_person.c.pos[dim_x]) != 0
       && (cur_person.c.pos[dim_y] + 1) != MAP_Y && (cur_person.c.pos[dim_x]) != MAP_X 
       && no_npc(world.cur_map->map[cur_person.c.pos[dim_y] + 1][cur_person.c.pos[dim_x]])) 
    { // S
      checkbattle(cur_person.c.pos[dim_y] + 1, cur_person.c.pos[dim_x], &cur_person.c);
      cheapest[dim_y] = cur_person.c.pos[dim_y] + 1;
      cheapest[dim_x] = cur_person.c.pos[dim_x];
      cheap = world.hiker_dist[cur_person.c.pos[dim_y] + 1][cur_person.c.pos[dim_x]];
      
  }

  if (world.hiker_dist[cur_person.c.pos[dim_y] - 1][cur_person.c.pos[dim_x]] < cheap
         && (cur_person.c.pos[dim_y] - 1) != 0 && (cur_person.c.pos[dim_x]) != 0
       && (cur_person.c.pos[dim_y] - 1) != MAP_Y && (cur_person.c.pos[dim_x]) != MAP_X
      && no_npc(world.cur_map->map[cur_person.c.pos[dim_y] - 1][cur_person.c.pos[dim_x]]))
  { // N
    checkbattle(cur_person.c.pos[dim_y] - 1, cur_person.c.pos[dim_x], &cur_person.c);
    cheapest[dim_y] = cur_person.c.pos[dim_y] - 1;
    cheapest[dim_x] = cur_person.c.pos[dim_x];
    cheap = world.hiker_dist[cur_person.c.pos[dim_y] - 1][cur_person.c.pos[dim_x]];
    
  }


  if (world.hiker_dist[cur_person.c.pos[dim_y] + 1][cur_person.c.pos[dim_x] - 1] < cheap 
       && (cur_person.c.pos[dim_y] + 1) != 0 && (cur_person.c.pos[dim_x] - 1) != 0
       && (cur_person.c.pos[dim_y] + 1) != MAP_Y && (cur_person.c.pos[dim_x] - 1) != MAP_X 
       && no_npc(world.cur_map->map[cur_person.c.pos[dim_y] + 1][cur_person.c.pos[dim_x] - 1])) 
    { // SW
      checkbattle(cur_person.c.pos[dim_y] + 1, cur_person.c.pos[dim_x] - 1, &cur_person.c);
      cheapest[dim_y] = cur_person.c.pos[dim_y] + 1;
      cheapest[dim_x] = cur_person.c.pos[dim_x] - 1;
      cheap = world.hiker_dist[cur_person.c.pos[dim_y] + 1][cur_person.c.pos[dim_x] - 1];
      
  }

  if (world.hiker_dist[cur_person.c.pos[dim_y] + 1][cur_person.c.pos[dim_x] + 1] < cheap 
       && (cur_person.c.pos[dim_y] + 1) != 0 && (cur_person.c.pos[dim_x] + 1) != 0
       && (cur_person.c.pos[dim_y] + 1) != MAP_Y && (cur_person.c.pos[dim_x] + 1) != MAP_X 
       && no_npc(world.cur_map->map[cur_person.c.pos[dim_y] + 1][cur_person.c.pos[dim_x] + 1])) 
    { // SE
      checkbattle(cur_person.c.pos[dim_y] + 1, cur_person.c.pos[dim_x] + 1, &cur_person.c);
      cheapest[dim_y] = cur_person.c.pos[dim_y] + 1;
      cheapest[dim_x] = cur_person.c.pos[dim_x] + 1;
      cheap = world.hiker_dist[cur_person.c.pos[dim_y] + 1][cur_person.c.pos[dim_x] + 1];
      
  }



  if (world.hiker_dist[cur_person.c.pos[dim_y] - 1][cur_person.c.pos[dim_x] + 1] < cheap
         && (cur_person.c.pos[dim_y] - 1) != 0 && (cur_person.c.pos[dim_x] + 1) != 0
       && (cur_person.c.pos[dim_y] - 1) != MAP_Y && (cur_person.c.pos[dim_x] + 1) != MAP_X
      && no_npc(world.cur_map->map[cur_person.c.pos[dim_y] - 1][cur_person.c.pos[dim_x] + 1]))
  { // NE
    checkbattle(cur_person.c.pos[dim_y] - 1, cur_person.c.pos[dim_x] + 1, &cur_person.c);
    cheapest[dim_y] = cur_person.c.pos[dim_y] - 1;
    cheapest[dim_x] = cur_person.c.pos[dim_x] + 1;
    cheap = world.hiker_dist[cur_person.c.pos[dim_y] - 1][cur_person.c.pos[dim_x] + 1];
    
  }

  if (world.hiker_dist[cur_person.c.pos[dim_y] - 1][cur_person.c.pos[dim_x] - 1] < cheap
         && (cur_person.c.pos[dim_y] - 1) != 0 && (cur_person.c.pos[dim_x] - 1) != 0
       && (cur_person.c.pos[dim_y] - 1) != MAP_Y && (cur_person.c.pos[dim_x] - 1) != MAP_X
      && no_npc(world.cur_map->map[cur_person.c.pos[dim_y] - 1][cur_person.c.pos[dim_x] - 1]))
  { // NW
    checkbattle(cur_person.c.pos[dim_y] - 1, cur_person.c.pos[dim_x] - 1, &cur_person.c);
    cheapest[dim_y] = cur_person.c.pos[dim_y] - 1;
    cheapest[dim_x] = cur_person.c.pos[dim_x] - 1;
    cheap = world.hiker_dist[cur_person.c.pos[dim_y] - 1][cur_person.c.pos[dim_x] - 1];
    
  }


  if (world.hiker_dist[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x] + 1] < cheap       
        && (cur_person.c.pos[dim_y]) != 0 && (cur_person.c.pos[dim_x] + 1) != 0
       && (cur_person.c.pos[dim_y]) != MAP_Y && (cur_person.c.pos[dim_x] + 1) != MAP_X
              && no_npc(world.cur_map->map[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x] + 1]))
  { // E
    checkbattle(cur_person.c.pos[dim_y], cur_person.c.pos[dim_x]+1, &cur_person.c);
    cheapest[dim_y] = cur_person.c.pos[dim_y];
    cheapest[dim_x] = cur_person.c.pos[dim_x] + 1;
    cheap = world.hiker_dist[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x] + 1];
    
  }

  if (world.hiker_dist[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x] - 1] < cheap
      && (cur_person.c.pos[dim_y]) != 0 && (cur_person.c.pos[dim_x] - 1) != 0
       && (cur_person.c.pos[dim_y]) != MAP_Y && (cur_person.c.pos[dim_x] - 1) != MAP_X
      && no_npc(world.cur_map->map[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x] - 1]))
  { // W
    checkbattle(cur_person.c.pos[dim_y], cur_person.c.pos[dim_x]-1, &cur_person.c);
    cheapest[dim_y] = cur_person.c.pos[dim_y];
    cheapest[dim_x] = cur_person.c.pos[dim_x] - 1;
    cheap = world.hiker_dist[cur_person.c.pos[dim_y] + 1][cur_person.c.pos[dim_x] - 1];
    
  }

  item_t t;
  cahr_t h;
  h.pos[dim_y] = cheapest[dim_y];
  h.pos[dim_x] = cheapest[dim_x];
  h.prevous = world.cur_map->map[cheapest[dim_y]][cheapest[dim_x]];
  h.symbol = cur_person.symbol;
  h.dead = cur_person.c.dead;

  t.c = h;
  t.next_turn = cur_person.next_turn;
  t.seq_num = cur_person.seq_num;
  t.symbol = cur_person.symbol;

  //printf("c2=%d//", cheap);
  world.cur_map->map[cheapest[dim_y]][cheapest[dim_x]] = ter_hiker;
  world.cur_map->map[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x]] = cur_person.c.prevous;

  

  *person = t;

  return world.hiker_dist[cheapest[dim_y]][cheapest[dim_x]] + cur_person.next_turn;
}

int move_rival(item_t *person){
  pair_t cheapest;
  item_t cur_person = *person;

  cheapest[dim_y] = cur_person.c.pos[dim_y];
  cheapest[dim_x] = cur_person.c.pos[dim_x];

  int cheap = SHRT_MAX; 

  if (world.rival_dist[cur_person.c.pos[dim_y] + 1][cur_person.c.pos[dim_x]] < cheap 
       && (cur_person.c.pos[dim_y] + 1) != 0 && (cur_person.c.pos[dim_x]) != 0
       && (cur_person.c.pos[dim_y] + 1) != MAP_Y && (cur_person.c.pos[dim_x]) != MAP_X 
       && no_npc(world.cur_map->map[cur_person.c.pos[dim_y] + 1][cur_person.c.pos[dim_x]])
       && checkplayer(cur_person.c.pos[dim_y] + 1, cur_person.c.pos[dim_x]))
    { // S
      checkbattle(cur_person.c.pos[dim_y] + 1, cur_person.c.pos[dim_x], &cur_person.c);
      cheapest[dim_y] = cur_person.c.pos[dim_y] + 1;
      cheapest[dim_x] = cur_person.c.pos[dim_x];
      cheap = world.rival_dist[cur_person.c.pos[dim_y] + 1][cur_person.c.pos[dim_x]];
      
  }

 if (world.hiker_dist[cur_person.c.pos[dim_y] + 1][cur_person.c.pos[dim_x] - 1] < cheap 
       && (cur_person.c.pos[dim_y] + 1) != 0 && (cur_person.c.pos[dim_x] - 1) != 0
       && (cur_person.c.pos[dim_y] + 1) != MAP_Y && (cur_person.c.pos[dim_x] - 1) != MAP_X 
       && no_npc(world.cur_map->map[cur_person.c.pos[dim_y] + 1][cur_person.c.pos[dim_x] - 1])) 
    { // SW
      checkbattle(cur_person.c.pos[dim_y] + 1, cur_person.c.pos[dim_x] - 1, &cur_person.c);
      cheapest[dim_y] = cur_person.c.pos[dim_y] + 1;
      cheapest[dim_x] = cur_person.c.pos[dim_x] - 1;
      cheap = world.hiker_dist[cur_person.c.pos[dim_y] + 1][cur_person.c.pos[dim_x] - 1];
      
  }

  if (world.hiker_dist[cur_person.c.pos[dim_y] + 1][cur_person.c.pos[dim_x] + 1] < cheap 
       && (cur_person.c.pos[dim_y] + 1) != 0 && (cur_person.c.pos[dim_x] + 1) != 0
       && (cur_person.c.pos[dim_y] + 1) != MAP_Y && (cur_person.c.pos[dim_x] + 1) != MAP_X 
       && no_npc(world.cur_map->map[cur_person.c.pos[dim_y] + 1][cur_person.c.pos[dim_x] + 1])) 
    { // SE
      checkbattle(cur_person.c.pos[dim_y] + 1, cur_person.c.pos[dim_x] + 1, &cur_person.c);
      cheapest[dim_y] = cur_person.c.pos[dim_y] + 1;
      cheapest[dim_x] = cur_person.c.pos[dim_x] + 1;
      cheap = world.hiker_dist[cur_person.c.pos[dim_y] + 1][cur_person.c.pos[dim_x] + 1];
      
  }



  if (world.hiker_dist[cur_person.c.pos[dim_y] - 1][cur_person.c.pos[dim_x] + 1] < cheap
         && (cur_person.c.pos[dim_y] - 1) != 0 && (cur_person.c.pos[dim_x] + 1) != 0
       && (cur_person.c.pos[dim_y] - 1) != MAP_Y && (cur_person.c.pos[dim_x] + 1) != MAP_X
      && no_npc(world.cur_map->map[cur_person.c.pos[dim_y] - 1][cur_person.c.pos[dim_x] + 1]))
  { // NE
    checkbattle(cur_person.c.pos[dim_y] - 1, cur_person.c.pos[dim_x] + 1, &cur_person.c);
    cheapest[dim_y] = cur_person.c.pos[dim_y] - 1;
    cheapest[dim_x] = cur_person.c.pos[dim_x] + 1;
    cheap = world.hiker_dist[cur_person.c.pos[dim_y] - 1][cur_person.c.pos[dim_x] + 1];
    
  }

  if (world.hiker_dist[cur_person.c.pos[dim_y] - 1][cur_person.c.pos[dim_x] - 1] < cheap
         && (cur_person.c.pos[dim_y] - 1) != 0 && (cur_person.c.pos[dim_x] - 1) != 0
       && (cur_person.c.pos[dim_y] - 1) != MAP_Y && (cur_person.c.pos[dim_x] - 1) != MAP_X
      && no_npc(world.cur_map->map[cur_person.c.pos[dim_y] - 1][cur_person.c.pos[dim_x] - 1]))
  { // NW
    checkbattle(cur_person.c.pos[dim_y] - 1, cur_person.c.pos[dim_x] - 1, &cur_person.c);
    cheapest[dim_y] = cur_person.c.pos[dim_y] - 1;
    cheapest[dim_x] = cur_person.c.pos[dim_x] - 1;
    cheap = world.hiker_dist[cur_person.c.pos[dim_y] - 1][cur_person.c.pos[dim_x] - 1];
    
  }


  if (world.rival_dist[cur_person.c.pos[dim_y] - 1][cur_person.c.pos[dim_x]] < cheap
         && (cur_person.c.pos[dim_y] - 1) != 0 && (cur_person.c.pos[dim_x]) != 0
       && (cur_person.c.pos[dim_y] - 1) != MAP_Y && (cur_person.c.pos[dim_x]) != MAP_X
      && no_npc(world.cur_map->map[cur_person.c.pos[dim_y] - 1][cur_person.c.pos[dim_x]])
       && checkplayer(cur_person.c.pos[dim_y] - 1, cur_person.c.pos[dim_x])
       )
  { // N
    checkbattle(cur_person.c.pos[dim_y] - 1, cur_person.c.pos[dim_x], &cur_person.c);
    cheapest[dim_y] = cur_person.c.pos[dim_y] - 1;
    cheapest[dim_x] = cur_person.c.pos[dim_x];
    cheap = world.rival_dist[cur_person.c.pos[dim_y] - 1][cur_person.c.pos[dim_x]];
    
  }

  if (world.rival_dist[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x] + 1] < cheap       
        && (cur_person.c.pos[dim_y]) != 0 && (cur_person.c.pos[dim_x] + 1) != 0
       && (cur_person.c.pos[dim_y]) != MAP_Y && (cur_person.c.pos[dim_x] + 1) != MAP_X
              && no_npc(world.cur_map->map[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x] + 1])
       && checkplayer(cur_person.c.pos[dim_y], cur_person.c.pos[dim_x] + 1))
  { // E
    checkbattle(cur_person.c.pos[dim_y], cur_person.c.pos[dim_x] + 1, &cur_person.c);
    cheapest[dim_y] = cur_person.c.pos[dim_y];
    cheapest[dim_x] = cur_person.c.pos[dim_x] + 1;
    cheap = world.rival_dist[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x] + 1];
    
  }

  if (world.rival_dist[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x] - 1] < cheap
      && (cur_person.c.pos[dim_y]) != 0 && (cur_person.c.pos[dim_x] - 1) != 0
       && (cur_person.c.pos[dim_y]) != MAP_Y && (cur_person.c.pos[dim_x] - 1) != MAP_X
      && no_npc(world.cur_map->map[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x] - 1])
       && checkplayer(cur_person.c.pos[dim_y], cur_person.c.pos[dim_x] - 1))
  { // W
    checkbattle(cur_person.c.pos[dim_y], cur_person.c.pos[dim_x] - 1, &cur_person.c);
    cheapest[dim_y] = cur_person.c.pos[dim_y];
    cheapest[dim_x] = cur_person.c.pos[dim_x] - 1;
    cheap = world.rival_dist[cur_person.c.pos[dim_y] + 1][cur_person.c.pos[dim_x] - 1];
    
  }

  item_t t;
  cahr_t h;

  h.pos[dim_y] = cheapest[dim_y];
  h.pos[dim_x] = cheapest[dim_x];
  h.prevous = world.cur_map->map[cheapest[dim_y]][cheapest[dim_x]];
  h.symbol = cur_person.symbol;
  h.dead = cur_person.c.dead;

  t.c = h;
  t.next_turn = cur_person.next_turn;
  t.seq_num = cur_person.seq_num;
  t.symbol = cur_person.symbol;

  

  world.cur_map->map[cheapest[dim_y]][cheapest[dim_x]] = ter_rival;
  world.cur_map->map[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x]] = cur_person.c.prevous;

  *person = t;

  
  return world.rival_dist[cheapest[dim_y]][cheapest[dim_x]] + cur_person.next_turn;
}

int move_wonder(item_t *person){
  pair_t cheapest;
  item_t cur_person = *person;

  int r = (rand() % 4) + 1;
  int done = 0;

  cheapest[dim_y] = cur_person.c.pos[dim_y];
  cheapest[dim_x] = cur_person.c.pos[dim_x];

  terrain_type_t cur_ter = world.cur_map->map[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_y]];

  // movement
  while(!done){
    r = (rand() % 5) + 1;
    if ((cur_person.c.pos[dim_y] + 1) != 0 && (cur_person.c.pos[dim_x]) != 0
       && (cur_person.c.pos[dim_y] + 1) != MAP_Y && (cur_person.c.pos[dim_x]) != MAP_X 
       && no_npc(world.cur_map->map[cur_person.c.pos[dim_y] + 1][cur_person.c.pos[dim_x]])
       && cur_ter == world.cur_map->map[cur_person.c.pos[dim_y] + 1][cur_person.c.pos[dim_x]]
      && r == 1)
    { // S
      checkbattle(cur_person.c.pos[dim_y] + 1, cur_person.c.pos[dim_x], &cur_person.c);
      cheapest[dim_y] = cur_person.c.pos[dim_y] + 1;
      cheapest[dim_x] = cur_person.c.pos[dim_x];
      done = 1;
      
  }

    else if((cur_person.c.pos[dim_y] - 1) != 0 && (cur_person.c.pos[dim_x]) != 0
        && (cur_person.c.pos[dim_y] - 1) != MAP_Y && (cur_person.c.pos[dim_x]) != MAP_X
        && no_npc(world.cur_map->map[cur_person.c.pos[dim_y] - 1][cur_person.c.pos[dim_x]])
        && cur_ter == world.cur_map->map[cur_person.c.pos[dim_y] - 1][cur_person.c.pos[dim_x]]
        && r == 2)
    { // N
      checkbattle(cur_person.c.pos[dim_y] - 1, cur_person.c.pos[dim_x], &cur_person.c);
      cheapest[dim_y] = cur_person.c.pos[dim_y] - 1;
      cheapest[dim_x] = cur_person.c.pos[dim_x];
      done = 1;
      
    }

    else if(cur_ter == world.cur_map->map[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x] + 1]
          && (cur_person.c.pos[dim_y]) != 0 && (cur_person.c.pos[dim_x] + 1) != 0
        && (cur_person.c.pos[dim_y]) != MAP_Y && (cur_person.c.pos[dim_x] + 1) != MAP_X
                && no_npc(world.cur_map->map[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x] + 1])
        && r == 3)
    { // E
      checkbattle(cur_person.c.pos[dim_y], cur_person.c.pos[dim_x] + 1, &cur_person.c);
      cheapest[dim_y] = cur_person.c.pos[dim_y];
      cheapest[dim_x] = cur_person.c.pos[dim_x] + 1;
      done = 1;
      
    }

    else if(cur_ter == world.cur_map->map[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x] - 1]
    && (cur_person.c.pos[dim_y]) != 0 && (cur_person.c.pos[dim_x] - 1) != 0
      && (cur_person.c.pos[dim_y]) != MAP_Y && (cur_person.c.pos[dim_x] - 1) != MAP_X
    && no_npc(world.cur_map->map[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x] - 1])
      && r == 4)
    { // W
      checkbattle(cur_person.c.pos[dim_y], cur_person.c.pos[dim_x] - 1, &cur_person.c); 
      cheapest[dim_y] = cur_person.c.pos[dim_y];
      cheapest[dim_x] = cur_person.c.pos[dim_x] - 1;
      done = 1;
      
    }

    else if(r == 5){
      done = 1;
    }

    usleep((rand() % 20000) + 2000);
  }

  //save and return
  item_t t;
  cahr_t h;

  h.pos[dim_y] = cheapest[dim_y];
  h.pos[dim_x] = cheapest[dim_x];
  h.prevous = cur_person.c.prevous;
  h.symbol = cur_person.symbol;
  h.dead = cur_person.c.dead;
  t.c = h;
  t.next_turn = cur_person.next_turn;
  t.seq_num = cur_person.seq_num;
  t.symbol = cur_person.symbol;

  world.cur_map->map[cheapest[dim_y]][cheapest[dim_x]] = ter_wonder;
  world.cur_map->map[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x]] = cur_person.c.prevous;

  

  *person = t;

  return world.hiker_dist[cheapest[dim_y]][cheapest[dim_x]] + cur_person.next_turn;
}

int move_exploer(item_t *person){
  pair_t cheapest;
  item_t cur_person = *person;

  int r = (rand() % 4) + 1;
  int done = 0;

  cheapest[dim_y] = cur_person.c.pos[dim_y];
  cheapest[dim_x] = cur_person.c.pos[dim_x];

  //terrain_type_t cur_ter = world.cur_map->map[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_y]];

  // movement
  while(!done){
    r = (rand() % 5) + 1;
    if ((cur_person.c.pos[dim_y] + 1) != 0 && (cur_person.c.pos[dim_x]) != 0
       && (cur_person.c.pos[dim_y] + 1) != MAP_Y && (cur_person.c.pos[dim_x]) != MAP_X 
       && no_npc(world.cur_map->map[cur_person.c.pos[dim_y] + 1][cur_person.c.pos[dim_x]])
      && r == 1)
    { // S
      checkbattle(cur_person.c.pos[dim_y] + 1, cur_person.c.pos[dim_x], &cur_person.c);
      cheapest[dim_y] = cur_person.c.pos[dim_y] + 1;
      cheapest[dim_x] = cur_person.c.pos[dim_x];
      done = 1;
      
  }

    else if((cur_person.c.pos[dim_y] - 1) != 0 && (cur_person.c.pos[dim_x]) != 0
        && (cur_person.c.pos[dim_y] - 1) != MAP_Y && (cur_person.c.pos[dim_x]) != MAP_X
        && no_npc(world.cur_map->map[cur_person.c.pos[dim_y] - 1][cur_person.c.pos[dim_x]])
        && r == 2)
    { // N
      checkbattle(cur_person.c.pos[dim_y] - 1, cur_person.c.pos[dim_x], &cur_person.c);
      cheapest[dim_y] = cur_person.c.pos[dim_y] - 1;
      cheapest[dim_x] = cur_person.c.pos[dim_x];
      done = 1;
      
    }


          else if((cur_person.c.pos[dim_y]) != 0 && (cur_person.c.pos[dim_x] + 1) != 0
        && (cur_person.c.pos[dim_y]) != MAP_Y && (cur_person.c.pos[dim_x] + 1) != MAP_X
                && no_npc(world.cur_map->map[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x] + 1])
        && r == 3)
    { // E
    checkbattle(cur_person.c.pos[dim_y], cur_person.c.pos[dim_x] + 1, &cur_person.c);
      cheapest[dim_y] = cur_person.c.pos[dim_y];
      cheapest[dim_x] = cur_person.c.pos[dim_x] + 1;
      done = 1;
      
    }


    else if((cur_person.c.pos[dim_y]) != 0 && (cur_person.c.pos[dim_x] - 1) != 0
      && (cur_person.c.pos[dim_y]) != MAP_Y && (cur_person.c.pos[dim_x] - 1) != MAP_X
    && no_npc(world.cur_map->map[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x] - 1])
      && r == 4)
    { // W
    checkbattle(cur_person.c.pos[dim_y], cur_person.c.pos[dim_x] - 1, &cur_person.c);
      cheapest[dim_y] = cur_person.c.pos[dim_y];
      cheapest[dim_x] = cur_person.c.pos[dim_x] - 1;
      done = 1;
      
    }

    else if(r == 5){
      done = 1;
    }

    usleep((rand() % 20000) + 2000);
  }

  //save and return
  item_t t;
  cahr_t h;

  h.pos[dim_y] = cheapest[dim_y];
  h.pos[dim_x] = cheapest[dim_x];
  h.prevous = cur_person.c.prevous;
  h.symbol = cur_person.symbol;
h.dead = cur_person.c.dead;
  t.c = h;
  t.next_turn = cur_person.next_turn;
  t.seq_num = cur_person.seq_num;
  t.symbol = cur_person.symbol;

  if(world.cur_map->map[cheapest[dim_y]][cheapest[dim_x]] == cur_person.c.prevous){
    world.cur_map->map[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x]] = cur_person.c.prevous;
  }
  else{

    switch (r)
    {
    case 1:
       world.cur_map->map[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x]] = 
      world.cur_map->map[cur_person.c.pos[dim_y] + 1][cur_person.c.pos[dim_x]];
      break;

    case 2:
       world.cur_map->map[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x]] = 
      world.cur_map->map[cur_person.c.pos[dim_y] - 1][cur_person.c.pos[dim_x]];
      break;

    case 3:
       world.cur_map->map[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x]] = 
      world.cur_map->map[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x] + 1];
      break;

    case 4:
       world.cur_map->map[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x]] = 
      world.cur_map->map[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x] - 1];
      break;

    default:
      break;
    }
  }
  world.cur_map->map[cheapest[dim_y]][cheapest[dim_x]] = ter_exploer;
  
  // world.cur_map->map[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x]] = cur_person.c.prevous;
  

  *person = t;

  return world.hiker_dist[cheapest[dim_y]][cheapest[dim_x]] + cur_person.next_turn;;
}

int move_pacer(item_t *person){
  pair_t cheapest;
  item_t cur_person = *person;

  int r,done = 0;

  cheapest[dim_y] = cur_person.c.pos[dim_y];
  cheapest[dim_x] = cur_person.c.pos[dim_x];

  terrain_type_t cur_ter = world.cur_map->map[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_y]];

  // movement

  while(!done){
    
    r = (rand() % 3) + 1;
    if(cur_ter == world.cur_map->map[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x] + 1]
      && (cur_person.c.pos[dim_y]) != 0 && (cur_person.c.pos[dim_x] + 1) != 0
    && (cur_person.c.pos[dim_y]) != MAP_Y && (cur_person.c.pos[dim_x] + 1) != MAP_X
            && no_npc(world.cur_map->map[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x] + 1])
    && r == 1)
    { // E
    checkbattle(cur_person.c.pos[dim_y], cur_person.c.pos[dim_x] + 1, &cur_person.c);
      cheapest[dim_y] = cur_person.c.pos[dim_y];
      cheapest[dim_x] = cur_person.c.pos[dim_x] + 1;
      done = 1 ;
      
    }

    else if(cur_ter == world.cur_map->map[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x] - 1]
    && (cur_person.c.pos[dim_y]) != 0 && (cur_person.c.pos[dim_x] - 1) != 0
      && (cur_person.c.pos[dim_y]) != MAP_Y && (cur_person.c.pos[dim_x] - 1) != MAP_X
    && no_npc(world.cur_map->map[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x] - 1])
      && r == 2)
    { // W
      checkbattle(cur_person.c.pos[dim_y], cur_person.c.pos[dim_x] - 1, &cur_person.c);
      cheapest[dim_y] = cur_person.c.pos[dim_y];
      cheapest[dim_x] = cur_person.c.pos[dim_x] - 1;
      done = 1;
      
    }

    else if(r == 3){
      done = 1;
    }

    usleep((rand() % 20000) + 2000);
  }

  //save and return
  item_t t;
  cahr_t h;

  h.pos[dim_y] = cheapest[dim_y];
  h.pos[dim_x] = cheapest[dim_x];
  h.prevous = cur_person.c.prevous;
  h.symbol = cur_person.symbol;
  h.dead = cur_person.c.dead;
  t.c = h;
  t.next_turn = cur_person.next_turn;
  t.seq_num = cur_person.seq_num;
  t.symbol = cur_person.symbol;

  world.cur_map->map[cheapest[dim_y]][cheapest[dim_x]] = ter_pacer;
  world.cur_map->map[cur_person.c.pos[dim_y]][cur_person.c.pos[dim_x]] = cur_person.c.prevous;

  

  *person = t;

  return world.hiker_dist[cheapest[dim_y]][cheapest[dim_x]] + cur_person.next_turn;
}

int check_ter(terrain_type_t t){

  switch (t)
  {
  case ter_water:
    printf("you can not swim\n");
    return 0;
    break;

  case ter_gate:
    printf("you have not unlocked this area yet\n");
    return 0;
    break;

  case ter_tree:
    printf("maybe if you punch it, it will give you wood!\n");
    return 0;
    break;    
   
  case ter_boulder:
    printf("please do not climb on the rocks...\n");
    return 0;
    break;

  default:
    break;
  }
  
  
  return 1;
}

int move_player(item_t *person, PQ_t *pqm){

  char answer;

  while(1){
    print_map();
    scanf(" %c", &answer);
    if(isalpha(answer)){
      answer = tolower(answer);
    }

    //quiting
    if(answer == 'q'){ //quit
      return -1;
    }

  //          world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x]] != ter_water &&


    // ====== movement ======
    else if((answer == 'y' || answer == '7') && 
          //player_no_npc(world.cur_map->map[world.pc.pos[dim_y] - 1][world.pc.pos[dim_x] - 1]) &&
          check_ter(world.cur_map->map[world.pc.pos[dim_y] - 1][world.pc.pos[dim_x] - 1]) &&
          (world.pc.pos[dim_y] - 1 != 0 && world.pc.pos[dim_y] - 1 != MAP_Y) && 
          (world.pc.pos[dim_x] - 1 != 0 && world.pc.pos[dim_x] - 1 != MAP_X) ){ //up left
      world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x]] = person->c.prevous;
      person->c.prevous = world.cur_map->map[world.pc.pos[dim_y] - 1][world.pc.pos[dim_x] - 1];
      world.pc.pos[dim_y]--;
      world.pc.pos[dim_x]--;
      pc_checkbattle(world.pc.pos[dim_y] - 1, world.pc.pos[dim_x] - 1);
      break;
    }
    else if((answer == 'k' || answer == '8') && 
           // player_no_npc(world.cur_map->map[world.pc.pos[dim_y]- 1][world.pc.pos[dim_x]]) &&
            check_ter(world.cur_map->map[world.pc.pos[dim_y]- 1][world.pc.pos[dim_x]]) &&
          (world.pc.pos[dim_y] - 1 != 0 && world.pc.pos[dim_y] - 1 != MAP_Y)){ //up
      pc_checkbattle(world.pc.pos[dim_y] - 1, world.pc.pos[dim_x]);
      world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x]] = person->c.prevous;
      person->c.prevous = world.cur_map->map[world.pc.pos[dim_y] - 1][world.pc.pos[dim_x]];
      world.pc.pos[dim_y]--;
      break;
    }
    else if((answer == 'u' || answer == '9') && 
           // player_no_npc(world.cur_map->map[world.pc.pos[dim_y]- 1][world.pc.pos[dim_x] + 1]) &&
          check_ter(world.cur_map->map[world.pc.pos[dim_y]- 1][world.pc.pos[dim_x] + 1]) &&
          (world.pc.pos[dim_y] - 1 != 0 && world.pc.pos[dim_y] - 1 != MAP_Y) && 
          (world.pc.pos[dim_x] + 1 != 0 && world.pc.pos[dim_x] + 1 != MAP_X) ){ //up right
     pc_checkbattle(world.pc.pos[dim_y] - 1, world.pc.pos[dim_x] + 1);
      world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x]] = person->c.prevous;
      person->c.prevous = world.cur_map->map[world.pc.pos[dim_y] - 1][world.pc.pos[dim_x] + 1];
      world.pc.pos[dim_y]--;
      world.pc.pos[dim_x]++;
      break;
    }

    else if((answer == 'l' || answer == '6') && 
            //player_no_npc(world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x] + 1]) &&
          check_ter(world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x] + 1])&& 
          (world.pc.pos[dim_x] + 1 != 0 && world.pc.pos[dim_x] + 1 != MAP_X) ){ //left
          pc_checkbattle(world.pc.pos[dim_y], world.pc.pos[dim_x] + 1);
      world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x]] = person->c.prevous;
      person->c.prevous = world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x] + 1];
      world.pc.pos[dim_x]++;
      break;
    }
    else if((answer == 'n' || answer == '3') && 
           // player_no_npc(world.cur_map->map[world.pc.pos[dim_y]+ 1][world.pc.pos[dim_x] + 1]) &&
          check_ter(world.cur_map->map[world.pc.pos[dim_y] + 1][world.pc.pos[dim_x] + 1])&&
          (world.pc.pos[dim_y] + 1 != 0 && world.pc.pos[dim_y] + 1 != MAP_Y) && 
          (world.pc.pos[dim_x] + 1 != 0 && world.pc.pos[dim_x] + 1 != MAP_X) ){ //lower right 
      pc_checkbattle(world.pc.pos[dim_y] + 1, world.pc.pos[dim_x] + 1);
    world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x]] = person->c.prevous;
    person->c.prevous = world.cur_map->map[world.pc.pos[dim_y] + 1][world.pc.pos[dim_x] + 1]; 
    world.pc.pos[dim_y]++;
    world.pc.pos[dim_x]++;
    break;
    }

    else if((answer == 'j' || answer == '2')&& 
           // player_no_npc(world.cur_map->map[world.pc.pos[dim_y]+ 1][world.pc.pos[dim_x]]) &&
          check_ter(world.cur_map->map[world.pc.pos[dim_y]+ 1][world.pc.pos[dim_x]]) && 
          (world.pc.pos[dim_y] + 1 != 0 && world.pc.pos[dim_y] + 1 != MAP_Y) ){ //down
          pc_checkbattle(world.pc.pos[dim_y] + 1, world.pc.pos[dim_x]);
      world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x]] = person->c.prevous;
      person->c.prevous = world.cur_map->map[world.pc.pos[dim_y] + 1][world.pc.pos[dim_x]];
      world.pc.pos[dim_y]++;
      break;
    }
    else if((answer == 'b' || answer == '1') && 
          //  player_no_npc(world.cur_map->map[world.pc.pos[dim_y]+ 1][world.pc.pos[dim_x] - 1]) &&
          check_ter(world.cur_map->map[world.pc.pos[dim_y]+ 1][world.pc.pos[dim_x] - 1]) && 
          (world.pc.pos[dim_y] + 1 != 0 && world.pc.pos[dim_y] + 1 != MAP_Y) && 
          (world.pc.pos[dim_x] - 1 != 0 && world.pc.pos[dim_x] - 1 != MAP_X) ){ //down left
          pc_checkbattle(world.pc.pos[dim_y] + 1, world.pc.pos[dim_x] - 1);
      world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x]] = person->c.prevous;
      person->c.prevous = world.cur_map->map[world.pc.pos[dim_y] + 1][world.pc.pos[dim_x] - 1];
      world.pc.pos[dim_y]++;
      world.pc.pos[dim_x]--;
      break;
    }

    else if((answer == '4' || answer == 'h') && 
        //  player_no_npc(world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x] - 1]) &&
          check_ter(world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x] - 1]) &&
          (world.pc.pos[dim_x] - 1!= 0 && world.pc.pos[dim_x] - 1!= MAP_X) ){ //left
          pc_checkbattle(world.pc.pos[dim_y], world.pc.pos[dim_x] - 1);
      world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x]] = person->c.prevous;
      person->c.prevous = world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x] - 1];
      world.pc.pos[dim_x]--;
      break;
    }

    else if(answer == '5' || answer == ' ' || answer == '.'){ //left
      //empty
      break;
    }

    // ======= extras =======
    //buy screen
    else if(answer == '>' && (world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x]] == ter_mart 
                        || world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x]] == ter_center)){
      printf("\nthis is buy screen q to leave");

      scanf(" %c", &answer);
      if(tolower(answer) == 'q'){

      }
      // scanf("%c", &buying);
      // buying = tolower(buying);
      // while(buying != 'q'){
      //   print("this is buy screen");
      // }
    }
    else if(answer == 't'){
      item_t cur_item;
      int vert, horz;
      char *hdir, *vdir;

      for(int i = 0; i < pqm->size; i++){
        cur_item = pqm->items[i];
        vert = cur_item.c.pos[dim_y] - world.pc.pos[dim_y];
        horz = cur_item.c.pos[dim_x] - world.pc.pos[dim_x];

        if(vert < 0){
          vdir = "north";
        }
        else{
          vdir = "south";
        }

      if(horz < 0){
          hdir = "east";
        }
        else{
          hdir = "west";
        } 

        if(cur_item.symbol != '@'){
          printf("%c, %d %s %d %s \n", cur_item.c.symbol, abs(vert), vdir, abs(horz), hdir);
        }
      }

      scanf(" %c", &answer);
      break;
    }
    
  }

  
  //update person with world.pc
  person->c.pos[dim_y] = world.pc.pos[dim_y];
  person->c.pos[dim_x] = world.pc.pos[dim_x];

  person->next_turn = person->next_turn + 75;

  pathfind(world.cur_map); //update dist maps

  return person->next_turn;
}

void game_loop(PQ_t *pqm){
  int index, futuremove;
  item_t cur_person;
  int done = 0;

  while(!done){
    index = peek(pqm);
    cur_person = pqm->items[index];

    // int i = peek(&pqm); debug
    // char symbol = pqm.items[i].symbol;
    // int next = pqm.items[i].next_turn;
    // int seq = pqm.items[i].seq_num;

    // printf(" %c %d %d - %c \n", symbol, next, seq, cur_person.symbol);

    switch (cur_person.symbol)
    {
    case 'h':
      futuremove = move_hiker(&cur_person);
      break;

    case 'r':
      futuremove = move_rival(&cur_person);
      break;

    case 'e':
      futuremove = move_exploer(&cur_person);
      break;

    case 'p':
      futuremove = move_pacer(&cur_person);
      break;

    case 'w':
      futuremove = move_wonder(&cur_person);
      break;

    case '@':
      futuremove = move_player(&cur_person, pqm);

      if(futuremove < 0){
        done = 1;
      }
      break;

    default:
      break;
    }

    print_map();
    //usleep(1000000 / 60);

    deque(pqm);
    if(!(cur_person.c.dead) && cur_person.symbol != '@'){
      enque(pqm, cur_person.symbol, futuremove, cur_person.seq_num, cur_person.c);
    }
    else if(cur_person.symbol == '@'){
     enque(pqm, cur_person.symbol, futuremove, cur_person.seq_num, cur_person.c); 
    }
    
  }
}

void init_everyone()
{
  PQ_t pqm;
  PQ_init(&pqm);

 cahr_t pc;
  pc.pos[dim_y] = world.pc.pos[dim_y];
  pc.pos[dim_x] = world.pc.pos[dim_x];
  pc.symbol = '@';
  pc.prevous = ter_path;

  enque(&pqm, '@', 100, 0, pc);

  for(int i = 0; i < global_count; i++){
    char sym = world.cur_map->cahr[i].symbol;
    int nextmove = 0;

    if(sym == 'h'){
      nextmove = world.hiker_dist[world.cur_map->cahr[i].pos[1]][world.cur_map->cahr[i].pos[0]];
    }
    else if(sym == 'r'){
      nextmove = world.rival_dist[world.cur_map->cahr[i].pos[1]][world.cur_map->cahr[i].pos[0]];
    }
    else if(sym == 'w'){
      nextmove = world.hiker_dist[world.cur_map->cahr[i].pos[1]][world.cur_map->cahr[i].pos[0]];
    }
    else if (sym == 'e')
    {
      nextmove = world.hiker_dist[world.cur_map->cahr[i].pos[1]][world.cur_map->cahr[i].pos[0]];
    }
    else if (sym == 'p')
    {
      nextmove = world.hiker_dist[world.cur_map->cahr[i].pos[1]][world.cur_map->cahr[i].pos[0]];
    }
    if(sym != 's'){
      enque(&pqm, sym, nextmove, i + 1, world.cur_map->cahr[i]);
    }
  }


  // for(int i = 0; i < pqm.size; i++){
  //   char symbol = pqm.items[i].symbol;
  //   int next = pqm.items[i].next_turn;
  //   int seq = pqm.items[i].seq_num;

  //   printf("\n%c %d %d", symbol, next, seq);
  // }

  game_loop(&pqm);
}

int main(int argc, char *argv[])
{
  struct timeval tv;
  uint32_t seed;
  char c;
  int x, y;
  int numtrainer = 10;
  int wasseed = 0;

  if(argc > 0){
    for(int i = 1; i < argc; i++){
      //printf("i = %d, agrv = %s\n", i, argv[i]);
      if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc)
      {
        seed = atoi(argv[i + 1]);
        wasseed = 1;
      }

      if(strcmp(argv[i], "--numtrainers") == 0 && i + 1 < argc){
        if (atoi(argv[i + 1]) > 0){
          numtrainer = atoi(argv[i + 1]);
        }
      }
    }
  }

  if (!wasseed){
    gettimeofday(&tv, NULL);
    seed = (tv.tv_usec ^ (tv.tv_sec << 20)) & 0xffffffff;
  }

  printf("Using seed: %u\n", seed);
  printf("Using count of trainers: %d\n", numtrainer);
  srand(seed);

  init_world();
  init_pc();
  pathfind(world.cur_map);

  place_npc(numtrainer);
  print_map();

  // printf("%d \n", global_count);
  // for(int i = 0; i < global_count; i++){
  //   printf("%c ", world.cur_map->cahr[i].symbol);
  // }

  init_everyone();
  //print_hiker_dist();
  //print_rival_dist();

  delete_world();

  return 0;

  do
  {
    print_map();
    printf("Current position is %d%cx%d%c (%d,%d).  "
           "Enter command: ",
           abs(world.cur_idx[dim_x] - (WORLD_SIZE / 2)),
           world.cur_idx[dim_x] - (WORLD_SIZE / 2) >= 0 ? 'E' : 'W',
           abs(world.cur_idx[dim_y] - (WORLD_SIZE / 2)),
           world.cur_idx[dim_y] - (WORLD_SIZE / 2) <= 0 ? 'N' : 'S',
           world.cur_idx[dim_x] - (WORLD_SIZE / 2),
           world.cur_idx[dim_y] - (WORLD_SIZE / 2));
    if (scanf(" %c", &c) != 1)
    {
      /* To handle EOF */
      putchar('\n');
      break;
    }
    switch (c)
    {
    case 'n':
      if (world.cur_idx[dim_y])
      {
        world.cur_idx[dim_y]--;
        new_map();
      }
      break;
    case 's':
      if (world.cur_idx[dim_y] < WORLD_SIZE - 1)
      {
        world.cur_idx[dim_y]++;
        new_map();
      }
      break;
    case 'e':
      if (world.cur_idx[dim_x] < WORLD_SIZE - 1)
      {
        world.cur_idx[dim_x]++;
        new_map();
      }
      break;
    case 'w':
      if (world.cur_idx[dim_x])
      {
        world.cur_idx[dim_x]--;
        new_map();
      }
      break;
    case 'q':
      break;
    case 'f':
      scanf(" %d %d", &x, &y);
      if (x >= -(WORLD_SIZE / 2) && x <= WORLD_SIZE / 2 &&
          y >= -(WORLD_SIZE / 2) && y <= WORLD_SIZE / 2)
      {
        world.cur_idx[dim_x] = x + (WORLD_SIZE / 2);
        world.cur_idx[dim_y] = y + (WORLD_SIZE / 2);
        new_map();
      }
      break;
    case '?':
    case 'h':
      printf("Move with 'e'ast, 'w'est, 'n'orth, 's'outh or 'f'ly x y.\n"
             "Quit with 'q'.  '?' and 'h' print this help message.\n");
      break;
    default:
      fprintf(stderr, "%c: Invalid input.  Enter '?' for help.\n", c);
      break;
    }
  } while (c != 'q');

  delete_world();

  printf("But how are you going to be the very best if you quit?\n");

  return 0;
}
