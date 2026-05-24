import json
import os
from pathlib import Path
from collections.abc import Callable


def getCMakeTargets(
        buildDir: str | Path,
        filter: Callable[[str], bool] | None = None) -> dict[str, Path]:
    buildDir = Path(buildDir).resolve()
    replyDir = buildDir / ".cmake" / "api" / "v1" / "reply"

    if not replyDir.exists():
        raise FileNotFoundError(
            f"CMake reply directory missing at: {replyDir}")

    # Find the latest index file
    indexFiles = list(replyDir.glob("index-*.json"))
    if not indexFiles:
        raise RuntimeError("No CMake index file found.")
    indexFile = max(indexFiles, key=os.path.getmtime)

    with open(indexFile, "r") as f:
        indexData = json.load(f)

    # Locate and read the codemodel file
    codemodelName = indexData["reply"]["codemodel-v2"]["jsonFile"]
    with open(replyDir / codemodelName, "r") as f:
        codemodelData = json.load(f)

    targets = {}
    config = codemodelData["configurations"][0]

    # Extract path information for every executable target
    for targetRef in config["targets"]:
        with open(replyDir / targetRef["jsonFile"], "r") as f:
            targetData = json.load(f)

        if targetData.get("type") == "EXECUTABLE":
            name = targetData["name"]
            if filter != None and not filter(name):
                continue

            nameOnDisk = targetData.get("nameOnDisk", "")

            for artifact in targetData.get("artifacts", []):
                pathStr = artifact.get("path", "")
                if pathStr.endswith(nameOnDisk):
                    artifactPath = Path(pathStr)

                    # Convert relative artifact paths to absolute ones
                    if not artifactPath.is_absolute():
                        artifactPath = buildDir / artifactPath

                    targets[name] = artifactPath.resolve()

    return targets
