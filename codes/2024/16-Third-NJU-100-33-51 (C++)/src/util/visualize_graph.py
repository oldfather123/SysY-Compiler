"""
Script for visualizing a directed graph from input edges.

Usage:
   Provide edges for the graph via standard input. 
   Each edge should be formatted as "source target".
   Example input:
      A B
      B C
      C D

Dependencies:
- networkx: Install via `pip install networkx`.
- matplotlib: Install via `pip install matplotlib`.
"""

import sys
import networkx as nx
import matplotlib.pyplot as plt

def main():
    edges = []
    for line in sys.stdin:
        if line.strip() == "":  # end input
            break
        edges.append(tuple(line.strip().split()))
        print(line)

    G = nx.DiGraph()
    G.add_edges_from(edges)

    plt.figure(figsize=(10, 8))
    pos = nx.spring_layout(G)
    nx.draw(G, pos, with_labels=True, node_size=2000, node_color='skyblue', font_size=16, font_weight='bold', arrowsize=20)
    plt.title('Graph Visualization', fontsize=20)
    plt.show()

if __name__ == "__main__":
    main()
