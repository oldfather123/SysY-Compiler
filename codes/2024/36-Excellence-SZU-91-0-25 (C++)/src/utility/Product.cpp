#include "../utility/Product.h"

vector<string> source;
vector<Token> tokens;
unique_ptr<ASTNode> ast;
MIR mir;
LIR lir;
ProgramAssembly assembly;

void PrintSource()
{
    cout<<endl<<"----- Source Begin -----"<<endl;

    for(int i=0;i<source.size();i++)
    {
        int line=i+1;
        cout<<"Line "<<line<<" ";

        if(line>0&&line<10)cout<<"--";
        else if(line>=10&&line<100)cout<<"-";
        cout<<"<|"<<source[i];
    }

    cout<<"-----  Source End  -----"<<endl<<endl;
}

void PrintTokens()
{
    cout<<"-----  Token Begin  -----"<<endl;

    cout<<"  Line\tColumn\tType\t\tLexeme"<<endl;
    for(Token& token:tokens)
    {
        cout<<"  "<<token.location.line+1<<"\t"
        <<token.location.column+1<<"\t"
        <<TokenTypeText[token.token_type]<<"\t";
		if(TokenTypeText[token.token_type].size()<8)cout<<"\t";
        cout<<token.lexeme<<endl;
    }

    cout<<"-----   Token End   -----"<<endl<<endl;
}

void PrintAST()
{
    cout<<endl<<"-----   AST Begin   -----";

    ast->Print(0);

    cout<<endl<<"-----    AST End    -----"<<endl;
}

void PrintMIR()
{
    cout<<endl<<"-----   MIR Begin    -----"<<endl;

    cout<<mir.GetStr();

    cout<<"-----    MIR End     -----"<<endl<<endl;
}

void PrintLIR()
{
    cout<<endl<<"-----   LIR Begin    -----"<<endl;

    cout<<lir.GetStr();

    cout<<"-----    LIR End     -----"<<endl<<endl;
}

void PrintAssembly()
{
    //cout<<"----- Assembly Begin ----"<<endl;

    cout<<assembly.GetAsm();

    //cout<<"----- Assembly End  -----"<<endl;
}
