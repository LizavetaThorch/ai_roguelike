#include "ecsTypes.h"
#include "dmapFollower.h"
#include "dijkstraMapGen.h"
#include <cmath>

void process_dmap_followers(flecs::world& ecs)
{
    auto processDmapFollowers = ecs.query<const Position, Action, const DmapWeights>();
    auto dungeonDataQuery = ecs.query<const DungeonData>();

    // Лямбда для получения значения из карты Дейкстры
    auto get_dmap_at = [&](const DijkstraMapData& dmap, const DungeonData& dd, size_t x, size_t y, float mult, float pow)
        {
            const float v = dmap.map[y * dd.width + x];
            if (v < 1e5f)
                return powf(v * mult, pow);
            return v;
        };

    // Лямбда для генерации флоумапа
    auto gen_flow_map = [&](const std::vector<float>& dijkstraMap, const DungeonData& dd, std::vector<Position>& flowMap) {
            flowMap.resize(dd.width * dd.height);

            for (size_t y = 0; y < dd.height; ++y) {
                for (size_t x = 0; x < dd.width; ++x) {
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
    };

    dungeonDataQuery.each([&](const DungeonData& dd) {
            processDmapFollowers.each([&](const Position& pos, Action& act, const DmapWeights& wt) {
                    float moveWeights[EA_MOVE_END];
                    for (size_t i = 0; i < EA_MOVE_END; ++i)
                        moveWeights[i] = 0.f;

                    for (const auto& pair : wt.weights)
                    {
                        ecs.entity(pair.first.c_str()).get([&](const DijkstraMapData& dmap)
                            {
                                std::vector<Position> flowMap;
                                gen_flow_map(dmap.map, dd, flowMap);

                                size_t idx = pos.y * dd.width + pos.x;
                                Position dir = flowMap[idx];

                                if (dir.x == -1) moveWeights[EA_MOVE_LEFT] += 1.f;
                                if (dir.x == 1) moveWeights[EA_MOVE_RIGHT] += 1.f;
                                if (dir.y == -1) moveWeights[EA_MOVE_UP] += 1.f;
                                if (dir.y == 1) moveWeights[EA_MOVE_DOWN] += 1.f;
                            });
                    }

                    float minWt = moveWeights[EA_NOP];
                    for (size_t i = 0; i < EA_MOVE_END; ++i)
                    {
                        if (moveWeights[i] < minWt)
                        {
                            minWt = moveWeights[i];
                            act.action = static_cast<int>(i);
                        }
                    }
            });
    });
}