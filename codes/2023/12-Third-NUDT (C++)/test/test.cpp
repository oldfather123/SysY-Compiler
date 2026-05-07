#include <bits/stdc++.h>
#include <dirent.h>

#include "SysYLexer.h"
#include "SysYParser.h"
#include "error.hpp"
#include "global.hpp"
#include "irgen.hpp"
#include "riscvgen.hpp"
#include "visitor.hpp"
#include "gtest/gtest.h"
#include "irgen.hpp"
#include "riscvgen.hpp"
#include "visitor.hpp"

namespace fs = std::filesystem;

void get_file_name(string path, vector<string> &filenames) {
  DIR *pDir;
  struct dirent *ptr;
  if (!(pDir = opendir(path.c_str()))) {
    printf("Folder doesn't Exist!\n");
    return;
  }
  while ((ptr = readdir(pDir)) != 0)
    if (strcmp(ptr->d_name, ".") != 0 && strcmp(ptr->d_name, "..") != 0) filenames.push_back(ptr->d_name);
  closedir(pDir);
}

namespace fs = std::filesystem;

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

namespace IR {

TEST(ir, test) {
  vector<string> names;
  fs::path sy_dir("../test/sy");
  fs::create_directories(sy_dir);
  std::ifstream sy_stream;
  std::string filename;
  get_file_name(sy_dir, names);
  for (int i = 0; i < names.size(); i++) {
    filename = names[i].substr(0, names[i].length() - 3);
    std::cerr << filename << "\n";
    sy_stream.open(sy_dir / (filename + ".sy"), std::ios::in);
    try {
      antlr4::ANTLRInputStream antlr_input_stream(sy_stream);
      SysYLexer sysy_lexer(&antlr_input_stream);
      antlr4::CommonTokenStream common_token_stream(&sysy_lexer);
      SysYParser sysy_parser(&common_token_stream);
      IR::Visitor sysy_visitor;
      std::any_cast<IR::CompileUnit *>(sysy_visitor.visitComp(sysy_parser.comp()));
    } catch (const std::exception &e) {
      std::cerr << filename << '\n';
      std::cerr << e.what() << '\n';
    }
    sy_stream.close();
  }
}

}  // namespace IR
