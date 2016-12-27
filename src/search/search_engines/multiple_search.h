#ifndef SEARCH_ENGINES_MULTIPLE_SEARCH_H
#define SEARCH_ENGINES_MULTIPLE_SEARCH_H

#include "../option_parser_util.h"
#include "../search_engine.h"
#include "../utils/timer.h"

#include <memory>

namespace options {
class Options;
}

namespace multiple_search {
class MultipleSearch : public SearchEngine {
    int phase;

    std::unique_ptr<SearchEngine> get_search_engine(int problem_index);
    std::unique_ptr<SearchEngine> create_phase(int phase);

    virtual SearchStatus step() override;

public:
    explicit MultipleSearch(const options::Options &opts);

    virtual void save_plan_if_necessary() const override;
    virtual void print_statistics() const override;
};
}

#endif
