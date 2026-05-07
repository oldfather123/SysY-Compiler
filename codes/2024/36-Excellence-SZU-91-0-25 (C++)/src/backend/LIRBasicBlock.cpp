#include "../backend/LIRBasicBlock.h"

void LIRBasicBlock::AddInsturction(shared_ptr<LIRInstruction> instruction_ptr)
{
    instructions.push_back(instruction_ptr);
}

string LIRBasicBlock::GetStr()
{
    string basic_block_str;
    
    //Block number
    basic_block_str+=" <"+to_string(number)+">\n";
    
    //Precursor and succeed
    //basic_block_str+="  [Precursor] ";for(const int& pre:precursors)basic_block_str+=to_string(pre)+" ";
    //basic_block_str+="[Succeed] ";for(const int& suc:succeeds)basic_block_str+=to_string(suc)+" ";
    //basic_block_str+="\n";
    
    //Instructions
    for(shared_ptr<LIRInstruction>& instruction_ptr:instructions)
    {
        basic_block_str+="    ";
        basic_block_str+=instruction_ptr->GetStr();
        basic_block_str+="\n";
    }
    //basic_block_str+="\n";

    return basic_block_str;
}
