import os
import shutil
import sys

Import("env")

FS_TARGETS = {"buildfs", "uploadfs", "uploadfsota"}


def read_lights_count(path):
    if not os.path.exists(path):
        return None
    with open(path) as config:
        for line in config:
            if line.startswith("CONFIG_APP_LIGHTS_COUNT="):
                return int(line.split("=")[1])
    return None


def resolve_lights_count(project_dir):
    generated = read_lights_count(os.path.join(project_dir, "sdkconfig." + env["PIOENV"]))
    if generated is not None:
        return generated

    args = env.GetProjectOption("board_build.cmake_extra_args")
    layers = args.split("=", 1)[1].strip('"').split(";")
    for layer in reversed(layers):
        count = read_lights_count(os.path.join(project_dir, layer))
        if count is not None:
            return count
    sys.exit("CONFIG_APP_LIGHTS_COUNT not found for " + env["PIOENV"])


def stage(project_dir):
    sys.path.insert(0, os.path.join(project_dir, "scripts"))
    from presets_selection import parse_scheme, select_presets

    source = os.path.join(project_dir, "presets")
    schemes = {}
    for file_name in os.listdir(source):
        with open(os.path.join(source, file_name), "rb") as scheme:
            schemes[file_name] = parse_scheme(scheme.read())

    lights_count = resolve_lights_count(project_dir)
    selected = select_presets(schemes, lights_count)

    staged = os.path.join(env.subst("$BUILD_DIR"), "fs_data")
    shutil.rmtree(staged, ignore_errors=True)
    shutil.copytree(env.subst("$PROJECT_DATA_DIR"), staged, ignore=shutil.ignore_patterns(".DS_Store"))
    target = os.path.join(staged, "presets")
    os.makedirs(target)
    for file_name in selected:
        shutil.copy(os.path.join(source, file_name), target)

    env.Replace(PROJECT_DATA_DIR=staged)
    print("Staged %d presets for %d lights" % (len(selected), lights_count))


if FS_TARGETS & set(COMMAND_LINE_TARGETS):
    stage(env.subst("$PROJECT_DIR"))
