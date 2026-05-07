#include "LabelManager.hh"

string LabelManager::getLabel(string str) {
  str = str.substr(0, 30);
  if (str == "") {
    return "r_" + std::to_string(cnt++);
  }
  auto it = labelMap.find(str);
  if (it == labelMap.end()) {
    labelMap[str] = 0;
    return str + "." + std::to_string(0);
  } else {
    it->second++;
    return str + "." + std::to_string(it->second);
  }
}

unordered_map<string, int> LabelManager::labelMap;
int LabelManager::cnt = 0;
