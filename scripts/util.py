from enum import Enum
from datetime import datetime
from pathlib import Path
import os
from collections.abc import Callable
import subprocess
import sys


class LogType(Enum):
    eInfo = "I"
    eDebug = "D"
    eWarning = "W"
    eError = "E"


class AnsiColor(Enum):
    eReset = "\033[0m"
    eBlack = "\033[30m"
    eRed = "\033[31m"
    eGreen = "\033[32m"
    eYellow = "\033[33m"
    eBlue = "\033[34m"
    ePurple = "\033[35m"
    eCyan = "\033[36m"
    eWhite = "\033[37m"


def logInternal(type: LogType, tag: str, msg: str, color: AnsiColor):
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(
        f"{color.value}{timestamp} {type.value} [{tag}] : {msg}{AnsiColor.eReset.value}"
    )


def logCategoryInfo(category: str, msg: str):
    logInternal(LogType.eInfo, category, msg, AnsiColor.eGreen)


def logCategoryWarn(category: str, msg: str):
    logInternal(LogType.eWarning, category, msg, AnsiColor.eYellow)


def logCategoryDebug(category: str, msg: str):
    logInternal(LogType.eDebug, category, msg, AnsiColor.eBlue)


def logCategoryError(category: str, msg: str):
    logInternal(LogType.eError, category, msg, AnsiColor.eRed)


def runCmd(cmd: str, tag: str, prefix: str = ""):
    logCategoryInfo(tag, f"{prefix} {cmd}")
    os.system(cmd)


# FILE UTILS


def getFileAtDirNonRecursive(dir: Path) -> list[str]:
    paths = []
    for item in os.listdir(dir):
        if Path.is_file(dir / item):
            paths.append(str(dir / item).replace(os.sep, "/"))
    return paths


def isValidInput(filter: Callable | None, input: str):
    return filter == None or filter(input)


def getFilesWithSuffix(
        dir: Path,
        suffix: str,
        sep: str,
        filenameFilter: Callable[[str], bool] | None = None) -> list[str]:
    paths = []
    for root, _, files in os.walk(dir, topdown=True):
        for file in files:
            path = Path(root) / file
            if path.suffix == suffix and isValidInput(filenameFilter,
                                                      path.name):
                paths.append(str(path).replace(os.sep, sep))
    return paths


def getFilesAtDirRecurisve(
    rootDir: Path,
    sep: str,
    outPaths: list[str],
    fileFilter: Callable[[str], bool] | None = None,
    dirFilter: Callable[[str], bool] | None = None,
):
    for root, dirs, files in os.walk(rootDir, topdown=True):
        validDirs = []
        for dir in dirs:
            dirPath = Path(root) / dir
            if isValidInput(dirFilter, str(dirPath)):
                validDirs.append(dir)
        dirs[:] = validDirs
        if isValidInput(dirFilter, str(root)):
            for file in files:
                path = Path(root) / file
                if isValidInput(fileFilter, path.name):
                    outPaths.append(str(path).replace(os.sep, sep))


# FZF


def findTargetWithFzf(targets: dict[str, Path],
                      searchQuery: str = "") -> tuple[str, Path] | None:
    if not targets:
        print("No targets available.", file=sys.stderr)
        return None

    inputData = "\n".join(targets.keys())
    fzfCmd = ["fzf", "--height=40%", "--reverse", "--prompt=Select Target > "]

    if searchQuery:
        fzfCmd.extend(["-q", searchQuery])
        fzfCmd.extend(["-1"])

    try:
        process = subprocess.run(fzfCmd,
                                 input=inputData,
                                 text=True,
                                 capture_output=True,
                                 check=True)

        selectedName = process.stdout.strip()
        if selectedName in targets:
            return selectedName, targets[selectedName]

    except subprocess.CalledProcessError as e:
        if e.returncode == 130:
            print("\nSelection cancelled.", file=sys.stderr)
        else:
            print(f"fzf error: {e.stderr}", file=sys.stderr)

    return None
