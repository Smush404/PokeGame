#ifndef POKE327_H
#define POKE327_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define MAP_X              80
#define MAP_Y              21

#include "heap.h"

typedef enum dim {
  dim_x,
  dim_y,
  num_dims
} dim_t;

typedef uint8_t pair_t[num_dims];

typedef struct queue_node {
  int x, y;
  struct queue_node *next;
} queue_node_t;

typedef enum __attribute__ ((__packed__)) terrain_type {
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
  ter_water
} terrain_type_t;

typedef struct path {
  heap_node_t *hn;
  uint8_t pos[2];
  uint8_t from[2];
  int32_t cost;
} path_t;

typedef struct map {
  terrain_type_t map[MAP_Y][MAP_X];
  uint8_t height[MAP_Y][MAP_X];
  uint8_t n, s, e, w;
} map_t;

static int32_t path_cmp(const void *key, const void *with);
static int32_t edge_penalty(uint8_t x, uint8_t y);
static void dijkstra_path(map_t *m, pair_t from, pair_t to);
static int build_paths(map_t *m);
static int smooth_height(map_t *m);
static void find_building_location(map_t *m, pair_t p);
static int place_pokemart(map_t *m);
static int place_center(map_t *m);
static terrain_type_t border_type(map_t *m, int32_t x, int32_t y);
static int map_terrain(map_t *m, uint8_t n, uint8_t s, uint8_t e, uint8_t w);
static int place_boulders(map_t *m);
static int place_trees(map_t *m);
static int new_map(map_t *m);
static void print_map(map_t *m);

#endif