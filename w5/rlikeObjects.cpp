#include "rlikeObjects.h"
#include "ecsTypes.h"
#include "dungeonUtils.h"
#include "blackboard.h"

#include <cstdio>  // Для отладочных сообщений


flecs::entity create_hive(flecs::entity e)
{
  e.add<Hive>();
  return e;
}

static Position find_free_dungeon_tile(flecs::world &ecs)
{
  auto findMonstersQuery = ecs.query<const Position, const Hitpoints>();
  bool done = false;
  while (!done)
  {
    done = true;
    Position pos = dungeon::find_walkable_tile(ecs);
    findMonstersQuery.each([&](const Position &p, const Hitpoints&)
    {
      if (p == pos)
        done = false;
    });
    if (done)
      return pos;
  };
  return {0, 0};
}


flecs::entity create_monster(flecs::world &ecs, Color col, const char *texture_src)
{
  Position pos = find_free_dungeon_tile(ecs);

  flecs::entity textureSrc = ecs.entity(texture_src);
  return ecs.entity()
    .set(Position{pos.x, pos.y})
    .set(MovePos{pos.x, pos.y})
    .set(Hitpoints{100.f})
    .set(Action{EA_NOP})
    .set(Color{col})
    .add<TextureSource>(textureSrc)
    .set(Team{1})
    .set(NumActions{1, 0})
    .set(MeleeDamage{45.f})
    .set(Blackboard{});
}

void create_player(flecs::world &ecs, const char *texture_src)
{
  Position pos = find_free_dungeon_tile(ecs);

  flecs::entity textureSrc = ecs.entity(texture_src);
  ecs.entity("player")
    .set(Position{pos.x, pos.y})
    .set(MovePos{pos.x, pos.y})
    .set(Hitpoints{100.f})
    .set(Action{EA_NOP})
    .add<IsPlayer>()
    .set(Team{0})
    .set(PlayerInput{})
    .set(NumActions{2, 0})
    .set(Color{255, 255, 255, 255})
    .add<TextureSource>(textureSrc)
    .set(MeleeDamage{30.f});
}

void create_powerup(flecs::world &ecs, int x, int y, float amount)
{
  ecs.entity()
    .set(Position{x, y})
    .set(PowerupAmount{amount})
    .set(Color{0xff, 0xff, 0x00, 0xff});
}

flecs::entity create_spawn_point(flecs::world& ecs, int team) {
    Position pos = dungeon::find_walkable_tile(ecs);
    return ecs.entity()
        .set(Position{ pos.x, pos.y })
        .set(SpawnPoint{ 0.f, 5.f, team });
}

flecs::entity create_heal(flecs::world& ecs, Position pos, float amount)
{
    return ecs.entity()
        .set(Position{ pos.x, pos.y })
        .set(HealAmount{ amount })
        .set(Color{ 30, 144, 255, 255 })
        .add<IsHealPoint>();
}
