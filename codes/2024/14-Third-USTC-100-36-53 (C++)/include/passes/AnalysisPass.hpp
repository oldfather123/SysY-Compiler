#pragma once

#include "Module.hpp"
#include "Type.hpp"
#include "TypeName.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>

#include "pair_hash.hpp"

class AnalysisPassManager;

template <typename IRUnit> class AnalysisPassBase {
    virtual void run(IRUnit *item, AnalysisPassManager &analysis) = 0;
    virtual void print(IRUnit *item, std::ostream &os) const {}
  public:
    virtual ~AnalysisPassBase() = default;
};

template <typename IRUnit> class AnalysisPassConcept {
  public:
    virtual void invalidate() = 0;
    virtual const void *getPassResult(IRUnit *item,
                                      AnalysisPassManager &analysis) = 0;
    virtual ~AnalysisPassConcept() = default;
};

template <typename PassT, typename IRUnit>
class AnalysisPassModel : public AnalysisPassConcept<IRUnit> {
    std::optional<PassT> _result;
    inline static const int id{};

  public:
    static const void *getID() { return &id; }
    void invalidate() override { _result.reset(); }
    const void *getPassResult(IRUnit *item,
                              AnalysisPassManager &analysis) override {
        if (!_result.has_value()) {
            _result = PassT();
            _result.value().run(item, analysis);
        }
        return &_result.value();
    }
};

class AnalysisTest : public AnalysisPassBase<Module> {
  public:
    int a;
    using IRUnit = Module;
    void run(Module *m, AnalysisPassManager &analysis) override {
        std::cout << "A running on module\n";
        a = m->get_functions().size();
    }
};

class AnalysisPassManager final {
    std::unordered_map<
        const void *,
        std::function<std::unique_ptr<AnalysisPassConcept<Module>>()>>
        _moduleAnalysisBuilders;
    std::unordered_map<
        const void *,
        std::function<std::unique_ptr<AnalysisPassConcept<Function>>()>>
        _functionAnalysisBuilders;
    std::unordered_map<std::pair<const void *, Module *>,
                       std::unique_ptr<AnalysisPassConcept<Module>>, PairHash,
                       PairEqual>
        _moduleAnalysisResults;
    std::unordered_map<std::pair<const void *, Function *>,
                       std::unique_ptr<AnalysisPassConcept<Function>>, PairHash,
                       PairEqual>
        _functionAnalysisResults;

    template <typename IRUnit> auto &getBuilders() {
        if constexpr (std::is_same_v<IRUnit, Module>) {
            return _moduleAnalysisBuilders;
        } else {
            return _functionAnalysisBuilders;
        }
    }

    template <typename IRUnit> auto &getResults() {
        if constexpr (std::is_same_v<IRUnit, Module>) {
            return _moduleAnalysisResults;
        } else {
            return _functionAnalysisResults;
        }
    }

    template <typename IRUnit>
    const void *getResultImpl(IRUnit *item, const void *id) {
        auto &builders = getBuilders<IRUnit>();
        auto &results = getResults<IRUnit>();
        std::pair<const void *, IRUnit *> key{id, item};
        if (results.find(key) == results.end()) {
            assert(builders.count(id) && "Pass not registered");
            results[key] = builders[id]();
        }
        return results[key]->getPassResult(item, *this);
    }

  public:
    template <typename PassT> void registerPass() {
        using PassModelT = AnalysisPassModel<PassT, typename PassT::IRUnit>;
        auto id = PassModelT::getID();
        auto &builders = getBuilders<typename PassT::IRUnit>();
        if (builders.count(id) == 0) {
            builders[id] = []() -> std::unique_ptr<PassModelT> {
                return std::make_unique<PassModelT>();
            };
        }
    }

    template <typename PassT>
    const PassT *getResult(typename PassT::IRUnit *item) {
        using PassModelT = AnalysisPassModel<PassT, typename PassT::IRUnit>;
        return static_cast<const PassT *>(
            getResultImpl(item, PassModelT::getID()));
    }

    template <typename PassT> void invalidate(typename PassT::IRUnit *item) {
        using PassModelT = AnalysisPassModel<PassT, typename PassT::IRUnit>;
        auto id = PassModelT::getID();
        auto &results = getResults<typename PassT::IRUnit>();
        std::pair<const void *, typename PassT::IRUnit *> key{id, item};
        if (results.find(key) != results.end()) {
            results[key]->invalidate();
        }
    }

    template <typename IRUnit> void invalidateAll(IRUnit *item) {
        auto &results = getResults<IRUnit>();
        for (auto &[key, result] : results) {
            if (key.second == item) {
                result->invalidate();
            }
        }
    }
};
