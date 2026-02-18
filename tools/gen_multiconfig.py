#!/usr/bin/env python3

"""
Generate multiple Kconfig header files for modular integration, with support for full
configuration, subsystem-specific headers, and querying symbol values.

Features:
- Generate headers for all Kconfig files or specific fragments
- Query symbol values

Usage:
  python gen_multiconfig Kconfig
  python gen_multiconfig Kconfig --kconfig-fragment drivers/gpio/Kconfig
  python gen_multiconfig Kconfig --symbol CONFIG_FOO
"""

import argparse
import os
import sys

import kconfiglib


def main():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=__doc__)

    parser.add_argument(
        "kconfig",
        metavar="KCONFIG",
        help="Top-level Kconfig file")

    parser.add_argument(
        "--output-dir",
        metavar="OUTPUT_DIR",
        default="include/generated",
        help="Output directory for header files (default: include/generated)")

    parser.add_argument(
        "--kconfig-fragment",
        metavar="FRAGMENT",
        help="Generate headers for a specific Kconfig fragment (e.g., drivers/gpio/Kconfig)")

    parser.add_argument(
        "--print-headers",
        action="store_true",
        help="Print generated header file paths")

    parser.add_argument(
        "--check-symbols",
        action="store_true",
        help="Print enabled symbols for feature detection")

    parser.add_argument(
        "--symbol",
        metavar="SYMBOL",
        help="Print value of a specific symbol")

    args = parser.parse_args()

    # Load Kconfig
    try:
        kconf = kconfiglib.Kconfig(args.kconfig, suppress_traceback=True)
        kconf.load_config()
    except kconfiglib.KconfigError as e:
        sys.exit("Error loading Kconfig: {}".format(e))

    # Check specific symbol value if requested
    if args.symbol:
        if args.symbol not in kconf.syms:
            sys.exit("Symbol {} not found".format(args.symbol))
        sym = kconf.syms[args.symbol]
        print(sym.str_value)
        return

    # Build mapping from Kconfig files to their symbols
    files_to_symbols = _build_file_to_symbols_map(kconf)

    # Filter by fragment if specified
    if args.kconfig_fragment:
        fragment = args.kconfig_fragment
        if fragment not in files_to_symbols:
            sys.stderr.write(
                "Warning: No symbols found for fragment '{}'. "
                "Available fragments:\n".format(fragment))
            for f in sorted(files_to_symbols.keys()):
                sys.stderr.write("  - {}\n".format(f))
            sys.exit(1)

        # Only process the requested fragment
        filtered_files = {fragment: files_to_symbols[fragment]}
    else:
        # Process all files
        filtered_files = files_to_symbols

    # Generate headers and collect paths
    generated_files = []
    enabled_symbols = []

    for filename, symbols in filtered_files.items():
        # Skip files with no enabled symbols
        if not symbols:
            continue

        header_path = _get_header_path(filename, args.output_dir)
        _write_header_file(kconf, symbols, header_path)
        generated_files.append(header_path)

        # Collect enabled symbols
        if args.check_symbols:
            for sym in symbols:
                val = sym.str_value
                if val == "y" or val == "m":
                    enabled_symbols.append("{}={}".format(sym.name, val))

    # Print header paths
    if args.print_headers:
        for header_path in generated_files:
            print(header_path)

    # Print enabled symbols
    if args.check_symbols:
        print("ENABLED_SYMBOLS:" + ";".join(enabled_symbols))


def _build_file_to_symbols_map(kconf):
    """
    Build a dictionary mapping Kconfig file paths to lists of symbols
    defined in those files.
    """
    files_to_symbols = {}

    for sym in kconf.unique_defined_syms:
        val = sym.str_value
        if not sym._write_to_conf:
            continue

        # Add this symbol to all files where it's defined
        for node in sym.nodes:
            filename = node.filename
            if filename not in files_to_symbols:
                files_to_symbols[filename] = []
            files_to_symbols[filename].append(sym)

    return files_to_symbols


def _get_header_path(kconfig_path, output_dir):
    """
    Convert a Kconfig file path to the corresponding header file path.

    Example: drivers/gpio/Kconfig -> {output_dir}/drivers/gpio/config.h
    Config files in the root directory map to {output_dir}/config.h
    """
    kconfig_dir = os.path.dirname(kconfig_path)

    if not kconfig_dir:
        # Root-level Kconfig
        return os.path.join(output_dir, "config.h")

    # Preserve directory structure
    return os.path.join(output_dir, kconfig_dir, "config.h")


def _write_header_file(kconf, symbols, header_path):
    """
    Write a header file containing #defines for the given symbols.
    """
    header_content = _generate_header_content(kconf, symbols)

    # Create directory if needed
    output_dir = os.path.dirname(header_path)
    if output_dir:
        os.makedirs(output_dir, exist_ok=True)

    # Write using kconfiglib's atomic write function
    if kconf._write_if_changed(header_path, header_content):
        print("Generated: {}".format(header_path), file=sys.stderr)
    else:
        print("Unchanged: {}".format(header_path), file=sys.stderr)


def _generate_header_content(kconf, symbols):
    """
    Generate header content for a list of symbols.
    Mirrors kconfiglib's _autoconf_contents() logic.
    """
    header = os.getenv("KCONFIG_AUTOHEADER_HEADER", "")
    if header:
        header += "\n"

    chunks = [header]
    add = chunks.append

    for sym in symbols:
        val = sym.str_value
        if not sym._write_to_conf:
            continue

        if sym.orig_type in kconfiglib._BOOL_TRISTATE:
            if val == "y":
                add("#define {}{} 1\n"
                    .format(kconf.config_prefix, sym.name))
            elif val == "m":
                add("#define {}{}_MODULE 1\n"
                    .format(kconf.config_prefix, sym.name))

        elif sym.orig_type is kconfiglib.STRING:
            add('#define {}{} "{}"\n'
                .format(kconf.config_prefix, sym.name,
                        kconfiglib.escape(val)))

        else:  # HEX or INT
            if sym.orig_type is kconfiglib.HEX and \
               not val.startswith(("0x", "0X")):
                val = "0x" + val

            add("#define {}{} {}\n"
                .format(kconf.config_prefix, sym.name, val))

    return "".join(chunks)


if __name__ == "__main__":
    main()
