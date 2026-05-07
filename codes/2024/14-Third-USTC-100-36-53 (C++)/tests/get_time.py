import re

# 假设 log_content1 和 log_content2 分别包含两次运行的日志文本
log_content1 = """
zox@zox-virtual-machine:~/compiler/2023ustc-jianmu-compiler/tests$ ./test_func.sh /home/zox/compiler/2023ustc-jianmu-compiler/tests/final_performance test
[info] Start testing, using testcase dir: /home/zox/compiler/2023ustc-jianmu-compiler/tests/final_performance
01_mm1...
Timer@0065-0084: 0H-0M-25S-446715us
TOTAL: 0H-0M-25S-446715us
OK
01_mm2...
Timer@0065-0084: 0H-0M-22S-498125us
TOTAL: 0H-0M-22S-498125us
OK
01_mm3...
Timer@0065-0084: 0H-0M-16S-470186us
TOTAL: 0H-0M-16S-470186us
OK
03_sort1...
Timer@0090-0102: 0H-0M-5S-363406us
TOTAL: 0H-0M-5S-363406us
OK
03_sort2...
Timer@0090-0102: 0H-1M-34S-799948us
TOTAL: 0H-1M-34S-799948us
OK
03_sort3...
Timer@0090-0102: 0H-0M-17S-816358us
TOTAL: 0H-0M-17S-816358us
OK
04_spmv1...
Timer@0039-0047: 0H-0M-12S-572414us
TOTAL: 0H-0M-12S-572414us
OK
04_spmv2...
Timer@0039-0047: 0H-0M-8S-467665us
TOTAL: 0H-0M-8S-467665us
OK
04_spmv3...
Timer@0039-0047: 0H-0M-10S-408268us
TOTAL: 0H-0M-10S-408268us
OK
fft0...
Timer@0060-0079: 0H-0M-19S-125276us
TOTAL: 0H-0M-19S-125276us
OK
fft1...
Timer@0060-0079: 0H-0M-41S-729901us
TOTAL: 0H-0M-41S-729901us
OK
fft2...
Timer@0060-0079: 0H-0M-40S-132057us
TOTAL: 0H-0M-40S-132057us
OK
gameoflife-gosper...
Timer@0095-0106: 0H-1M-57S-772100us
TOTAL: 0H-1M-57S-772100us
OK
gameoflife-oscillator...
Timer@0095-0106: 0H-1M-45S-714948us
TOTAL: 0H-1M-45S-714948us
OK
gameoflife-p61glidergun...
Timer@0095-0106: 0H-1M-39S-902200us
TOTAL: 0H-1M-39S-902200us
OK
if-combine1...
Timer@0324-0328: 0H-0M-39S-540393us
TOTAL: 0H-0M-39S-540393us
OK
if-combine2...
Timer@0324-0328: 0H-0M-56S-851175us
TOTAL: 0H-0M-56S-851175us
OK
if-combine3...
Timer@0324-0328: 0H-1M-34S-202679us
TOTAL: 0H-1M-34S-202679us
OK
large_loop_array_1...
Timer@0022-0039: 0H-0M-48S-511890us
TOTAL: 0H-0M-48S-511890us
OK
large_loop_array_2...
Timer@0021-0038: 0H-1M-37S-282428us
TOTAL: 0H-1M-37S-282428us
OK
large_loop_array_3...
Timer@0021-0038: 0H-0M-38S-534315us
TOTAL: 0H-0M-38S-534315us
OK
matmul1...
Timer@0023-0092: 0H-0M-16S-443189us
TOTAL: 0H-0M-16S-443189us
OK
matmul2...
Timer@0023-0092: 0H-0M-16S-292031us
TOTAL: 0H-0M-16S-292031us
OK
matmul3...
Timer@0023-0092: 0H-0M-16S-346925us
TOTAL: 0H-0M-16S-346925us
OK
recursive_call_1...
Timer@0021-0032: 0H-1M-24S-655304us
TOTAL: 0H-1M-24S-655304us
OK
recursive_call_2...
Timer@0021-0032: 0H-0M-21S-318062us
TOTAL: 0H-0M-21S-318062us
OK
recursive_call_3...
Timer@0021-0032: 0H-0M-42S-604323us
TOTAL: 0H-0M-42S-604323us
OK
shuffle0...
Timer@0078-0090: 0H-0M-8S-324554us
TOTAL: 0H-0M-8S-324554us
OK
shuffle1...
Timer@0078-0090: 0H-0M-3S-138397us
TOTAL: 0H-0M-3S-138397us
OK
shuffle2...
Timer@0078-0090: 0H-0M-5S-360695us
TOTAL: 0H-0M-5S-360695us
OK
sl1...
Timer@0013-0053: 0H-0M-25S-246621us
TOTAL: 0H-0M-25S-246621us
OK
sl2...
Timer@0013-0053: 0H-0M-7S-199984us
TOTAL: 0H-0M-7S-199984us
OK
sl3...
Timer@0013-0053: 0H-0M-3S-26566us
TOTAL: 0H-0M-3S-26566us
OK
transpose0...
Timer@0028-0047: 0H-0M-10S-870701us
TOTAL: 0H-0M-10S-870701us
OK
transpose1...
Timer@0028-0047: 0H-0M-18S-619052us
TOTAL: 0H-0M-18S-619052us
OK
transpose2...
Timer@0028-0047: 0H-0M-17S-754001us
TOTAL: 0H-0M-17S-754001us
OK

"""

log_content2 = """
zox@zox-virtual-machine:~/compiler/2023ustc-jianmu-compiler/tests$ ./test_func.sh /home/zox/compiler/2023ustc-jianmu-compiler/tests/final_performance ll
[info] Start testing, using testcase dir: /home/zox/compiler/2023ustc-jianmu-compiler/tests/final_performance
01_mm1...
Timer@0065-0084: 0H-0M-0S-330214us
TOTAL: 0H-0M-0S-330214us
OK
01_mm2...
Timer@0065-0084: 0H-0M-0S-272855us
TOTAL: 0H-0M-0S-272855us
OK
01_mm3...
Timer@0065-0084: 0H-0M-0S-210419us
TOTAL: 0H-0M-0S-210419us
OK
03_sort1...
Timer@0090-0102: 0H-0M-0S-168244us
TOTAL: 0H-0M-0S-168244us
OK
03_sort2...
Timer@0090-0102: 0H-0M-2S-701144us
TOTAL: 0H-0M-2S-701144us
OK
03_sort3...
Timer@0090-0102: 0H-0M-0S-546064us
TOTAL: 0H-0M-0S-546064us
OK
04_spmv1...
Timer@0039-0047: 0H-0M-0S-409059us
TOTAL: 0H-0M-0S-409059us
OK
04_spmv2...
Timer@0039-0047: 0H-0M-0S-412215us
TOTAL: 0H-0M-0S-412215us
OK
04_spmv3...
Timer@0039-0047: 0H-0M-0S-276771us
TOTAL: 0H-0M-0S-276771us
OK
fft0...
Timer@0060-0079: 0H-0M-1S-691254us
TOTAL: 0H-0M-1S-691254us
OK
fft1...
Timer@0060-0079: 0H-0M-3S-634721us
TOTAL: 0H-0M-3S-634721us
OK
fft2...
Timer@0060-0079: 0H-0M-3S-428046us
TOTAL: 0H-0M-3S-428046us
OK
gameoflife-gosper...
Timer@0095-0106: 0H-0M-1S-728229us
TOTAL: 0H-0M-1S-728229us
OK
gameoflife-oscillator...
Timer@0095-0106: 0H-0M-1S-627332us
TOTAL: 0H-0M-1S-627332us
OK
gameoflife-p61glidergun...
Timer@0095-0106: 0H-0M-1S-660240us
TOTAL: 0H-0M-1S-660240us
OK
if-combine1...
Timer@0324-0328: 0H-0M-0S-540784us
TOTAL: 0H-0M-0S-540784us
OK
if-combine2...
Timer@0324-0328: 0H-0M-0S-825383us
TOTAL: 0H-0M-0S-825383us
OK
if-combine3...
Timer@0324-0328: 0H-0M-1S-346342us
TOTAL: 0H-0M-1S-346342us
OK
large_loop_array_1...
Timer@0022-0039: 0H-0M-1S-175874us
TOTAL: 0H-0M-1S-175874us
OK
large_loop_array_2...
Timer@0021-0038: 0H-0M-2S-442211us
TOTAL: 0H-0M-2S-442211us
OK
large_loop_array_3...
Timer@0021-0038: 0H-0M-0S-993037us
TOTAL: 0H-0M-0S-993037us
OK
matmul1...
Timer@0023-0092: 0H-0M-0S-959795us
TOTAL: 0H-0M-0S-959795us
OK
matmul2...
Timer@0023-0092: 0H-0M-0S-957412us
TOTAL: 0H-0M-0S-957412us
OK
matmul3...
Timer@0023-0092: 0H-0M-0S-950983us
TOTAL: 0H-0M-0S-950983us
OK
recursive_call_1...
Timer@0021-0032: 0H-0M-6S-674612us
TOTAL: 0H-0M-6S-674612us
OK
recursive_call_2...
Timer@0021-0032: 0H-0M-1S-684208us
TOTAL: 0H-0M-1S-684208us
OK
recursive_call_3...
Timer@0021-0032: 0H-0M-3S-339260us
TOTAL: 0H-0M-3S-339260us
OK
shuffle0...
Timer@0078-0090: 0H-0M-1S-840717us
TOTAL: 0H-0M-1S-840717us
OK
shuffle1...
Timer@0078-0090: 0H-0M-1S-796821us
TOTAL: 0H-0M-1S-796821us
OK
shuffle2...
Timer@0078-0090: 0H-0M-0S-792723us
TOTAL: 0H-0M-0S-792723us
OK
sl1...
Timer@0013-0053: 0H-0M-4S-873701us
TOTAL: 0H-0M-4S-873701us
OK
sl2...
Timer@0013-0053: 0H-0M-0S-842931us
TOTAL: 0H-0M-0S-842931us
OK
sl3...
Timer@0013-0053: 0H-0M-0S-380335us
TOTAL: 0H-0M-0S-380335us
OK
transpose0...
Timer@0028-0047: 0H-0M-0S-393457us
TOTAL: 0H-0M-0S-393457us
OK
transpose1...
Timer@0028-0047: 0H-0M-0S-523661us
TOTAL: 0H-0M-0S-523661us
OK
transpose2...
Timer@0028-0047: 0H-0M-0S-954121us
TOTAL: 0H-0M-0S-954121us
OK

"""
log_content3 = """
./test_func.sh /home/zox/compiler/2023ustc-jianmu-compiler/tests/final_performance ll
[info] Start testing, using testcase dir: /home/zox/compiler/2023ustc-jianmu-compiler/tests/final_performance
01_mm1...
Timer@0065-0084: 0H-0M-0S-863282us
TOTAL: 0H-0M-0S-863282us
OK
01_mm2...
Timer@0065-0084: 0H-0M-0S-727239us
TOTAL: 0H-0M-0S-727239us
OK
01_mm3...
Timer@0065-0084: 0H-0M-0S-541839us
TOTAL: 0H-0M-0S-541839us
OK
03_sort1...
Timer@0090-0102: 0H-0M-1S-535us
TOTAL: 0H-0M-1S-535us
OK
03_sort2...
Timer@0090-0102: 0H-0M-14S-327686us
TOTAL: 0H-0M-14S-327686us
OK
03_sort3...
Timer@0090-0102: 0H-0M-2S-988073us
TOTAL: 0H-0M-2S-988073us
OK
04_spmv1...
Timer@0039-0047: 0H-0M-0S-547606us
TOTAL: 0H-0M-0S-547606us
OK
04_spmv2...
Timer@0039-0047: 0H-0M-0S-443598us
TOTAL: 0H-0M-0S-443598us
OK
04_spmv3...
Timer@0039-0047: 0H-0M-0S-385199us
TOTAL: 0H-0M-0S-385199us
OK
fft0...
Timer@0060-0079: 0H-0M-3S-999194us
TOTAL: 0H-0M-3S-999194us
OK
fft1...
Timer@0060-0079: 0H-0M-8S-295507us
TOTAL: 0H-0M-8S-295507us
OK
fft2...
Timer@0060-0079: 0H-0M-7S-823764us
TOTAL: 0H-0M-7S-823764us
OK
gameoflife-gosper...
Timer@0095-0106: 0H-0M-3S-460661us
TOTAL: 0H-0M-3S-460661us
OK
gameoflife-oscillator...
Timer@0095-0106: 0H-0M-3S-81508us
TOTAL: 0H-0M-3S-81508us
OK
gameoflife-p61glidergun...
Timer@0095-0106: 0H-0M-3S-192347us
TOTAL: 0H-0M-3S-192347us
OK
if-combine1...
Timer@0324-0328: 0H-0M-1S-693501us
TOTAL: 0H-0M-1S-693501us
OK
if-combine2...
Timer@0324-0328: 0H-0M-2S-420233us
TOTAL: 0H-0M-2S-420233us
OK
if-combine3...
Timer@0324-0328: 0H-0M-4S-2239us
TOTAL: 0H-0M-4S-2239us
OK
large_loop_array_1...
Timer@0022-0039: 0H-0M-1S-202441us
TOTAL: 0H-0M-1S-202441us
OK
large_loop_array_2...
Timer@0021-0038: 0H-0M-2S-448672us
TOTAL: 0H-0M-2S-448672us
OK
large_loop_array_3...
Timer@0021-0038: 0H-0M-0S-985641us
TOTAL: 0H-0M-0S-985641us
OK
matmul1...
Timer@0023-0092: 0H-0M-1S-24730us
TOTAL: 0H-0M-1S-24730us
OK
matmul2...
Timer@0023-0092: 0H-0M-1S-128651us
TOTAL: 0H-0M-1S-128651us
OK
matmul3...
Timer@0023-0092: 0H-0M-1S-34706us
TOTAL: 0H-0M-1S-34706us
OK
recursive_call_1...
Timer@0021-0032: 0H-0M-6S-993671us
TOTAL: 0H-0M-6S-993671us
OK
recursive_call_2...
Timer@0021-0032: 0H-0M-1S-702829us
TOTAL: 0H-0M-1S-702829us
OK
recursive_call_3...
Timer@0021-0032: 0H-0M-3S-504642us
TOTAL: 0H-0M-3S-504642us
OK
shuffle0...
Timer@0078-0090: 0H-0M-1S-888051us
TOTAL: 0H-0M-1S-888051us
OK
shuffle1...
Timer@0078-0090: 0H-0M-2S-99811us
TOTAL: 0H-0M-2S-99811us
OK
shuffle2...
Timer@0078-0090: 0H-0M-1S-235934us
TOTAL: 0H-0M-1S-235934us
OK
sl1...
Timer@0013-0053: 0H-0M-2S-126718us
TOTAL: 0H-0M-2S-126718us
OK
sl2...
Timer@0013-0053: 0H-0M-0S-777895us
TOTAL: 0H-0M-0S-777895us
OK
sl3...
Timer@0013-0053: 0H-0M-0S-344186us
TOTAL: 0H-0M-0S-344186us
OK
transpose0...
Timer@0028-0047: 0H-0M-0S-446925us
TOTAL: 0H-0M-0S-446925us
OK
transpose1...
Timer@0028-0047: 0H-0M-0S-645804us
TOTAL: 0H-0M-0S-645804us
OK
transpose2...
Timer@0028-0047: 0H-0M-0S-984076us
TOTAL: 0H-0M-0S-984076us
OK
"""

# 正则表达式匹配格式：测试点名称...Timer@XXXX-XXXX: 0H-0M-0S-123456us
test_point_pattern = re.compile(r'(\w+)\.\.\.\n.*?Timer@\d+-\d+: (\d+)H-(\d+)M-(\d+)S-(\d+)us', re.DOTALL)

def parse_log(log_content):
    results = {}
    for match in test_point_pattern.finditer(log_content):
        test_name, hours, minutes, seconds, microseconds = match.groups()
        total_microseconds = (int(hours) * 3600 + int(minutes) * 60 + int(seconds)) * 1000000 + int(microseconds)
        results[test_name] = total_microseconds
    return results

# 解析两次运行的日志
results1 = parse_log(log_content2)
results2 = parse_log(log_content1)
results3 = parse_log(log_content3)

# 计算并打印每个测试点的倍率差异
rate_diffs = []
for test_name in results1:
    if test_name in results2:
        rate_diff = (results2[test_name] / results1[test_name] ) / (results3[test_name] / results1[test_name])
        # rate_diff = (results3[test_name] / results1[test_name] )
        rate_diffs.append(rate_diff)
        print(f"{test_name}: {rate_diff:.3f}")

# 计算并打印平均倍率差异
if rate_diffs:
    avg_rate_diff = sum(rate_diffs) / len(rate_diffs)
    print(f"average = {avg_rate_diff:.3f}")
    max_rate_diff = max(rate_diffs)
    print(f"max = {max_rate_diff:.3f}")
    min_rate_diff = min(rate_diffs)
    print(f"min = {min_rate_diff:.3f}")
    rate_diffs.sort()
    mid_rate_diff = rate_diffs[len(rate_diffs) // 2]
    print(f"mid = {mid_rate_diff:.3f}")
else:
    print("没有可比较的测试点。")

# 绘图并且保存
import matplotlib.pyplot as plt
plt.plot(rate_diffs)
plt.savefig("rate_diffs.png")
plt.show()

