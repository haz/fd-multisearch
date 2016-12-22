#include "multiple_search.h"

#include "../option_parser.h"
#include "../plugin.h"

#include <iostream>

using namespace std;

namespace multiple_search {
MultipleSearch::MultipleSearch(const Options &opts)
    : SearchEngine(opts),
      engine_config(opts.get_list<ParseTree>("engine_configs")[0]),
      phase(0) {}

SearchEngine *MultipleSearch::get_search_engine(int problem_index) {

    cout << "\n------------------------------------------" << endl;
    cout << "\nNew Init:" << endl;
    for (size_t i = 0; i < g_variable_domain.size(); ++i) {
        g_initial_state_data[i] = (*(g_initial_state_data_MUISE[problem_index]))[i];
        cout << "var" << i << " = " << g_initial_state_data[i] << endl;
    }

    cout << "\nNew Goal:" << endl;
    for (auto varval : *(g_goal_MUISE[problem_index])) {
        cout << "var" << varval.first << " = " << varval.second << endl;
        g_goal.push_back(make_pair(varval.first, varval.second));
    }
    cout << endl;

    OptionParser parser(engine_config, false);
    SearchEngine *engine = parser.start_parsing<SearchEngine *>();

    cout << "Starting search: ";
    kptree::print_tree_bracketed(engine_config, cout);
    cout << endl;

    return engine;
}

SearchEngine *MultipleSearch::create_phase(int phase) {
    int num_phases = g_goal_MUISE.size();
    if (phase >= num_phases)
        return nullptr;
    else
        return get_search_engine(phase);
}

SearchStatus MultipleSearch::step() {
    SearchEngine *current_search = create_phase(phase);
    if (!current_search) {
        return found_solution() ? SOLVED : FAILED;
    }
    
    current_search->set_bound(999999);
    ++phase;

    current_search->search();

    SearchEngine::Plan found_plan;
    cout << endl;
    if (current_search->found_solution())
        save_plan(current_search->get_plan(), true);
    cout << endl;
    current_search->print_statistics();

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
    cout << "Cumulative statistics:" << endl;
    statistics.print_detailed_statistics();
}

void MultipleSearch::save_plan_if_necessary() const {
    // We don't need to save here, as we automatically save after
    // each successful search iteration.
}

static SearchEngine *_parse(OptionParser &parser) {
    parser.document_synopsis("Multiple search", "");
    parser.document_note(
        "Note 1",
        "We don't cache heuristic values between search iterations at"
        " the moment. If you perform a LAMA-style iterative search,"
        " heuristic values will be computed multiple times.");
    parser.document_note(
        "Note 2",
        "The configuration\n```\n"
        "--search \"multiple([lazy_wastar(merge_and_shrink(),w=10), "
        "lazy_wastar(merge_and_shrink(),w=5), lazy_wastar(merge_and_shrink(),w=3), "
        "lazy_wastar(merge_and_shrink(),w=2), lazy_wastar(merge_and_shrink(),w=1)])\"\n"
        "```\nwould perform the preprocessing phase of the merge and shrink heuristic "
        "5 times (once before each iteration).\n\n"
        "To avoid this, use heuristic predefinition, which avoids duplicate "
        "preprocessing, as follows:\n```\n"
        "--heuristic \"h=merge_and_shrink()\" --search "
        "\"multiple([lazy_wastar(h,w=10), lazy_wastar(h,w=5), lazy_wastar(h,w=3), "
        "lazy_wastar(h,w=2), lazy_wastar(h,w=1)])\"\n"
        "```");
    parser.document_note(
        "Note 3",
        "If you reuse the same landmark count heuristic "
        "(using heuristic predefinition) between iterations, "
        "the path data (that is, landmark status for each visited state) "
        "will be saved between iterations.");
    parser.add_list_option<ParseTree>("engine_configs",
                                      "list of search engines for each phase");
    parser.add_option<bool>(
        "pass_bound",
        "use bound from previous search. The bound is the real cost "
        "of the plan found before, regardless of the cost_type parameter.",
        "true");
    parser.add_option<bool>("repeat_last",
                            "repeat last phase of search",
                            "false");
    parser.add_option<bool>("continue_on_fail",
                            "continue search after no solution found",
                            "false");
    parser.add_option<bool>("continue_on_solve",
                            "continue search after solution found",
                            "true");
    SearchEngine::add_options_to_parser(parser);
    Options opts = parser.parse();

    opts.verify_list_non_empty<ParseTree>("engine_configs");

    if (parser.help_mode()) {
        return nullptr;
    } else if (parser.dry_run()) {
        //check if the supplied search engines can be parsed
        for (const ParseTree &config : opts.get_list<ParseTree>("engine_configs")) {
            OptionParser test_parser(config, true);
            test_parser.start_parsing<SearchEngine *>();
        }
        return nullptr;
    } else {
        return new MultipleSearch(opts);
    }
}

static Plugin<SearchEngine> _plugin("multiple", _parse);
}
