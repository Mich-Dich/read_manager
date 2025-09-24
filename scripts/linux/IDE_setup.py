
import os
import sys
import glob
import platform
import subprocess
import stat
import json
import datetime

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
import utils

def detect_rider():
    # Check common Rider installation paths on Linux
    rider_paths = [
        os.path.expanduser("~/.local/share/JetBrains/Toolbox/apps/Rider"),
        "/opt/JetBrains Rider",
        os.path.expanduser("~/.local/share/applications/jetbrains-rider.desktop")
    ]
    
    # Check if rider command exists in PATH
    try:
        subprocess.run(["which", "rider"], check=True, capture_output=True)
        return True
    except subprocess.CalledProcessError:
        pass

    # Check physical paths
    for path in rider_paths:
        if os.path.exists(path) or len(glob.glob(path)) > 0:
            return True
    return False


def detect_vscode():
    # Check if the `code` CLI is available
    try:
        subprocess.run(["which", "code"], check=True, capture_output=True)
        return True
    except subprocess.CalledProcessError:
        pass

    # Check common VSCode install paths (Linux)
    vscode_paths = [
        "/usr/share/code",
        "/opt/visual-studio-code",
        os.path.expanduser("~/.vscode"),
        os.path.expanduser("~/.var/app/com.visualstudio.code"),  # Flatpak
    ]
    for path in vscode_paths:
        if os.path.exists(path) or glob.glob(path):
            return True

    return False


def detect_IDEs():
    IDEs = []
    
    if detect_vscode():
        IDEs.append("VSCode")
    
    if detect_rider():                      # Detect available IDEs
        IDEs.append("JetBrains Rider")

    IDEs.append("Makefile (CLion/Ninja compatible)")
    
    if not IDEs:
        print("No supported IDEs detected.")
        sys.exit(1)
    
    return IDEs

        
def prompt_ide_selection():
    
    IDEs = detect_IDEs()
    print("\nDetected IDEs:")
    for i, ide in enumerate(IDEs):
        print(f"{i}. {ide}")

    if len(IDEs) == 1:
        print("Only one IDE detected")
        return IDEs[0]

    while True:
        choice = input("Select an IDE to use (enter the number): ")
        try:
            choice_index = int(choice)
            if 0 <= choice_index < len(IDEs):
                return IDEs[choice_index]
            print("Invalid selection number.")
        except ValueError:
            print("Please enter a valid number.")


def prompt_build_config():
    configs = ["Debug", "RelWithDebInfo", "Release"]
    print("\nAvailable build configurations:")
    for i, cfg in enumerate(configs):
        print(f"{i}. {cfg}")
    
    while True:
        choice = input("Select a configuration (enter the number): ")
        try:
            index = int(choice)
            if 0 <= index < len(configs):
                return configs[index]
            else:
                print("Invalid number.")
        except ValueError:
            print("Please enter a valid number.")


def setup_vscode_configs(project_root, build_config, application_name):

    vscode_dir = os.path.join(project_root, ".vscode")
    os.makedirs(vscode_dir, exist_ok=True)
    arch = "x86_64"
    system = platform.system().lower()  # "linux" or "windows"
    output_dir = f"{build_config}-{system}-{arch}"
    bin_dir = os.path.join(project_root, "bin", output_dir)

    # create build.sh
    build_script_path = os.path.join(vscode_dir, "build.sh")
    build_script_content = f"""#!/usr/bin/env bash
set -e  # Stop on error

# ------------- CONFIGURATION -------------
build_config="{build_config}"
project_name="{application_name}"
workspace_dir="{project_root}"
build_dir="${{workspace_dir}}/build"
timestamp=$(date '+%Y-%m-%d-%H:%M:%S')

# Detect platform and architecture automatically
platform=$(uname | tr '[:upper:]' '[:lower:]')
arch=$(uname -m)
stage_name="${{project_name}}_${{build_config}}_${{timestamp}}"
config_dir="${{build_config}}-${{platform}}-${{arch}}"
bin_dir="${{build_dir}}/${{config_dir}}/bin"
stage_dir="${{bin_dir}}/${{stage_name}}"

stage_name="_${{build_config}}_${{timestamp}}"
STAGE_DIR="{bin_dir}/${{stage_name}}"
timestamp=$(date '+%Y-%m-%d-%H:%M:%S')


# ------------- CLEANUP PREVIOUS ARTIFACTS -------------
echo "------ Preparing stage: ${{stage_dir}} ------"
mkdir -p "${{stage_dir}}"

# Move previous binary into staging dir (ignore missing files)
if [[ -f "${{bin_dir}}/${{project_name}}" ]]; then
    mv "${{bin_dir}}/${{project_name}}" "${{stage_dir}}/" || true
fi

# Trash the staging dir
gio trash "${{stage_dir}}" --force || true


# ------------- REGENERATE CMAKE BUILD FILES -------------
echo "------ Configuring CMake ------"
cmake -S "${{workspace_dir}}" -B "${{build_dir}}" -DCMAKE_BUILD_TYPE="${{build_config}}"


# ------------- BUILD WITH CMAKE -------------
echo "------ Building (${{build_config}}) ------"
cmake --build "${{build_dir}}" --config "${{build_config}}" -j"$(nproc)"

echo "------ Done! ------"
echo "Binary available at: ${{bin_dir}}/${{project_name}}"
"""
    with open(build_script_path, "w") as f:
        f.write(build_script_content)
    os.chmod(build_script_path, os.stat(build_script_path).st_mode | stat.S_IEXEC)

    # create tasks.json
    tasks_json_path = os.path.join(vscode_dir, "tasks.json")
    tasks_data = {
        "version": "2.0.0",
        "tasks": [
            {
                "label": "Clean & Build",
                "type": "shell",
                "command": "${workspaceFolder}/.vscode/build.sh",
                "problemMatcher": [],
                "group": { "kind": "build", "isDefault": True },
                "presentation": {
                    "reveal": "always",
                    "panel": "shared"
                }
            }
        ]
    }
    with open(tasks_json_path, "w") as f:
        json.dump(tasks_data, f, indent=4)

    # create launch.json
    launch_json_path = os.path.join(vscode_dir, "launch.json")
    program_path = os.path.join("${workspaceFolder}", "build", "bin", output_dir, f"{application_name}")
    cwd_path = os.path.join("${workspaceFolder}", "build", output_dir)

    launch_data = {
        "version": "0.2.0",
        "configurations": [
            {
                "name": f"Launch {application_name} ({build_config})",
                "type": "cppdbg",
                "request": "launch",
                "program": program_path,
                "args": [],
                "cwd": cwd_path,
                "preLaunchTask": "Clean & Build",
                "stopAtEntry": False,
                "environment": [],
                "externalConsole": False,
                "MIMode": "gdb",
                "miDebuggerPath": "/usr/bin/gdb",
                "setupCommands": [
                    {
                        "description": "Enable pretty-printing for gdb",
                        "text": "-enable-pretty-printing",
                        "ignoreFailures": True
                    }
                ]
            }
        ]
    }
    with open(launch_json_path, "w") as f:
        json.dump(launch_data, f, indent=4)



    recommendations_data = {
        "recommendations": [
            "ms-vscode.cpptools",
            "ms-vscode.cpptools-extension-pack",
            "ms-vscode.cpptools-themes",
            "twxs.cmake",
            "ms-vscode.cmake-tools",
            "emeraldwalk.RunOnSave",
            "iliazeus.vscode-ansi"
        ]
    }
    launch_json_path = os.path.join(vscode_dir, "extensions.json")
    with open(launch_json_path, "w") as f:
        json.dump(recommendations_data, f, indent=4)



    file_associations_data = {
        "files.associations": {
            "compare": "c",
            "data_types.h": "c",
            "stat.h": "c",
            "logger.h": "c",
            "*.log": "ansi"
        }
    }
    launch_json_path = os.path.join(vscode_dir, "settings.json")
    with open(launch_json_path, "w") as f:
        json.dump(file_associations_data, f, indent=4)



    # c_cpp_properties.json         Always define PLATFORM_LINUX, plus one based on build_config
    config_define = {
        "debug": "DEBUG",
        "release_with_debug_info": "RELEASE_WITH_DEBUG_INFO",
        "release": "RELEASE"
    }.get(build_config.lower(), None)
    defines = ["CIMGUI_DEFINE_ENUMS_AND_STRUCTS",
                "IMGUI_DISABLE_OBSOLETE_FUNCTIONS",
                "IMGUI_DISABLE_OBSOLETE_KEYIO",
                "PLATFORM_LINUX"]
    if config_define:
        defines.append(config_define)
    defines.append("_POSIX_C_SOURCE=200809L")

    cpp_props = {
        "version": 4,
        "configurations": [
            {
                "name": f"{build_config} ({system}-{arch})",
                "includePath": [
                    "${workspaceFolder}/**"
                ],
                "defines": defines,
                "compilerPath": "/usr/bin/gcc",
                "cStandard": "c11",
                "intelliSenseMode": "linux-gcc-x64"
            }
        ]
    }
    cpp_props_path = os.path.join(vscode_dir, "c_cpp_properties.json")
    with open(cpp_props_path, "w") as f:
        json.dump(cpp_props, f, indent=4)


def print_vscode_help():
    print("VSCode integration files generated in .vscode/")
    utils.print_c("\nVSCode usage", "blue")
    print("  Build the project:                 Ctrl+Shift+B (runs 'Clean & Build')")
    print("  Launch the application:            F5 (runs the debugger with selected config)")
