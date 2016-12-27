#include "search_engine.h"

#include "evaluation_context.h"
#include "globals.h"
#include "option_parser.h"
#include "plugin.h"

#include "algorithms/ordered_set.h"

#include "utils/countdown_timer.h"
#include "utils/system.h"
#include "utils/timer.h"

#include <cassert>
#include <iostream>
#include <limits>

using namespace std;
using utils::ExitCode;


static const bool VERBOSE = false;


SearchEngine::SearchEngine(const Options &opts)
    : status(IN_PROGRESS),
      solution_found(false),
      state_registry(
          *g_root_task(), *g_state_packer, *g_axiom_evaluator, g_initial_state_data),
      search_space(state_registry,
                   static_cast<OperatorCost>(opts.get_enum("cost_type"))),
      cost_type(static_cast<OperatorCost>(opts.get_enum("cost_type"))),
      max_time(opts.get<double>("max_time")) {
    if (opts.get<int>("bound") < 0) {
        cerr << "error: negative cost bound " << opts.get<int>("bound") << endl;
        utils::exit_with(ExitCode::INPUT_ERROR);
    }
    bound = opts.get<int>("bound");
}

SearchEngine::~SearchEngine() {
}

void SearchEngine::print_statistics() const {
    cout << "Bytes per state: "
         << state_registry.get_state_size_in_bytes() << endl;
}

bool SearchEngine::found_solution() const {
    return solution_found;
}

SearchStatus SearchEngine::get_status() const {
    return status;
}

const SearchEngine::Plan &SearchEngine::get_plan() const {
    assert(solution_found);
    return plan;
}

void SearchEngine::set_plan(const Plan &p) {
    solution_found = true;
    plan = p;
}

void SearchEngine::search() {
    initialize();
    utils::CountdownTimer timer(max_time);
    while (status == IN_PROGRESS) {
        status = step();
        if (timer.is_expired()) {
            cout << "Time limit reached. Abort search." << endl;
            status = TIMEOUT;
            break;
        }
    }
    // TODO: Revise when and which search times are logged.
    if (VERBOSE) {
        cout << "Actual search time: " << timer
             << " [t=" << utils::g_timer << "]" << endl;
    }
}

bool SearchEngine::check_goal_and_set_plan(const GlobalState &state) {
    if (test_goal(state)) {
        if (VERBOSE)
            cout << "Solution found!" << endl;
        Plan plan;
        search_space.trace_path(state, plan);
        set_plan(plan);
        return true;
    }
    return false;
}

void SearchEngine::save_plan_if_necessary() const {
    if (found_solution())
        save_plan(get_plan());
}

int SearchEngine::get_adjusted_cost(const GlobalOperator &op) const {
    return get_adjusted_action_cost(op, cost_type);
}

void SearchEngine::add_options_to_parser(OptionParser &parser) {
    ::add_cost_type_option_to_parser(parser);
    parser.add_option<int>(
        "bound",
        "exclusive depth bound on g-values. Cutoffs are always performed according to "
        "the real cost, regardless of the cost_type parameter", "infinity");
    parser.add_option<double>(
        "max_time",
        "maximum time in seconds the search is allowed to run for. The "
        "timeout is only checked after each complete search step "
        "(usually a node expansion), so the actual runtime can be arbitrarily "
        "longer. Therefore, this parameter should not be used for time-limiting "
        "experiments. Timed-out searches are treated as failed searches, "
        "just like incomplete search algorithms that exhaust their search space.",
        "infinity");
}

void print_initial_h_values(const EvaluationContext &eval_context) {
    eval_context.get_cache().for_each_heuristic_value(
        [] (const Heuristic *heur, const EvaluationResult &result) {
        cout << "Initial heuristic value for "
             << heur->get_description() << ": ";
        if (result.is_infinite())
            cout << "infinity";
        else
            cout << result.get_h_value();
        cout << endl;
    }
        );
}


static PluginTypePlugin<SearchEngine> _type_plugin(
    "SearchEngine",
    // TODO: Replace empty string by synopsis for the wiki page.
    "");


algorithms::OrderedSet<const GlobalOperator *> collect_preferred_operators(
    EvaluationContext &eval_context,
    const vector<Heuristic *> &preferred_operator_heuristics) {
    algorithms::OrderedSet<const GlobalOperator *> preferred_operators;
    for (Heuristic *heuristic : preferred_operator_heuristics) {
        /*
          Unreliable heuristics might consider solvable states as dead
          ends. We only want preferred operators from finite-value
          heuristics.
        */
        if (!eval_context.is_heuristic_infinite(heuristic)) {
            for (const GlobalOperator *op :
                 eval_context.get_preferred_operators(heuristic)) {
                preferred_operators.insert(op);
            }
        }
    }
    return preferred_operators;
}
