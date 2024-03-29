/*
    TODO
        - double check the specs so full points :)
        - make the rival pathing
        - double check specs for R

        - DONE!

        - start assignment 1.04
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/time.h>
#include <assert.h>

#include "heap.h"

#define malloc(size) ({          \
  void *_tmp;                    \
  assert((_tmp = malloc(size))); \
  _tmp;                          \
})

typedef struct path
{
  heap_node_t *hn;
  uint8_t pos[2];
  uint8_t from[2];
  int32_t cost;
} path_t;

typedef enum dim
{
  dim_x,
  dim_y,
  num_dims
} dim_t;

typedef int16_t pair_t[num_dims];

#define MAP_X 80
#define MAP_Y 21
#define MIN_TREES 10
#define MIN_BOULDERS 10
#define TREE_PROB 95
#define BOULDER_PROB 95
#define WORLD_SIZE 401

#define mappair(pair) (m->map[pair[dim_y]][pair[dim_x]])
#define mapxy(x, y) (m->map[y][x])
#define heightpair(pair) (m->height[pair[dim_y]][pair[dim_x]])
#define heightxy(x, y) (m->height[y][x])

typedef enum __attribute__((__packed__)) terrain_type
{
  ter_debug,
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
  ter_pc
} terrain_type_t;

typedef struct person
{
  char symbol;
  int y, x;
} person_t;
typedef struct map
{
  terrain_type_t map[MAP_Y][MAP_X];
  path_t map_pathh[MAP_Y][MAP_X];
  path_t map_pathr[MAP_Y][MAP_X];
  uint8_t height[MAP_Y][MAP_X];
  int8_t n, s, e, w;
  person_t player;
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
} world_t;

/*
  - discrete event simulator (1.04)
    - method to keep track of whos turn it is
    - event que with whos turn  it is
                                - their symbols
                                - their next turn (init as 0 & cost of next turn) and
                                - a seq number (to break ties)
*/
/*
  - WHERE DATA IS
  - the ditance maps belong in the map not the maps them selfs
  - PC is in the world
  - make chariter struct
  - make  a struct to keep track of all the cahricters
*/

/* Even unallocated, a WORLD_SIZE x WORLD_SIZE array of pointers is a very *
 * large thing to put on the stack.  To avoid that, world is a global.     */
world_t world;

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
      path[y][x].cost = INT_MAX;
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
        // Don't overwrite the gate
        if (x != to[dim_x] || y != to[dim_y])
        {
          mapxy(x, y) = ter_path;
          heightxy(x, y) = 0;
        }
      }
      heap_delete(&h);
      return;
    }

    // printf("%d ", p->cost);

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
        m->map[y][x] != ter_gate &&
        m->map[y][x] != ter_water)
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

static void place_PC(map_t *m){
  person_t pc;
  int done = 0;

  pc.symbol = '@';
  while (!done)
  {
    pc.y = rand() % (MAP_Y - 2) + 1;
    pc.x = rand() % (MAP_X - 2) + 1;
    if ((pc.y != 0 && pc.y != 20) &&
        (pc.x != 0 && pc.x != 79) &&
        (m->map[pc.y][pc.x] != ter_water))
    {
      done = 1;
    }
  }

  m->player = pc;
  m->map[pc.y][pc.x] = ter_pc;
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

  place_PC(world.cur_map);

  return 0;
}

static void print_map()
{
  int x, y;
  int default_reached = 0;

  printf("player (y, x): (%d, %d) \n", world.cur_map->player.y, world.cur_map->player.x);

  for (y = 0; y < MAP_Y; y++)
  {
    for (x = 0; x < MAP_X; x++)
    {
      // printf("y = %d x = %d \n", y, x);
      switch (world.cur_map->map[y][x])
      {
      case ter_boulder:
      case ter_mountain:
        putchar('%');
        break;
      case ter_tree:
      case ter_forest:
        putchar('^');
        break;
      case ter_pc:
        putchar(world.cur_map->player.symbol);
        break;
      case ter_path:
      case ter_gate:
        putchar('#');
        break;
      case ter_mart:
        putchar('M');
        break;
      case ter_center:
        putchar('C');
        break;
      case ter_grass:
        putchar(':');
        break;
      case ter_clearing:
        putchar('.');
        break;
      case ter_water:
        putchar('~');
        break;
      default:
        default_reached = 1;
        break;
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

int cost_hiker(terrain_type_t c)
{
  int ccost = 5;

  if (c == ter_boulder || c == ter_mountain)
  {
    ccost = 15;
  }
  if (c == ter_tree || c == ter_forest)
  {
    ccost = 15;
  }
  if (c == ter_path || c == ter_gate)
  {
    ccost = 10;
  }
  if (c == ter_mart || c == ter_center)
  {
    ccost = 50;
  }
  if (c == ter_grass)
  {
    ccost = 15;
  }
  if (c == ter_clearing)
  {
    ccost = 10;
  }

  return ccost;
}

static void dijkstra_path_hiker(map_t *m)
{
  static path_t path[MAP_Y][MAP_X], *p; // path_t path[MAP_Y][MAP_X]
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
        if (world.cur_map->map[y][x] == ter_water || world.cur_map->map[y][x] == ter_gate)
        {
          path[y][x].cost = -1;
        }
        else if (y == world.cur_map->player.y && x == world.cur_map->player.x)
        {
          path[y][x].cost = 0;
        }
        else
        {
          path[y][x].cost = SHRT_MAX;
        }
      }
    }
  }

  initialized = 1;

  // path[from[dim_y]][from[dim_x]].cost = 0;

  heap_init(&h, path_cmp, NULL);

  for (y = 0; y < MAP_Y; y++)
  {
    for (x = 0; x < MAP_X; x++)
    {
      if (path[y][x].cost >= 0)
      { // cost of terrain is not infinity and we are not in water
        path[y][x].hn = heap_insert(&h, &path[y][x]);
      }
      else
      {
        path[y][x].hn = NULL;
      }
    }
  }

  // for (y = 0; y < MAP_Y; y++)
  // {
  //   for (x = 0; x < MAP_X; x++)
  //   {
  //     if  (){
  //     }
  //   }
  // }

  while ((p = heap_remove_min(&h)))
  {
    p->hn = NULL;

    if ((path[p->pos[dim_y] - 1][p->pos[dim_x]].hn) && // N
        (path[p->pos[dim_y] - 1][p->pos[dim_x]].cost > path[p->pos[dim_y]][p->pos[dim_x]].cost) &&
        (path[p->pos[dim_y] - 1][p->pos[dim_x]].cost > 0))
    {

      path[p->pos[dim_y] - 1][p->pos[dim_x]].cost = path[p->pos[dim_y]][p->pos[dim_x]].cost +
                                                    cost_hiker(world.cur_map->map[p->pos[dim_y]][p->pos[dim_x]]);

      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1][p->pos[dim_x]].hn);
    }

    if ((path[p->pos[dim_y] - 1][p->pos[dim_x] + 1].hn) && // NE
        (path[p->pos[dim_y] - 1][p->pos[dim_x] + 1].cost > path[p->pos[dim_y]][p->pos[dim_x]].cost) &&
        (path[p->pos[dim_y] - 1][p->pos[dim_x] + 1].cost > 0))
    {

      path[p->pos[dim_y] - 1][p->pos[dim_x] + 1].cost = path[p->pos[dim_y]][p->pos[dim_x]].cost +
                                                        cost_hiker(world.cur_map->map[p->pos[dim_y]][p->pos[dim_x]]);

      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1][p->pos[dim_x] + 1].hn);
    }

    if ((path[p->pos[dim_y] - 1][p->pos[dim_x] - 1].hn) && // NW
        (path[p->pos[dim_y] - 1][p->pos[dim_x] - 1].cost > path[p->pos[dim_y]][p->pos[dim_x]].cost) &&
        (path[p->pos[dim_y] - 1][p->pos[dim_x] - 1].cost > 0))
    {

      path[p->pos[dim_y] - 1][p->pos[dim_x] - 1].cost = path[p->pos[dim_y]][p->pos[dim_x]].cost +
                                                        cost_hiker(world.cur_map->map[p->pos[dim_y]][p->pos[dim_x]]);

      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1][p->pos[dim_x] - 1].hn);
    }
    if ((path[p->pos[dim_y]][p->pos[dim_x] - 1].hn) && // W
        (path[p->pos[dim_y]][p->pos[dim_x] - 1].cost > path[p->pos[dim_y]][p->pos[dim_x]].cost) &&
        (path[p->pos[dim_y]][p->pos[dim_x] - 1].cost > 0))
    {
      path[p->pos[dim_y]][p->pos[dim_x] - 1].cost = p->cost +
                                                    cost_hiker(world.cur_map->map[p->pos[dim_y]][p->pos[dim_x]]);

      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]][p->pos[dim_x] - 1].hn);
    }

    if ((path[p->pos[dim_y]][p->pos[dim_x] + 1].hn) && // E
        (path[p->pos[dim_y]][p->pos[dim_x] + 1].cost > path[p->pos[dim_y]][p->pos[dim_x]].cost) &&
        (path[p->pos[dim_y]][p->pos[dim_x] + 1].cost > 0))
    {

      path[p->pos[dim_y]][p->pos[dim_x] + 1].cost = p->cost +
                                                    cost_hiker(world.cur_map->map[p->pos[dim_y]][p->pos[dim_x]]);

      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]][p->pos[dim_x] + 1].hn);
    }

    if ((path[p->pos[dim_y] + 1][p->pos[dim_x]].hn) && // S
        (path[p->pos[dim_y] + 1][p->pos[dim_x]].cost > path[p->pos[dim_y]][p->pos[dim_x]].cost) &&
        (path[p->pos[dim_y] + 1][p->pos[dim_x]].cost > 0))
    {

      path[p->pos[dim_y] + 1][p->pos[dim_x]].cost = p->cost +
                                                    cost_hiker(world.cur_map->map[p->pos[dim_y]][p->pos[dim_x]]);

      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1][p->pos[dim_x]].hn);
    }

    if ((path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].hn) && // SW
        (path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].cost > path[p->pos[dim_y]][p->pos[dim_x]].cost) &&
        (path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].cost > 0))
    {

      path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].cost = p->cost +
                                                        cost_hiker(world.cur_map->map[p->pos[dim_y]][p->pos[dim_x]]);

      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].hn);
    }

    if ((path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].hn) && // SE
        (path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].cost > path[p->pos[dim_y]][p->pos[dim_x]].cost) &&
        (path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].cost > 0))
    {

      path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].cost = p->cost +
                                                        cost_hiker(world.cur_map->map[p->pos[dim_y]][p->pos[dim_x]]);

      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].hn);
    }
  }

  heap_delete(&h);
  printf("Hiker dist map:\n");

  for (int i = 0; i < MAP_Y; i++)
  {
    for (int j = 0; j < MAP_X; j++)
    {
      world.cur_map->map_pathh[i][j] = path[i][j];
      if (path[i][j].cost < 0)
      {
        printf("   ");
      }
      else
      {
        if (i == world.cur_map->player.y && j == world.cur_map->player.x)
        {
          printf(" @ ");
        }
        else
        {
          printf("%02d ", path[i][j].cost % 100);
        }
      }
    }
    printf("\n");
  }
}

int cost_rival(terrain_type_t c)
{
  int ccost = 5;

  if (c == ter_mart || c == ter_center)
  {
    ccost = 50;
  }
  if (c == ter_grass)
  {
    ccost = 20;
  }
  if (c == ter_clearing)
  {
    ccost = 10;
  }

  return ccost;
}

static void dijkstra_path_rival(map_t *m)
{
  static path_t path[MAP_Y][MAP_X], *p; // path_t path[MAP_Y][MAP_X]
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
        if (world.cur_map->map[y][x] == ter_water ||
            world.cur_map->map[y][x] == ter_gate ||
            world.cur_map->map[y][x] == ter_mountain || world.cur_map->map[y][x] == ter_boulder ||
            world.cur_map->map[y][x] == ter_forest || world.cur_map->map[y][x] == ter_tree)
        {
          path[y][x].cost = -1;
        }
        else if (y == world.cur_map->player.y && x == world.cur_map->player.x)
        {
          path[y][x].cost = 0;
        }
        else
        {
          path[y][x].cost = SHRT_MAX;
        }
      }
    }
  }

  initialized = 1;

  // path[from[dim_y]][from[dim_x]].cost = 0;

  heap_init(&h, path_cmp, NULL);

  for (y = 0; y < MAP_Y; y++)
  {
    for (x = 0; x < MAP_X; x++)
    {
      if (path[y][x].cost >= 0)
      { // cost of terrain is not infinity and we are not in water
        path[y][x].hn = heap_insert(&h, &path[y][x]);
      }
      else
      {
        path[y][x].hn = NULL;
      }
    }
  }

  // for (y = 0; y < MAP_Y; y++)
  // {
  //   for (x = 0; x < MAP_X; x++)
  //   {
  //     if  (){
  //     }
  //   }
  // }

  while ((p = heap_remove_min(&h)))
  {
    p->hn = NULL;

    if ((path[p->pos[dim_y] - 1][p->pos[dim_x]].hn) && // N
        (path[p->pos[dim_y] - 1][p->pos[dim_x]].cost > path[p->pos[dim_y]][p->pos[dim_x]].cost) &&
        (path[p->pos[dim_y] - 1][p->pos[dim_x]].cost > 0))
    {

      path[p->pos[dim_y] - 1][p->pos[dim_x]].cost = path[p->pos[dim_y]][p->pos[dim_x]].cost +
                                                    cost_rival(world.cur_map->map[p->pos[dim_y]][p->pos[dim_x]]);

      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1][p->pos[dim_x]].hn);
    }

    if ((path[p->pos[dim_y] - 1][p->pos[dim_x] + 1].hn) && // NE
        (path[p->pos[dim_y] - 1][p->pos[dim_x] + 1].cost > path[p->pos[dim_y]][p->pos[dim_x]].cost) &&
        (path[p->pos[dim_y] - 1][p->pos[dim_x] + 1].cost > 0))
    {

      path[p->pos[dim_y] - 1][p->pos[dim_x] + 1].cost = path[p->pos[dim_y]][p->pos[dim_x]].cost +
                                                        cost_rival(world.cur_map->map[p->pos[dim_y]][p->pos[dim_x]]);

      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1][p->pos[dim_x] + 1].hn);
    }

    if ((path[p->pos[dim_y] - 1][p->pos[dim_x] - 1].hn) && // NW
        (path[p->pos[dim_y] - 1][p->pos[dim_x] - 1].cost > path[p->pos[dim_y]][p->pos[dim_x]].cost) &&
        (path[p->pos[dim_y] - 1][p->pos[dim_x] - 1].cost > 0))
    {

      path[p->pos[dim_y] - 1][p->pos[dim_x] - 1].cost = path[p->pos[dim_y]][p->pos[dim_x]].cost +
                                                        cost_rival(world.cur_map->map[p->pos[dim_y]][p->pos[dim_x]]);

      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1][p->pos[dim_x] - 1].hn);
    }
    if ((path[p->pos[dim_y]][p->pos[dim_x] - 1].hn) && // W
        (path[p->pos[dim_y]][p->pos[dim_x] - 1].cost > path[p->pos[dim_y]][p->pos[dim_x]].cost) &&
        (path[p->pos[dim_y]][p->pos[dim_x] - 1].cost > 0))
    {
      path[p->pos[dim_y]][p->pos[dim_x] - 1].cost = p->cost +
                                                    cost_rival(world.cur_map->map[p->pos[dim_y]][p->pos[dim_x]]);

      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]][p->pos[dim_x] - 1].hn);
    }

    if ((path[p->pos[dim_y]][p->pos[dim_x] + 1].hn) && // E
        (path[p->pos[dim_y]][p->pos[dim_x] + 1].cost > path[p->pos[dim_y]][p->pos[dim_x]].cost) &&
        (path[p->pos[dim_y]][p->pos[dim_x] + 1].cost > 0))
    {

      path[p->pos[dim_y]][p->pos[dim_x] + 1].cost = p->cost +
                                                    cost_rival(world.cur_map->map[p->pos[dim_y]][p->pos[dim_x]]);

      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]][p->pos[dim_x] + 1].hn);
    }

    if ((path[p->pos[dim_y] + 1][p->pos[dim_x]].hn) && // S
        (path[p->pos[dim_y] + 1][p->pos[dim_x]].cost > path[p->pos[dim_y]][p->pos[dim_x]].cost) &&
        (path[p->pos[dim_y] + 1][p->pos[dim_x]].cost > 0))
    {

      path[p->pos[dim_y] + 1][p->pos[dim_x]].cost = p->cost +
                                                    cost_rival(world.cur_map->map[p->pos[dim_y]][p->pos[dim_x]]);

      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1][p->pos[dim_x]].hn);
    }

    if ((path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].hn) && // SW
        (path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].cost > path[p->pos[dim_y]][p->pos[dim_x]].cost) &&
        (path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].cost > 0))
    {

      path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].cost = p->cost +
                                                        cost_rival(world.cur_map->map[p->pos[dim_y]][p->pos[dim_x]]);

      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].hn);
    }

    if ((path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].hn) && // SE
        (path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].cost > path[p->pos[dim_y]][p->pos[dim_x]].cost) &&
        (path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].cost > 0))
    {

      path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].cost = p->cost +
                                                        cost_rival(world.cur_map->map[p->pos[dim_y]][p->pos[dim_x]]);

      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].hn);
    }
  }

  heap_delete(&h);
  printf("Rival dist map:\n");

  for (int i = 0; i < MAP_Y; i++)
  {
    for (int j = 0; j < MAP_X; j++)
    {
      world.cur_map->map_pathr[i][j] = path[i][j];
      if (path[i][j].cost < 0)
      {
        printf("   ");
      }
      else
      {
        if (i == world.cur_map->player.y && j == world.cur_map->player.x)
        {
          printf(" @ ");
        }
        else
        {
          printf("%02d ", path[i][j].cost % 100);
        }
      }
    }
    printf("\n");
  }
}

int main(int argc, char *argv[]){
  struct timeval tv;
  uint32_t seed;

  if (argc == 2)
  {
    seed = atoi(argv[1]);
  }
  else
  {
    gettimeofday(&tv, NULL);
    seed = (tv.tv_usec ^ (tv.tv_sec << 20)) & 0xffffffff;
  }

  printf("Using seed: %u\n", seed);
  srand(seed);

  init_world();

  print_map();

  // path prints
  dijkstra_path_hiker(world.cur_map);
  dijkstra_path_rival(world.cur_map);

  delete_world();

  return 0;
}
