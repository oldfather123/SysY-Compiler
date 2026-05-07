#include "../optimize/Optimizer.h"
#include "../midend/MIR.h"
#include "../utility/Product.h"

void NoPrecursorBasicBlockEliminate()
{
    for(MIRFunction& function:mir.functions)
    {
        for(int i=0;i<function.basic_blocks.size();i++)
        {
            if(i!=function.entry_block_number&&i!=function.exit_block_number
                &&function.basic_blocks[i].is_valid&&function.basic_blocks[i].precursors.empty())
            {
                for(const int& suc_number:function.basic_blocks[i].succeeds)
                    function.GetBasicBlock(suc_number).precursors.erase(i);
                
                function.basic_blocks[i].is_valid=false;
            }
        }
    }
}

void BasicBlockClearUp()
{
    for(MIRFunction& mir_function:mir.functions)
    {
        int is_chaotic=false;
        unordered_map<int,int> oldnew_number_map;
        int old_block_number=0;
        int new_block_number=0;

        //Gather old new basic block numebr map
        for(MIRBasicBlock& old_block:mir_function.basic_blocks)
        {
            if(old_block.is_valid)
            {
                oldnew_number_map[old_block_number]=new_block_number;
                old_block.number=new_block_number++;
            }
            else is_chaotic=true;

            old_block_number++;
        }

        if(is_chaotic)
        {
            //Modify old basic block information
            for(MIRBasicBlock& old_block:mir_function.basic_blocks)
            {
                if(old_block.is_valid)
                {
                    //Update precursors and succeeds
                    unordered_set<int> new_precursors;
                    for(const int& precursor:old_block.precursors)
                        new_precursors.insert(oldnew_number_map[precursor]);
                    old_block.precursors=new_precursors;

                    unordered_set<int> new_succeeds;
                    for(const int& succeed:old_block.succeeds)
                        new_succeeds.insert(oldnew_number_map[succeed]);
                    old_block.succeeds=new_succeeds;

                    //Update jump and br
                    if(!old_block.instructions.empty())
                    {
                        shared_ptr<MIRInstruction> mir_instruction_ptr=old_block.instructions.back();
                        if(mir_instruction_ptr->mir_instruction_type==BR)
                        {
                            shared_ptr<Branch> branch=static_pointer_cast<Branch>(mir_instruction_ptr);
                            branch->true_number=oldnew_number_map[branch->true_number];
                            branch->false_number=oldnew_number_map[branch->false_number];
                        }
                        else if(mir_instruction_ptr->mir_instruction_type==JUMP)
                        {
                            shared_ptr<Jump> jump=static_pointer_cast<Jump>(mir_instruction_ptr);
                            jump->number=oldnew_number_map[jump->number];
                        }
                    }
                }
            }

            //Make new basic blocks
            vector<MIRBasicBlock> new_basic_blocks;
            for(MIRBasicBlock& old_block:mir_function.basic_blocks)
                if(old_block.is_valid)
                    new_basic_blocks.push_back(std::move(old_block));
            
            //Replace old basic blocks
            mir_function.basic_blocks=std::move(new_basic_blocks);
        }
    }
}



void Optimizer::BasicBlockEliminate()
{
    NoPrecursorBasicBlockEliminate();
    BasicBlockClearUp();
}
