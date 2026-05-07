#ifndef GEECEECEE_OPT_ENGINE_HPP
#define GEECEECEE_OPT_ENGINE_HPP

#include <deque>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include "ir/mod.hpp"
#include "opt/pass.hpp"

namespace opt {
enum OptStatus {
    // clang-format off
    CFG, DOM, SSA, CALL_GRAPH, PURE_FUNC, FUNC,
    SIDE_EFFECT, SIMPLE_LOOP, LCSSA, ALIAS,
    IND_VAR, SCEV, TRIP_COUNT, LOOP, POINTER,

    END
    // clang-format on
};

class OptEngine;

class OptGroup {
    using OptNode = std::variant<pass::Pass *, OptGroup *>;

public:
    OptGroup(std::string name);
    ~OptGroup() = default;

    OptGroup *attach(OptStatus status);
    OptGroup *execute(std::vector<std::pair<OptNode, bool>> nodes);
    OptGroup *require(std::vector<OptStatus> requirements);
    OptGroup *expose(std::vector<OptGroup *> exposures);
    OptGroup *follow(std::vector<OptGroup *> followings);
    OptGroup *disable(std::vector<OptStatus> destructions);
    friend class OptEngine;

    bool run(ir::Module *module);

private:
    // debug use
    std::string name;
    // global opt engine
    OptEngine *engine;
    // {executing node, once or execute until completion}
    std::vector<std::pair<OptNode, bool>> nodes;
    // the status this group activates
    std::optional<OptStatus> active_status;
    // required status
    std::vector<OptStatus> requirements;
    // the groups pushed to engine queue after this group runs
    std::vector<OptGroup *> exposures;
    // the groups executed immediately after this group runs
    std::vector<OptGroup *> followings;
    // the status this group destorys
    std::vector<OptStatus> destructions;
};

class OptEngine {
public:
    OptEngine() : assiatance_status(END, {nullptr, false}) {}
    void init();
    void compile(ir::Module *module);
    void pre_lauch(std::vector<OptGroup *> groups);
    void defer(std::vector<OptGroup *> groups);
    void cycle(std::vector<OptGroup *> groups);
    void set_cycle_times(int times);
    friend class OptGroup;

private:
    // OptStatus index -> {trigger group, is active}
    std::vector<std::pair<OptGroup *, bool>> assiatance_status;
    // if the group is in pipeline
    std::unordered_set<OptGroup *> in_pipeline;
    // opt pipeline (mid cycle)
    std::deque<OptGroup *> pipeline;
    // pre lauch opts
    std::vector<OptGroup *> early_opts;
    // post lauch opts
    std::vector<OptGroup *> late_opts;
    // pushed to pipeline `cycle_times` times
    std::vector<OptGroup *> mid_opts;
    // pipeline cycle times
    int cycle_times;
};

// global opt engine instance
extern OptEngine opt_engine;

}  // namespace opt

#endif  // GEECEECEE_OPT_ENGINE_HPP
