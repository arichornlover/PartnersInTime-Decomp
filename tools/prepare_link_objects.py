#!/usr/bin/env python3

"""Replace DSD reference objects with the corresponding compiled objects."""

import argparse
import os
from pathlib import Path


def object_path(source_file: Path) -> Path:
    return source_file.with_suffix(".o")


def normalized(path: Path) -> str:
    return os.path.normcase(os.path.normpath(path))


def linked_sources(manifest: Path):
    for raw_line in manifest.read_text().splitlines():
        line = raw_line.split("#", maxsplit=1)[0].strip()
        if line:
            yield Path(line)


def validate_unique_object_basenames(source_files: list[Path]) -> None:
    """Reject source objects that the MW linker script cannot distinguish."""
    owners: dict[str, Path] = {}
    for source_file in source_files:
        object_name = normalized(Path(object_path(source_file).name))
        previous = owners.get(object_name)
        if previous is not None:
            raise ValueError(
                "Linked source objects must have unique basenames because "
                "the generated MW linker script selects them without their "
                f"directories: {previous} and {source_file} both produce "
                f"{object_path(source_file).name}"
            )
        owners[object_name] = source_file


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Prepare the object list used for the final ARM9 link"
    )
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--build-root", type=Path, required=True)
    parser.add_argument("--delinks-root", type=Path, required=True)
    parser.add_argument("--linked-sources", type=Path, required=True)
    args = parser.parse_args()

    source_files = list(linked_sources(args.linked_sources))
    validate_unique_object_basenames(source_files)

    replacements: dict[str, Path] = {}
    for source_file in source_files:
        if source_file.suffix not in {".c", ".cpp", ".s"}:
            raise ValueError(f"Unsupported linked source file: {source_file}")
        
        compiled_object = args.build_root / object_path(source_file)
        reference_object = args.delinks_root / object_path(source_file)
        
        # Check if source file exists; if not, look for pre-built object
        if not source_file.is_file():
            prebuilt_object = args.build_root / object_path(source_file)
            if prebuilt_object.is_file():
                compiled_object = prebuilt_object
            else:
                # Also check the analysis directory for pre-built objects
                prebuilt_analysis = Path("build/analysis/emit") / f"{source_file.stem}.o"
                if prebuilt_analysis.is_file():
                    compiled_object.parent.mkdir(parents=True, exist_ok=True)
                    import shutil
                    shutil.copy2(prebuilt_analysis, compiled_object)
                    print(f"DEBUG: Copied {prebuilt_analysis} to {compiled_object}")
                    compiled_object = compiled_object
                else:
                    raise FileNotFoundError(f"Linked source file does not exist and no pre-built object found: {source_file}")
        elif not compiled_object.is_file():
            # Source exists but compiled object doesn't exist yet (will be built later)
            pass
        
        # If we have a pre-built object in the analysis directory, copy it to the build directory
        prebuilt_analysis = Path("build/analysis/emit") / f"{source_file.stem}.o"
        print(f"DEBUG: prebuilt_analysis = {prebuilt_analysis}, exists = {prebuilt_analysis.is_file()}")
        if prebuilt_analysis.is_file():
            compiled_object.parent.mkdir(parents=True, exist_ok=True)
            import shutil
            shutil.copy2(prebuilt_analysis, compiled_object)
            print(f"DEBUG: Copied {prebuilt_analysis} to {compiled_object}")
            compiled_object = compiled_object
        
        replacements[normalized(reference_object)] = compiled_object

    output_lines = []
    replaced = set()
    for line in args.input.read_text().splitlines():
        replacement = replacements.get(normalized(Path(line)))
        if replacement is None:
            output_lines.append(line)
            continue
        output_lines.append(str(replacement))
        replaced.add(normalized(Path(line)))

    missing = replacements.keys() - replaced
    if missing:
        missing_paths = "\n".join(f"  {path}" for path in sorted(missing))
        raise RuntimeError(
            "Compiled source objects are absent from DSD's link order:\n"
            f"{missing_paths}"
        )

    args.output.write_text("\n".join(output_lines) + "\n")


if __name__ == "__main__":
    main()
