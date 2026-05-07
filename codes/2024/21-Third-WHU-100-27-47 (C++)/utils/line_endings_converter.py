from pathlib import Path
import os


def get_root_dir() -> str:
    """
    获取 `sysy-compiler` 的根目录
    """
    root_dir = os.path.dirname(os.path.realpath(__file__))
    while os.path.basename(root_dir) != "sysy-compiler":
        root_dir = os.path.dirname(root_dir)
    return root_dir


def convert_line_endings_to_lf(file_path):
    with open(file_path, "r", encoding="utf-8") as file:
        content = file.read()

    with open(file_path, "w", encoding="utf-8") as file:
        file.write(content.replace("\r\n", "\n"))


def main():
    print("Converting line endings...")

    # Define the directory to start from (current directory)
    root_dir = Path(get_root_dir())

    # Define the file extensions to format
    file_extensions = [
        ".h",
        ".cpp",
        ".sy",
        ".out",
        ".txt",
        ".md",
        ".sh",
        ".py",
        ".json",
        ".clang-format",
    ]
    dirs_to_exclude = [
        ".git",
        "build",
    ]

    # Traverse the directory tree
    for path in root_dir.rglob("*"):
        if any(
            part == dir_name_to_exclude
            for part in path.parts
            for dir_name_to_exclude in dirs_to_exclude
        ):
            continue
        if path.is_file():
            if path.suffix not in file_extensions:
                continue
            print(f"- Converting {path}")
            try:
                convert_line_endings_to_lf(path)
            except Exception as e:
                print(f"  - Error: {e}")

    print("Converting line endings DONE")


if __name__ == "__main__":
    main()
