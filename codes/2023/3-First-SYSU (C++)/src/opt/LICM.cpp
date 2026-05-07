#include "LICM.h"
void LICM(BlockPtr block)
{
    for(auto basicBlock: block->basicBlocks)
    {
        if(basicBlock->isWhileCond)
        {
            cerr<<basicBlock->label->getStr();
            for(auto pre: basicBlock->predBasicBlocks)
            {
                if(pre.second==true&&!basicBlock->succBasicBlocks[pre.first])
                {
                    cerr<<" "<< pre.first->label->getStr()<<endl;

                }
            }
        }
    }
}
