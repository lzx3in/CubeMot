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

import os
import sys
import argparse
from collections import defaultdict

import kconfiglib
from kconfiglib import Kconfig


# Output file mapping for header generation
OUTPUT_FILE_MAPPING = {
    'system_config': {
        'file': 'src/boards/include/boards/system_config.h',
        'guard': 'SYSTEM_CONFIG_H',
        'comment': 'System Configuration'
    },
    'board_config': {
        'file': 'src/boards/include/boards/board_config.h',
        'guard': 'BOARD_CONFIG_H',
        'comment': 'Board Configuration'
    },
    'driver_config': {
        'file': 'src/drivers/driver_config.h',
        'guard': 'DRIVER_CONFIG_H',
        'comment': 'Driver Configuration'
    },
    'app_config': {
        'file': 'src/application/app_config.h',
        'guard': 'APP_CONFIG_H',
        'comment': 'Application Configuration'
    }
}


def load_kconfig(kconfig_root):
    """Load Kconfig tree from root Kconfig file"""
    try:
        return Kconfig(kconfig_root, warn=False)
    except Exception as e:
        print(f"Error loading Kconfig from {kconfig_root}: {e}", file=sys.stderr)
        sys.exit(1)


def find_defconfig(board, search_path):
    """Find defconfig file based on board and search path

    Priority: board-specific defconfig > global defconfig > None
    """
    if not search_path:
        return None

    # Board-specific defconfig
    if board:
        board_defconfig = os.path.join(search_path, board, 'defconfig')
        if os.path.exists(board_defconfig):
            print(f"Found board defconfig: {board_defconfig}")
            return board_defconfig

    # Global defconfig
    global_defconfig = os.path.join(search_path, 'defconfig')
    if os.path.exists(global_defconfig):
        print(f"Found global defconfig: {global_defconfig}")
        return global_defconfig

    return None


def cmd_generate_config(args):
    """Generate complete .config from defconfig or Kconfig defaults"""
    try:
        kconfig_dir = os.path.dirname(os.path.abspath(args.kconfig_root)) or os.getcwd()
        original_cwd = os.getcwd()

        # Change to Kconfig directory to handle relative source paths
        os.chdir(kconfig_dir)

        print(f"Loading Kconfig from {args.kconfig_root}...")
        kconf = load_kconfig(os.path.basename(args.kconfig_root))

        # Determine defconfig to load
        defconfig_path = None
        if args.defconfig_file:
            # Explicitly specified defconfig file
            if os.path.exists(args.defconfig_file):
                defconfig_path = args.defconfig_file
            else:
                print(f"Warning: Specified defconfig not found: {args.defconfig_file}", file=sys.stderr)
        elif args.search_path:
            # Auto-find defconfig based on board and search path
            defconfig_path = find_defconfig(args.board, args.search_path)

        # Load defconfig if found
        if defconfig_path:
            print(f"Loading defconfig from {defconfig_path}...")
            kconf.load_config(os.path.join(original_cwd, defconfig_path))
        else:
            print("Using Kconfig defaults...")

        os.chdir(original_cwd)

        print(f"Writing complete config to {args.output_config}...")
        kconf.write_config(args.output_config)

        print(f"Successfully generated {args.output_config}")
        return True

    except Exception as e:
        print(f"Error generating config: {e}", file=sys.stderr)
        return False


def get_value_for_header(sym):
    """Convert Kconfig symbol value to appropriate C macro value"""
    if sym.type in (kconfiglib.BOOL, kconfiglib.TRISTATE):
        return "1" if sym.tri_value == 2 else "0"
    return sym.str_value


# Mapping of Kconfig file prefixes to config types
LOCATION_MAP = [
    ('src/boards/', 'board_config'),
    ('src/drivers/', 'driver_config'),
    ('src/application/', 'app_config'),
]

def get_symbol_location_type(sym):
    """Determine which output file a symbol should go to based on its location"""
    if not sym.nodes:
        return None

    filename = sym.nodes[0].filename

    # Root Kconfig file
    if filename == 'Kconfig':
        return 'system_config'

    # Check subdirectory prefixes
    for prefix, config_type in LOCATION_MAP:
        if filename.startswith(prefix):
            return config_type

    return None


def group_symbols_by_location(kconf):
    """Group all symbols by their output file location"""
    grouped = defaultdict(list)
    for sym in kconf.unique_defined_syms:
        loc_type = get_symbol_location_type(sym)
        if loc_type:
            grouped[loc_type].append(sym)
    return grouped


def get_menu_path(sym):
    """Get menu path for a symbol"""
    path = []
    node = sym.nodes[0]
    while node.parent:
        if node.parent.is_menuconfig:
            prompt = node.parent.prompt[0] if node.parent.prompt else "Other"
            path.insert(0, prompt)
        node = node.parent
    return "/".join(path) if path else "Other"

def write_macro(f, sym):
    """Write a single macro definition"""
    value = get_value_for_header(sym)
    macro_name = sym.name

    # Add help text as comments
    if sym.nodes and sym.nodes[0].help:
        for line in sym.nodes[0].help.strip().split('\n'):
            if line.strip():
                f.write(f"/* {line.strip()} */\n")

    # Write the macro
    val = f'"{value}"' if sym.type == kconfiglib.STRING else value
    f.write(f"#define {macro_name}  {val}\n\n")

def generate_header_file(kconf, symbols, config_type, output_dir):
    """Generate a configuration header file for a group of symbols"""
    if not symbols:
        return None

    info = OUTPUT_FILE_MAPPING[config_type]
    output_file = os.path.join(output_dir, info['file'])
    os.makedirs(os.path.dirname(output_file), exist_ok=True)

    # Group by menu path
    by_menu = defaultdict(list)
    for sym in symbols:
        by_menu[get_menu_path(sym)].append(sym)

    with open(output_file, 'w') as f:
        f.write(f"/* Auto-generated by gen_config.py for {info['comment']} - DO NOT EDIT */\n\n")
        f.write(f"#ifndef {info['guard']}\n")
        f.write(f"#define {info['guard']}\n\n")
        f.write(f"/* {info['comment']} */\n\n")

        # Write symbols grouped by menu
        for menu_name, menu_syms in sorted(by_menu.items()):
            if menu_name != "Other":
                f.write(f"/* {menu_name} */\n")
            for sym in menu_syms:
                write_macro(f, sym)
            f.write("\n")

        # Add BOARD_LED_COUNT for board_config
        if config_type == 'board_config':
            led_count = sum(1 for i in [1,2,3]
                          if kconf.syms.get(f'BOARD_HAS_LED{i}', kconfiglib.Symbol()).tri_value == 2)
            f.write(f"\n#define BOARD_LED_COUNT {led_count}\n")

        f.write(f"\n#endif /* {info['guard']} */\n")

    print(f"Generated: {output_file}")
    return output_file


def cmd_generate_headers(args):
    """Generate C header files from .config"""
    try:
        kconfig_dir = os.path.dirname(os.path.abspath(args.kconfig_root)) or os.getcwd()
        original_cwd = os.getcwd()
        os.chdir(kconfig_dir)

        print(f"Loading Kconfig from {args.kconfig_root}...")
        kconf = load_kconfig(os.path.basename(args.kconfig_root))

        print(f"Loading config from {args.config_file}...")
        kconf.load_config(os.path.join(original_cwd, args.config_file))

        print("Generating configuration headers...")
        grouped = group_symbols_by_location(kconf)
        generated = []

        for config_type, symbols in sorted(grouped.items()):
            result = generate_header_file(kconf, symbols, config_type,
                                          os.path.join(original_cwd, args.output_dir))
            if result:
                generated.append(result)

        os.chdir(original_cwd)

        print(f"\nGenerated {len(generated)} header file(s)")
        for f in generated:
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
    gen_config_parser.add_argument('--board', default=None,
                                   help='Board name for auto-finding defconfig')
    gen_config_parser.add_argument('--search-path', default=None,
                                   help='Search path for defconfig files')

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
