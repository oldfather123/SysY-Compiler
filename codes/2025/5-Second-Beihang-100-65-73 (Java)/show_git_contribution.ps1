# 设置控制台编码为 UTF-8
chcp 65001 > $null
$OutputEncoding = [Console]::OutputEncoding = [System.Text.Encoding]::UTF8

# 计算字符串显示宽度（中文字符宽度为2）
function Get-DisplayWidth($str) {
    $width = 0
    foreach ($char in $str.ToCharArray()) {
        $code = [int][char]$char
        if ($code -gt 127) {
            $width += 2  # 非 ASCII 字符宽度按2处理
        } else {
            $width += 1
        }
    }
    return $width
}

# 按显示宽度左对齐填充空格
function Pad-DisplayLeft($str, $targetWidth) {
    $currentWidth = Get-DisplayWidth $str
    $padding = $targetWidth - $currentWidth
    return $str + (" " * [Math]::Max(0, $padding))
}

# 获取作者列表（去重）
$authors = git log --format='%aN' | Sort-Object -Unique

# 设置作者列宽（注意中文字符显示宽度）
$authorColumnWidth = 25

# 输出表头
"{0} {1,10} {2,12} {3,12} {4,12}" -f (Pad-DisplayLeft "Author" $authorColumnWidth), "Commits", "Added", "Deleted", "Net"

# 遍历每位作者
foreach ($author in $authors) {
    # 提交次数
    $commitCount = (git log --author="$author" --pretty=oneline | Measure-Object -Line).Lines

    # 初始化行数统计
    $added = 0
    $deleted = 0

    # 获取该作者的所有 diff 行数（numstat）
    $numstat = git log --author="$author" --pretty=tformat: --numstat

    foreach ($line in $numstat) {
        if ($line -match "^\d+\s+\d+") {
            $parts = $line -split "\s+"
            $added += [int]$parts[0]
            $deleted += [int]$parts[1]
        }
    }

    $net = $added - $deleted

    # 格式化作者列宽并输出
    $authorStr = Pad-DisplayLeft $author $authorColumnWidth
    "{0} {1,10} {2,12} {3,12} {4,12}" -f $authorStr, $commitCount, $added, $deleted, $net
}
