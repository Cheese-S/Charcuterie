import argparse
import cmake
import util
from pathlib import Path
import shutil
import os
from typing import NamedTuple
from enum import Enum

from collections.abc import Callable

DEFAULT_LOG_CATEGORY = "cc"

PROJECT_ROOT_DIRS = ["milk", "swiss"]


class Config(Enum):
    eDebug = "debug"
    eRelease = "release"


class BuildOption(NamedTuple):
    config: Config
    verbose: bool
    regen: bool
    target: str
    jobs: int


class RunOption(NamedTuple):
    config: Config
    target: str


class CleanOption(NamedTuple):
    debug: bool


class TestOpiton(NamedTuple):
    all: bool
    target: str


class SwitchOption(NamedTuple):
    config: str


def getBuildConfigs():
    return [Config.eDebug, Config.eRelease]


def getBuildConfigsStr():
    return [Config.eDebug.value, Config.eRelease.value]


def configToBuildDir(config: Config) -> Path:
    return Path(".") / "build" / config.value


def configToBinDir(config: Config) -> Path:
    return Path(".") / "build" / config.value / "bin"


def logInfo(msg: str):
    util.logCategoryInfo(DEFAULT_LOG_CATEGORY, msg)


def logDebug(msg: str):
    util.logCategoryDebug(DEFAULT_LOG_CATEGORY, msg)


def logError(msg: str):
    util.logCategoryError(DEFAULT_LOG_CATEGORY, msg)


def runCmd(cmd: str, prefix: str = ""):
    util.runCmd(cmd, DEFAULT_LOG_CATEGORY, prefix)


def regen():
    files = getCMakeFiles()
    for file in files:
        genCMakeFile(file)
    genCompileCmds()
    moveCompileCmds()


def getCMakeFiles() -> list[Path]:
    paths = []
    util.getFilesAtDirRecurisve(
        Path("."),
        '/',
        paths,
        lambda filename: filename == "CMakeLists.txt",
        lambda dir: any(x in dir for x in PROJECT_ROOT_DIRS),
    )

    return [Path(x) for x in paths]


# Report proper errors when missing args for gen
# Possibly just rewrite this
def genCMakeFile(file: Path):
    logInfo(f"Generating cmake file: {file}")

    originalDir = os.getcwd()

    os.chdir(file.parent)
    output = []
    foundStub = False
    with open("CMakeLists.txt") as f:
        lines = f.readlines()
        for line in lines:
            if "END_GEN" in line:
                assert foundStub, f"END_GEN does not have a corresponding BEGIN_GEN before it.{output[-1]}"
                foundStub = False
                output.append(line)
            elif "BEGIN_FILE_GEN" in line:
                assert not foundStub, "Nested stubs is not supported"
                output.append(f"{line}\n")
                foundStub = True
                dir = getGenFileDir(Path("."), line)
                if (dir == None):
                    logError(
                        "Invalid BEGIN_FILE_GEN parameters, expected: \"# BEGIN_FILE_GEN {dir} \""
                    )
                    return
                genFile(output, dir)
            elif "BEGIN_TEST_GEN" in line:
                assert not foundStub, "Nested stubs is not supported"
                output.append(f"{line}\n")
                dir, libs = getGenTestParams(Path("."), line)
                foundStub = True
                if (dir == None or libs == None):
                    logError(
                        "Invalid BEGIN_TEST_GEN parameters, expected \"# BEGIN_TEST_GEN {dir} {lib1} {lib2} ...\""
                    )
                    return
                genTest(output, dir, libs)
            elif foundStub:
                continue
            else:
                output.append(line)

    with open("CMakeLists.txt", "w+") as f:
        f.writelines(output)

    os.chdir(originalDir)


def genFile(dst: list[str], dir: Path):
    files = util.getFileAtDirNonRecursive(dir)
    for file in files:
        dst.append(f"\t{file}\n")
    dst.append("\n")


def getGenFileDir(root: Path, line: str) -> Path | None:
    # Expect to be
    # #               [0]
    # BEGIN_FILE_GEN  [1]
    # dirPath         [2]
    params = line.strip().split(" ")
    if (len(params) < 3):
        return None

    return root / params[2]


def getGenTestParams(root: Path,
                     line: str) -> tuple[Path | None, list[str] | None]:
    # Expect to be
    # #                  [0]
    # BEGIN_FILE_GEN     [1]
    # dirPath            [2]
    # link to libraries  [3...]
    params = line.strip().split(" ")
    if (len(params) < 3):
        return None, None

    dirPath = root / params[2]
    libs = []
    if (len(params) >= 4):
        libs = params[3:]

    return dirPath, libs


def genTest(dst: list[str], dir: Path, includeLibs: list[str]):
    files = []
    util.getFilesAtDirRecurisve(dir, "/", files,
                                lambda filename: ".cpp" in filename,
                                lambda dirName: "test" in dirName)
    includeLibsStr = " ".join(includeLibs)
    for file in files:
        testExe = f"{file.split('/')[-1][:-4]}"
        dst.append(f"""
add_test({testExe} 
         SOURCES {file} 
         INCLUDES include interface milk
         LIBS {includeLibsStr} milk_test)
""")


# add_executable(
#     {testExe}
#     {file}
# )
#
# target_compile_options({testExe} PUBLIC ${{cc_compile_options}})
# target_link_libraries({testExe} PUBLIC {includeLibsStr})
# target_include_directories({testExe} PRIVATE milk/core/include milk/test/interface)


def genCompileCmds():
    logInfo(
        "-------------------- GENERATING FOR CLANG CL DEBUG BUILD ——————————————————————"
    )
    runCmd(
        f"cmake -G Ninja -B .\\{configToBuildDir(Config.eDebug)} -DCMAKE_BUILD_TYPE=debug -DCMAKE_C_COMPILER_FRONTEND_VARIANT=MSVC -DCMAKE_CXX_COMPILER_FRONTEND_VARIANT=MSVC --toolchain .\\third_party\\cmake_toolchain\\Windows.Clang.toolchain.cmake"
    )
    logInfo(
        "-------------------- GENERATING FOR CLANG CL RELEASE BUILD ——————————————————————"
    )
    runCmd(
        f"cmake -G Ninja -B .\\{configToBuildDir(Config.eRelease)} -DCMAKE_BUILD_TYPE=release -DCMAKE_C_COMPILER_FRONTEND_VARIANT=MSVC -DCMAKE_CXX_COMPILER_FRONTEND_VARIANT=MSVC --toolchain .\\third_party\\cmake_toolchain\\Windows.Clang.toolchain.cmake"
    )
    logInfo(
        "-------------------- GENERATING FOR COMPILE COMMAND DEBUG BUILD ——————————————————————"
    )
    runCmd(
        f"cmake -G Ninja -B .\\build\\compile_cmds\\debug -DCMAKE_BUILD_TYPE=debug --toolchain .\\third_party\\cmake_toolchain\\Windows.Clang.toolchain.cmake"
    )
    # CC_LOG_INFO(
    #     "-------------------- GENERATING FOR VK NINJA ——————————————————————")
    # os.system(f"cmake -G Ninja -B .\\build\\ninja_vk -DCMAKE_BUILD_TYPE=debug")
    #


def getCpuCount() -> int:
    cpuCount = os.cpu_count()
    if (cpuCount == None):
        cpuCount = 8
    return cpuCount


def fuzzySelectTarget(config: Config,
                      target: str,
                      filter: Callable[[str], bool] | None = None
                      ) -> tuple[str, Path] | None:
    targets = cmake.getCMakeTargets(configToBuildDir(config), filter)
    selection = util.findTargetWithFzf(targets, target)
    return selection


def buildTarget(config: Config, target: str, jobs: int, verbose: bool):

    verboseFlag = "--verbose" if verbose else ""
    runCmd(
        f"cmake --build {configToBuildDir(config)} {verboseFlag} -j {jobs} -t {target}",
        "Building: ")


def build(opt: BuildOption):
    selection = fuzzySelectTarget(opt.config, opt.target)
    if selection == None:
        logError(f"Failed to find a build target named {opt.target}")
        return

    target, _ = selection

    buildDir = configToBuildDir(opt.config)
    if (opt.regen or not os.path.exists(f"{buildDir}")):
        regen()

    buildTarget(opt.config, target, opt.jobs, opt.verbose)


def run(opt: RunOption):
    selection = fuzzySelectTarget(opt.config, opt.target)
    if selection == None:
        logError(f"Unable to find a target named {opt.target}.")
        return

    target, path = selection

    buildTarget(opt.config, target, getCpuCount(), False)

    runCmd(f"{path}", "Running App: ")


def clean(opt: CleanOption):
    buildConfigDir = "debug" if opt.debug else "release"
    os.system(f"cmake --build .\\build\\{buildConfigDir} --target clean")


def isTestExe(filename: str):
    return "Test" in filename


def test(opt: TestOpiton):
    selection = fuzzySelectTarget(Config.eDebug, opt.target,
                                  lambda name: "Test" in name)
    if (selection == None):
        logError(f"Unable to find a target named {opt.target}")
        return

    target, path = selection

    buildTarget(Config.eDebug, target, getCpuCount(), False)

    runCmd(f"{path}", "Testing: ")


def switch(opt: SwitchOption):
    root = Path(".")
    moveCompileCmds()

    lines = []
    with open(root / ".clangd") as f:
        lines = f.readlines()
        if opt.config == "dx":
            lines[2] = "   Add: [-DCC_D3D12]\n"
        else:
            lines[2] = "   Add: [-DCC_VULKAN]\n"

    with open(root / ".clangd", "w+") as f:
        f.writelines(lines[:3])


def moveCompileCmds():
    root = Path(".")
    shutil.copyfile(
        root / "build" / "compile_cmds" / "debug" / "compile_commands.json",
        root / "compile_commands.json")


def addArgParser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="command", required=True)

    # ------------------------------ regen -----------------------------
    regenParser = subparsers.add_parser(
        "regen", description="regenerate CMakeLists.txt and run cmake -G")

    # ------------------------------ build -----------------------------
    buildParser = subparsers.add_parser(
        "build", description="build the project along with the tests")
    buildParser.add_argument("--asan", action="store_true", help="enable asan")
    buildParser.add_argument("-c",
                             "--config",
                             type=Config,
                             default=Config.eDebug,
                             help=f"supported configs: {getBuildConfigsStr()}")
    buildParser.add_argument("-v",
                             "--verbose",
                             action="store_true",
                             help="verbose build")
    buildParser.add_argument("-r",
                             "--regen",
                             action="store_true",
                             help="invoke regen as well")
    buildParser.add_argument("-t",
                             "--target",
                             type=str,
                             default="all",
                             help="choose which target to build")
    buildParser.add_argument("-j",
                             "--jobs",
                             type=int,
                             default=os.cpu_count(),
                             help="number of threads to run the build process")
    # ----------------------------- clean -------------------------------
    cleanParser = subparsers.add_parser(
        "clean", description="clean all the build artifacts")
    cleanParser.add_argument("-c",
                             "--config",
                             type=Config,
                             help=f"supported config: {getBuildConfigsStr()}")

    # ------------------------------ test --------------------------------
    testParser = subparsers.add_parser("test", description="run tests")
    testParser.add_argument("-a",
                            "--all",
                            action="store_true",
                            help="run all tests")
    testParser.add_argument("-t",
                            "--target",
                            type=str,
                            default="",
                            help="run specific test")

    # ------------------------------ run --------------------------------
    runParser = subparsers.add_parser("run", description="run cc.exe")
    runParser.add_argument("-c",
                           "--config",
                           type=Config,
                           required=True,
                           help=f"supported config: {getBuildConfigsStr()}")
    runParser.add_argument("-t",
                           "--target",
                           type=str,
                           required=True,
                           help=f"select which target to run")
    # ------------------------------ switch -----------------------------
    switchParser = subparsers.add_parser(
        "switch", description="switch which clangd config currently using")
    switchParser.add_argument("-c",
                              "--config",
                              type=str,
                              help="choose which clangd config to use.",
                              choices=["dx", "vk"])

    subparsers.add_parser("dev")

    return parser


if __name__ == "__main__":
    parser = addArgParser()
    args = parser.parse_args()

    match args.command:
        case "regen":
            regen()
        case "build":
            build(
                BuildOption(args.config, args.verbose, args.regen, args.target,
                            args.jobs))
        case "run":
            run(RunOption(args.config, args.target))
        case "test":
            test(TestOpiton(args.all, args.target))
        case "clean":
            clean(CleanOption(args.config))
        case "switch":
            switch(SwitchOption(args.config))
        case "dev":
            print("")
