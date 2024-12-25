#include "goapPlanner.h"

#include <vector>
#include <cmath>
#include <queue>
#include <map>
#include <algorithm>


goap::Planner goap::create_planner()
{
    return Planner();
}

void goap::add_states_to_planner(Planner& planner, const std::vector<std::string>& state_names)
{
    for (const std::string& name : state_names)
        planner.wdesc.emplace(name, planner.wdesc.size());
}


void goap::add_action_to_planner(Planner& planner, const char* name, float cost, const Precond& precond,
    const Effect& effect,
    const Effect& additive_effect)
{
    Action act = create_action(name, planner.wdesc, cost);
    for (auto st : precond)
        set_action_precond(act, planner.wdesc, st.first, int8_t(st.second));
    for (auto st : effect)
        set_action_effect(act, planner.wdesc, st.first, int8_t(st.second));
    for (auto st : additive_effect)
        set_additive_action_effect(act, planner.wdesc, st.first, int8_t(st.second));

    planner.actionNames.emplace(name, planner.actions.size());
    planner.actions.emplace_back(act);
}

static void set_planner_worldstate(const goap::Planner& planner, goap::WorldState& st, const char* st_name, int8_t val)
{
    auto itf = planner.wdesc.find(st_name);
    if (itf == planner.wdesc.end())
        return;
    st[itf->second] = val;
}

goap::WorldState goap::produce_planner_worldstate(const Planner& planner, const WorldStateList& states)
{
    WorldState res;
    for (size_t i = 0; i < planner.wdesc.size(); ++i)
        res.emplace_back(int8_t(-1));
    for (auto st : states)
        set_planner_worldstate(planner, res, st.first, int8_t(st.second));
    return res;
}

float goap::get_action_cost(const Planner& planner, size_t act_id)
{
    return planner.actions[act_id].cost;
}

static bool not_eq_states(const goap::WorldState& lhs, const goap::WorldState& rhs)
{
    if (lhs.size() != rhs.size())
        return true;
    for (size_t i = 0; i < lhs.size(); ++i)
        if (lhs[i] != rhs[i])
            return true;
    return false;
}

std::vector<size_t> goap::find_valid_state_transitions(const Planner& planner, const WorldState& from)
{
    std::vector<size_t> res;

    for (size_t i = 0; i < planner.actions.size(); ++i)
    {
        const Action& action = planner.actions[i];
        bool isValidAction = true;
        for (size_t j = 0; j < action.precondition.size() && isValidAction; ++j)
            isValidAction &= action.precondition[j] < 0 || from[j] == action.precondition[j];
        WorldState newWs = apply_action(planner, i, from);
        if (isValidAction && not_eq_states(newWs, from))
            res.emplace_back(i);
    }
    return res;
}

goap::WorldState goap::apply_action(const Planner& planner, size_t act, const WorldState& from)
{
    WorldState res = from;
    const Action& action = planner.actions[act];
    for (size_t i = 0; i < action.effect.size(); ++i)
    {
        if (!action.setBitset[i])
            res[i] += action.effect[i];
        else if (action.effect[i] >= 0)
            res[i] = action.effect[i];
    }
    return res;
}

namespace goap {

    float heuristic(const WorldState& from, const WorldState& to);

    float ida_search(const Planner& planner, const WorldState& current,
        const WorldState& goal, std::vector<PlanStep>& plan,
        float g, float limit)
    {
        float f = g + heuristic(current, goal);
        if (f > limit) return f;
        if (current == goal)
        {
            plan.emplace_back(PlanStep{ size_t(-1), current });
            return -1;
        }

        float min_cost = FLT_MAX;
        for (size_t action_id : find_valid_state_transitions(planner, current))
        {
            WorldState next_state = apply_action(planner, action_id, current);
            std::vector<PlanStep> temp_plan = plan;
            float result = ida_search(planner, next_state, goal, temp_plan,
                g + get_action_cost(planner, action_id), limit);
            if (result == -1)
            {
                plan.emplace_back(PlanStep{ action_id, next_state });
                return -1;
            }
            if (result < min_cost) min_cost = result;
        }
        return min_cost;
    }

    float make_plan_ida(const Planner& planner, const WorldState& from, const WorldState& to, std::vector<PlanStep>& plan)
    {
        float limit = heuristic(from, to);
        while (true)
        {
            std::vector<PlanStep> local_plan;
            float result = ida_search(planner, from, to, local_plan, 0, limit);
            if (result == -1)
            {
                plan = local_plan;
                return limit;
            }
            if (result == FLT_MAX) return -1;
            limit = result;
        }
    }

    struct Node {
        WorldState state;
        float g;
        float h;
        size_t action;
        Node* parent;
    };

    auto cmp = [](const Node* a, const Node* b) {
        return (a->g + a->h) > (b->g + b->h);
        };


    float make_plan_with_epsilon(const Planner& planner, const WorldState& from, const WorldState& to,
        std::vector<PlanStep>& plan, float epsilon) {
        std::priority_queue<Node*, std::vector<Node*>, decltype(cmp)> openList(cmp);
        std::map<WorldState, float> gScore;


        gScore[from] = 0;
        openList.push(new Node{ from, 0, heuristic(from, to), size_t(-1), nullptr });

        while (!openList.empty()) {
            Node* current = openList.top();
            openList.pop();

            if (current->state == to) {
                while (current->action != size_t(-1)) {
                    plan.emplace_back(PlanStep{ current->action, current->state });
                    current = current->parent;
                }
                std::reverse(plan.begin(), plan.end());
                return gScore[to];
            }

            for (size_t actionId : find_valid_state_transitions(planner, current->state)) {
                WorldState nextState = apply_action(planner, actionId, current->state);
                float tentativeG = gScore[current->state] + get_action_cost(planner, actionId);

                if (gScore.find(nextState) == gScore.end() || tentativeG < gScore[nextState]) {
                    gScore[nextState] = tentativeG;
                    float h = heuristic(nextState, to);
                    openList.push(new Node{ nextState, tentativeG, epsilon * h, actionId, current });
                }
            }
        }

        return -1;
    }
}
