#!/usr/bin/env python3
"""
Test Naming and Consistency Validator

This script validates test file naming consistency and checks for common issues:
- Test file naming convention (test_<module>_<name>.c)
- Kconfig option naming matches test files
- Orphaned test files or Kconfig options
- Invalid characters in filenames
- Duplicate test names
- Missing test registration
- CMake integration issues

Usage:
    # Validate all tests
    python3 validate_tests.py

    # Validate specific category
    python3 validate_tests.py --category unit

    # Check specific module
    python3 validate_tests.py --category unit --module osal

    # Show detailed suggestions
    python3 validate_tests.py --verbose

    # Check for missing test registration in code
    python3 validate_tests.py --check-registration

Options:
    --category          Category to check (unit, performance, stress, system, all)
    --module            Module to check (osal, hal, pdl, etc.)
    --verbose           Show detailed output and fix suggestions
    --check-registration Check if tests are properly registered in code
    --strict            Enable strict checks (fail on warnings)
"""

import os
import sys
import re
import argparse
from pathlib import Path
from typing import List, Dict, Set, Tuple, Optional
from collections import defaultdict


def get_project_root() -> Path:
    """Get the EMS project root directory."""
    script_dir = Path(__file__).resolve().parent
    return script_dir.parent.parent.parent


def validate_filename(filename: str) -> Tuple[bool, List[str]]:
    """
    Validate test filename follows naming convention.

    Returns: (is_valid, list of issues)
    """
    issues = []

    # Must be .c file
    if not filename.endswith('.c'):
        issues.append(f"Not a C source file (must end with .c)")
        return False, issues

    # Must start with test_
    if not filename.startswith('test_'):
        issues.append(f"Does not start with 'test_' prefix")
        return False, issues

    # Check for invalid characters (only alphanumeric and underscore)
    name_part = filename[:-2]  # Remove .c
    if not re.match(r'^test_[a-z0-9_]+$', name_part):
        issues.append(f"Contains invalid characters (use only lowercase a-z, 0-9, and _)")
        return False, issues

    # Check for double underscores
    if '__' in name_part:
        issues.append(f"Contains double underscores (__)")

    # Check for leading/trailing underscores
    if name_part.endswith('_'):
        issues.append(f"Ends with underscore")

    # Check for reasonable length
    if len(name_part) > 50:
        issues.append(f"Filename too long ({len(name_part)} chars, recommended < 50)")

    # Must have at least module name (test_<module>_something.c)
    parts = name_part.split('_')
    if len(parts) < 3:  # test, module, name
        issues.append(f"Missing module or test name (expected format: test_<module>_<name>.c)")
        return False, issues

    return len(issues) == 0, issues


def check_test_registration(test_file: Path) -> Tuple[bool, List[str]]:
    """
    Check if test file has proper test registration.

    Looks for:
    - TEST_SUITE_REGISTER() or TEST_SUITE_AUTO_REGISTER() or TEST_MODULE_BEGIN/END
    - At least one TEST_CASE() definition
    """
    issues = []

    try:
        with open(test_file, 'r', encoding='utf-8') as f:
            content = f.read()

        # Check for test suite registration (actual code, not comments)
        # Remove comments to avoid false positives
        code_only = re.sub(r'//.*?$', '', content, flags=re.MULTILINE)  # Remove // comments
        code_only = re.sub(r'/\*.*?\*/', '', code_only, flags=re.DOTALL)  # Remove /* */ comments

        has_suite_register = re.search(r'\bTEST_SUITE_REGISTER\s*\(', code_only) is not None
        has_auto_register = re.search(r'\bTEST_SUITE_AUTO_REGISTER\s*\(', code_only) is not None
        has_module_begin = re.search(r'\bTEST_MODULE_BEGIN\s*\(', code_only) is not None
        has_module_end = re.search(r'\bTEST_MODULE_END\s*\(', code_only) is not None

        if not (has_suite_register or has_auto_register or (has_module_begin and has_module_end)):
            issues.append("Missing test registration (no TEST_SUITE_REGISTER/AUTO_REGISTER or TEST_MODULE_BEGIN/END)")

        # Check for test cases
        test_cases = re.findall(r'TEST_CASE\s*\(\s*\w+\s*\)', content)
        if not test_cases:
            issues.append("No TEST_CASE() definitions found")

        # Check if both old and new style are used (inconsistency)
        if (has_suite_register or has_auto_register) and (has_module_begin or has_module_end):
            issues.append("Mixed registration styles (uses both SUITE_REGISTER/AUTO_REGISTER and MODULE_BEGIN/END)")

    except Exception as e:
        issues.append(f"Failed to read file: {e}")

    return len(issues) == 0, issues


def file_to_config_name(filename: str) -> str:
    """Convert test filename to expected Kconfig option name."""
    name = Path(filename).stem
    return f"CONFIG_{name.upper()}"


def find_duplicate_names(test_files: List[Path]) -> Dict[str, List[Path]]:
    """Find test files with duplicate names across different modules."""
    name_map = defaultdict(list)

    for test_file in test_files:
        name_map[test_file.name].append(test_file)

    # Return only duplicates
    return {name: paths for name, paths in name_map.items() if len(paths) > 1}


def parse_kconfig_options(kconfig_path: Path) -> Set[str]:
    """Parse Kconfig file and extract CONFIG_TEST_* options."""
    if not kconfig_path.exists():
        return set()

    configs = set()
    try:
        with open(kconfig_path, 'r', encoding='utf-8') as f:
            for line in f:
                match = re.match(r'\s*config\s+(TEST_\w+)', line)
                if match:
                    config_name = match.group(1)
                    # Skip meta configs like TEST_*_ALL
                    if not config_name.endswith('_ALL'):
                        configs.add(f"CONFIG_{config_name}")
    except Exception as e:
        print(f"Warning: Failed to parse {kconfig_path}: {e}", file=sys.stderr)

    return configs


def check_cmake_integration(module_dir: Path, test_files: Set[str]) -> Tuple[bool, List[str]]:
    """
    Check if CMakeLists.txt properly integrates test files.

    Looks for test_discover_and_add() or manual target_sources() listing.
    """
    issues = []

    # Check multiple possible CMakeLists.txt locations
    cmake_locations = [
        module_dir / "CMakeLists.txt",           # In module dir
        module_dir.parent / "CMakeLists.txt",    # In category dir (unit/)
        module_dir.parent.parent / "CMakeLists.txt",  # In tests/ dir
    ]

    cmake_path = None
    for path in cmake_locations:
        if path.exists():
            cmake_path = path
            break

    if not cmake_path:
        issues.append(f"No CMakeLists.txt found in module, category, or tests directory")
        return False, issues

    try:
        with open(cmake_path, 'r', encoding='utf-8') as f:
            content = f.read()

        # Check for auto-discovery
        has_discovery = 'test_discover_and_add' in content

        if has_discovery:
            # Good, uses auto-discovery
            return True, []

        # Check if test files are manually listed
        missing_files = []
        for test_file in test_files:
            if test_file not in content:
                missing_files.append(test_file)

        if missing_files:
            issues.append(f"Test files not found in CMakeLists.txt: {', '.join(missing_files[:3])}")
            if len(missing_files) > 3:
                issues.append(f"  ... and {len(missing_files) - 3} more")
            issues.append(f"Consider using test_discover_and_add() for automatic discovery")

    except Exception as e:
        issues.append(f"Failed to check CMakeLists.txt: {e}")

    return len(issues) == 0, issues


def validate_module(category: str, module: str, project_root: Path,
                   check_registration: bool = False) -> Dict:
    """
    Validate a single module's tests.

    Returns dict with validation results.
    """
    module_dir = project_root / "products" / "tests" / category / module
    kconfig_path = module_dir / "Kconfig"

    results = {
        "category": category,
        "module": module,
        "test_files": [],
        "kconfig_options": set(),
        "issues": {
            "naming": [],
            "registration": [],
            "missing_config": [],
            "orphaned_config": [],
            "cmake": [],
            "duplicates": []
        },
        "warnings": []
    }

    if not module_dir.exists():
        results["warnings"].append(f"Module directory not found: {module_dir}")
        return results

    # Scan test files
    test_files = sorted(module_dir.glob("test_*.c"))
    results["test_files"] = test_files

    # Validate each test file
    for test_file in test_files:
        # Check naming
        is_valid, naming_issues = validate_filename(test_file.name)
        if not is_valid or naming_issues:
            results["issues"]["naming"].append((test_file.name, naming_issues))

        # Check registration if requested
        if check_registration:
            is_valid, reg_issues = check_test_registration(test_file)
            if not is_valid or reg_issues:
                results["issues"]["registration"].append((test_file.name, reg_issues))

    # Parse Kconfig
    if kconfig_path.exists():
        results["kconfig_options"] = parse_kconfig_options(kconfig_path)
    else:
        results["warnings"].append(f"Kconfig not found: {kconfig_path}")

    # Check Kconfig consistency
    test_file_names = {f.name for f in test_files}

    # Missing Kconfig options
    for test_file in test_files:
        expected_config = file_to_config_name(test_file.name)
        if expected_config not in results["kconfig_options"]:
            results["issues"]["missing_config"].append((test_file.name, expected_config))

    # Orphaned Kconfig options
    for config in results["kconfig_options"]:
        # Derive expected filename from config
        expected_file = config[7:].lower() + ".c"  # Remove CONFIG_ prefix
        if expected_file not in test_file_names:
            results["issues"]["orphaned_config"].append((config, expected_file))

    # Check CMake integration
    is_valid, cmake_issues = check_cmake_integration(module_dir, test_file_names)
    if not is_valid or cmake_issues:
        results["issues"]["cmake"] = cmake_issues

    return results


def find_all_duplicates(category: str, project_root: Path) -> Dict[str, List[Path]]:
    """Find duplicate test file names across all modules in a category."""
    category_dir = project_root / "products" / "tests" / category

    if not category_dir.exists():
        return {}

    all_test_files = []
    for module_dir in category_dir.iterdir():
        if module_dir.is_dir() and not module_dir.name.startswith('.'):
            all_test_files.extend(module_dir.glob("test_*.c"))

    return find_duplicate_names(all_test_files)


def print_module_results(results: Dict, verbose: bool = False):
    """Print validation results for a module."""
    category = results["category"]
    module = results["module"]
    issues = results["issues"]

    # Count total issues
    total_issues = (
        len(issues["naming"]) +
        len(issues["registration"]) +
        len(issues["missing_config"]) +
        len(issues["orphaned_config"]) +
        len(issues["cmake"])
    )

    # Print header
    print(f"\n{'='*80}")
    print(f"Module: {category}/{module}")
    print(f"{'='*80}")
    print(f"Test files: {len(results['test_files'])}")
    print(f"Kconfig options: {len(results['kconfig_options'])}")

    # Print warnings
    for warning in results["warnings"]:
        print(f"⚠️  {warning}")

    if total_issues == 0:
        print(f"✅ All checks passed!")
        return

    # Print naming issues
    if issues["naming"]:
        print(f"\n❌ Naming convention violations: {len(issues['naming'])}")
        for filename, file_issues in issues["naming"]:
            print(f"  • {filename}")
            for issue in file_issues:
                print(f"    - {issue}")
            if verbose:
                print(f"    Fix: Rename to follow test_<module>_<name>.c convention")

    # Print registration issues
    if issues["registration"]:
        print(f"\n❌ Test registration issues: {len(issues['registration'])}")
        for filename, reg_issues in issues["registration"]:
            print(f"  • {filename}")
            for issue in reg_issues:
                print(f"    - {issue}")
            if verbose:
                print(f"    Fix: Add TEST_SUITE_AUTO_REGISTER() macro to the file")

    # Print missing configs
    if issues["missing_config"]:
        print(f"\n❌ Test files without Kconfig: {len(issues['missing_config'])}")
        for filename, config_name in issues["missing_config"]:
            print(f"  • {filename} -> missing {config_name}")
            if verbose:
                print(f"    Fix: Add to {category}/{module}/Kconfig or run sync_kconfig.py")

    # Print orphaned configs
    if issues["orphaned_config"]:
        print(f"\n❌ Kconfig options without files: {len(issues['orphaned_config'])}")
        for config_name, expected_file in issues["orphaned_config"]:
            print(f"  • {config_name} -> missing {expected_file}")
            if verbose:
                print(f"    Fix: Create {expected_file} or remove config from Kconfig")

    # Print CMake issues
    if issues["cmake"]:
        print(f"\n❌ CMake integration issues:")
        for issue in issues["cmake"]:
            print(f"  - {issue}")


def print_summary(all_results: List[Dict], duplicates: Dict[str, List[Path]]):
    """Print overall summary."""
    total_files = sum(len(r["test_files"]) for r in all_results)
    total_issues = sum(
        len(r["issues"]["naming"]) +
        len(r["issues"]["registration"]) +
        len(r["issues"]["missing_config"]) +
        len(r["issues"]["orphaned_config"]) +
        len(r["issues"]["cmake"])
        for r in all_results
    )

    print(f"\n{'='*80}")
    print(f"Summary")
    print(f"{'='*80}")
    print(f"Total test files: {total_files}")
    print(f"Total issues: {total_issues}")

    # Breakdown
    naming_issues = sum(len(r["issues"]["naming"]) for r in all_results)
    reg_issues = sum(len(r["issues"]["registration"]) for r in all_results)
    missing_config = sum(len(r["issues"]["missing_config"]) for r in all_results)
    orphaned_config = sum(len(r["issues"]["orphaned_config"]) for r in all_results)
    cmake_issues = sum(len(r["issues"]["cmake"]) for r in all_results)

    print(f"  - Naming violations: {naming_issues}")
    print(f"  - Registration issues: {reg_issues}")
    print(f"  - Missing Kconfig: {missing_config}")
    print(f"  - Orphaned Kconfig: {orphaned_config}")
    print(f"  - CMake issues: {cmake_issues}")

    # Duplicates
    if duplicates:
        print(f"\n⚠️  Duplicate test names found: {len(duplicates)}")
        for name, paths in duplicates.items():
            print(f"  • {name}")
            for path in paths:
                print(f"    - {path.parent.name}/{name}")

    if total_issues == 0 and not duplicates:
        print(f"\n✅ All validations passed!")
    else:
        print(f"\n⚠️  Please fix the issues above.")
        print(f"\nNext steps:")
        print(f"  1. Fix naming convention violations")
        print(f"  2. Add missing test registration macros")
        print(f"  3. Run: python3 sync_kconfig.py --write (to auto-generate Kconfig)")
        print(f"  4. Run: python3 validate_test_config.py (to verify)")


def main():
    parser = argparse.ArgumentParser(
        description="Validate test naming and consistency",
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--category", choices=["unit", "performance", "stress", "system", "all"],
                       default="all", help="Category to validate")
    parser.add_argument("--module", help="Specific module to check")
    parser.add_argument("--verbose", "-v", action="store_true",
                       help="Show detailed output and suggestions")
    parser.add_argument("--check-registration", action="store_true",
                       help="Check if tests have proper registration")
    parser.add_argument("--strict", action="store_true",
                       help="Enable strict mode (warnings become errors)")

    args = parser.parse_args()

    project_root = get_project_root()

    # Determine categories
    if args.category == "all":
        categories = ["unit", "performance", "stress", "system"]
    else:
        categories = [args.category]

    all_results = []

    for category in categories:
        # Get modules
        if args.module:
            modules = [args.module]
        else:
            category_dir = project_root / "products" / "tests" / category
            if not category_dir.exists():
                continue
            modules = [d.name for d in category_dir.iterdir()
                      if d.is_dir() and not d.name.startswith('.')]

        # Find duplicates across all modules in category
        duplicates = find_all_duplicates(category, project_root)

        # Validate each module
        for module in modules:
            results = validate_module(category, module, project_root, args.check_registration)
            all_results.append(results)
            print_module_results(results, args.verbose)

    # Print summary
    duplicates = {}
    for category in categories:
        duplicates.update(find_all_duplicates(category, project_root))

    print_summary(all_results, duplicates)

    # Exit code
    total_issues = sum(
        len(r["issues"]["naming"]) +
        len(r["issues"]["registration"]) +
        len(r["issues"]["missing_config"]) +
        len(r["issues"]["orphaned_config"]) +
        len(r["issues"]["cmake"])
        for r in all_results
    )

    if args.strict and any(r["warnings"] for r in all_results):
        total_issues += 1

    sys.exit(1 if total_issues > 0 or duplicates else 0)


if __name__ == "__main__":
    main()
