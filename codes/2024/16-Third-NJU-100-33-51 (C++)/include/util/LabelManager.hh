#ifndef _LABEL_MANAGER_
#define _LABEL_MANAGER_

#include <string>
#include <unordered_map>
using std::string;
using std::unordered_map;

class LabelManager {
 private:
  static int cnt;
  static unordered_map<string, int> labelMap;

 public:
  static string getLabel(string str);
};


#endif