NAME = "CONFIG_APP_REMOTE_ENABLED"
ENABLED_LINE = NAME + "=y"
DISABLED_LINE = "# " + NAME + " is not set"


def read_remote(text):
    for line in text.splitlines():
        if line.strip() == ENABLED_LINE:
            return True
        if line.strip() in (DISABLED_LINE, NAME + "=n"):
            return False
    return None


def resolve_remote(env_value, layer_texts):
    if env_value in ("0", "1"):
        return env_value == "1"
    for text in reversed(layer_texts):
        value = read_remote(text)
        if value is not None:
            return value
    return False


def apply_remote(config_text, enabled):
    target = ENABLED_LINE if enabled else DISABLED_LINE
    lines = config_text.splitlines()
    for index, line in enumerate(lines):
        if read_remote(line) is not None:
            lines[index] = target
            return "\n".join(lines) + "\n"
    return "\n".join(lines + [target]) + "\n"
