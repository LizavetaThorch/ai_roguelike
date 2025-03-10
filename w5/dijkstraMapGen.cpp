#include "dijkstraMapGen.h"
#include "ecsTypes.h"
#include "dungeonUtils.h"

template<typename Callable>
static void query_dungeon_data(flecs::world &ecs, Callable c)
{
  auto dungeonDataQuery = ecs.query<const DungeonData>();

  dungeonDataQuery.each(c);
}

template<typename Callable>
static void query_characters_positions(flecs::world &ecs, Callable c)
{
  auto characterPositionQuery = ecs.query<const Position, const Team>();

  characterPositionQuery.each(c);
}

constexpr float invalid_tile_value = 1e5f;

static void init_tiles(std::vector<float> &map, const DungeonData &dd)
{
  map.resize(dd.width * dd.height);
  for (float &v : map)
    v = invalid_tile_value;
}

// scan version, could be implemented as Dijkstra version as well
static void process_dmap(std::vector<float> &map, const DungeonData &dd)
{
  bool done = false;
  auto getMapAt = [&](size_t x, size_t y, float def)
  {
    if (x < dd.width && y < dd.width && dd.tiles[y * dd.width + x] == dungeon::floor)
      return map[y * dd.width + x];
    return def;
  };
  auto getMinNei = [&](size_t x, size_t y)
  {
    float val = map[y * dd.width + x];
    val = std::min(val, getMapAt(x - 1, y + 0, val));
    val = std::min(val, getMapAt(x + 1, y + 0, val));
    val = std::min(val, getMapAt(x + 0, y - 1, val));
    val = std::min(val, getMapAt(x + 0, y + 1, val));
    return val;
  };
  while (!done)
  {
    done = true;
    for (size_t y = 0; y < dd.height; ++y)
      for (size_t x = 0; x < dd.width; ++x)
      {
        const size_t i = y * dd.width + x;
        if (dd.tiles[i] != dungeon::floor)
          continue;
        const float myVal = getMapAt(x, y, invalid_tile_value);
        const float minVal = getMinNei(x, y);
        if (minVal < myVal - 1.f)
        {
          map[i] = minVal + 1.f;
          done = false;
        }
      }
  }
}

void dmaps::gen_player_approach_map(flecs::world &ecs, std::vector<float> &map)
{
  query_dungeon_data(ecs, [&](const DungeonData &dd)
  {
    init_tiles(map, dd);
    query_characters_positions(ecs, [&](const Position &pos, const Team &t)
    {
      if (t.team == 0) // player team hardcode
        map[pos.y * dd.width + pos.x] = 0.f;
    });
    process_dmap(map, dd);
  });
}

void dmaps::gen_player_flee_map(flecs::world &ecs, std::vector<float> &map)
{
  gen_player_approach_map(ecs, map);
  for (float &v : map)
    if (v < invalid_tile_value)
      v *= -1.2f;
  query_dungeon_data(ecs, [&](const DungeonData &dd)
  {
    process_dmap(map, dd);
  });
}

void dmaps::gen_hive_pack_map(flecs::world &ecs, std::vector<float> &map)
{
  auto hiveQuery = ecs.query<const Position, const Hive>();
  query_dungeon_data(ecs, [&](const DungeonData &dd)
  {
    init_tiles(map, dd);
    hiveQuery.each([&](const Position &pos, const Hive &)
    {
      map[pos.y * dd.width + pos.x] = 0.f;
    });
    process_dmap(map, dd);
  });
}

void dmaps::gen_spawn_points_map(flecs::world& ecs, std::vector<float>& map, int team)
{
    auto spawnQuery = ecs.query<const Position, const SpawnPoint>();

    query_dungeon_data(ecs, [&](const DungeonData& dd)
        {
            init_tiles(map, dd);

            spawnQuery.each([&](const Position& pos, const SpawnPoint& sp)
                {
                    if (sp.team == team)
                        map[pos.y * dd.width + pos.x] = 0.f;
                });

            process_dmap(map, dd);
        });
}

void dmaps::gen_team_positions_map(flecs::world& ecs, std::vector<float>& map, int team)
{
    auto teamQuery = ecs.query<const Position, const Team>();

    query_dungeon_data(ecs, [&](const DungeonData& dd)
        {
            init_tiles(map, dd);

            teamQuery.each([&](const Position& pos, const Team& t)
                {
                    if (t.team == team)
                        map[pos.y * dd.width + pos.x] = 0.f;
                });

            process_dmap(map, dd);
        });
}

void dmaps::gen_heal_points_map(flecs::world& ecs, std::vector<float>& map)
{
    auto healQuery = ecs.query<const Position, const HealAmount>();

    query_dungeon_data(ecs, [&](const DungeonData& dd)
        {
            init_tiles(map, dd);

            healQuery.each([&](const Position& pos, const HealAmount&)
                {
                    map[pos.y * dd.width + pos.x] = 0.f;
                });

            process_dmap(map, dd);
        });
}

void dmaps::gen_flow_map(const std::vector<float>& dijkstraMap, const DungeonData& dd, std::vector<Position>& flowMap)
{
    flowMap.resize(dd.width * dd.height);

    for (size_t y = 0; y < dd.height; ++y)
    {
        for (size_t x = 0; x < dd.width; ++x)
        {
            size_t idx = y * dd.width + x;
            float minVal = dijkstraMap[idx];
            Position dir = { 0, 0 };

            if (x > 0 && dijkstraMap[idx - 1] < minVal)
            {
                minVal = dijkstraMap[idx - 1];
                dir = { -1, 0 };
            }
            if (x < dd.width - 1 && dijkstraMap[idx + 1] < minVal)
            {
                minVal = dijkstraMap[idx + 1];
                dir = { 1, 0 };
            }
            if (y > 0 && dijkstraMap[idx - dd.width] < minVal)
            {
                minVal = dijkstraMap[idx - dd.width];
                dir = { 0, -1 };
            }
            if (y < dd.height - 1 && dijkstraMap[idx + dd.width] < minVal)
            {
                minVal = dijkstraMap[idx + dd.width];
                dir = { 0, 1 };
            }

            flowMap[idx] = dir;
        }
    }
}