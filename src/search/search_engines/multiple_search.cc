#include "multiple_search.h"

#include "../option_parser.h"
#include "../plugin.h"

#include <iostream>

using namespace std;

namespace multiple_search {

static const bool VERBOSE = false;
static const bool SAVE_PLAN = false;

MultipleSearch::MultipleSearch(const Options &opts)
    : SearchEngine(opts),
      engine_config(opts.get<ParseTree>("engine_config")),
      phase(0) {}

SearchEngine *MultipleSearch::get_search_engine(int _problem_index) {

    int problem_index = _problem_index % g_goal_MUISE.size();

    if (VERBOSE) {
        cout << "\n------------------------------------------" << endl;
        cout << "\nNew Init:" << endl;
    }
    for (size_t i = 0; i < g_variable_domain.size(); ++i) {
        g_initial_state_data[i] = (*(g_initial_state_data_MUISE[problem_index]))[i];
        if (VERBOSE)
            cout << "var" << i << " = " << g_initial_state_data[i] << endl;
    }

    if (VERBOSE)
        cout << "\nNew Goal:" << endl;
    g_goal.clear();
    for (auto varval : *(g_goal_MUISE[problem_index])) {
        if (VERBOSE)
            cout << "var" << varval.first << " = " << varval.second << endl;
        g_goal.push_back(make_pair(varval.first, varval.second));
    }
    if (VERBOSE)
        cout << endl;

    OptionParser parser(engine_config, false);
    SearchEngine *engine = parser.start_parsing<SearchEngine *>();

    if (VERBOSE) {
        cout << "Starting search: ";
        kptree::print_tree_bracketed(engine_config, cout);
        cout << endl;
    }

    return engine;
}

SearchEngine *MultipleSearch::create_phase(int phase) {
    if (utils::g_timer() > 10.0) {
        cout << "Time limit exceeded!\n"
             << phase << " phases completed" << endl;
        return nullptr;
    } else {
        return get_search_engine(phase);
    }
}

SearchStatus MultipleSearch::step() {
    SearchEngine *current_search = create_phase(phase);
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
    parser.add_option<ParseTree>("engine_config", "search engine for use in all phases");
    SearchEngine::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.help_mode())
        return nullptr;
    else
        return new MultipleSearch(opts);
}

static Plugin<SearchEngine> _plugin("multiple", _parse);
}
