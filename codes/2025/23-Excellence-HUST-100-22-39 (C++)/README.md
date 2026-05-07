# 编译系统设计赛（第六届）编译系统实现赛

**队名**：都别卷了让我来  
**队长**：李佳润  
**队员**：张家万、陈联民、付敏宇

---

## 项目结构说明

- `src/`：主源代码目录
  - `main.cpp`：主程序入口
  - `frontend/`：语法分析与抽象语法树相关代码
  - `middleend/`：中间代码生成与优化
  - `backend/`：RISC-V 相关后端实现
- `CMakeLists.txt`：项目的 CMake 构建配置文件
- `README.md`：项目说明文档
- `.gitignore`：Git 忽略文件配置

如需进一步了解各模块功能，请参考对应目录下的 `README.md` 或源代码注释。

---

## Git使用指南（简易版）
由于本项目部分代码的耦合性，为了减少错误合并，现给出一份简单指南，最后同时附上一份[**demo**](#demo)供参考。
1. 获取远程仓库的未同步的更改但不合并，相当于只是让本地仓库知道有这么个更改
    ```bash
    git fetch origin # 将所有远程分支的更改全部拉到本地
    git fetch origin <remote-branch> # 指定远程分支
    ```

2. 将远程仓库的更改合并到本地
    1. **如果本地的代码已经做出了一些修改但未提交**
    ```bash
    # -u参数可以包括未跟踪的文件，具体功能询问ChatGPT即可
    git stash [-u] -m <information-by-yourself> # 暂存当前更改
    ```
    2. 切换到具体需要同步的分支
    ```bash
    git checkout <target-branch> # 需要同步的分支
    git merge origin/<target-branch> # 合并远程分支的更改
    ```
    3. 将远程分支的更改同步到自己写代码的本地分支
    ```bash
    git checkout <your-branch> # 切换到自己的分支
    git merge <target-branch> # 将合并的分支的更改同步到自己的分支
    ```
    4. 合并自己之前的修改和远程仓库的修改
    ```bash
    git stash pop # 在当前分支应用之前暂存的所有修改
    ```
    如果很不幸，自己之前的修改和远程仓库的修改在**同一个地方**，那么则需要解决冲突，打开有冲突的文件会显示冲突的地方，然后**选择**一下需要哪一种更改方案或者**手动合并**两种方案的长处

3. 将本地分支的修改提交到远程仓库
    **注意不要直接开始下面的过程，建议先进行第1、2步以同步远程仓库的更改，减少可能存在的队友push了代码但是没同步给其他队友而导致这次push的代码被覆盖的情况**
    ```bash
    git add . # 添加当前目录的所有更改
    git commit -m <commit-information> # 提交更改
    git checkout <commit-branch> # 切换到提交分支
    git merge <your-branch> # 将刚才提交的更改合并到提交分支
    git push origin <commit-branch> # 将自己的更改推送到远程仓库
    ```

#### demo
假定当前在 **test** 本地分支上写代码：
```bash
git fetch origin baseline_version # 获取推送到 baseline_version 分支的更改
git stash -um Get Update in baseline_version # 暂存自己的修改
git checkout baseline_version # 切换到 baseline_version 分支
git merge origin/baseline_version # 同步修改到本地
git checkout test # 切回 test 分支
git merge baseline_version # 同步修改到自己写代码的分支
git stash pop # 取出自己之前做出的修改
# 处理可能存在的冲突，不建议自动处理，可能出错
# 继续写代码
```