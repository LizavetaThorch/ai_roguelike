#include "roguelike.h"
#include "ecsTypes.h"
#include "raylib.h"
#include "stateMachine.h"
#include "aiLibrary.h"
#include "blackboard.h"
#include "math.h"
#include "dungeonUtils.h"
#include "dijkstraMapGen.h"
#include "dmapFollower.h"
#include "dmapBeh.h"
#include "rlikeObjects.h"


static void register_roguelike_systems(flecs::world& ecs)
{
    ecs.system<PlayerInput, Action, const IsPlayer>()
        .each([&](PlayerInput& inp, Action& a, const IsPlayer)
            {
                bool left = IsKeyDown(KEY_LEFT);
                bool right = IsKeyDown(KEY_RIGHT);
                bool up = IsKeyDown(KEY_UP);
                bool down = IsKeyDown(KEY_DOWN);
                if (left && !inp.left)
                    a.action = EA_MOVE_LEFT;
                if (right && !inp.right)
                    a.action = EA_MOVE_RIGHT;
                if (up && !inp.up)
                    a.action = EA_MOVE_UP;
                if (down && !inp.down)
                    a.action = EA_MOVE_DOWN;
                inp.left = left;
                inp.right = right;
                inp.up = up;
                inp.down = down;

                bool pass = IsKeyDown(KEY_SPACE);
                if (pass && !inp.passed)
                    a.action = EA_PASS;
                inp.passed = pass;
            });
    ecs.system<const Position, const Color>()
        .with<TextureSource>(flecs::Wildcard)
        .with<BackgroundTile>()
        .each([&](flecs::entity e, const Position& pos, const Color color)
            {
                const auto textureSrc = e.target<TextureSource>();
                DrawTextureQuad(*textureSrc.get<Texture2D>(),
                    Vector2{ 1, 1 }, Vector2{ 0, 0 },
                    Rectangle{ float(pos.x) * tile_size, float(pos.y) * tile_size, tile_size, tile_size }, color);
            });
    ecs.system<const Position, const Color>()
        .without<TextureSource>(flecs::Wildcard)
        .each([&](const Position& pos, const Color color)
            {
                const Rectangle rect = { float(pos.x) * tile_size, float(pos.y) * tile_size, tile_size, tile_size };
                DrawRectangleRec(rect, color);
            });
    ecs.system<const Position, const Color>()
        .with<TextureSource>(flecs::Wildcard)
        .without<BackgroundTile>()
        .each([&](flecs::entity e, const Position& pos, const Color color)
            {
                const auto textureSrc = e.target<TextureSource>();
                DrawTextureQuad(*textureSrc.get<Texture2D>(),
                    Vector2{ 1, 1 }, Vector2{ 0, 0 },
                    Rectangle{ float(pos.x) * tile_size, float(pos.y) * tile_size, tile_size, tile_size }, color);
            });
    ecs.system<const Position, const Hitpoints>()
        .each([&](const Position& pos, const Hitpoints& hp)
            {
                constexpr float hpPadding = 0.05f;
                const float hpWidth = 1.f - 2.f * hpPadding;
                const Rectangle underRect = { float(pos.x + hpPadding) * tile_size, float(pos.y - 0.25f) * tile_size,
                                             hpWidth * tile_size, 0.1f * tile_size };
                DrawRectangleRec(underRect, BLACK);
                const Rectangle hpRect = { float(pos.x + hpPadding) * tile_size, float(pos.y - 0.25f) * tile_size,
                                          hp.hitpoints / 100.f * hpWidth * tile_size, 0.1f * tile_size };
                DrawRectangleRec(hpRect, RED);
            });

    ecs.system<Texture2D>()
        .each([&](Texture2D& tex)
            {
                SetTextureFilter(tex, TEXTURE_FILTER_POINT);
            });
    ecs.system<const DmapWeights>()
        .with<VisualiseMap>()
        .each([&](const DmapWeights& wt)
            {
                auto dungeonDataQuery = ecs.query<const DungeonData>();
                dungeonDataQuery.each([&](const DungeonData& dd)
                    {
                        for (size_t y = 0; y < dd.height; ++y)
                            for (size_t x = 0; x < dd.width; ++x)
                            {
                                float sum = 0.f;
                                for (const auto& pair : wt.weights)
                                {
                                    ecs.entity(pair.first.c_str()).get([&](const DijkstraMapData& dmap)
                                        {
                                            float v = dmap.map[y * dd.width + x];
                                            if (v < 1e5f)
                                                sum += powf(v * pair.second.mult, pair.second.pow);
                                            else
                                                sum += v;
                                        });
                                }
                                if (sum < 1e5f)
                                    DrawText(TextFormat("%.1f", sum),
                                        int((float(x) + 0.2f) * tile_size), int((float(y) + 0.5f) * tile_size), 150, WHITE);
                            }
                    });
            });
    ecs.system<const DijkstraMapData>()
        .with<VisualiseMap>()
        .each([&](const DijkstraMapData& dmap)
            {
                auto dungeonDataQuery = ecs.query<const DungeonData>();
                dungeonDataQuery.each([&](const DungeonData& dd)
                    {
                        for (size_t y = 0; y < dd.height; ++y)
                            for (size_t x = 0; x < dd.width; ++x)
                            {
                                const float val = dmap.map[y * dd.width + x];
                                if (val < 1e5f)
                                    DrawText(TextFormat("%.1f", val),
                                        int((float(x) + 0.2f) * tile_size), int((float(y) + 0.5f) * tile_size), 150, WHITE);
                            }
                    });
        });
 }


void init_roguelike(flecs::world& ecs)
{
    printf("INIT roguelike START\n");
    register_roguelike_systems(ecs);
    create_spawn_point(ecs, 0); // ������
    create_spawn_point(ecs, 1); // �������

    ecs.system<>().each([&]() {
        spawn_system(ecs);
    });

    create_spawn_point(ecs, 0); // ������
    create_spawn_point(ecs, 1); // �������

    if (!FileExists("assets/swordsman.png") || !FileExists("assets/minotaur.png")) {
        printf("ERROR: could not find swordsman.png or minotaur.png!\n");
    }
    else {
        printf("Textures swordsman.png and minotaur.png uploaded successfully\n");
    }

    ecs.entity("swordsman_tex")
        .set(Texture2D{ LoadTexture("assets/swordsman.png") });
    ecs.entity("minotaur_tex")
        .set(Texture2D{ LoadTexture("assets/minotaur.png") });

    ecs.observer<Texture2D>()
        .event(flecs::OnRemove)
        .each([](Texture2D texture)
            {
                UnloadTexture(texture);
            });

    printf("Creating monsters and a player\n");
    create_hive_monster(create_monster(ecs, Color{ 0xee, 0x00, 0xee, 0xff }, "minotaur_tex"));
    create_hive_monster(create_monster(ecs, Color{ 0xee, 0x00, 0xee, 0xff }, "minotaur_tex"));
    create_hive_monster(create_monster(ecs, Color{ 0x11, 0x11, 0x11, 0xff }, "minotaur_tex"));
    create_hive(create_player_fleer(create_monster(ecs, Color{ 0, 255, 0, 255 }, "minotaur_tex")));

    create_player(ecs, "swordsman_tex");

    ecs.entity("world")
        .set(TurnCounter{})
        .set(ActionLog{});
    
    for (int i = 0; i < 5; ++i)
    {
        Position pos = dungeon::find_walkable_tile(ecs);
        create_heal(ecs, pos, 50.f);
    }
    printf("Initialization of the roguelike is complete\n");
}

void init_dungeon(flecs::world& ecs, char* tiles, size_t w, size_t h)
{
    printf("Initialization of the dungeon has begun\n");
    flecs::entity wallTex = ecs.entity("wall_tex")
        .set(Texture2D{ LoadTexture("assets/wall.png") });
    flecs::entity floorTex = ecs.entity("floor_tex")
        .set(Texture2D{ LoadTexture("assets/floor.png") });

    if (!FileExists("assets/wall.png") || !FileExists("assets/floor.png")) {
        printf("Error: could not find wall.png or floor.png!\n");
    }
    else {
        printf("Textures wall.png and floor.png uploaded successfully\n");
    }

    std::vector<char> dungeonData;
    dungeonData.resize(w * h);
    for (size_t y = 0; y < h; ++y)
        for (size_t x = 0; x < w; ++x)
            dungeonData[y * w + x] = tiles[y * w + x];

    ecs.entity("dungeon")
        .set(DungeonData{ dungeonData, w, h })
        .add<TextureSource>(floorTex);

    for (size_t y = 0; y < h; ++y)
        for (size_t x = 0; x < w; ++x)
        {
            char tile = tiles[y * w + x];
            flecs::entity tileEntity = ecs.entity()
                .add<BackgroundTile>()
                .set(Position{ int(x), int(y) })
                .set(Color{ 255, 255, 255, 255 });

            if (tile == dungeon::wall) {
                tileEntity.add<TextureSource>(wallTex);
                printf("The wall at (%d, %d) has been added\n", int(x), int(y));
            }
            else if (tile == dungeon::floor) {
                tileEntity.add<TextureSource>(floorTex);
                printf("FLOOR on (%d, %d) added{n}", int(x), int(y));
            }
        }
    printf("The initialization of the dungeon is complete\n");
}



static bool is_player_acted(flecs::world& ecs)
{
    auto processPlayer = ecs.query<const IsPlayer, const Action>();
    bool playerActed = false;
    processPlayer.each([&](const IsPlayer, const Action& a)
        {
            playerActed = a.action != EA_NOP;
        });
    return playerActed;
}

static bool upd_player_actions_count(flecs::world& ecs)
{
    auto updPlayerActions = ecs.query<const IsPlayer, NumActions>();
    bool actionsReached = false;
    updPlayerActions.each([&](const IsPlayer, NumActions& na)
        {
            na.curActions = (na.curActions + 1) % na.numActions;
            actionsReached |= na.curActions == 0;
        });
    return actionsReached;
}

static Position move_pos(Position pos, int action)
{
    if (action == EA_MOVE_LEFT)
        pos.x--;
    else if (action == EA_MOVE_RIGHT)
        pos.x++;
    else if (action == EA_MOVE_UP)
        pos.y--;
    else if (action == EA_MOVE_DOWN)
        pos.y++;
    return pos;
}

static void push_to_log(flecs::world& ecs, const char* msg)
{
    auto queryLog = ecs.query<ActionLog, const TurnCounter>();
    queryLog.each([&](ActionLog& l, const TurnCounter& c)
        {
            l.log.push_back(std::to_string(c.count) + ": " + msg);
            if (l.log.size() > l.capacity)
                l.log.erase(l.log.begin());
        });
}

static void process_actions(flecs::world& ecs)
{
    auto processActions = ecs.query<Action, Position, MovePos, const MeleeDamage, const Team>();
    auto processHeals = ecs.query<Action, Hitpoints>();
    auto checkAttacks = ecs.query<const MovePos, Hitpoints, const Team>();

    auto processEnemyAttacks = ecs.query<const Position, const Team, const MeleeDamage>();
    auto checkPlayerAttacks = ecs.query<const Position, Hitpoints, const IsPlayer>();

    // Process all actions
    ecs.defer([&]
        {
            processHeals.each([&](Action& a, Hitpoints& hp)
                {
                    if (a.action != EA_HEAL_SELF)
                        return;
                    a.action = EA_NOP;
                    push_to_log(ecs, "Monster healed itself");
                    hp.hitpoints += 10.f;

                });
            processActions.each([&](flecs::entity entity, Action& a, Position& pos, MovePos& mpos, const MeleeDamage& dmg, const Team& team)
                {
                    Position nextPos = move_pos(pos, a.action);
                    bool blocked = !dungeon::is_tile_walkable(ecs, nextPos);
                    checkAttacks.each([&](flecs::entity enemy, const MovePos& epos, Hitpoints& hp, const Team& enemy_team)
                        {
                            if (entity != enemy && epos == nextPos)
                            {
                                blocked = true;
                                if (team.team != enemy_team.team)
                                {
                                    push_to_log(ecs, "damaged entity");
                                    hp.hitpoints -= dmg.damage;
                                }
                            }
                        });
                    if (blocked)
                        a.action = EA_NOP;
                    else
                        mpos = nextPos;
                });
            // now move
            processActions.each([&](Action& a, Position& pos, MovePos& mpos, const MeleeDamage&, const Team&)
                {
                    pos = mpos;
                    a.action = EA_NOP;
                });
        });

    auto deleteAllDead = ecs.query<const Hitpoints>();
    ecs.defer([&]
        {
            deleteAllDead.each([&](flecs::entity entity, const Hitpoints& hp)
                {
                    if (hp.hitpoints <= 0.f)
                        entity.destruct();
                });
        });

    auto playerPickup = ecs.query<const IsPlayer, const Position, Hitpoints, MeleeDamage>();
    auto healPickup = ecs.query<const Position, const HealAmount>();
    auto powerupPickup = ecs.query<const Position, const PowerupAmount>();

    ecs.defer([&]
        {
            playerPickup.each([&](const IsPlayer&, const Position& pos, Hitpoints& hp, MeleeDamage& dmg)
                {
                    healPickup.each([&](flecs::entity entity, const Position& ppos, const HealAmount& amt)
                        {
                            if (pos == ppos && hp.hitpoints < 100.f)
                            {
                                hp.hitpoints = hp.maxHitpoints;
                                entity.destruct();
                                push_to_log(ecs, "Player healed by a health pack");
                            }
                        });

                    powerupPickup.each([&](flecs::entity entity, const Position& ppos, const PowerupAmount& amt)
                        {
                            if (pos == ppos)
                            {
                                dmg.damage += amt.amount;
                                entity.destruct();
                                push_to_log(ecs, "Player picked up a powerup");
                            }
                        });
                });
        });
    
}

template<typename T>
static void push_info_to_bb(Blackboard& bb, const char* name, const T& val)
{
    size_t idx = bb.regName<T>(name);
    bb.set(idx, val);
}

// sensors
static void gather_world_info(flecs::world& ecs)
{
    auto gatherWorldInfo = ecs.query<Blackboard,
        const Position, const Hitpoints,
        const WorldInfoGatherer,
        const Team>();
    auto alliesQuery = ecs.query<const Position, const Team>();
    gatherWorldInfo.each([&](Blackboard& bb, const Position& pos, const Hitpoints& hp,
        WorldInfoGatherer, const Team& team)
        {
            // first gather all needed names (without cache)
            push_info_to_bb(bb, "hp", hp.hitpoints);
            float numAllies = 0; // note float
            float closestEnemyDist = 100.f;
            alliesQuery.each([&](const Position& apos, const Team& ateam)
                {
                    constexpr float limitDist = 5.f;
                    if (team.team == ateam.team && dist_sq(pos, apos) < sqr(limitDist))
                        numAllies += 1.f;
                    if (team.team != ateam.team)
                    {
                        const float enemyDist = dist(pos, apos);
                        if (enemyDist < closestEnemyDist)
                            closestEnemyDist = enemyDist;
                    }
                });
            push_info_to_bb(bb, "alliesNum", numAllies);
            push_info_to_bb(bb, "enemyDist", closestEnemyDist);
        });
}


static void process_melee_damage(flecs::world& ecs) {
    auto damageDealersQuery = ecs.query<const Position, const MeleeDamage, const Team>();
    auto damageReceiversQuery = ecs.query<const Position, Hitpoints, const Team>();

    damageDealersQuery.each([&](flecs::entity dealer, const Position& dealerPos, const MeleeDamage& damage, const Team& dealerTeam) {
        damageReceiversQuery.each([&](flecs::entity receiver, const Position& receiverPos, Hitpoints& hp, const Team& receiverTeam) {
            if (dealer != receiver && dealerTeam.team != receiverTeam.team) {
                int dx = abs(dealerPos.x - receiverPos.x);
                int dy = abs(dealerPos.y - receiverPos.y);

                if ((dx == 1 && dy == 0) || (dx == 0 && dy == 1)) {
                    hp.hitpoints -= damage.damage;
                    push_to_log(ecs, "Entity damaged another entity");
                }
            }
            });
        });
}


void process_turn(flecs::world& ecs)
{
    auto stateMachineAct = ecs.query<StateMachine>();
    auto behTreeUpdate = ecs.query<BehaviourTree, Blackboard>();
    auto turnIncrementer = ecs.query<TurnCounter>();
    if (is_player_acted(ecs))
    {
        if (upd_player_actions_count(ecs))
        {
            // Plan action for NPCs
            gather_world_info(ecs);
            ecs.defer([&]
                {
                    stateMachineAct.each([&](flecs::entity e, StateMachine& sm)
                        {
                            sm.act(0.f, ecs, e);
                        });
                    behTreeUpdate.each([&](flecs::entity e, BehaviourTree& bt, Blackboard& bb)
                        {
                            bt.update(ecs, e, bb);
                        });
                    process_dmap_followers(ecs);
                });
            turnIncrementer.each([](TurnCounter& tc) { tc.count++; });
        }
        process_actions(ecs);
        process_melee_damage(ecs);

        // ��������� ���� ��������
        std::vector<float> approachMap;
        dmaps::gen_player_approach_map(ecs, approachMap);
        ecs.entity("approach_map")
            .set(DijkstraMapData{ approachMap });

        std::vector<float> fleeMap;
        dmaps::gen_player_flee_map(ecs, fleeMap);
        ecs.entity("flee_map")
            .set(DijkstraMapData{ fleeMap });

        std::vector<float> hiveMap;
        dmaps::gen_hive_pack_map(ecs, hiveMap);
        ecs.entity("hive_map")
            .set(DijkstraMapData{ hiveMap });

        // ����� �������� ��� ����� ������
        std::vector<float> spawnMapKnights;
        std::vector<float> spawnMapMonsters;
        dmaps::gen_spawn_points_map(ecs, spawnMapKnights, 0);  // ������
        dmaps::gen_spawn_points_map(ecs, spawnMapMonsters, 1); // �������

        ecs.entity("spawn_map_knights").set(DijkstraMapData{ spawnMapKnights });
        ecs.entity("spawn_map_monsters").set(DijkstraMapData{ spawnMapMonsters });

        // ����� �������� ��� ������� �������� � �������
        std::vector<float> teamMapKnights;
        std::vector<float> teamMapMonsters;
        dmaps::gen_team_positions_map(ecs, teamMapKnights, 0);  // ����� �������
        dmaps::gen_team_positions_map(ecs, teamMapMonsters, 1); // ����� ��������

        ecs.entity("team_map_knights").set(DijkstraMapData{ teamMapKnights });
        ecs.entity("team_map_monsters").set(DijkstraMapData{ teamMapMonsters });

        // ����� �������� ��� ����� �������
        std::vector<float> healMap;
        dmaps::gen_heal_points_map(ecs, healMap);

        ecs.entity("heal_map").set(DijkstraMapData{ healMap });

        //ecs.entity("flee_map").add<VisualiseMap>();
        ecs.entity("hive_follower_sum")
            .set(DmapWeights{ {{"hive_map", {1.f, 1.f}}, {"approach_map", {1.8f, 0.8f}}} })
            .add<VisualiseMap>();
    }
}

void print_stats(flecs::world& ecs)
{
    auto playerStatsQuery = ecs.query<const IsPlayer, const Hitpoints, const MeleeDamage>();
    playerStatsQuery.each([&](const IsPlayer&, const Hitpoints& hp, const MeleeDamage& dmg)
        {
            DrawText(TextFormat("hp: %d", int(hp.hitpoints)), 20, 20, 20, WHITE);
            DrawText(TextFormat("power: %d", int(dmg.damage)), 20, 40, 20, WHITE);
        });

    auto actionLogQuery = ecs.query<const ActionLog>();
    actionLogQuery.each([&](const ActionLog& l)
        {
            int yPos = GetRenderHeight() - 20;
            for (const std::string& msg : l.log)
            {
                DrawText(msg.c_str(), 20, yPos, 20, WHITE);
                yPos -= 20;
            }
        });
}


static void spawn_system(flecs::world& ecs) {
    auto spawnQuery = ecs.query<const Position, SpawnPoint>();

    spawnQuery.each([&](const Position& pos, SpawnPoint& sp) {
        sp.timeToSpawn -= ecs.delta_time();
        if (sp.timeToSpawn <= 0.f) {
            sp.timeToSpawn = sp.timeBetweenSpawns;

            if (sp.team == 0) {
                // ����� ������
                create_monster(ecs, Color{ 0, 255, 0, 255 }, "swordsman_tex");
            }
            else {
                // ����� �������
                create_monster(ecs, Color{ 0xff, 0x00, 0x00, 0xff }, "minotaur_tex");
            }
        }
        });
}
