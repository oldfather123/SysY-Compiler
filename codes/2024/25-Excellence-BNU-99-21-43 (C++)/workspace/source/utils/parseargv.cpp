#include "parseargv.h"
#include "error.h"

namespace utils
{
    void CompilerArgs::verify()
    {
        if (inputFileName.empty())
        {
            Error::Error("CompilerArgs::verify", "no input file specified");
        }
        if (genASM && ASMFileName.empty())
        {
            Error::Error("CompilerArgs::verify", "no asm output file specified");
        }
        if (genIR && IRFileName.empty())
        {
            Error::Error("CompilerArgs::verify", "no ir output file specified");
        }

        if (optLevel < 0 || optLevel > 1)
        {
            Error::Error("CompilerArgs::verify", "invalid opt level");
        }

        if (!genASM && !genIR)
        {
            Error::Error("CompilerArgs::verify", "no output specified");
        }
    }

    CompilerArgs ArgvParser::run(int argc, const char *argv[])
    {
        CompilerArgs args;
        for (int i = 1; i < argc; i++)
        {
            std::string arg = argv[i];
            if (arg[0] == '-')
            {
                std::string key = arg.substr(1);
                if (key[0] == '-')
                {
                    key = key.substr(1);
                }
                if (key == "S" || key == "s")
                {
                    args.genASM = true;
                }
                else if (key == "E" || key == "e")
                {
                    args.genIR = true;
                }
                else if (key == "O" || key == "o")
                {
                    if (i + 1 < argc)
                    {
                        args.ASMFileName = argv[i + 1];
                        i++;
                    }
                }
                else if (key == "O1" || key == "o1")
                {
                    args.optLevel = 1;
                }
            }
            else
            {
                args.inputFileName = arg;
            }
        }
        args.verify();
        return args;
    }
};