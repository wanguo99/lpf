#!/usr/bin/env python3
"""
Kconfig Auto-Generation Script

This script automatically generates Kconfig files from test directory structure.
It scans test_*.c files and creates corresponding Kconfig entries with proper
structure and defaults.

Features:
- Auto-discover test files in directory structure
- Generate Kconfig entries with proper naming
- Create TEST_*_ALL selection configs
- Preserve existing help text and manual customizations
- Support for unit, performance, stress, and system test categories

Usage:
    # Generate Kconfig for all modules (dry-run)
    python3 sync_kconfig.py --dry-run

    # Generate Kconfig for specific category/module
    python3 sync_kconfig.py --category unit --module osal

    # Actually write Kconfig files
    python3 sync_kconfig.py --write

    # Interactive mode (preview changes, ask before writing)
    python3 sync_kconfig.py --interactive

Options:
    --category      Target category (unit, performance, stress, system, all)
    --module        Target module (osal, hal, pdl, prl, pconfig, aconfig, all)
    --write         Actually write files (default is dry-run)
    --interactive   Preview and confirm each change
    --preserve      Preserve existing help text from current Kconfig
    --verbose       Show detailed generation process
"""

import os
import sys
import re
import argparse
from pathlib import Path
from typing import List, Dict, Set, Tuple, Optional
from collections import OrderedDict


def get_project_root() -> Path:
    """Get the ES-Middleware project root directory."""
    script_dir = Path(__file__).resolve().parent
    return script_dir.parent.parent.parent


def file_to_config_name(filename: str) -> str:
    """
    Convert test filename to Kconfig option name.

    Example: test_osal_mutex.c -> TEST_OSAL_MUTEX
    """
    name = Path(filename).stem  # Remove .c extension
    return name.upper()


def parse_test_metadata(filepath: Path) -> Dict[str, str]:
    """
    Parse test file for metadata comments.

    Looks for comments like:
    // @test_description: Test OSAL mutex operations
    // @test_runtime: <50ms
    // @test_hardware: None
    // @test_tags: fast

    Returns dict with metadata or defaults.
    """
    metadata = {
        'description': '',
        'runtime': '',
        'hardware': '',
        'tags': '',
        'coverage': []
    }

    if not filepath.exists():
        return metadata

    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read(2048)  # Read first 2KB for metadata

            # Look for metadata comments
            desc_match = re.search(r'@test_description:\s*(.+)', content)
            if desc_match:
                metadata['description'] = desc_match.group(1).strip()

            runtime_match = re.search(r'@test_runtime:\s*(.+)', content)
            if runtime_match:
                metadata['runtime'] = runtime_match.group(1).strip()

            hw_match = re.search(r'@test_hardware:\s*(.+)', content)
            if hw_match:
                metadata['hardware'] = hw_match.group(1).strip()

            tags_match = re.search(r'@test_tags:\s*(.+)', content)
            if tags_match:
                metadata['tags'] = tags_match.group(1).strip()

            # Extract coverage items from comments
            coverage_items = re.findall(r'//\s*-\s*(.+)', content)
            if coverage_items:
                metadata['coverage'] = [item.strip() for item in coverage_items[:10]]

    except Exception as e:
        print(f"Warning: Failed to parse metadata from {filepath}: {e}", file=sys.stderr)

    return metadata


def parse_existing_help_text(kconfig_path: Path, config_name: str) -> Optional[str]:
    """
    Extract existing help text for a config option from Kconfig file.

    Returns the help text block or None if not found.
    """
    if not kconfig_path.exists():
        return None

    try:
        with open(kconfig_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()

        # Find the config line
        config_line_idx = None
        for idx, line in enumerate(lines):
            if re.match(rf'\s*config\s+{config_name}\s*$', line):
                config_line_idx = idx
                break

        if config_line_idx is None:
            return None

        # Extract help text
        help_lines = []
        in_help = False
        for line in lines[config_line_idx + 1:]:
            # Check if we've reached another config or end
            if re.match(r'\s*config\s+\w+', line) or re.match(r'\s*endmenu', line) or re.match(r'\s*endif', line):
                break

            if re.match(r'\s*help\s*$', line):
                in_help = True
                continue

            if in_help:
                # Help text is indented
                if line.strip() == '' or line.startswith('\t') or line.startswith('    '):
                    help_lines.append(line.rstrip())
                else:
                    # Non-indented line means end of help
                    break

        if help_lines:
            return '\n'.join(help_lines)

    except Exception as e:
        print(f"Warning: Failed to parse existing help text: {e}", file=sys.stderr)

    return None


def generate_help_text(test_file: str, module: str, metadata: Dict[str, str],
                      existing_help: Optional[str] = None) -> str:
    """
    Generate help text for a Kconfig option.

    If existing_help is provided, use it. Otherwise generate from metadata.
    """
    if existing_help:
        # Preserve existing help text
        return existing_help

    lines = []

    # Description
    if metadata['description']:
        lines.append(f"  {metadata['description']}")
    else:
        # Generate default description
        config_name = file_to_config_name(test_file)
        pretty_name = config_name.replace('_', ' ').lower()
        lines.append(f"  Test {pretty_name}.")

    lines.append("")

    # Coverage
    if metadata['coverage']:
        lines.append("  Coverage:")
        for item in metadata['coverage']:
            lines.append(f"    - {item}")
        lines.append("")

    # Runtime
    runtime = metadata['runtime'] if metadata['runtime'] else '<100ms'
    lines.append(f"  Runtime: {runtime}")

    # Hardware
    hardware = metadata['hardware'] if metadata['hardware'] else 'None (software only)'
    lines.append(f"  Hardware: {hardware}")

    # Dependencies
    lines.append(f"  Dependencies: CONFIG_{module.upper()}")

    # Tags
    if metadata['tags']:
        lines.append(f"  Tags: {metadata['tags']}")

    # Test file reference
    lines.append(f"  Test file: {test_file}")

    return '\n'.join(lines)


def generate_kconfig_entry(test_file: str, module: str, category: str,
                          kconfig_path: Path, preserve_help: bool = True) -> str:
    """
    Generate a single Kconfig config entry for a test file.
    """
    config_name = file_to_config_name(test_file)

    # Parse metadata from test file
    test_file_path = kconfig_path.parent / test_file
    metadata = parse_test_metadata(test_file_path)

    # Try to preserve existing help text
    existing_help = None
    if preserve_help:
        existing_help = parse_existing_help_text(kconfig_path, config_name)

    # Generate help text
    help_text = generate_help_text(test_file, module, metadata, existing_help)

    # Build config entry
    lines = []
    lines.append(f"config {config_name}")

    # Description (bool line)
    if metadata['description']:
        # Extract first sentence for bool description
        desc = metadata['description'].split('.')[0]
        lines.append(f'\tbool "{desc}"')
    else:
        pretty_name = config_name.replace('_', ' ').title()
        lines.append(f'\tbool "{pretty_name} tests"')

    lines.append('\tdefault y')
    lines.append('\thelp')
    lines.append(help_text)

    return '\n'.join(lines)


def generate_kconfig_file(category: str, module: str, project_root: Path,
                         preserve_help: bool = True) -> str:
    """
    Generate complete Kconfig file for a module.

    Returns the generated Kconfig content as string.
    """
    module_dir = project_root / "products" / "tests" / category / module
    kconfig_path = module_dir / "Kconfig"

    # Scan for test files
    test_files = sorted([f.name for f in module_dir.glob("test_*.c")])

    if not test_files:
        return ""

    # Build Kconfig content
    lines = []

    # Header
    module_title = module.upper()
    category_title = category.title()
    lines.append(f"# {module_title} {category_title} Test Configuration")
    lines.append("#")
    lines.append(f"# Fine-grained control for individual {module_title} test files.")
    lines.append(f"# Each option corresponds to a single test_{module}_*.c file.")
    lines.append("")

    # Conditional block
    lines.append(f"if TEST_{module_title}")
    lines.append("")

    # Menu
    lines.append(f'menu "{module_title} {category_title} Tests"')
    lines.append("")

    # Generate TEST_*_ALL config
    all_config_name = f"TEST_{module_title}_ALL"
    lines.append(f"config {all_config_name}")
    lines.append(f'\tbool "Enable all {module_title} {category} tests"')
    lines.append('\tdefault y')

    # Select all test configs
    for test_file in test_files:
        config_name = file_to_config_name(test_file)
        lines.append(f'\tselect {config_name}')

    lines.append('\thelp')
    lines.append(f'  Enable all {module_title} {category} tests at once.')
    lines.append('  Disabling this allows individual test selection below.')
    lines.append("")

    # Generate individual test configs
    for test_file in test_files:
        entry = generate_kconfig_entry(test_file, module, category, kconfig_path, preserve_help)
        lines.append(entry)
        lines.append("")

    # Close menu and conditional
    lines.append("endmenu")
    lines.append("")
    lines.append(f"endif # TEST_{module_title}")

    return '\n'.join(lines)


def scan_modules(category: str, project_root: Path) -> List[str]:
    """
    Scan for modules in a category that have test files.
    """
    category_dir = project_root / "products" / "tests" / category

    if not category_dir.exists():
        return []

    modules = []
    for item in sorted(category_dir.iterdir()):
        if item.is_dir() and not item.name.startswith('.'):
            # Check if it has test files
            test_files = list(item.glob("test_*.c"))
            if test_files:
                modules.append(item.name)

    return modules


def preview_diff(kconfig_path: Path, new_content: str) -> bool:
    """
    Show diff between existing and new Kconfig content.

    Returns True if there are differences, False otherwise.
    """
    if not kconfig_path.exists():
        print(f"\n[NEW FILE] {kconfig_path}")
        print("=" * 80)
        print(new_content)
        print("=" * 80)
        return True

    try:
        with open(kconfig_path, 'r', encoding='utf-8') as f:
            old_content = f.read()

        if old_content.strip() == new_content.strip():
            return False

        print(f"\n[MODIFIED] {kconfig_path}")
        print("=" * 80)

        # Simple line-by-line diff
        old_lines = old_content.splitlines()
        new_lines = new_content.splitlines()

        # Show first 50 lines of diff
        max_lines = 50
        shown = 0
        for i, (old, new) in enumerate(zip(old_lines, new_lines)):
            if old != new and shown < max_lines:
                print(f"- {old}")
                print(f"+ {new}")
                shown += 1

        if len(new_lines) > len(old_lines):
            for line in new_lines[len(old_lines):]:
                if shown >= max_lines:
                    break
                print(f"+ {line}")
                shown += 1

        if shown >= max_lines:
            print(f"... ({len(new_lines)} total lines, showing first {max_lines} differences)")

        print("=" * 80)
        return True

    except Exception as e:
        print(f"Error comparing files: {e}", file=sys.stderr)
        return True


def write_kconfig_file(kconfig_path: Path, content: str, dry_run: bool = True) -> bool:
    """
    Write Kconfig file to disk.

    Returns True if written, False otherwise.
    """
    if dry_run:
        print(f"[DRY-RUN] Would write: {kconfig_path}")
        return False

    try:
        kconfig_path.parent.mkdir(parents=True, exist_ok=True)
        with open(kconfig_path, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"✅ Written: {kconfig_path}")
        return True
    except Exception as e:
        print(f"❌ Failed to write {kconfig_path}: {e}", file=sys.stderr)
        return False


def main():
    parser = argparse.ArgumentParser(
        description="Auto-generate Kconfig files from test directory structure",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument("--category", choices=["unit", "performance", "stress", "system", "all"],
                       default="all", help="Target category")
    parser.add_argument("--module", help="Target module (default: all modules)")
    parser.add_argument("--write", action="store_true", help="Actually write files (default: dry-run)")
    parser.add_argument("--interactive", "-i", action="store_true",
                       help="Interactive mode (preview and confirm)")
    parser.add_argument("--preserve", action="store_true", default=True,
                       help="Preserve existing help text (default: true)")
    parser.add_argument("--verbose", "-v", action="store_true", help="Verbose output")

    args = parser.parse_args()

    project_root = get_project_root()

    # Determine categories to process
    if args.category == "all":
        categories = ["unit", "performance", "stress", "system"]
    else:
        categories = [args.category]

    dry_run = not args.write

    if dry_run and not args.interactive:
        print("DRY-RUN MODE: No files will be written. Use --write to apply changes.")
        print("=" * 80)

    total_generated = 0
    total_unchanged = 0

    for category in categories:
        # Get modules to process
        if args.module and args.module != "all":
            modules = [args.module]
        else:
            modules = scan_modules(category, project_root)

        if not modules:
            if args.verbose:
                print(f"No modules found in category '{category}'")
            continue

        print(f"\n{'='*80}")
        print(f"Category: {category}")
        print(f"{'='*80}")

        for module in modules:
            kconfig_path = project_root / "products" / "tests" / category / module / "Kconfig"

            if args.verbose:
                print(f"\nProcessing: {category}/{module}")

            # Generate Kconfig content
            content = generate_kconfig_file(category, module, project_root, args.preserve)

            if not content:
                if args.verbose:
                    print(f"  No test files found, skipping")
                continue

            # Preview changes
            has_changes = preview_diff(kconfig_path, content)

            if not has_changes:
                total_unchanged += 1
                if args.verbose:
                    print(f"  ✅ {kconfig_path} - unchanged")
                continue

            # Interactive confirmation
            if args.interactive:
                response = input(f"\nWrite {kconfig_path}? [y/N]: ")
                if response.lower() != 'y':
                    print("  Skipped")
                    continue

                # Write in interactive mode
                if write_kconfig_file(kconfig_path, content, dry_run=False):
                    total_generated += 1
            else:
                # Write if --write was specified
                if write_kconfig_file(kconfig_path, content, dry_run):
                    total_generated += 1

    # Summary
    print(f"\n{'='*80}")
    print("Summary")
    print(f"{'='*80}")
    print(f"Files generated: {total_generated}")
    print(f"Files unchanged: {total_unchanged}")

    if dry_run and not args.interactive:
        print("\n💡 This was a dry-run. Use --write to apply changes.")
        print("💡 Use --interactive to review and confirm each change.")

    return 0


if __name__ == "__main__":
    sys.exit(main())
