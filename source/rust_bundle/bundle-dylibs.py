import os
import sys
import subprocess
import shutil
import getopt

relpath = "../Frameworks"  # dylibs go to @executable_path/../Frameworks by default

usage_text = f"""Usage: {os.path.basename(sys.argv[0])} [-h] [-l relative_path] path_to_app

-h Show this usage description and exit.

-l relative_path
   Specifies, where to put the bundled dylibs relative to the executable.
   When omitted, defaults to: "{relpath}"."""

def usage(exit_code=1):
    print(usage_text, file=sys.stderr)
    sys.exit(exit_code)

def list_dylibs(file):
    result = subprocess.run(["otool", "-L", file], capture_output=True, text=True)
    dylibs = result.stdout.splitlines()
    filtered_dylibs = [
        line.split()[0].split(":")[0] for line in dylibs
        if not (line.startswith("\t/System") or line.startswith("\t/usr/lib"))
    ]
    return filtered_dylibs

def process_dylibs(dylibs, file, libdir, loader_path):
    for dylib_raw in dylibs:
        dylib = dylib_raw.replace("@loader_path", loader_path)
        name = os.path.basename(dylib)
        dest = os.path.join(libdir, name)
        if not os.path.exists(dest):
            shutil.copy(dylib, dest)
            os.chmod(dest, 0o644)
            # subprocess.run(["install_name_tool", "-id", f"@rpath/{name}", dest],
            #                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            # subprocess.run(["codesign", "--force", "-s", "-", dest])
            new_dylibs = list_dylibs(dest)
            process_dylibs(new_dylibs, dest, libdir, os.path.dirname(dylib))
        subprocess.run(["install_name_tool", "-change", dylib_raw, f"@rpath/{name}", file])
        subprocess.run(["codesign", "--force", "-s", "-", file])

def process_executable(executable, libdir):
    dylibs = list_dylibs(executable)
    if dylibs:
        os.makedirs(libdir, exist_ok=True)
        process_dylibs(dylibs, executable, libdir, os.path.dirname(executable))
        subprocess.run(["install_name_tool", "-add_rpath", f"@executable_path/{relpath}", executable])

def main(argv):
    global relpath

    try:
        opts, args = getopt.getopt(argv, "hl:")
    except getopt.GetoptError as err:
        print(err)
        usage()

    for opt, arg in opts:
        if opt == '-h':
            usage(0)
        elif opt == '-l':
            relpath = arg
        else:
            usage()

    if len(args) != 1:
        usage()

    bundlepath = args[0]
    libdir = os.path.join(bundlepath, "Contents", "MacOS", relpath)

    if (os.path.basename(bundlepath).endswith(".app") and
            os.path.isfile(os.path.join(bundlepath, "Contents", "Info.plist")) and
            os.path.isdir(os.path.join(bundlepath, "Contents", "MacOS"))):
        for root, _, files in os.walk(os.path.join(bundlepath, "Contents", "MacOS")):
            for file in files:
                executable = os.path.join(root, file)
                if os.access(executable, os.X_OK) and "Mach-O" in subprocess.run(["file", executable], capture_output=True, text=True).stdout:
                    process_executable(executable, libdir)
    else:
        print(f'"{bundlepath}" does not appear to be an application bundle.', file=sys.stderr)
        sys.exit(1)

    sys.exit(0)

if __name__ == "__main__":
    main(sys.argv[1:])
