#include "MachineModule.hpp"
#include "MachinePass.hpp"
#include "LivenessAnalysis.hpp"
#include "MachineInstr.hpp"
#include <memory>

using RegisterSet = std::unordered_set<std::shared_ptr<Register>>;
class MIDeadCodeElimination : public MachinePass{
public:
    MIDeadCodeElimination(std::shared_ptr<MachineModule> MM) : MachinePass(MM) {}
    void run_on_func(std::shared_ptr<MachineFunction> func);
    void run() override;
private:
    std::unique_ptr<LivenessAnalysis> LA;
};