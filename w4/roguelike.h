#pragma once

#ifndef ROGUELIKE_H
#define ROGUELIKE_H

#include <flecs.h>
#include "ecsTypes.h"

constexpr float tile_size = 512.f;

void init_roguelike(flecs::world &ecs);
void init_dungeon(flecs::world &ecs, char *tiles, size_t w, size_t h);
void process_turn(flecs::world &ecs);
void print_stats(flecs::world &ecs);

// NEW
Position find_free_dungeon_tile(flecs::world& ecs);


#endif