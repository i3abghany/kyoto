import os
import subprocess
import json
import argparse
import tempfile

arg_parser = argparse.ArgumentParser(description='Run include-what-you-use on Kyoto sources and headers')
arg_parser.add_argument('--noapply', action='store_true', help="Don't apply the changes made by include-what-you-use")
args = arg_parser.parse_args()

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

def build(repo_root):
    build_dir = os.path.join(repo_root, 'build')
    os.chdir(build_dir)

    cmake_cmd = ['cmake', '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON', '..']
    subprocess.run(cmake_cmd)

    make_cmd = ['make', '-j8']
    subprocess.run(make_cmd)

def run_iwyu():
    iwyu_cmd = ['iwyu_tool.py', '-p', '.', '-j8']
    output = subprocess.run(iwyu_cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True).stdout
    return output

def apply_iwyu_changes(iwyu_output):
    with tempfile.NamedTemporaryFile(mode='w', delete=False) as f:
        f.write(iwyu_output)
        f.flush()
        f_name = f.name

    with open(f_name, 'r') as f:
        apply_cmd = ['fix_includes.py', '--nosafe_headers', '--nocomments', '--noreorder']
        subprocess.run(apply_cmd, stdin=f, universal_newlines=True)

def ensure_iwyu():
    ret = True
    if which('include-what-you-use') is None:
        print('include-what-you-use binary not found')
        ret = False
        
    
    if which('iwyu_tool.py') is None:
        print('iwyu_tool.py not found')
        ret = False

    if which('fix_includes.py') is None:
        print('fix_includes.py not found')
        ret = False
    
    return ret

def ensure_repo():
    repo_root = get_repo_root()
    if repo_root is None:
        print("Could not find git repository root")
        return None
    return repo_root


def main():
    if not ensure_iwyu():
        return

    repo_root = ensure_repo()
    if repo_root is None:
        return

    if not os.path.exists(os.path.join(repo_root, 'build')):
        print("Build directory does not exist. Creating it.")
        os.makedirs(os.path.join(repo_root, 'build'))

    build(repo_root)
    filter_compile_commands()
    iwyu_output = run_iwyu()

    if args.noapply: print(iwyu_output)
    else: apply_iwyu_changes(iwyu_output)

    

if __name__ == '__main__':
    main()