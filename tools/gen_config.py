#!/usr/bin/env python3
"""Generate configuration header from Kconfig definitions.

Usage:
    gen_config.py Kconfig --output config.h
    gen_config.py Kconfig --symbol CONFIG_BOARD_NAME
    gen_config.py Kconfig --tree
"""

import argparse
import os
import sys
from dataclasses import dataclass, field
from typing import Optional, List, Dict, Any, Iterator
from enum import Enum, auto

import kconfiglib


class NodeType(Enum):
    """Kconfig node types in tree structure."""

    ROOT = auto()
    MENU = auto()
    CONFIG = auto()
    CHOICE = auto()
    COMMENT = auto()


@dataclass
class ConfigNode:
    """A node in the Kconfig tree.

    Each node knows its parent and children, forming a proper tree structure.
    """

    name: str
    node_type: NodeType
    symbol: Optional[kconfiglib.Symbol] = None
    prompt: Optional[str] = None
    help_text: Optional[str] = None
    parent: Optional["ConfigNode"] = field(default=None, repr=False)
    children: List["ConfigNode"] = field(default_factory=list)
    props: Dict[str, Any] = field(default_factory=dict)

    def add_child(self, child: "ConfigNode") -> "ConfigNode":
        """Add a child node and set its parent."""
        child.parent = self
        self.children.append(child)
        return child

    def get_value(self) -> Optional[str]:
        """Get the configuration value if this is a config node."""
        if self.symbol and hasattr(self.symbol, "str_value"):
            return self.symbol.str_value
        return None

    def is_enabled(self) -> bool:
        """Check if this config is enabled (y or numeric values)."""
        val = self.get_value()
        return val in ("y", "m") or (val and val.isdigit() and int(val) > 0)

    def iter_tree(self) -> Iterator["ConfigNode"]:
        """Iterate through this node and all descendants (pre-order)."""
        yield self
        for child in self.children:
            yield from child.iter_tree()

    def iter_leaf_configs(self) -> Iterator["ConfigNode"]:
        """Iterate through all leaf config nodes."""
        for node in self.iter_tree():
            if node.node_type == NodeType.CONFIG:
                yield node

    def get_depth(self) -> int:
        """Get depth in the tree (root = 0)."""
        depth = 0
        current = self.parent
        while current:
            depth += 1
            current = current.parent
        return depth

    def get_path(self) -> str:
        """Get the path from root to this node (e.g., "Board Selection > Target Board")."""
        parts = []
        current: Optional[ConfigNode] = self
        while current and current.parent:  # Exclude root
            parts.append(current.name)
            current = current.parent
        return " > ".join(reversed(parts))


class ConfigTree:
    """Tree representation of Kconfig structure.

    This is the core data structure that organizes all Kconfig nodes
    hierarchically, making dependencies and relationships explicit.
    """

    def __init__(self, kconf: kconfiglib.Kconfig):
        self.kconf = kconf
        self.root = ConfigNode(
            name=kconf.mainmenu_text or "Configuration", node_type=NodeType.ROOT
        )
        self._symbol_map: Dict[str, ConfigNode] = {}
        self._build_tree()

    def _build_tree(self) -> None:
        """Build the tree from kconfiglib's node structure."""
        # Build a path-based parent lookup
        menu_stack: List[ConfigNode] = [self.root]

        for node in self.kconf.node_iter():
            if node.item == kconfiglib.MENU:
                # Create menu node - item is the MENU constant
                prompt = node.prompt[0] if node.prompt else "Menu"
                menu_node = ConfigNode(
                    name=prompt, node_type=NodeType.MENU, prompt=prompt
                )
                # Add to current parent (top of stack)
                menu_stack[-1].add_child(menu_node)
                # Push this menu onto stack for its children
                menu_stack.append(menu_node)

            elif node.item == kconfiglib.COMMENT:
                # Skip comments in tree view
                pass

            elif isinstance(node.item, kconfiglib.Symbol):
                # Create config node
                sym = node.item
                prompt_text = node.prompt[0] if node.prompt else None
                help_text = node.help if node.help else None

                config_node = ConfigNode(
                    name=sym.name,
                    node_type=NodeType.CONFIG,
                    symbol=sym,
                    prompt=prompt_text,
                    help_text=help_text,
                    props={
                        "type": kconfiglib.TYPE_TO_STR[sym.type]
                        if sym.type in kconfiglib.TYPE_TO_STR
                        else str(sym.type),
                    },
                )

                # Register in symbol map for quick lookup
                self._symbol_map[sym.name] = config_node

                # Add to current parent
                menu_stack[-1].add_child(config_node)

            elif isinstance(node.item, kconfiglib.Choice):
                # Create choice node
                choice = node.item
                prompt_text = node.prompt[0] if node.prompt else None

                choice_node = ConfigNode(
                    name=prompt_text or "Choice",
                    node_type=NodeType.CHOICE,
                    prompt=prompt_text,
                    props={"symbols": [s.name for s in choice.syms]},
                )

                # Add to current parent
                menu_stack[-1].add_child(choice_node)

        self._merge_symbol_nodes()

    def _merge_symbol_nodes(self) -> None:
        """Merge duplicate symbol nodes (same symbol defined in multiple locations)."""
        # When a symbol appears under multiple menus, we keep the first occurrence
        # and move all children to it, removing duplicates
        seen: Dict[str, ConfigNode] = {}

        def dedup_node(node: ConfigNode) -> None:
            if node.node_type == NodeType.CONFIG and node.name in seen:
                # This is a duplicate, mark for removal
                node._is_duplicate = True  # type: ignore
            elif node.node_type == NodeType.CONFIG:
                seen[node.name] = node

            # Process children
            for child in list(node.children):
                dedup_node(child)

        dedup_node(self.root)

        # Remove marked duplicates
        def remove_duplicates(node: ConfigNode) -> None:
            node.children = [
                c for c in node.children if not getattr(c, "_is_duplicate", False)
            ]
            for child in node.children:
                remove_duplicates(child)

        remove_duplicates(self.root)

    def get_symbol_node(self, name: str) -> Optional[ConfigNode]:
        """Get a config node by symbol name."""
        lookup_name = name[7:] if name.startswith("CONFIG_") else name
        return self._symbol_map.get(lookup_name)

    def get_all_configs(self) -> List[ConfigNode]:
        """Get all configuration nodes."""
        return [n for n in self.root.iter_tree() if n.node_type == NodeType.CONFIG]

    def get_enabled_configs(self) -> List[ConfigNode]:
        """Get all enabled configuration nodes."""
        return [n for n in self.get_all_configs() if n.is_enabled()]

    def format_tree(self, show_values: bool = True) -> str:
        """Format the tree as a string for display."""
        lines = []

        def format_node(
            node: ConfigNode, prefix: str = "", is_last: bool = True
        ) -> None:
            # Skip root node in output
            if node.node_type != NodeType.ROOT:
                marker = "└── " if is_last else "├── "

                # Format node display name
                display_name = node.name
                if node.node_type == NodeType.CONFIG and show_values:
                    value = node.get_value()
                    enabled_mark = "✓" if node.is_enabled() else "○"
                    if value:
                        display_name = f"{enabled_mark} {node.name} = {value}"
                    else:
                        display_name = f"○ {node.name}"
                elif node.node_type == NodeType.MENU:
                    display_name = f"📁 {node.name}"
                elif node.node_type == NodeType.CHOICE:
                    display_name = f"◈ {node.name}"

                lines.append(f"{prefix}{marker}{display_name}")

            # Process children
            children = node.children
            if children:
                new_prefix = prefix + ("    " if is_last else "│   ")
                for i, child in enumerate(children):
                    is_last_child = i == len(children) - 1
                    format_node(child, new_prefix, is_last_child)

        for i, child in enumerate(self.root.children):
            is_last = i == len(self.root.children) - 1
            format_node(child, "", is_last)

        return "\n".join(lines)


@dataclass
class SConf:
    """Kconfig configuration session with tree-based organization."""

    DEFAULT_HEADER: str = "config.h"
    DEFAULT_USRCONFIG: str = ".config"

    kconfig_root: str = ""
    output: str = DEFAULT_HEADER
    usrconfig: str = DEFAULT_USRCONFIG
    defconfig: Optional[str] = None
    symbol: Optional[str] = None
    show_tree: bool = False

    project_root: str = field(init=False)
    _kconf: Optional[kconfiglib.Kconfig] = field(default=None, repr=False)
    _tree: Optional[ConfigTree] = field(default=None, repr=False)

    def __post_init__(self):
        self.project_root = (
            os.path.dirname(os.path.abspath(self.kconfig_root)) or os.getcwd()
        )

    @property
    def kconf(self) -> kconfiglib.Kconfig:
        if self._kconf is None:
            self._load_kconfig()
        return self._kconf

    @property
    def tree(self) -> ConfigTree:
        if self._tree is None:
            self._tree = ConfigTree(self.kconf)
        return self._tree

    def _load_kconfig(self) -> None:
        os.environ.setdefault("SRCTREE", self.project_root)
        self._kconf = kconfiglib.Kconfig(self.kconfig_root, suppress_traceback=True)

        if self.defconfig and os.path.exists(self.defconfig):
            self._kconf.load_config(self.defconfig)

        usrconfig_path = os.path.join(self.project_root, self.usrconfig)
        if os.path.exists(usrconfig_path):
            self._kconf.load_config(usrconfig_path)

    def reload(self) -> None:
        self._kconf = None
        self._tree = None
        _ = self.kconf

    def query_symbol(self, symbol_name: str) -> str:
        node = self.tree.get_symbol_node(symbol_name)
        if node is None:
            raise KeyError(f"Symbol '{symbol_name}' not found")
        value = node.get_value()
        if value is None:
            raise KeyError(f"Symbol '{symbol_name}' has no value")
        return value

    def generate_header(self, output_path: Optional[str] = None) -> str:
        output = output_path or self.output
        if not os.path.isabs(output):
            output = os.path.join(self.project_root, output)

        self.kconf.write_autoconf(output)
        return output

    def print_tree(self) -> None:
        """Print the configuration tree."""
        print(f"\n{self.tree.root.name}")
        print("=" * 50)
        print(self.tree.format_tree())
        print()


def parse_args() -> SConf:
    parser = argparse.ArgumentParser(
        description="Generate configuration header from Kconfig",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("kconfig", help="Root Kconfig file path")
    parser.add_argument(
        "-o",
        "--output",
        default=SConf.DEFAULT_HEADER,
        help=f"Output header path (default: {SConf.DEFAULT_HEADER})",
    )
    parser.add_argument(
        "--usrconfig",
        default=SConf.DEFAULT_USRCONFIG,
        help=f"User config file (default: {SConf.DEFAULT_USRCONFIG})",
    )
    parser.add_argument("--defconfig", help="Default config file")
    parser.add_argument("--symbol", help="Query specific symbol value")
    parser.add_argument(
        "--tree", action="store_true", help="Display the configuration tree"
    )

    args = parser.parse_args()

    return SConf(
        kconfig_root=args.kconfig,
        output=args.output,
        usrconfig=args.usrconfig,
        defconfig=args.defconfig,
        symbol=args.symbol,
        show_tree=args.tree,
    )


def main():
    sconf = parse_args()

    try:
        if sconf.show_tree:
            sconf.print_tree()
        elif sconf.symbol:
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


if __name__ == "__main__":
    main()
