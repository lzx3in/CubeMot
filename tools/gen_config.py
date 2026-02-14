#!/usr/bin/env python3
"""
Unified KConfig configuration tool

This script provides two main functions:
1. Generate complete .config from defconfig or Kconfig defaults
2. Generate C header files from .config

Usage:
  gen_config.py generate-config <kconfig_root> <output_config> [defconfig_file]
  gen_config.py generate-headers <kconfig_root> <config_file> <output_dir>
"""

import sys
import os
import argparse
from collections import defaultdict

# Try to import Kconfiglib from virtual environment first
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '.venv', 'lib',
                               f'python{sys.version_info.major}.{sys.version_info.minor}', 'site-packages'))

try:
    import kconfiglib
except ImportError:
    print("Error: kconfiglib not found. Please install it:", file=sys.stderr)
    print("  pip install kconfiglib", file=sys.stderr)
    sys.exit(1)


# Output file mapping for header generation
OUTPUT_FILE_MAPPING = {
    'system_config': {
        'file': 'boards/system_config.h',
        'guard': 'SYSTEM_CONFIG_H',
        'comment': 'System Configuration'
    },
    'board_config': {
        'file': 'boards/board_config.h',
        'guard': 'BOARD_CONFIG_H',
        'comment': 'Board Configuration'
    },
    'driver_config': {
        'file': 'drivers/driver_config.h',
        'guard': 'DRIVER_CONFIG_H',
        'comment': 'Driver Configuration'
    },
    'app_config': {
        'file': 'application/app_config.h',
        'guard': 'APP_CONFIG_H',
        'comment': 'Application Configuration'
    }
}


def load_kconfig(kconfig_root):
    """Load Kconfig tree from root Kconfig file"""
    try:
        kconf = kconfiglib.Kconfig(kconfig_root, warn=False)
        return kconf
    except Exception as e:
        print(f"Error loading Kconfig from {kconfig_root}: {e}", file=sys.stderr)
        sys.exit(1)


def load_config(kconf, config_file):
    """Load .config file into Kconfig object"""
    try:
        kconf.load_config(config_file)
    except Exception as e:
        print(f"Error loading config file {config_file}: {e}", file=sys.stderr)
        sys.exit(1)


def cmd_generate_config(args):
    """
    Generate complete .config from defconfig or Kconfig defaults
    """
    try:
        # Load Kconfig tree
        print(f"Loading Kconfig from {args.kconfig_root}...")
        kconf = load_kconfig(args.kconfig_root)

        # Change to Kconfig directory to handle relative source paths
        kconfig_dir = os.path.dirname(os.path.abspath(args.kconfig_root))
        original_cwd = os.getcwd()
        if kconfig_dir:
            os.chdir(kconfig_dir)

        # Load defconfig if provided, otherwise use defaults
        if args.defconfig_file and os.path.exists(args.defconfig_file):
            print(f"Loading defconfig from {args.defconfig_file}...")
            # Kconfiglib automatically applies defaults for symbols not in defconfig
            kconf.load_config(os.path.join(original_cwd, args.defconfig_file))
        else:
            print("No defconfig provided, using Kconfig defaults...")

        # Change back to original directory for output
        os.chdir(original_cwd)

        # Write complete .config
        print(f"Writing complete config to {args.output_config}...")
        kconf.write_config(args.output_config)

        print(f"Successfully generated {args.output_config}")
        return True

    except Exception as e:
        print(f"Error generating config: {e}", file=sys.stderr)
        return False


def get_value_for_header(sym):
    """Convert Kconfig symbol value to appropriate C macro value"""
    if sym.type == kconfiglib.BOOL:
        return "1" if sym.tri_value == 2 else "0"
    elif sym.type == kconfiglib.TRISTATE:
        return "1" if sym.tri_value == 2 else "0" if sym.tri_value == 1 else "0"
    elif sym.type in (kconfiglib.STRING, kconfiglib.INT, kconfiglib.HEX):
        return sym.str_value
    else:
        return sym.str_value


def get_symbol_location_type(sym):
    """
    Determine which output file a symbol should go to based on its location

    Returns:
        str: Type of config ('system_config', 'board_config', 'driver_config', etc.)
               or None if the symbol should not be included
    """
    if not sym.nodes:
        return None

    # Get the filename where this symbol is defined
    filename = sym.nodes[0].filename

    # Root Kconfig file
    if filename == 'Kconfig':
        return 'system_config'

    # Subdirectory Kconfig files
    if filename.startswith('boards/'):
        return 'board_config'
    elif filename.startswith('drivers/'):
        return 'driver_config'
    elif filename.startswith('application/'):
        return 'app_config'

    # Unknown location - skip
    return None


def group_symbols_by_location(kconf):
    """
    Group all symbols by their output file location

    Returns:
        dict: Mapping of config_type to list of symbols
    """
    grouped_symbols = defaultdict(list)

    for sym in kconf.unique_defined_syms:
        location_type = get_symbol_location_type(sym)
        if location_type:
            grouped_symbols[location_type].append(sym)

    return grouped_symbols


def generate_header_file(kconf, symbols, config_type, output_dir):
    """
    Generate a configuration header file for a group of symbols

    Args:
        kconf: Kconfig object
        symbols: List of symbols to include in this header
        config_type: Type of config (e.g., 'board_config', 'driver_config')
        output_dir: Output directory for the header file

    Returns:
        str: Path to generated file, or None if no symbols
    """
    if not symbols:
        return None

    config_info = OUTPUT_FILE_MAPPING[config_type]
    output_file = os.path.join(output_dir, config_info['file'])
    guard = config_info['guard']
    comment = config_info['comment']

    # Ensure output directory exists
    os.makedirs(os.path.dirname(output_file), exist_ok=True)

    # Generate header file
    with open(output_file, 'w') as f:
        # Write header
        f.write(f"/* Auto-generated by gen_config.py for {comment} - DO NOT EDIT */\n\n")
        f.write(f"#ifndef {guard}\n")
        f.write(f"#define {guard}\n\n")

        f.write(f"/* {comment} */\n\n")

        # Group symbols by menu for better readability
        symbols_by_menu = {}
        for sym in symbols:
            # Try to find menu path
            menu_path = []
            node = sym.nodes[0]
            while node.parent:
                if node.parent.is_menuconfig:
                    menu_prompt = node.parent.prompt[0] if node.parent.prompt else "Other"
                    menu_path.insert(0, menu_prompt)
                node = node.parent

            menu_key = "/".join(menu_path) if menu_path else "Other"
            if menu_key not in symbols_by_menu:
                symbols_by_menu[menu_key] = []
            symbols_by_menu[menu_key].append(sym)

        # Write symbols grouped by menu
        for menu_name, menu_symbols in symbols_by_menu.items():
            if menu_name != "Other":
                f.write(f"/* {menu_name} */\n")

            for sym in menu_symbols:
                value = get_value_for_header(sym)
                macro_name = sym.name

                # Add comment with help text if available
                if sym.nodes and sym.nodes[0].help:
                    help_lines = sym.nodes[0].help.split('\n')
                    for line in help_lines:
                        if line.strip():
                            f.write(f"/* {line.strip()} */\n")

                # Write the macro definition
                if sym.type == kconfiglib.STRING:
                    f.write(f"#define {macro_name} \"{value}\"\n")
                else:
                    f.write(f"#define {macro_name}  {value}\n")

                f.write("\n")

            f.write("\n")

        # Add BOARD_LED_COUNT for board_config.h specifically
        if config_type == 'board_config':
            # Calculate LED count
            led_count = 0
            if kconf.syms.get('BOARD_HAS_LED1') and kconf.syms.get('BOARD_HAS_LED1').tri_value == 2:
                led_count += 1
            if kconf.syms.get('BOARD_HAS_LED2') and kconf.syms.get('BOARD_HAS_LED2').tri_value == 2:
                led_count += 1
            if kconf.syms.get('BOARD_HAS_LED3') and kconf.syms.get('BOARD_HAS_LED3').tri_value == 2:
                led_count += 1

            f.write(f"\n#define BOARD_LED_COUNT {led_count}\n")

        # Write footer
        f.write(f"#endif /* {guard} */\n")

    print(f"Generated: {output_file}")
    return output_file


def cmd_generate_headers(args):
    """
    Generate C header files from .config
    """
    try:
        # Change to the directory containing Kconfig root to handle relative source paths
        kconfig_dir = os.path.dirname(os.path.abspath(args.kconfig_root))
        if not kconfig_dir:
            kconfig_dir = os.getcwd()

        original_cwd = os.getcwd()
        os.chdir(kconfig_dir)

        # Load Kconfig tree
        print(f"Loading Kconfig from {args.kconfig_root}...")
        kconf = load_kconfig(os.path.basename(args.kconfig_root))

        # Load .config file
        print(f"Loading config from {args.config_file}...")
        load_config(kconf, os.path.join(original_cwd, args.config_file))

        # Group symbols by their location
        print("Grouping symbols by location...")
        grouped_symbols = group_symbols_by_location(kconf)

        # Generate header files for each group
        print("Generating configuration headers...")
        generated_files = []

        for config_type, symbols in sorted(grouped_symbols.items()):
            if symbols:
                result = generate_header_file(kconf, symbols, config_type, os.path.join(original_cwd, args.output_dir))
                if result:
                    generated_files.append(result)

        # Change back to original directory
        os.chdir(original_cwd)

        print(f"\nGenerated {len(generated_files)} header file(s)")
        for f in generated_files:
            print(f"  - {f}")

        return True

    except Exception as e:
        print(f"Error generating headers: {e}", file=sys.stderr)
        return False


def main():
    parser = argparse.ArgumentParser(
        description='Unified KConfig configuration tool',
        epilog="Commands:\n"
               "  generate-config   Generate .config from defconfig or defaults\n"
               "  generate-headers  Generate C headers from .config",
        formatter_class=argparse.RawDescriptionHelpFormatter
    )

    subparsers = parser.add_subparsers(dest='command', help='Command to execute')

    # generate-config subcommand
    gen_config_parser = subparsers.add_parser('generate-config', help='Generate .config file')
    gen_config_parser.add_argument('kconfig_root', help='Root Kconfig file')
    gen_config_parser.add_argument('output_config', help='Output .config file')
    gen_config_parser.add_argument('defconfig_file', nargs='?', default=None,
                                   help='Input defconfig file (optional)')

    # generate-headers subcommand
    gen_headers_parser = subparsers.add_parser('generate-headers', help='Generate C header files')
    gen_headers_parser.add_argument('kconfig_root', help='Root Kconfig file')
    gen_headers_parser.add_argument('config_file', help='Input .config file')
    gen_headers_parser.add_argument('output_dir', help='Output directory for generated headers')

    args = parser.parse_args()

    if not args.command:
        parser.print_help()
        sys.exit(1)

    # Execute the appropriate command
    if args.command == 'generate-config':
        success = cmd_generate_config(args)
    elif args.command == 'generate-headers':
        success = cmd_generate_headers(args)
    else:
        print(f"Unknown command: {args.command}", file=sys.stderr)
        sys.exit(1)

    sys.exit(0 if success else 1)


if __name__ == '__main__':
    main()
