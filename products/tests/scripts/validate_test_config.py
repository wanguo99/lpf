#!/usr/bin/env python3
"""
Test Configuration Validation Script

This script validates consistency between test files and Kconfig options.
It detects:
- Test files without corresponding Kconfig options
- Kconfig options without corresponding test files
- Naming convention violations
- Invalid characters in test file names

Usage:
    python3 validate_test_config.py [--fix] [--category CATEGORY]

Options:
    --fix           Automatically generate missing Kconfig entries (interactive)
    --category      Check specific category only (unit, performance, stress, system)
"""

import os
import sys
import re
import argparse
from pathlib import Path
from typing import List, Dict, Set, Tuple


def get_project_root() -> Path:
    """Get the EMS project root directory."""
    script_dir = Path(__file__).resolve().parent
    return script_dir.parent.parent.parent


def file_to_config(filename: str) -> str:
    """
    Convert test filename to Kconfig option name.

    Example: test_osal_mutex.c -> CONFIG_TEST_OSAL_MUTEX
    """
    name = Path(filename).stem  # Remove .c extension
    return f"CONFIG_{name.upper()}"


def config_to_file(config: str) -> str:
    """
    Convert Kconfig option to test filename.

    Example: CONFIG_TEST_OSAL_MUTEX -> test_osal_mutex.c
    """
    if config.startswith("CONFIG_"):
        config = config[7:]  # Remove CONFIG_ prefix
    return f"{config.lower()}.c"


def validate_naming_convention(filename: str) -> Tuple[bool, str]:
    """
    Validate that test filename follows naming convention.

    Returns: (is_valid, error_message)
    """
    # Check if starts with test_
    if not filename.startswith("test_"):
        return False, f"File '{filename}' does not start with 'test_'"

    # Check for invalid characters
    if re.search(r'[^a-zA-Z0-9_\.]', filename):
        return False, f"File '{filename}' contains invalid characters (use only a-z, A-Z, 0-9, _)"

    # Check extension
    if not filename.endswith(".c"):
        return False, f"File '{filename}' does not have .c extension"

    return True, ""


def scan_test_files(directory: Path) -> Set[str]:
    """Scan directory for test_*.c files."""
    if not directory.exists():
        return set()

    test_files = set()
    for file in directory.glob("test_*.c"):
        test_files.add(file.name)
    return test_files


def parse_kconfig_options(kconfig_path: Path) -> Set[str]:
    """Parse Kconfig file and extract CONFIG_TEST_* options."""
    if not kconfig_path.exists():
        return set()

    configs = set()
    with open(kconfig_path, 'r') as f:
        for line in f:
            # Match lines like: config TEST_OSAL_MUTEX
            match = re.match(r'\s*config\s+(TEST_\w+)', line)
            if match:
                config_name = match.group(1)
                # Skip meta configs like TEST_*_ALL
                if not config_name.endswith("_ALL"):
                    configs.add(f"CONFIG_{config_name}")

    return configs


def validate_module(category: str, module: str, project_root: Path) -> Dict:
    """
    Validate consistency between test files and Kconfig for a module.

    Returns dict with validation results.
    """
    module_dir = project_root / "products" / "tests" / category / module
    kconfig_path = module_dir / "Kconfig"

    results = {
        "module": module,
        "category": category,
        "test_files": set(),
        "kconfig_options": set(),
        "missing_configs": set(),
        "orphaned_configs": set(),
        "naming_violations": [],
        "errors": []
    }

    # Scan test files
    if module_dir.exists():
        results["test_files"] = scan_test_files(module_dir)
    else:
        results["errors"].append(f"Module directory not found: {module_dir}")
        return results

    # Validate naming conventions
    for filename in results["test_files"]:
        is_valid, error_msg = validate_naming_convention(filename)
        if not is_valid:
            results["naming_violations"].append((filename, error_msg))

    # Parse Kconfig options
    if kconfig_path.exists():
        results["kconfig_options"] = parse_kconfig_options(kconfig_path)
    else:
        results["errors"].append(f"Kconfig not found: {kconfig_path}")
        return results

    # Find test files without Kconfig options
    for filename in results["test_files"]:
        config_name = file_to_config(filename)
        if config_name not in results["kconfig_options"]:
            results["missing_configs"].add((filename, config_name))

    # Find Kconfig options without test files
    for config_name in results["kconfig_options"]:
        expected_file = config_to_file(config_name)
        if expected_file not in results["test_files"]:
            results["orphaned_configs"].add((config_name, expected_file))

    return results


def print_results(results: Dict, verbose: bool = True):
    """Print validation results."""
    module = results["module"]
    category = results["category"]

    # Print header
    print(f"\n{'='*80}")
    print(f"Module: {category}/{module}")
    print(f"{'='*80}")

    # Print statistics
    num_files = len(results["test_files"])
    num_configs = len(results["kconfig_options"])
    num_missing = len(results["missing_configs"])
    num_orphaned = len(results["orphaned_configs"])
    num_violations = len(results["naming_violations"])

    print(f"Test files: {num_files}")
    print(f"Kconfig options: {num_configs}")

    # Print errors
    if results["errors"]:
        print(f"\n❌ Errors:")
        for error in results["errors"]:
            print(f"  - {error}")
        return

    # Print issues
    has_issues = num_missing > 0 or num_orphaned > 0 or num_violations > 0

    if not has_issues:
        print(f"✅ All checks passed!")
        return

    # Print naming violations
    if num_violations > 0:
        print(f"\n⚠️  Naming convention violations: {num_violations}")
        for filename, error_msg in results["naming_violations"]:
            print(f"  - {error_msg}")

    # Print missing configs
    if num_missing > 0:
        print(f"\n⚠️  Test files without Kconfig options: {num_missing}")
        for filename, config_name in sorted(results["missing_configs"]):
            print(f"  - {filename} -> missing {config_name}")
            if verbose:
                print(f"    Add to {category}/{module}/Kconfig:")
                print(f"    config {config_name[7:]}")  # Remove CONFIG_ prefix
                print(f"        bool \"<description>\"")
                print(f"        default y")
                print(f"        help")
                print(f"          Test <functionality>.")
                print(f"          Runtime: <time>")
                print(f"          Hardware: <requirements>")
                print(f"          Dependencies: CONFIG_{module.upper()}")
                print()

    # Print orphaned configs
    if num_orphaned > 0:
        print(f"\n⚠️  Kconfig options without test files: {num_orphaned}")
        for config_name, expected_file in sorted(results["orphaned_configs"]):
            print(f"  - {config_name} -> missing {expected_file}")
            if verbose:
                print(f"    Either:")
                print(f"      1. Create test file: {category}/{module}/{expected_file}")
                print(f"      2. Remove from Kconfig: {category}/{module}/Kconfig")
                print()


def validate_category(category: str, project_root: Path, modules: List[str]) -> List[Dict]:
    """Validate all modules in a category."""
    all_results = []

    for module in modules:
        results = validate_module(category, module, project_root)
        all_results.append(results)

    return all_results


def print_summary(all_results: List[Dict]):
    """Print overall summary."""
    total_missing = sum(len(r["missing_configs"]) for r in all_results)
    total_orphaned = sum(len(r["orphaned_configs"]) for r in all_results)
    total_violations = sum(len(r["naming_violations"]) for r in all_results)

    print(f"\n{'='*80}")
    print(f"Summary")
    print(f"{'='*80}")
    print(f"Total issues found:")
    print(f"  - Missing Kconfig options: {total_missing}")
    print(f"  - Orphaned Kconfig options: {total_orphaned}")
    print(f"  - Naming violations: {total_violations}")

    if total_missing + total_orphaned + total_violations == 0:
        print(f"\n✅ All validations passed!")
    else:
        print(f"\n⚠️  Please fix the issues above.")
        print(f"\nNext steps:")
        print(f"  1. Add missing Kconfig options to respective Kconfig files")
        print(f"  2. Create missing test files or remove orphaned configs")
        print(f"  3. Fix naming convention violations")
        print(f"  4. Run: python3 build.py menuconfig")
        print(f"  5. Run: python3 build.py build")


def main():
    parser = argparse.ArgumentParser(description="Validate test configuration consistency")
    parser.add_argument("--category", choices=["unit", "performance", "stress", "system", "all"],
                        default="all", help="Category to validate")
    parser.add_argument("--verbose", "-v", action="store_true", help="Verbose output")
    args = parser.parse_args()

    project_root = get_project_root()

    # Define modules per category
    unit_modules = ["osal", "hal", "pdl", "prl", "pconfig", "aconfig"]

    categories_to_check = []
    if args.category == "all":
        categories_to_check = [("unit", unit_modules)]
    elif args.category == "unit":
        categories_to_check = [("unit", unit_modules)]
    # Add other categories when implemented

    all_results = []
    for category, modules in categories_to_check:
        results = validate_category(category, project_root, modules)
        all_results.extend(results)

        # Print results for each module
        for result in results:
            print_results(result, verbose=args.verbose)

    # Print summary
    print_summary(all_results)

    # Exit with error code if issues found
    total_issues = sum(
        len(r["missing_configs"]) + len(r["orphaned_configs"]) + len(r["naming_violations"])
        for r in all_results
    )

    sys.exit(1 if total_issues > 0 else 0)


if __name__ == "__main__":
    main()
