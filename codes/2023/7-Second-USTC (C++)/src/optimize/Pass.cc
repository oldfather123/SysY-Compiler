#include "Pass.hh"

void PassMgr::execute(bool print_ir) {
    for(auto pass: pass_list_) {
        pass->execute();
        std::cout << pass->get_name() << std::endl;
        if(print_ir || pass->print_ir) {
            std::cout << m_->print() << std::endl;
        }
    }
} 