#pragma once

#include "Common.h"
#include "Values.h"
#include "Graph.h"
#include "CFG.h"

namespace ir_builder {
    class Builder;
}

namespace ir {
    class Function;
    class Module;
    class Instruction;
    class PhiInst;
    class ExitInst;
    class GlobalInst;

    /// @brief Module, which is generally the scope of a code file, and consists of several global registers and functions
    class Module : public std::enable_shared_from_this<Module> {
        friend class ir_builder::Builder;
        friend class Function;

    private:
        Set<FuncPtr> _funcSet;
        FuncPtr _globalInitFunc;
        void _addFunction(const FuncPtr &func);
        void _removeFunction(const FuncPtr &func);

    public: /* constructors */
        Module() { }
        static Ptr<Module> create() {
            return makePtr<Module>();
        }

    public: /* properties */
        const Set<FuncPtr> &funcSet() const { return _funcSet; }
        const Vector<Ptr<GlobalInst>> getGlobalInitInsts() const;
        const Set<RegPtr> getGlobalRegs() const;

    public: /* methods */
        String toString() const;
    };

    /// @brief Function, which consists of several parameter declarations and several basic blocks
    class Function : public std::enable_shared_from_this<Function>, public IPrintOrderable<Function> {
        friend class IPrintOrderable;
        friend class ir_builder::Builder;
        friend class Module;
        friend class BasicBlock;

    private:
        Map<String, unsigned> _usedBBLabels{}; // For maintaining uniqueness of basic block labels
        Map<String, unsigned> _usedRegNames{}; // For maintaining uniqueness of register names
        Ptr<Module> _parentModule;             // CAUSES LOOP REFERENCE
        DataType _retDataType;
        String _name;
        Vector<RegPtr> _paramList;
        Set<BBPtr> _bbSet;
        BBPtr _entryBB;
        BBPtr _exitBB;
        bool _isPrototype = false;
        bool _isPure = false;
        void _addBB(const BBPtr &bb);
        void _removeBB(const BBPtr &bb);

    public: /* constructors */
        Function(const DataType &retDataType, const String &name) : IPrintOrderable(), _retDataType(retDataType), _name(name) {
            _printOrder = _printOrderCounter++;
        }
        static FuncPtr create(const Ptr<Module> &parentModule, const DataType &retDataType, const String &name);
        static FuncPtr create(const Ptr<Module> &parentModule, const DataType &retDataType, const String &name, const Vector<RegPtr> &paramList);
        static FuncPtr createPrototype(const Ptr<Module> &parentModule, const DataType &retDataType, const String &name, const Vector<DataType> &paramTypeList);
        ~Function() {
            _parentModule.reset(); // BREAK THE LOOP REFERENCE
        }

    public: /* properties */
        const Ptr<Module> parentModule() const { return _parentModule; }
        DataType &retDataType() { return _retDataType; }
        String &name() { return _name; }
        Vector<RegPtr> &paramList() { return _paramList; }
        const Set<BBPtr> &bbSet() const { return _bbSet; }
        BBPtr &entryBB() { return _entryBB; }
        BBPtr &exitBB() { return _exitBB; }
        bool isGlobalInitFunc() const;
        const bool &isPrototype() const { return _isPrototype; }
        bool &isPure() { return _isPure; }

    public: /* methods */
        /// @brief Traverse the CFG of the function in depth-first order and return the basic block list in the order.
        /// @param startBB Start basic block. If not provided or null, the function will start from the entry BB.
        /// @param reverse If true, the CFG will be traversed in reverse order, otherwise it will be traversed in forward order.
        /// @return The depth-first-ordered basic block list of the function.
        Vector<BBPtr> dfsBBList(BBPtr startBB = nullptr, bool reverse = false) const;
        /// @brief Traverse the CFG of the function in breadth-first order and return the basic block list in the order.
        /// @param startBB Start basic block. If not provided or null, the function will start from the entry BB.
        /// @param reverse If true, the CFG will be traversed in reverse order, otherwise it will be traversed in forward order.
        /// @return The breadth-first-ordered basic block list of the function.
        Vector<BBPtr> bfsBBList(BBPtr startBB = nullptr, bool reverse = false) const;
        String getUniqueRegName(const String &name = "");
        static void replace(const FuncPtr &func, const FuncPtr &newNode);
        static void remove(const FuncPtr &func);
        static FuncPtr clone(const FuncPtr &func);

        String toString() const;
    };

    /// @brief Basic block, which consists of several instructions
    class BasicBlock : public std::enable_shared_from_this<BasicBlock>, public GraphNode<BasicBlock, CFGEdge>, public IPrintOrderable<BasicBlock> {
        friend class IPrintOrderable;
        friend class ir_builder::Builder;
        friend class Function;
        friend class Instruction;
        friend class GraphNode<BasicBlock, CFGEdge>;
        friend class CFGUtils;

    private:
        FuncPtr _parentFunc; // CAUSES LOOP REFERENCE
        String _label;
        Set<InstPtr> _nonPhiInstSet;
        Set<Ptr<PhiInst>> _phiNodes;
        InstPtr _entryInst = nullptr;
        InstPtr _exitInst = nullptr;
        void _addInst(const InstPtr &inst);
        void _removeInst(const InstPtr &inst, bool checkEntryExit = true);

    public: /* constructors */
        BasicBlock(const String &label) : GraphNode(), IPrintOrderable(), _label(label) {
            _printOrder = _printOrderCounter++;
        }
        static BBPtr create(const FuncPtr &parentFunc, const String &label);
        ~BasicBlock() {
            _parentFunc.reset(); // BREAK THE LOOP REFERENCE
        }

    public: /* properties */
        const FuncPtr &parentFunc() const { return _parentFunc; }
        String &label() { return _label; }
        const InstPtr &entryInst() const { return _entryInst; }
        Ptr<ExitInst> exitInst() const;
        bool isEmpty() const;

    public: /* methods */
        // Get a copy of the _phiNodes set
        const Set<Ptr<PhiInst>> getPhiNodes() const {
            return _phiNodes;
        }
        const Set<InstPtr> getInstSet() const;
        Vector<InstPtr> getInstTopoList() const;

        static Ptr<CFGEdge> addUnconditionalEdge(const BBPtr &src, const BBPtr &dest);
        static Ptr<CFGEdge> addConditionalEdge(const BBPtr &src, const BBPtr &dest, const Value &cond, const bool &condBool);
        static void replace(const BBPtr &bb, const BBPtr &newNode);
        static void remove(const BBPtr &bb);
        static BBPtr clone(const FuncPtr &func, const BBPtr &bb, const String &labelPrefix = "", const String &labelSuffix = "clone");
        static std::pair<BBPtr, BBPtr> split(const BBPtr &bb, const InstPtr &splitInst, const String &labelPrefix = "", const String &labelSuffix = "split");
        static BBPtr merge(const BBPtr &bb1, const BBPtr &bb2);

        String toString() const;
    };

} // namespace ir
