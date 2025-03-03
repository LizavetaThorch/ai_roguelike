#pragma once
#include <flecs.h>
#include "raylib.h"
#include "ecsTypes.h"

flecs::entity create_hive(flecs::entity e);
flecs::entity create_monster(flecs::world &ecs, Color col, const char *texture_src);
void create_player(flecs::world &ecs, const char *texture_src);
void create_heal(flecs::world &ecs, int x, int y, float amount);
void create_powerup(flecs::world &ecs, int x, int y, float amount);


flecs::entity create_spawn_point(flecs::world& ecs, int team);
flecs::entity create_heal(flecs::world& ecs, Position pos, float amount);