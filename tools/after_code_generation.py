#!/usr/bin/env python3
"""
STM32CubeMX post-generation script
Reorganizes generated code files to proper project locations
"""

import sys
import shutil
from pathlib import Path
from typing import Final

# Relative path offset from script directory to project root
PROJECT_ROOT_OFFSET: Final[str] = ".."


def move_path(src: Path, dst: Path, item_name: str) -> bool:
    """
    Move a file or directory from source to destination

    Args:
        src: Source path
        dst: Destination path
        item_name: Descriptive name of the item being moved (for logging)

    Returns:
        True if successful, False otherwise
    """
    if not src.exists():
        print(f"Warning: Source not found: {src}")
        return False

    # Ensure parent directory exists
    dst.parent.mkdir(parents=True, exist_ok=True)

    if dst.exists():
        print(f"Cleaning old {item_name}...")
        if dst.is_dir():
            shutil.rmtree(dst)
        else:
            dst.unlink()

    print(f"Moving {item_name}...")
    print(f"  Source: {src}")
    print(f"  Target: {dst}")

    try:
        shutil.move(src, dst)
        return True
    except Exception as e:
        print(f"Error: Failed to move {item_name} - {e}")
        return False


def move_startup_file(script_dir: Path) -> bool:
    """Move startup file to the correct project location"""
    startup_filename = "startup_stm32g431xx.s"

    board_dir = (
        script_dir / PROJECT_ROOT_OFFSET / "src" / "boards" / "nucleo_g431rb"
    ).resolve()
    src_file = board_dir / "stm32cubemx_generated" / startup_filename
    dst_file = board_dir / "startup" / startup_filename

    return move_path(src_file, dst_file, startup_filename)


def execute_operations(script_dir: Path) -> tuple[int, int]:
    """Execute all post-generation operations, return (success, total)"""
    operations = (move_startup_file(script_dir),)
    return sum(operations), len(operations)


def print_summary(success_count: int, total: int) -> None:
    """Print execution summary"""
    print("\n" + "=" * 60)
    if success_count == total:
        print(f"SUCCESS: All {total} operations completed successfully!")
    else:
        print(f"FAILED: {success_count}/{total} operations succeeded")
    print("=" * 60)


def main() -> int:
    script_dir = Path(__file__).parent

    success_count, total = execute_operations(script_dir)
    print_summary(success_count, total)

    return 0 if success_count == total else 1


if __name__ == "__main__":
    sys.exit(main())
