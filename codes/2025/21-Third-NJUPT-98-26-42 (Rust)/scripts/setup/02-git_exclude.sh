#!/bin/bash

# 向 tests/.git/info/exclude 写入反向规则，只保留特定文件类型
# 由于tests目录很大且无法fork，使用git exclude而不是.gitignore

TESTS_DIR="tests"

# 检查tests目录是否存在
if [ ! -d "$TESTS_DIR" ]; then
    echo "错误: tests目录不存在"
    exit 1
fi

# 检查tests是否是git仓库或submodule
if [ -f "$TESTS_DIR/.git" ]; then
    # 这是一个git submodule，读取实际的git目录路径
    GIT_DIR=$(cat "$TESTS_DIR/.git" | sed 's/gitdir: //')
    # 如果是相对路径，转换为绝对路径
    if [[ "$GIT_DIR" != /* ]]; then
        GIT_DIR="$TESTS_DIR/$GIT_DIR"
    fi
    EXCLUDE_FILE="$GIT_DIR/info/exclude"
elif [ -d "$TESTS_DIR/.git" ]; then
    # 这是一个普通的git仓库
    EXCLUDE_FILE="$TESTS_DIR/.git/info/exclude"
else
    echo "错误: tests目录不是git仓库或submodule"
    exit 1
fi

# 备份原有的exclude文件
if [ -f "$EXCLUDE_FILE" ]; then
    cp "$EXCLUDE_FILE" "$EXCLUDE_FILE.backup"
    echo "已备份原有exclude文件为 $EXCLUDE_FILE.backup"
fi

# 写入新的exclude规则 - 只排除编译生成的文件
cat > "$EXCLUDE_FILE" << 'EOF'
# git ls-files --others --exclude-from=.git/info/exclude
# Lines that start with '#' are comments.
# For a project mostly in C, the following would be a good set of
# exclude patterns (uncomment them if you want to use them):
# *.[oa]
# *~

# 排除编译生成的文件
*.ast
*.ssa
*.log
*.interp
*.S
CLAUDE.md

# 保留的文件类型: .sy .in .out .pdf libsysy.a sylib.c sylib.h
# 以上规则只排除编译输出，保留所有源文件
EOF

echo "已更新 $EXCLUDE_FILE 文件"
echo ""
echo "可以使用以下命令查看被排除的文件:"
echo "  cd tests && git ls-files --others --exclude-from=\$(cat .git | sed 's/gitdir: //')/info/exclude"