#!/usr/bin/env python3
"""
STM32CubeMX post-generation script
Reorganizes generated code files to proper project locations
"""

import os
import sys
import shutil
from dataclasses import dataclass
from pathlib import Path
from typing import List, Final, Iterator


# Configuration constants
ARCH_CHIP: Final[str] = "arm-cortex-m4f-stm32g4"
DRIVERS_DIR_NAME: Final[str] = "drivers"
PROJECT_ROOT_OFFSET: Final[str] = "../../.."  # Relative path offset from script directory to project root


@dataclass(frozen=True)
class PostGenerationConfig:
    """Configuration for post-generation file movements"""
    script_dir: Path
    arch_chip: str
    mcu_family: str
    
    @classmethod
    def create(cls, script_dir: Path, arch_chip: str) -> 'PostGenerationConfig':
        """Factory method to create config with MCU family detection"""
        mcu_family = cls._extract_mcu_family(script_dir)
        return cls(script_dir, arch_chip, mcu_family)
    
    @staticmethod
    def _extract_mcu_family(script_dir: Path) -> str:
        """Extract MCU family from CubeMX .ioc file"""
        print("Detecting MCU family...")
        ioc_file = script_dir / "stm32cubemx_generated.ioc"
        
        if not ioc_file.exists():
            raise FileNotFoundError(f"IOC file not found: {ioc_file}")
        
        # Use pathlib's read_text and generator expression for better performance
        content = ioc_file.read_text(encoding='utf-8')
        family_line = next(
            (line for line in content.splitlines() if line.startswith("Mcu.Family=")),
            None
        )
        
        if family_line is None:
            raise ValueError("Cannot extract MCU family from .ioc file")
        
        family = family_line.split('=', 1)[1].strip()
        print(f"Detected MCU family: {family}")
        return family


def move_directory(src_dir: Path, dst_dir: Path, item_name: str) -> bool:
    """
    Move a directory from source to destination
    
    Args:
        src_dir: Source directory path
        dst_dir: Destination directory path
        item_name: Descriptive name of the item being moved (for logging)
    
    Returns:
        True if successful, False otherwise
    """
    if not src_dir.exists():
        print(f"Warning: Source not found: {src_dir}")
        return False
    
    if dst_dir.exists():
        print(f"Cleaning old {item_name}...")
        shutil.rmtree(dst_dir)
    
    # Ensure parent directory exists
    dst_dir.parent.mkdir(parents=True, exist_ok=True)
    
    print(f"Moving {item_name}...")
    print(f"  Source: {src_dir}")
    print(f"  Target: {dst_dir}")
    
    try:
        shutil.move(src_dir, dst_dir)
        return True
    except Exception as e:
        print(f"Error: Failed to move {item_name} - {e}")
        return False


def move_hal_driver(config: PostGenerationConfig) -> bool:
    """Move HAL driver to the correct project location"""
    driver_name = f"{config.mcu_family}xx_HAL_Driver"
    
    src_dir = config.script_dir / "Drivers" / driver_name
    dst_dir = (config.script_dir / PROJECT_ROOT_OFFSET / "platform" / DRIVERS_DIR_NAME / 
               config.arch_chip / driver_name).resolve()
    
    return move_directory(src_dir, dst_dir, driver_name)


def main() -> int:
    """Main entry point"""
    script_dir = Path(__file__).parent
    
    try:
        config = PostGenerationConfig.create(script_dir, ARCH_CHIP)
    except (FileNotFoundError, ValueError) as e:
        print(f"Error: {e}")
        return 1
    
    # Use tuple instead of list for immutable operations collection
    operations = (
        move_hal_driver(config),
    )
    
    print("\n" + "="*60)
    success_count = sum(operations)
    total_operations = len(operations)
    
    # Use f-string with conditional expression for concise output
    status_msg = (
        f"SUCCESS: All {total_operations} operations completed successfully!"
        if success_count == total_operations
        else f"FAILED: {success_count}/{total_operations} operations succeeded"
    )
    print(status_msg)
    print("="*60)
    
    return 0 if success_count == total_operations else 1


if __name__ == "__main__":
    sys.exit(main())
