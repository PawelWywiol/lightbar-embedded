import os
import sys

Import("env")


def read_text(path):
    if not os.path.exists(path):
        return ""
    with open(path) as config:
        return config.read()


def configure(project_dir):
    sys.path.insert(0, os.path.join(project_dir, "scripts"))
    from remote_selection import apply_remote, resolve_remote

    env_value = os.environ.get("REMOTE")
    if env_value not in (None, "0", "1"):
        sys.exit("REMOTE must be 0 or 1")

    args = env.GetProjectOption("board_build.cmake_extra_args")
    layers = args.split("=", 1)[1].strip('"').split(";")
    enabled = resolve_remote(env_value, [read_text(os.path.join(project_dir, layer)) for layer in layers])

    generated = os.path.join(project_dir, "sdkconfig." + env["PIOENV"])
    current = read_text(generated)
    updated = apply_remote(current, enabled)
    if updated != current:
        with open(generated, "w") as config:
            config.write(updated)

    print("Remote %s" % ("enabled" if enabled else "disabled"))


configure(env.subst("$PROJECT_DIR"))
