# https://community.platformio.org/t/solved-exclude-directory-from-imported-external-library/10052/7
from pathlib import Path

Import("env")


def skip_from_build(env, node):
    """
    `node.name` - a name of File System Node
    `node.get_path()` - a relative path
    `node.get_abspath()` - an absolute path
     to ignore file from a build process, just return None
    """
    ignore_list = [
        "littlefs/bd",
        "littlefs/runners",
        "srxecore/main.c",
        "srxecore/smoketest.h",
        # "u8g2/cppsrc",
        # "u8g2/sys",
        # "u8g2/tools",
    ]
    for ignore in ignore_list:
        if ignore in Path(node.get_path()).as_posix():
            # Return None for exclude
            return None
    return node


# Register callback
env.AddBuildMiddleware(skip_from_build)
