#include "VisualizeGraph.hh"

void visualizeGraph(
    const std::vector<std::pair<std::string, std::string>>& edges) {
  FILE* pipe = popen("python3 src/util/visualize_graph.py", "w");
  if (!pipe) {
    std::cerr << "Failed to start python script" << std::endl;
    return;
  }

  for (const auto& edge : edges) {
    fprintf(pipe, "%s %s\n", edge.first.c_str(), edge.second.c_str());
  }

  fprintf(pipe, "\n");  // end input
  pclose(pipe);
}