#include "backend.h"
#include "ADT/CFG.h"
#include "sym.h"

namespace aaac {
namespace backend {
  // 这是一个 Wrapper，调用 ADT/CFG.h 的方法，并将数据存储为后端可读的格式
  void LivenessPass(frontend::ControlFlowGraph *cfg) {
    auto useDefInfo = cfg->calculateUseDef();
    auto livenessInfo = cfg->calculateLiveness(useDefInfo);
    auto &BBMap = cfg->getBBMap();
    for (auto bb_ptr : cfg->getBBList()) {
      auto &bb = *bb_ptr;
      auto &info = livenessInfo[bb_ptr.get()];
      backend::Liveness liveProp;
      for (auto var : info.getIn()) {
        if ((size_t)liveProp.live_in_idx < config::MAX_ANALYSIS_STACK) {
          liveProp.live_in[liveProp.live_in_idx++] =
            // 防止 double free，但我不确定是否正确，error-prone
            std::shared_ptr<sym::Var>(var, [](sym::Var*){});
        } else {
          throw std::runtime_error("live_in overflow in liveness()");
        }
      }
      for (auto var : info.getOut()) {
        if ((size_t)liveProp.live_out_idx < config::MAX_ANALYSIS_STACK) {
          liveProp.live_out[liveProp.live_out_idx++] =
            std::shared_ptr<sym::Var>(var, [](sym::Var*){});
        } else {
          throw std::runtime_error("live_out overflow in liveness()");
        }
      }
      auto &ud = useDefInfo[bb_ptr.get()];
      for (auto var : ud.use) {
        if ((size_t)liveProp.live_gen_idx < config::MAX_ANALYSIS_STACK) {
          liveProp.live_gen[liveProp.live_gen_idx++] =
            std::shared_ptr<sym::Var>(var, [](sym::Var*){});
        }
      }
      for (auto var : ud.def) {
        if ((size_t)liveProp.live_kill_idx < config::MAX_ANALYSIS_STACK) {
          liveProp.live_kill[liveProp.live_kill_idx++] =
            std::shared_ptr<sym::Var>(var, [](sym::Var*){});
        }
      }
      // auto &prop = utils::g_property_mgr.init<backend::Liveness>(&bb);
      auto &prop = utils::g_property_mgr.init<backend::Liveness>(BBMap[bb_ptr].get());
      prop = std::move(liveProp);
      // std::cout << bb_ptr->getLabel()->getName() << " " << bb_ptr << " -> " << &BBMap[bb_ptr] << "\n";
    }

    for (auto bb_ptr : cfg->getBBList()) {
      std::cout << bb_ptr->getLabel()->getName() << ": \n";
      auto &info = livenessInfo[bb_ptr.get()];
      std::cout << "live out : ";
      for (auto var : info.getOut()) {
        std::cout << var->getName() << " ";
      }
      std::cout << "\n";
    }
  }
} // namespace backend
} // namespace aaac
