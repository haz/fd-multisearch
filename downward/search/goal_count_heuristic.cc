#include "goal_count_heuristic.h"

#include "globals.h"
#include "option_parser.h"
#include "plugin.h"
#include "state.h"


static ScalarEvaluatorPlugin goal_count_heuristic_plugin(
    "goalcount", GoalCountHeuristic::create);


GoalCountHeuristic::GoalCountHeuristic() {
}

GoalCountHeuristic::~GoalCountHeuristic() {
}

void GoalCountHeuristic::initialize() {
    cout << "Initializing goal count heuristic..." << endl;
}

int GoalCountHeuristic::compute_heuristic(const State &state) {
    int unsatisfied_goal_count = 0;
    for (int i = 0; i < g_goal.size(); i++) {
        int var = g_goal[i].first, value = g_goal[i].second;
        if (state[var] != value)
            unsatisfied_goal_count++;
    }
    return unsatisfied_goal_count;
}

ScalarEvaluator *GoalCountHeuristic::create(const std::vector<string> &config,
                                            int start, int &end, bool dry_run) {
    OptionParser::instance()->set_end_for_simple_config(config, start, end);
    if (dry_run)
        return 0;
    else
        return new GoalCountHeuristic;
}
