#include "aiLibrary.h"
#include <flecs.h>
#include "ecsTypes.h"
#include "raylib.h"
#include "math.h"
#include "aiUtils.h"

class AttackEnemyState : public State
{
public:
    void enter() const override {}
    void exit() const override {}
    void act(float/* dt*/, flecs::world&/*ecs*/, flecs::entity /*entity*/) const override {}
};

class MoveToEnemyState : public State
{
public:
    void enter() const override {}
    void exit() const override {}
    void act(float/* dt*/, flecs::world& ecs, flecs::entity entity) const override
    {
        on_closest_enemy_pos(ecs, entity, [&](Action& a, const Position& pos, const Position& enemy_pos)
            {
                a.action = move_towards(pos, enemy_pos);
            });
    }
};

class FleeFromEnemyState : public State
{
public:
    FleeFromEnemyState() {}
    void enter() const override {}
    void exit() const override {}
    void act(float/* dt*/, flecs::world& ecs, flecs::entity entity) const override
    {
        on_closest_enemy_pos(ecs, entity, [&](Action& a, const Position& pos, const Position& enemy_pos)
            {
                a.action = inverse_move(move_towards(pos, enemy_pos));
            });
    }
};

class PatrolState : public State
{
    float patrolDist;
public:
    PatrolState(float dist) : patrolDist(dist) {}
    void enter() const override {}
    void exit() const override {}
    void act(float/* dt*/, flecs::world&, flecs::entity entity) const override
    {
        entity.insert([&](const Position& pos, const PatrolPos& ppos, Action& a)
            {
                if (dist(pos, ppos) > patrolDist)
                    a.action = move_towards(pos, ppos); // do a recovery walk
                else
                {
                    // do a random walk
                    a.action = GetRandomValue(EA_MOVE_START, EA_MOVE_END - 1);
                }
            });
    }
};

class NopState : public State
{
public:
    void enter() const override {}
    void exit() const override {}
    void act(float/* dt*/, flecs::world&, flecs::entity) const override {}
};

class EnemyAvailableTransition : public StateTransition
{
    float triggerDist;
public:
    EnemyAvailableTransition(float in_dist) : triggerDist(in_dist) {}
    bool isAvailable(flecs::world& ecs, flecs::entity entity) const override
    {
        auto enemiesQuery = ecs.query<const Position, const Team>();
        bool enemiesFound = false;
        entity.get([&](const Position& pos, const Team& t)
            {
                enemiesQuery.each([&](const Position& epos, const Team& et)
                    {
                        if (t.team == et.team)
                            return;
                        float curDist = dist(epos, pos);
                        enemiesFound |= curDist <= triggerDist;
                    });
            });
        return enemiesFound;
    }
};

class HitpointsLessThanTransition : public StateTransition
{
    float threshold;
public:
    HitpointsLessThanTransition(float in_thres) : threshold(in_thres) {}
    bool isAvailable(flecs::world&, flecs::entity entity) const override
    {
        bool hitpointsThresholdReached = false;
        entity.get([&](const Hitpoints& hp)
            {
                hitpointsThresholdReached |= hp.hitpoints < threshold;
            });
        return hitpointsThresholdReached;
    }
};

class EnemyReachableTransition : public StateTransition
{
public:
    bool isAvailable(flecs::world&, flecs::entity) const override
    {
        return false;
    }
};

class NegateTransition : public StateTransition
{
    const StateTransition* transition; // we own it
public:
    NegateTransition(const StateTransition* in_trans) : transition(in_trans) {}
    ~NegateTransition() override { delete transition; }

    bool isAvailable(flecs::world& ecs, flecs::entity entity) const override
    {
        return !transition->isAvailable(ecs, entity);
    }
};

class AndTransition : public StateTransition
{
    const StateTransition* lhs; // we own it
    const StateTransition* rhs; // we own it
public:
    AndTransition(const StateTransition* in_lhs, const StateTransition* in_rhs) : lhs(in_lhs), rhs(in_rhs) {}
    ~AndTransition() override
    {
        delete lhs;
        delete rhs;
    }

    bool isAvailable(flecs::world& ecs, flecs::entity entity) const override
    {
        return lhs->isAvailable(ecs, entity) && rhs->isAvailable(ecs, entity);
    }
};

// NEW
class SurroundPlayerState : public State
{
public:
    void enter() const override {}
    void exit() const override {}
    void act(float /*dt*/, flecs::world& ecs, flecs::entity entity) const override
    {
        on_closest_enemy_pos(ecs, entity, [&](Action& a, const Position& pos, const Position& enemy_pos)
            {
                Position surroundPos = calculate_surround_position(pos, enemy_pos);
                a.action = move_towards(pos, surroundPos);
            });
    }

private:
    Position calculate_surround_position(const Position& current, const Position& target) const
    {
        Position offset = { target.x - current.x, target.y - current.y };
        if (offset.x != 0) offset.x /= abs(offset.x);
        if (offset.y != 0) offset.y /= abs(offset.y);
        return { target.x + offset.y, target.y - offset.x };
    }
};


// states
State* create_attack_enemy_state()
{
    return new AttackEnemyState();
}
State* create_move_to_enemy_state()
{
    return new MoveToEnemyState();
}

State* create_flee_from_enemy_state()
{
    return new FleeFromEnemyState();
}


State* create_patrol_state(float patrol_dist)
{
    return new PatrolState(patrol_dist);
}

State* create_nop_state()
{
    return new NopState();
}

// NEW
State* create_surround_player_state()
{
    return new SurroundPlayerState();
}

// transitions
StateTransition* create_enemy_available_transition(float dist)
{
    return new EnemyAvailableTransition(dist);
}

StateTransition* create_enemy_reachable_transition()
{
    return new EnemyReachableTransition();
}

StateTransition* create_hitpoints_less_than_transition(float thres)
{
    return new HitpointsLessThanTransition(thres);
}

StateTransition* create_negate_transition(StateTransition* in)
{
    return new NegateTransition(in);
}
StateTransition* create_and_transition(StateTransition* lhs, StateTransition* rhs)
{
    return new AndTransition(lhs, rhs);
}


// NEW
void update_monster_behavior(flecs::world& ecs) {
    auto monsterQuery = ecs.query<const Position, const Team, Action>();

    monsterQuery.each([&](flecs::entity entity, const Position& pos, const Team& team, Action& action) {
        if (team.team != 1) return;

        Position playerPos = find_player_position(ecs);
        bool playerVisible = dist(pos, playerPos) <= 10.0f;

        if (playerVisible) {
            bool allyAttacking = false;
            ecs.query<const Action>().each([&](const Action& allyAction) {
                if (allyAction.action == EA_ATTACK) {
                    allyAttacking = true;
                }
                });

            if (allyAttacking) {
                action.action = EA_SURROUND;
            }
            else {
                action.action = EA_ATTACK;
            }
        }
        else {
            action.action = EA_NOP;
        }
        });
}


bool is_player_attacked(flecs::world& ecs, const Position& playerPos)
{
    bool attacked = false;
    ecs.query<const Position, const Action>().each([&](const Position& pos, const Action& action)
        {
            if (pos == playerPos && action.action == EA_ATTACK)
            {
                attacked = true;
            }
        });
    return attacked;
}

void process_monster_actions(flecs::world& ecs) 
{
    auto actionQuery = ecs.query<Action, Position>();

    actionQuery.each([&](flecs::entity entity, Action& action, Position& monsterPos) 
        {
        if (action.action == EA_SURROUND) 
        {
            Position playerPos = find_player_position(ecs);
            Position enemyPos = find_enemy_position(ecs, monsterPos, 6.0f);
            Position surroundPos = calculate_surround_position(monsterPos, enemyPos);
            move_towards(monsterPos, surroundPos);
        }
        else if (action.action == EA_ATTACK) 
        {
            action.action = EA_ATTACK;
        }
        else if (action.action == EA_NOP)
        {
            action.action == EA_NOP;
        }
        });
}

Position calculate_surround_position(const Position& current, const Position& target) 
{
    Position offset = { target.x - current.x, target.y - current.y };
    if (offset.x != 0) offset.x /= abs(offset.x);
    if (offset.y != 0) offset.y /= abs(offset.y);
    return { target.x + offset.y, target.y - offset.x };
}

Position find_enemy_position(flecs::world& ecs, const Position& currentPos, float maxDistance)
{
    Position closestEnemyPos = { 0, 0 };
    float closestDistance = maxDistance;

    auto enemiesQuery = ecs.query<const Position, const Team>();
    enemiesQuery.each([&](const Position& enemyPos, const Team& enemyTeam)
        {
            if (enemyTeam.team == 1)
            {
                float curDist = dist(currentPos, enemyPos);
                if (curDist < closestDistance)
                {
                    closestEnemyPos = enemyPos;
                    closestDistance = curDist;
                }
            }
        });

    return closestEnemyPos;
}


Position find_player_position(flecs::world& ecs)
{
    Position playerPos = { 0, 0 };
    auto playerQuery = ecs.query<const Position>();
    playerQuery.each([&](const Position& pos) {
        playerPos = pos;
        });
    return playerPos;
}
