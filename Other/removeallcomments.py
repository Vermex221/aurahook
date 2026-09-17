import os
import sys

extensions = {".cpp", ".hpp", ".h", ".c"}

ignored_files = {
    os.path.normcase(os.path.abspath(
        r"C:\Users\harry\Downloads\aurahook-cs2\src\features\skins\skins.cpp"
    ))
}


def strip_comments_from_source(source: str) -> str:
    result = []
    i = 0
    n = len(source)

    in_string = False
    in_char = False
    in_single_comment = False
    in_multi_comment = False

    while i < n:
        c = source[i]
        c_next = source[i + 1] if i + 1 < n else ''

        if in_single_comment:
            if c == '\n':
                in_single_comment = False
                result.append(c)
            elif c == '\r' and c_next == '\n':
                in_single_comment = False
                result.append(c)
                result.append(c_next)
                i += 1
            i += 1
            continue

        if in_multi_comment:
            if c == '*' and c_next == '/':
                in_multi_comment = False
                i += 2
            else:
                i += 1
            continue

        if in_string:
            result.append(c)
            if c == '\\':
                if i + 1 < n:
                    i += 1
                    result.append(source[i])
            elif c == '"':
                in_string = False
            i += 1
            continue

        if in_char:
            result.append(c)
            if c == '\\':
                if i + 1 < n:
                    i += 1
                    result.append(source[i])
            elif c == "'":
                in_char = False
            i += 1
            continue

        if c == '/' and c_next == '/':
            in_single_comment = True
            i += 2
            continue

        if c == '/' and c_next == '*':
            in_multi_comment = True
            i += 2
            continue

        if c == '"':
            in_string = True
            result.append(c)
            i += 1
            continue

        if c == "'":
            in_char = True
            result.append(c)
            i += 1
            continue

        result.append(c)
        i += 1

    cleaned_lines = []

    for line in "".join(result).splitlines(keepends=True):
        stripped_line = line.rstrip(" \t\r\n")

        if stripped_line or line.endswith("\n"):
            cleaned_lines.append(line)

    return "".join(cleaned_lines)


def process_directory(target_path):
    if not os.path.exists(target_path):
        print(f"Error: Path not found: {target_path}")
        return

    modified_count = 0
    total_files = 0

    for root, _, files in os.walk(target_path):
        for filename in files:
            ext = os.path.splitext(filename)[1].lower()

            if ext not in extensions:
                continue

            total_files += 1
            file_path = os.path.join(root, filename)

            if os.path.normcase(os.path.abspath(file_path)) in ignored_files:
                print(f"[ignored] {file_path}")
                continue

            try:
                with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
                    original = f.read()

                stripped = strip_comments_from_source(original)

                if stripped != original:
                    with open(file_path, "w", encoding="utf-8", newline="") as f:
                        f.write(stripped)

                    rel_path = os.path.relpath(file_path, target_path)
                    print(f"[removed] {rel_path}")
                    modified_count += 1

            except Exception as e:
                print(f"[error] {file_path}: {e}")

    print(
        f"\nFinished. Removed comments from "
        f"{modified_count} of {total_files} files."
    )


if __name__ == "__main__":
    if len(sys.argv) > 1:
        target = sys.argv[1]
    else:
        base = os.path.dirname(os.path.abspath(__file__))

        if os.path.exists(os.path.join(base, "src")):
            target = os.path.join(base, "src")
        elif os.path.exists(os.path.join(base, "..", "src")):
            target = os.path.normpath(os.path.join(base, "..", "src"))
        elif os.path.exists("src"):
            target = os.path.abspath("src")
        else:
            target = os.path.join(base, "src")

    print(f"Stripping all comments in: {target} ...")
    process_directory(target)
