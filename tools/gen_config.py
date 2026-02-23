#!/usr/bin/env python3
"""Generate configuration header from Kconfig definitions.

Usage:
    gen_config.py Kconfig --output config.h
    gen_config.py Kconfig --symbol CONFIG_BOARD_NAME
"""
import argparse
import os
import sys
from dataclasses import dataclass, field
from typing import Optional, ClassVar, List

import kconfiglib


@dataclass
class SConf:
    """Kconfig configuration session."""
    
    DEFAULT_HEADER: ClassVar[str] = 'config.h'
    DEFAULT_USRCONFIG: ClassVar[str] = '.config'
    
    kconfig_root: str
    output: str = DEFAULT_HEADER
    usrconfig: str = DEFAULT_USRCONFIG
    defconfig: Optional[str] = None
    symbol: Optional[str] = None
    
    project_root: str = field(init=False)
    _kconf: Optional[kconfiglib.Kconfig] = field(init=False, default=None, repr=False)
    
    def __post_init__(self):
        self.project_root = os.path.dirname(os.path.abspath(self.kconfig_root)) or os.getcwd()
    
    @property
    def kconf(self) -> kconfiglib.Kconfig:
        if self._kconf is None:
            self._load_kconfig()
        return self._kconf
    
    def _load_kconfig(self) -> None:
        self._kconf = kconfiglib.Kconfig(self.kconfig_root, suppress_traceback=True)
        
        if self.defconfig and os.path.exists(self.defconfig):
            self._kconf.load_config(self.defconfig)
        
        usrconfig_path = os.path.join(self.project_root, self.usrconfig)
        if os.path.exists(usrconfig_path):
            self._kconf.load_config(usrconfig_path)
    
    def reload(self) -> None:
        self._kconf = None
        _ = self.kconf
    
    def get_symbol(self, name: str) -> Optional[kconfiglib.Symbol]:
        lookup_name = name[7:] if name.startswith('CONFIG_') else name
        return self.kconf.syms.get(lookup_name)
    
    def query_symbol(self, symbol_name: str) -> str:
        sym = self.get_symbol(symbol_name)
        if sym is None:
            raise KeyError(f"Symbol '{symbol_name}' not found")
        return sym.str_value
    
    def generate_header(self, output_path: Optional[str] = None) -> str:
        output = output_path or self.output
        if not os.path.isabs(output):
            output = os.path.join(self.project_root, output)
        
        self.kconf.write_autoconf(output)
        return output
    
    def get_defined_symbols(self) -> List:
        return list(self.kconf.unique_defined_syms)
    
    def get_visible_symbols(self) -> List:
        return [s for s in self.kconf.unique_defined_syms if s._write_to_conf and s.name]


def parse_args() -> SConf:
    parser = argparse.ArgumentParser(
        description='Generate configuration header from Kconfig',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument('kconfig', help='Root Kconfig file path')
    parser.add_argument('-o', '--output', default=SConf.DEFAULT_HEADER,
                        help=f'Output header path (default: {SConf.DEFAULT_HEADER})')
    parser.add_argument('--usrconfig', default=SConf.DEFAULT_USRCONFIG,
                        help=f'User config file (default: {SConf.DEFAULT_USRCONFIG})')
    parser.add_argument('--defconfig', help='Default config file')
    parser.add_argument('--symbol', help='Query specific symbol value')
    
    args = parser.parse_args()
    
    return SConf(
        kconfig_root=args.kconfig,
        output=args.output,
        usrconfig=args.usrconfig,
        defconfig=args.defconfig,
        symbol=args.symbol
    )


def main():
    sconf = parse_args()
    
    try:
        if sconf.symbol:
            print(sconf.query_symbol(sconf.symbol))
        else:
            output_path = sconf.generate_header()
            print(f"Generated: {output_path}")
    except KeyError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
