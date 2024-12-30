import os
import subprocess
import json

def which(program):
    def is_exe(fpath):
        return os.path.isfile(fpath) and os.access(fpath, os.X_OK)

    fpath, _ = os.path.split(program)

    if fpath:
        if is_exe(program):
            return program
    else:
        for path in os.environ['PATH'].split(os.pathsep):
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                return exe_file
    return None


def get_repo_root():
    try:
        out = subprocess.check_output(['git', 'rev-parse', '--show-toplevel']).strip()
        return out.decode('utf-8')
    except subprocess.CalledProcessError:
        return None


def filter_compile_commands():
    with open('compile_commands.json', 'r') as f:
        compile_commands = json.load(f)

    compile_commands = [cc for cc in compile_commands if 'generated' not in cc['file']]
    with open('compile_commands.json', 'w') as f:
        json.dump(compile_commands, f, indent=2)


def main():
    if which('include-what-you-use') is None:
        print('include-what-you-use binary not found')
        return
    
    if which('iwyu_tool.py') is None:
        print('iwyu_tool.py not found')
        return

    repo_root = get_repo_root()
    if repo_root is None:
        print("Could not find git repository root")
        return

    if not os.path.exists(os.path.join(repo_root, 'build')):
        print("Build directory does not exist. Creating it.")
        os.makedirs(os.path.join(repo_root, 'build'))

    build_dir = os.path.join(repo_root, 'build')
    os.chdir(build_dir)

    cmake_cmd = ['cmake', '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON', '..']
    subprocess.run(cmake_cmd)

    make_cmd = ['make', '-j8']
    subprocess.run(make_cmd)

    filter_compile_commands()

    iwyu_cmd = ['iwyu_tool.py', '-p', '.', '-j8']
    subprocess.run(iwyu_cmd)

    

if __name__ == '__main__':
    main()