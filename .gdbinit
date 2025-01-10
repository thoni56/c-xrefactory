python
import os

def find_project_root():
    """Find the project root by searching for a .git directory."""
    dir_path = os.getcwd()
    while dir_path != '/':
        if os.path.exists(os.path.join(dir_path, '.git')):
            return dir_path
        dir_path = os.path.dirname(dir_path)
    raise RuntimeError("Could not find project root (no .git directory found)")

try:
    # Hitta projektroten
    project_root = find_project_root()

    # Konstruera sökvägen till Python-skriptet
    python_script = os.path.join(project_root, "utils/yacc_gdb.py")

    # Ladda Python-skriptet
    gdb.execute(f"source {python_script}")
    gdb.write(f"Loaded Python script: {python_script}\n")
except Exception as e:
    gdb.write(f"Error loading Python script: {e}\n", gdb.STDERR)
end
