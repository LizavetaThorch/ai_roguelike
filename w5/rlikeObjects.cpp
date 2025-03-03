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
    .set(MeleeDamage{20.f})
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
    .set(MeleeDamage{50.f});
}

void create_heal(flecs::world &ecs, int x, int y, float amount)
{
  ecs.entity()
    .set(Position{x, y})
    .set(HealAmount{amount})
    .set(Color{0xff, 0x44, 0x44, 0xff});
}

void create_powerup(flecs::world &ecs, int x, int y, float amount)
{
  ecs.entity()
    .set(Position{x, y})
    .set(PowerupAmount{amount})
    .set(Color{0xff, 0xff, 0x00, 0xff});
}

// Функция для создания спавн-точек
void create_spawn_points(flecs::world& ecs, int num_spawns, int team) {
    for (int i = 0; i < num_spawns; ++i) {
        Position spawn_pos = dungeon::find_walkable_tile(ecs); // Ищем проходимую клетку
        printf("Создана спавн-точка команды %d в (%.2f, %.2f)\n", team, spawn_pos.x, spawn_pos.y);

        ecs.entity()
            .set(spawn_pos)
            .set(SpawnPoint{ 0.0f, 5.0f, team });
    }
}

// ?? Система, которая спавнит юнитов
void spawn_units(flecs::world& ecs) {
    ecs.system<SpawnPoint, Position>()
        .each([&](flecs::entity e, SpawnPoint& spawner, Position& pos) {
        spawner.spawnTimer -= ecs.delta_time(); // Уменьшаем таймер

        if (spawner.spawnTimer <= 0.0f) { // Если пора спавнить нового юнита
            const char* texture = spawner.team == 0 ? "swordsman_tex" : "minotaur_tex";
            Color color = spawner.team == 0 ? WHITE : RED;

            printf("Спавним %s в (%.2f, %.2f)\n", (spawner.team == 0 ? "рыцаря" : "монстра"), pos.x, pos.y);

            create_monster(ecs, color, texture); // Используем ecs, а не e.world()

            spawner.spawnTimer = spawner.spawnRate; // Сброс таймера
        }
            });
}

// Добавляем спавны при инициализации игры
void setup_spawners(flecs::world& ecs) {
    create_spawn_points(ecs, 3, 0); // 3 спавна для рыцарей
    create_spawn_points(ecs, 3, 1); // 3 спавна для монстров

    ecs.system<SpawnPoint, Position>()
        .each([&](flecs::entity e, SpawnPoint& spawner, Position& pos) {
        spawner.spawnTimer -= ecs.delta_time(); // Уменьшаем таймер

        if (spawner.spawnTimer <= 0.0f) {
            const char* texture = spawner.team == 0 ? "swordsman_tex" : "minotaur_tex";
            Color color = spawner.team == 0 ? WHITE : RED;

            printf("Спавним %s в (%.2f, %.2f)\n", (spawner.team == 0 ? "рыцаря" : "монстра"), pos.x, pos.y);

            create_monster(ecs, color, texture); // Используем ecs, а не e.world()

            spawner.spawnTimer = spawner.spawnRate;
        }
            });
}