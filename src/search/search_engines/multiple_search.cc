#include "multiple_search.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "lazy_search.h"

#include "../globals.h"
#include "../heuristics/ff_heuristic.h"
#include "../open_lists/open_list_factory.h"
#include "../open_lists/standard_scalar_open_list.h"
#include "../operator_cost.h"
#include "../utils/memory.h"

#include <iostream>
#include <limits>

using namespace std;
using namespace utils;

namespace multiple_search {

static const bool VERBOSE = false;
static const bool SAVE_PLAN = false;

static unique_ptr<SearchEngine> build_lazy_ff_search() {
    /*
      Build the same search engine that would be generated by the
      command-line option "lazy_greedy(ff())". This is a bit complex
      because we need to manually set options that have default values
      when used from the command line and because "lazy_greedy" is a
      factory function with somewhat complex behaviour.
    */

    // Build FF heuristic object.
    Options ff_options;
    ff_options.set<bool>("cache_estimates", true);
    ff_options.set<shared_ptr<AbstractTask>>("transform", g_root_task());
    // NOTE: The following line leaks.
    ScalarEvaluator *h = new ff_heuristic::FFHeuristic(ff_options);

    // Build open list object.
    Options open_list_options;
    open_list_options.set("eval", h);
    open_list_options.set("pref_only", false);
    shared_ptr<OpenListFactory> open_list = make_shared<
        StandardScalarOpenListFactory>(open_list_options);

    // Build lazy search object.
    Options lazy_options;
    lazy_options.set<int>("cost_type", NORMAL);
    lazy_options.set<double>("max_time", numeric_limits<double>::infinity());
    lazy_options.set<int>("bound", numeric_limits<int>::max());
    lazy_options.set<shared_ptr<OpenListFactory>>("open", open_list);
    lazy_options.set<bool>("reopen_closed", false);
    lazy_options.set<bool>("randomize_successors", false);
    lazy_options.set<bool>("preferred_successors_first", false);
    lazy_options.set<int>("random_seed", -1);
    unique_ptr<SearchEngine> engine = make_unique_ptr<
        lazy_search::LazySearch>(lazy_options);
    return engine;
}

MultipleSearch::MultipleSearch(const Options &opts)
    : SearchEngine(opts),
      phase(0) {}

unique_ptr<SearchEngine> MultipleSearch::get_search_engine(int _problem_index) {
    int problem_index = _problem_index % g_goal_MUISE.size();

    g_initial_state_data = *(g_initial_state_data_MUISE[problem_index]);
    g_goal = *(g_goal_MUISE[problem_index]);

    return build_lazy_ff_search();
}

unique_ptr<SearchEngine> MultipleSearch::create_phase(int phase) {
    if (utils::g_timer() > 10.0) {
        cout << "Time limit exceeded!\n"
             << phase << " phases completed" << endl;
        return nullptr;
    } else {
        return get_search_engine(phase);
    }
}

SearchStatus MultipleSearch::step() {
    unique_ptr<SearchEngine> current_search = create_phase(phase);
    if (!current_search) {
        return found_solution() ? SOLVED : FAILED;
    }

    ++phase;

    current_search->search();

    SearchEngine::Plan found_plan;
    if (VERBOSE)
        cout << endl;
    if (SAVE_PLAN) {
        if (current_search->found_solution())
            save_plan(current_search->get_plan(), true);
    }
    if (!(current_search->found_solution())) {
        cout << "ERROR: Should always have a solution during this test." << endl;
        return FAILED;
    }
    if (VERBOSE) {
        cout << endl;
        current_search->print_statistics();
    }

    const SearchStatistics &current_stats = current_search->get_statistics();
    statistics.inc_expanded(current_stats.get_expanded());
    statistics.inc_evaluated_states(current_stats.get_evaluated_states());
    statistics.inc_evaluations(current_stats.get_evaluations());
    statistics.inc_generated(current_stats.get_generated());
    statistics.inc_generated_ops(current_stats.get_generated_ops());
    statistics.inc_reopened(current_stats.get_reopened());

    return IN_PROGRESS;
}

void MultipleSearch::print_statistics() const {
    if (VERBOSE) {
        cout << "Cumulative statistics:" << endl;
        statistics.print_detailed_statistics();
    }
}

void MultipleSearch::save_plan_if_necessary() const { }

static SearchEngine *_parse(OptionParser &parser) {
    SearchEngine::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return new MultipleSearch(opts);
}

static Plugin<SearchEngine> _plugin("multiple", _parse);
}
