import re
import subprocess
import concurrent.futures

def run_dot(graph_dot):
    proc = subprocess.Popen('python3 -m xdot -', shell = True, stdin = subprocess.PIPE)
    proc.communicate(input = graph_dot.encode())

with open('info_right', 'r') as f:
    text = f.read()
    func_pat = r'\{(.*?)\}'
    all_funcs = [match.strip('\n') for match in re.findall(func_pat, text, re.DOTALL)]
    all_graph_dot = []
    for i in range(len(all_funcs)):
        lines = [line.strip() for line in all_funcs[i].splitlines()]
        trans = {}
        bb2lines = {}
        bb_name = ""
        bb_lines = []
        for line in lines:
            label = re.findall(r'([\w_]+):', line, re.DOTALL)
            if label:
                bb_name = label[0]
                trans[bb_name] = []
            else:
                bb_lines.append(line)
                if line.startswith('br'):
                   br_pat = r'label %(label\w+)'
                   all_br_locs = re.findall(br_pat, line, re.DOTALL)
                   trans[bb_name] = all_br_locs
                   bb2lines[bb_name] = '\n'.join(bb_lines)
                   bb_lines = []
                elif line.startswith('ret'):
                   bb2lines[bb_name] = '\n'.join(bb_lines)
                   bb_lines = []
        graph_dot = "digraph G {" + "\n" + "node [shape=rect, fontname=\"Arial\", fontsize=12, fontcolor=\"#333333\", style=\"filled\", fillcolor=\"#ffffff\", labeljust=\"l\", labelloc=\"b\"]" + "\n\t"
        for bb in bb2lines:
            graph_dot += bb + "\t" + "[label=\"" + bb + "\n" + bb2lines[bb] + "\"];" + "\n\t"
        for bb, br_locs in trans.items():
            for br_loc in br_locs:
                graph_dot += bb + " -> " + br_loc + ";" + "\n\t"
                
        graph_dot += "\n}"
        all_graph_dot.append(graph_dot)
        
    with concurrent.futures.ThreadPoolExecutor() as executor:
        futures = []
        for graph_dot in all_graph_dot:
            futures.append(executor.submit(run_dot, graph_dot))
        concurrent.futures.wait(futures)
    
         
                    
                   
            
                
                

