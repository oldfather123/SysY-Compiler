#include "../backend/LIRFunction.h"

int LIRFunction::NewBasicBlock()
{
    int block_number=basic_blocks.size();
    basic_blocks.push_back(LIRBasicBlock(block_number));

    return block_number;
}


string LIRFunction::GetStr()
{
    string function_str;

    //Name
    function_str+=name+"\n";

    //Basic blocks
    function_str+="entry:";function_str+=basic_blocks[entry_block_number].GetStr();
    for(int i=0;i<basic_blocks.size();i++)
    {
        if(i!=entry_block_number&&i!=exit_block_number)
            function_str+=basic_blocks[i].GetStr();
    }
    function_str+="exit:";function_str+=basic_blocks[exit_block_number].GetStr();

    return function_str;
}
