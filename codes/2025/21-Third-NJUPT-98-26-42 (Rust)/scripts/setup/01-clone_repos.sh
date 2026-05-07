#!/bin/bash

# 克隆竞赛作品仓库脚本 - 将ARM和RISCV赛道参赛作品克隆为git子模块
# 基于 LycorisRecompile/LOCAL/repo.md 文件中的仓库列表

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
LOGS_DIR="$PROJECT_ROOT/LOGS"
FAILED_REPOS_LOG="$LOGS_DIR/failed_repos_$(date +%Y%m%d_%H%M%S).log"
mkdir -p "$LOGS_DIR"


add_submodule() {
    local url="$1"
    local path="$2"
    (
        echo "正在添加子模块: $path"
        local max_retries=3
        local retry_count=0
        local success=false

        if [ -d "$path" ] && [ -f "$path/.git" ]; then
            echo "  ◐ 子模块已存在: $path"
            return 0
        fi
        
        while [ $retry_count -lt $max_retries ] && [ "$success" = false ]; do
            if [ $retry_count -gt 0 ]; then
                echo "  第 $retry_count 次重试: $path"
                sleep $((retry_count * 2))  # 递增延迟: 2秒, 4秒, 6秒
            fi
            
            local output
            output=$(git submodule add "$url" "$path" 2>&1)
            local exit_code=$?
            
            if [ $exit_code -eq 0 ]; then
                echo "  ✓ 成功添加: $path"
                success=true
            elif echo "$output" | grep -q "already exists in the index\|Adding existing repo"; then
                echo "  ◐ 子模块已存在于索引中: $path"
                success=true
            else
                retry_count=$((retry_count + 1))
                if [ $retry_count -eq $max_retries ]; then
                    echo "  ✗ 重试 $max_retries 次后仍然失败: $path"
                    echo "  错误信息: $output"
                    echo "  # git submodule add $url \"$path\"  # 错误: $output" >> "$FAILED_REPOS_LOG"
                fi
            fi
        done
    ) &
}

CLONE_TARGET_DIR="$PROJECT_ROOT/.."
pushd "$CLONE_TARGET_DIR" > /dev/null

echo "正在创建目录..."
mkdir -p ARM_repos
mkdir -p RISCV_repos

pushd ARM_repos > /dev/null
git init
popd > /dev/null

pushd RISCV_repos > /dev/null
git init
popd > /dev/null

pushd ARM_repos > /dev/null

add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410006203413-1955 "T202410006203413-喵喵队花样滑冰-北京航空航天大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26172-2487152/T202410006203854-3456 "T202410006203854-周末疯狂拼-北京航空航天大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26172-2487152/T202410006203618-2881 "T202410006203618-原末鸣初-北京航空航天大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26172-2487152/T202410013203408-4017 "T202410013203408-OneLastCompiler-北京邮电大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26172-2487152/T202410614203530-2812 "T202410614203530-伪指令-电子科技大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26172-2487152/T202410246203809-555 "T202410246203809-Compiler_vs_Bugs-复旦大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26172-2487152/compiler2024-sysy "T202490002203625-编译你好香-国防科技大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26172-2487152/T202490002203562-586 "T202490002203562-牛牛向前队-国防科技大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410336203416-1229 "T202410336203416-pinyinggaoshou-杭州电子科技大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26172-2487152/T202410561203866-2221 "T202410561203866-汪汪队立大功队-华南理工大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410284203580-3764 "T202410284203580-BanGDream_ItsMySYSY-南京大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26172-2487152/T202411065203807-3933 "T202411065203807-编译成蓝色疾旋鼬-青岛大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26172-2487152/T202410699203500-3366 "T202410699203500-正道的光-西北工业大学"

echo "正在等待ARM仓库克隆完成..."
wait
echo "ARM仓库处理完成。"

popd > /dev/null

echo "正在添加RISCV赛道仓库作为子模块..."
pushd RISCV_repos > /dev/null

add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410006203585-3730 "T202410006203585-睿睿也想打编译队-北京航空航天大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410006203374-422 "T202410006203374-四元式-北京航空航天大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410006202880-2761 "T202410006202880-逸动山楂队-北京航空航天大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410006203450-2508 "T202410006203450-海底小纵队-北京航空航天大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410006203387-1337 "T202410006203387-YellowYaks-北京航空航天大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410006203104-3288 "T202410006203104-这是个队名队-北京航空航天大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410006203413-1955 "T202410006203413-喵喵队花样滑冰-北京航空航天大学-RISCV"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410336203416-1229 "T202410336203416-pinyinggaoshou-杭州电子科技大学-RISCV"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410284203423-2518 "T202410284203423-冲向广州队-南京大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410006203109-3846 "T202410006203109-NEL-北京航空航天大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410006203533-3019 "T202410006203533-素履_译往队-北京航空航天大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410027203528-232 "T202410027203528-ACM2Compiler-北京师范大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410614202951-722 "T202410614202951-gpt5_0-电子科技大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410614203403-1360 "T202410614203403-编编又译译-电子科技大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202490002203537-615 "T202490002203537-三进制冒险家-国防科技大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202490002203688-1262 "T202490002203688-firefox-国防科技大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202418123202876-2939 "T202418123202876-duskphantom-哈尔滨工业大学_深圳_"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410359203149-675 "T202410359203149-星晴-合肥工业大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202419359203337-3752 "T202419359203337-决不放弃-合肥工业大学_宣城校区_"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410269203414-1848 "T202410269203414-起床困难户-华东师范大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410487203858-2311 "T202410487203858-呼啦啦队-华中科技大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410284203623-538 "T202410284203623-NNVM-南京大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410284203546-2971 "T202410284203546-蚂蚁派-南京大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410055203436-1338 "T202410055203436-人工式生成智能-南开大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410055203113-826 "T202410055203113-八云蓝架构编译器与RE的橙-南开大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410003203847-1935 "T202410003203847-RRVM-清华大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410003203057-1317 "T202410003203057-return_0-清华大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410590203330-200 "T202410590203330-文山湖之狼-深圳大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410486202978-1811 "T202410486202978-Compiler_in_C_Minor-武汉大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410699203233-2083 "T202410699203233-honkaiCC-西北工业大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410423203862-1353 "T202410423203862-水军出击-中国海洋大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410356203271-2091 "T202410356203271-nhwc-中国计量大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410358203100-2802 "T202410358203100-一刻也没有为段错误而哀悼-中国科学技术大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410358203143-3735 "T202410358203143-青春科蝻不会梦到CE队-中国科学技术大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410558203459-3564 "T202410558203459-四个圣甲虫-中山大学"
add_submodule https://gitlab.eduxiji.net/educg-group-26173-2487151/T202410558203130-3063 "T202410558203130-派大星说搞优化就像光头强抓美羊羊-中山大学"

echo "正在等待RISCV仓库克隆完成..."
wait
echo "RISCV仓库处理完成。"

popd > /dev/null

echo "正在克隆子模块..."

pushd ARM_repos > /dev/null
echo "正在更新ARM仓库的子模块..."
git submodule update --init --recursive
popd > /dev/null

pushd RISCV_repos > /dev/null
echo "正在更新RISCV仓库的子模块..."
for submodule in $(ls | grep -v "T202418123202876-duskphantom"); do
    if [ -d "$submodule" ]; then
        pushd "$submodule" > /dev/null
        if [ -f ".gitmodules" ]; then
            echo "正在更新子模块: $submodule"
            git submodule update --init --recursive || echo "  警告: $submodule 的某些子模块更新失败"
        fi
        popd > /dev/null
    fi
    
done
echo "T202418123202876-duskphantom 只更新主仓库 跳过损坏的子模块..."

popd > /dev/null
popd > /dev/null

# 统计结果
ARM_COUNT=$(ls "$CLONE_TARGET_DIR/ARM_repos" 2>/dev/null | wc -l)
RISCV_COUNT=$(ls "$CLONE_TARGET_DIR/RISCV_repos" 2>/dev/null | wc -l)
TOTAL_COUNT=$((ARM_COUNT + RISCV_COUNT))

echo "ARM赛道仓库数量: $ARM_COUNT"
echo "RISCV赛道仓库数量: $RISCV_COUNT"
echo "总计: $TOTAL_COUNT 个仓库"
echo ""
echo "文件位置:"
echo "- ARM仓库目录: $CLONE_TARGET_DIR/ARM_repos/"
echo "- RISCV仓库目录: $CLONE_TARGET_DIR/RISCV_repos/"

# 检查是否有失败的仓库
if [ -f "$FAILED_REPOS_LOG" ]; then
    FAILED_COUNT=$(wc -l < "$FAILED_REPOS_LOG")
    echo "- 失败仓库日志: $FAILED_REPOS_LOG ($FAILED_COUNT 个失败)"
else
    echo "- 所有仓库克隆成功"
fi
