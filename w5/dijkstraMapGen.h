#pragma once
#include <vector>
#include <flecs.h>

#include "ecsTypes.h"

namespace dmaps
{
  void gen_player_approach_map(flecs::world &ecs, std::vector<float> &map);
  void gen_player_flee_map(flecs::world &ecs, std::vector<float> &map);
  void gen_hive_pack_map(flecs::world &ecs, std::vector<float> &map);

  void gen_spawn_points_map(flecs::world& ecs, std::vector<float>& map, int team);
  void gen_team_positions_map(flecs::world& ecs, std::vector<float>& map, int team);
  void gen_heal_points_map(flecs::world& ecs, std::vector<float>& map);

  void generate_flow_map(const std::vector<float>& dijkstra_map, const DungeonData& dd, std::vector<Position>& flow_map);
};
