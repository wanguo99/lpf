#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
ES-Middleware SDK Root Build Script
Unified build entry point for all products

Version: 2.3 (Optimized Performance & Error Handling)
"""

import argparse
import os
import sys
import subprocess
from pathlib import Path
from multiprocessing import cpu_count
import shutil
import time
import hashlib

def get_root_dir():
    """获取项目根目录"""
    return Path(__file__).parent.resolve()

def get_products():
    """获取所有可用的产品"""
    products_dir = get_root_dir() / "products"
    if not products_dir.exists():
        return []

    products = []
    for item in products_dir.iterdir():
        if item.is_dir() and (item / "CMakeLists.txt").exists():
            products.append(item.name)
    return sorted(products)

def get_configs():
    """获取所有可用的配置"""
    configs_dir = get_root_dir() / "configs"
    if not configs_dir.exists():
        return []

    configs = []
    for item in configs_dir.iterdir():
        if item.is_file() and item.name.endswith("_defconfig"):
            configs.append(item.name)
    return sorted(configs)

def parse_config_metadata(config_file):
    """解析配置文件元数据（从注释中提取描述信息）"""
    metadata = {
        'name': config_file.name,
        'description': '',
        'platform': '',
        'build_type': ''
    }

    try:
        with open(config_file, 'r') as f:
            for line in f:
                line = line.strip()
                if line.startswith('#'):
                    # Extract metadata from comments
                    if 'Description:' in line:
                        metadata['description'] = line.split('Description:', 1)[1].strip()
                    elif 'Platform:' in line:
                        metadata['platform'] = line.split('Platform:', 1)[1].strip()
                    elif 'Build:' in line:
                        metadata['build_type'] = line.split('Build:', 1)[1].strip()
                elif line.startswith('CONFIG_'):
                    # Stop at first config line
                    break
    except Exception as e:
        print(f"Warning: Failed to parse metadata from {config_file.name}: {e}", file=sys.stderr)

    return metadata

def list_configs_detailed():
    """列出所有配置并显示详细信息"""
    configs_dir = get_root_dir() / "configs"
    if not configs_dir.exists():
        print("No configurations found")
        return

    configs = []
    for item in configs_dir.iterdir():
        if item.is_file() and item.name.endswith("_defconfig"):
            metadata = parse_config_metadata(item)
            configs.append(metadata)

    if not configs:
        print("No configurations found")
        return

    # Sort by name
    configs.sort(key=lambda x: x['name'])

    # Group by type
    product_configs = [c for c in configs if not c['name'].startswith('tests_')]
    test_configs = [c for c in configs if c['name'].startswith('tests_')]

    if product_configs:
        print("\nProduct Configurations:")
        print("-" * 80)
        for cfg in product_configs:
            name = cfg['name'].replace('_defconfig', '')
            print(f"  {name}")
            if cfg['description']:
                print(f"    Description: {cfg['description']}")
            if cfg['platform']:
                print(f"    Platform:    {cfg['platform']}")
            if cfg['build_type']:
                print(f"    Build Type:  {cfg['build_type']}")
            print()

    if test_configs:
        print("Test Configurations:")
        print("-" * 80)
        for cfg in test_configs:
            name = cfg['name'].replace('_defconfig', '')
            print(f"  {name}")
            if cfg['description']:
                print(f"    Description: {cfg['description']}")
            print()

def validate_config(config_file):
    """验证配置文件格式和完整性"""
    if not config_file.exists():
        return False, f"Config file not found: {config_file}"

    # Basic syntax check
    try:
        config_lines = 0
        invalid_lines = []
        with open(config_file, 'r') as f:
            for line_num, line in enumerate(f, 1):
                stripped = line.strip()
                if not stripped or stripped.startswith('#'):
                    continue
                # Check if it's a valid config line
                if not (stripped.startswith('CONFIG_') or stripped.startswith('# CONFIG_')):
                    invalid_lines.append((line_num, stripped[:50]))
                    if len(invalid_lines) >= 5:  # Limit error reporting
                        break
                config_lines += 1

        if invalid_lines:
            error_msg = "Invalid syntax found:\n"
            for line_num, content in invalid_lines:
                error_msg += f"  Line {line_num}: {content}\n"
            return False, error_msg.rstrip()

        if config_lines == 0:
            return False, "Configuration file is empty (no CONFIG_* lines)"

        return True, f"Valid ({config_lines} config lines)"
    except Exception as e:
        return False, f"Error reading config file: {e}"

def check_current_config():
    """检查当前配置状态"""
    root_dir = get_root_dir()
    dotconfig = root_dir / ".config"

    if not dotconfig.exists():
        return None, "No configuration loaded"

    # Try to identify which defconfig this came from
    configs_dir = root_dir / "configs"
    if configs_dir.exists():
        for defconfig in configs_dir.glob("*_defconfig"):
            try:
                with open(dotconfig, 'r') as f1, open(defconfig, 'r') as f2:
                    # Simple comparison (ignoring comments and blank lines)
                    lines1 = [l.strip() for l in f1 if l.strip() and not l.strip().startswith('#')]
                    lines2 = [l.strip() for l in f2 if l.strip() and not l.strip().startswith('#')]
                    if lines1 == lines2:
                        return defconfig.name, "Matches defconfig"
            except:
                pass

    # Count enabled options
    try:
        with open(dotconfig, 'r') as f:
            enabled = sum(1 for l in f if l.strip().startswith('CONFIG_') and '=y' in l)
        return "custom", f"Custom configuration ({enabled} options enabled)"
    except:
        return "unknown", "Configuration exists but cannot be read"

def compute_file_hash(filepath):
    """计算文件的 MD5 哈希值用于缓存检测"""
    try:
        with open(filepath, 'rb') as f:
            return hashlib.md5(f.read()).hexdigest()
    except:
        return None

def check_kconfig_tools_cache(build_dir):
    """检查 Kconfig 工具是否需要重新构建"""
    build_path = get_root_dir() / build_dir
    conf_tool = build_path / "kconfig-tools-build" / "conf"

    if not conf_tool.exists():
        return False, "Kconfig tools not built"

    # Check if tool is executable and recent
    if not os.access(conf_tool, os.X_OK):
        return False, "Kconfig tool not executable"

    # Tools exist and are valid
    return True, "Kconfig tools cached"

def load_config_cmake(config_name, build_dir="_build", force_rebuild=False):
    """使用 CMake + Kconfig 加载配置文件（优化版本）"""
    root_dir = get_root_dir()
    build_path = root_dir / build_dir

    # Resolve config file path
    if config_name.endswith('_defconfig'):
        config_file = root_dir / "configs" / config_name
    else:
        config_file = root_dir / "configs" / f"{config_name}_defconfig"

    if not config_file.exists():
        print(f"Error: Config file not found: {config_file}")
        print(f"\nAvailable configurations:")
        list_configs_detailed()
        return False

    # Validate config file
    start_time = time.time()
    valid, msg = validate_config(config_file)
    if not valid:
        print(f"Error: Configuration validation failed")
        print(f"  {msg}")
        return False

    display_name = config_name.replace("_defconfig", "") if config_name.endswith("_defconfig") else config_name
    print(f"\nLoading configuration: {display_name}")
    print(f"  Config file: {config_file}")
    print(f"  Validation: {msg}")

    # Copy config to .config
    dotconfig = root_dir / ".config"
    shutil.copy2(config_file, dotconfig)
    print(f"  Created .config: {dotconfig}")

    # Check if we can reuse Kconfig tools
    tools_cached, cache_msg = check_kconfig_tools_cache(build_dir)
    if tools_cached and not force_rebuild:
        print(f"  Using cached Kconfig tools")
    else:
        print(f"  Building Kconfig tools... ({cache_msg})")

    # Run CMake to build kconfig tools and process the configuration
    build_path.mkdir(parents=True, exist_ok=True)

    print(f"  Running CMake configuration...")

    # Set environment variables for Kconfig
    env = os.environ.copy()
    env['srctree'] = str(root_dir)
    env['KCONFIG_CONFIG'] = str(dotconfig)

    cmake_cmd = [
        "cmake",
        "-S", str(root_dir),
        "-B", str(build_path),
        "-G", "Unix Makefiles",
        "-DCMAKE_BUILD_TYPE=Debug",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    ]

    result = subprocess.run(
        cmake_cmd,
        cwd=root_dir,
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True
    )

    if result.returncode != 0:
        print("\nError: CMake configuration failed!")
        print("\nCMake output:")
        print(result.stdout)
        print("\nTroubleshooting:")
        print("  1. Check if CMake 3.16+ is installed: cmake --version")
        print("  2. Check if ncurses-dev is installed (for menuconfig)")
        print("  3. Try clean rebuild: python3 build.py distclean")
        return False

    # Now normalize the config using the built conf tool to generate derived values
    conf_tool = build_path / "kconfig-tools-build" / "conf"
    if conf_tool.exists():
        print(f"  Normalizing configuration...")
        result = subprocess.run(
            [str(conf_tool), "--olddefconfig", "Kconfig"],
            cwd=root_dir,
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True
        )

        if result.returncode != 0:
            print(f"  Warning: Config normalization failed: {result.stdout}")
        else:
            # Re-run CMake to process the normalized config
            result = subprocess.run(
                cmake_cmd,
                cwd=root_dir,
                env=env,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True
            )

            if result.returncode != 0:
                print("\nError: CMake reconfiguration after normalization failed!")
                print("\nCMake output:")
                print(result.stdout)
                return False

    # Check if autoconf.h was generated
    autoconf_h = build_path / "config" / "autoconf.h"
    if autoconf_h.exists():
        try:
            with open(autoconf_h, 'r') as f:
                defines = sum(1 for l in f if l.strip().startswith('#define CONFIG_'))
            print(f"  Generated: autoconf.h ({defines} CONFIG_* defines)")
        except:
            print(f"  Generated: autoconf.h")
    else:
        print(f"  Warning: autoconf.h not generated")

    # Check if kconfig.cmake was generated
    kconfig_cmake = build_path / "config" / "kconfig.cmake"
    if kconfig_cmake.exists():
        try:
            with open(kconfig_cmake, 'r') as f:
                content = f.read()
                config_count = content.count('set(CONFIG_')
            print(f"  Generated: kconfig.cmake ({config_count} CMake variables)")
        except:
            print(f"  Generated: kconfig.cmake")

    elapsed = time.time() - start_time
    print(f"\n✓ Configuration loaded successfully in {elapsed:.2f}s")
    print(f"\nNext steps:")
    print(f"  Build:        python3 build.py build")
    print(f"  Customize:    python3 build.py menuconfig")
    print(f"  Save changes: python3 build.py savedefconfig")
    return True

def run_cmake_target(target_name, build_dir="_build", capture_output=False):
    """运行 CMake 自定义目标（menuconfig, nconfig, savedefconfig 等）"""
    root_dir = get_root_dir()
    build_path = root_dir / build_dir

    # Check if build directory exists and is configured
    if not (build_path / "Makefile").exists():
        print(f"Error: Build directory not configured")
        print(f"\nPlease run: python3 build.py config <config_name>")
        print(f"\nAvailable configurations:")
        for cfg in get_configs()[:5]:  # Show first 5
            print(f"  - {cfg.replace('_defconfig', '')}")
        if len(get_configs()) > 5:
            print(f"  ... and {len(get_configs()) - 5} more")
        print(f"\nTo see all: python3 build.py --list")
        return False, None

    # Set environment variables
    env = os.environ.copy()
    env['srctree'] = str(root_dir)

    if not capture_output:
        print(f"Running: make {target_name}")

    # Run make target
    kwargs = {'cwd': build_path, 'env': env}
    if capture_output:
        kwargs['capture_output'] = True
        kwargs['text'] = True

    result = subprocess.run(["make", target_name], **kwargs)

    if result.returncode != 0 and not capture_output:
        print(f"\nError: 'make {target_name}' failed with code {result.returncode}")
        if target_name in ['menuconfig', 'nconfig']:
            print("\nTroubleshooting:")
            print("  - Ensure ncurses-dev is installed: sudo apt install libncurses-dev")
            print("  - Try rebuilding tools: python3 build.py distclean && python3 build.py config <name>")

    if capture_output:
        return result.returncode == 0, result.stdout
    else:
        return result.returncode == 0, None

def menuconfig(build_dir="_build"):
    """打开 Kconfig menuconfig 界面"""
    root_dir = get_root_dir()
    build_path = root_dir / build_dir

    # Set environment and run menuconfig
    env = os.environ.copy()
    env['srctree'] = str(root_dir)

    result = subprocess.run(
        ["make", "menuconfig"],
        cwd=build_path,
        env=env,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL
    )

    if result.returncode == 0:
        print("✓ Configuration updated")
    else:
        print(f"Error: menuconfig failed (code {result.returncode})")
        print("Install ncurses: sudo apt install libncurses-dev")

    return result.returncode == 0

def nconfig(build_dir="_build"):
    """打开 Kconfig nconfig 界面（ncurses 界面）"""
    root_dir = get_root_dir()
    build_path = root_dir / build_dir

    # Set environment and run nconfig
    env = os.environ.copy()
    env['srctree'] = str(root_dir)

    result = subprocess.run(
        ["make", "nconfig"],
        cwd=build_path,
        env=env,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL
    )

    if result.returncode == 0:
        print("✓ Configuration updated")
    else:
        print(f"Error: nconfig failed (code {result.returncode})")
        print("Install ncurses: sudo apt install libncurses-dev")

    return result.returncode == 0

def savedefconfig(output_name=None, build_dir="_build"):
    """保存最小化配置（只保存非默认值）"""
    root_dir = get_root_dir()
    build_path = root_dir / build_dir

    # Check if .config exists
    dotconfig = root_dir / ".config"
    if not dotconfig.exists():
        print("Error: No configuration to save (.config not found)")
        print("\nPlease load a configuration first:")
        print("  python3 build.py config <config_name>")
        return False

    print("Generating minimal defconfig...")

    # Run savedefconfig target
    success, _ = run_cmake_target("savedefconfig", build_dir)

    if not success:
        return False

    # Check if defconfig was generated
    defconfig = root_dir / "defconfig"
    if not defconfig.exists():
        print("Error: defconfig file not generated")
        return False

    # Count lines
    try:
        with open(defconfig, 'r') as f:
            lines = [l.strip() for l in f if l.strip() and not l.strip().startswith('#')]
        print(f"  Generated: defconfig ({len(lines)} config lines)")
    except:
        pass

    # If output name provided, copy to configs directory
    if output_name:
        if not output_name.endswith('_defconfig'):
            output_name = f"{output_name}_defconfig"

        output_path = root_dir / "configs" / output_name

        # Check if file exists and prompt for overwrite
        if output_path.exists():
            print(f"\nWarning: {output_path} already exists")
            response = input("Overwrite? [y/N]: ").strip().lower()
            if response != 'y':
                print("Cancelled")
                return False

        shutil.copy2(defconfig, output_path)
        print(f"  Saved as:  {output_path}")
        print(f"\n✓ Configuration saved: {output_name}")
    else:
        print(f"\n✓ Minimal defconfig generated: {defconfig}")
        print(f"\nTo save permanently:")
        print(f"  python3 build.py savedefconfig <name>")

    return True

def oldconfig(build_dir="_build"):
    """更新配置（处理新增的 Kconfig 选项）"""
    root_dir = get_root_dir()
    dotconfig = root_dir / ".config"

    if not dotconfig.exists():
        print("Error: No configuration found (.config not found)")
        print("\nPlease load a configuration first:")
        print("  python3 build.py config <config_name>")
        return False

    print("Updating configuration with new Kconfig options...")

    success, _ = run_cmake_target("oldconfig", build_dir)

    if success:
        print(f"\n✓ Configuration updated")

    return success

def build(config=None, build_dir="_build", jobs=None, verbose=False):
    """构建项目"""
    root_dir = get_root_dir()
    build_path = root_dir / build_dir

    # If config specified, load it
    if config:
        if not load_config_cmake(config, build_dir):
            return False
        print()  # Blank line separator

    # Check if .config exists
    dotconfig = root_dir / ".config"
    if not dotconfig.exists():
        print("Error: No configuration found")
        print("\nAvailable configurations:")
        list_configs_detailed()
        print("\nPlease load a configuration first:")
        print("  python3 build.py config <config_name>")
        return False

    # Check if CMake is configured
    if not (build_path / "Makefile").exists():
        print("CMake not configured, running configuration...")
        # Re-run CMake with current .config
        build_path.mkdir(parents=True, exist_ok=True)

        env = os.environ.copy()
        env['srctree'] = str(root_dir)
        env['KCONFIG_CONFIG'] = str(dotconfig)

        cmake_cmd = [
            "cmake",
            "-S", str(root_dir),
            "-B", str(build_path),
            "-G", "Unix Makefiles",
            "-DCMAKE_BUILD_TYPE=Debug",
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
        ]

        result = subprocess.run(cmake_cmd, cwd=root_dir, env=env)
        if result.returncode != 0:
            print("\nError: CMake configuration failed!")
            return False

    print("Building ES-Middleware...")

    # Set environment variables
    env = os.environ.copy()
    env['srctree'] = str(root_dir)

    # Determine number of jobs
    if jobs is None:
        jobs = cpu_count()

    print(f"  Using {jobs} parallel jobs")

    # Build command
    build_cmd = ["cmake", "--build", str(build_path), "--", f"-j{jobs}"]
    if verbose:
        build_cmd.append("VERBOSE=1")

    start_time = time.time()
    result = subprocess.run(build_cmd, cwd=root_dir, env=env)

    if result.returncode != 0:
        print("\nError: Build failed!")
        print("\nTroubleshooting:")
        print("  1. Check error messages above for specific compilation errors")
        print("  2. Try verbose build: python3 build.py build --verbose")
        print("  3. Try clean rebuild: python3 build.py clean && python3 build.py build")
        return False

    elapsed = time.time() - start_time

    # Show build outputs
    bin_dir = build_path / "bin"
    lib_dir = build_path / "lib"

    print(f"\n✓ Build successful in {elapsed:.2f}s!")
    print(f"\nOutput directories:")
    print(f"  Binaries:  {bin_dir}")
    print(f"  Libraries: {lib_dir}")

    # List executables
    if bin_dir.exists():
        executables = list(bin_dir.glob("*"))
        if executables:
            print(f"\nExecutables:")
            for exe in sorted(executables):
                if exe.is_file() and os.access(exe, os.X_OK):
                    size = exe.stat().st_size
                    size_kb = size / 1024
                    print(f"  {exe.name:30s} ({size_kb:>7.1f} KB)")

    return True

def clean(build_dir="_build"):
    """清理构建目录"""
    root_dir = get_root_dir()
    build_path = root_dir / build_dir

    if build_path.exists():
        print(f"Cleaning build directory: {build_path}")
        try:
            shutil.rmtree(build_path)
            print("✓ Clean complete")
        except Exception as e:
            print(f"Error: Failed to clean build directory: {e}")
            return False
    else:
        print(f"Build directory does not exist: {build_path}")

    return True

def distclean(build_dir="_build"):
    """完全清理（包括配置文件）"""
    root_dir = get_root_dir()

    # Clean build directory
    clean(build_dir)

    # Remove .config
    dotconfig = root_dir / ".config"
    if dotconfig.exists():
        print(f"Removing configuration: {dotconfig}")
        dotconfig.unlink()

    # Remove defconfig
    defconfig = root_dir / "defconfig"
    if defconfig.exists():
        print(f"Removing defconfig: {defconfig}")
        defconfig.unlink()

    # Remove Python cache
    pycache = root_dir / "__pycache__"
    if pycache.exists():
        print(f"Removing Python cache: {pycache}")
        shutil.rmtree(pycache)

    print("✓ Distclean complete")
    return True

def list_all():
    """列出所有产品和配置"""
    products = get_products()

    print("\n" + "=" * 80)
    print("ES-Middleware SDK - Available Configurations")
    print("=" * 80)

    if products:
        print("\nProducts:")
        for product in products:
            print(f"  - {product}")

    list_configs_detailed()

    print("\nUsage:")
    print("  python3 build.py config <config_name>  # Load configuration")
    print("  python3 build.py menuconfig            # Interactive configuration")
    print("  python3 build.py build                 # Build with current config")
    print("  python3 build.py build -c <config>     # Load config and build")
    print()

def list_defconfigs_cmake(build_dir="_build"):
    """使用 CMake 目标列出所有可用的 defconfig（需要已配置的构建）"""
    root_dir = get_root_dir()
    build_path = root_dir / build_dir

    # Check if build directory exists and is configured
    if not (build_path / "Makefile").exists():
        print("Note: Build directory not configured yet")
        print("Using local configs directory instead\n")
        list_configs_detailed()
        return True

    print("Available defconfigs (from CMake):\n")

    # Run list_defconfigs target
    success, output = run_cmake_target("list_defconfigs", build_dir, capture_output=True)

    if success and output:
        print(output)
    else:
        print("Failed to retrieve defconfigs from CMake")
        print("Falling back to local configs directory:\n")
        list_configs_detailed()

    return True

def main():
    parser = argparse.ArgumentParser(
        description="ES-Middleware SDK Build Tool (v2.3 - Optimized)",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # List all configurations with details
  python3 build.py --list

  # Load a configuration
  python3 build.py config ccm_h200_100p_am625_debug

  # Open menuconfig (graphical configuration)
  python3 build.py menuconfig

  # Build with current configuration
  python3 build.py build

  # Build with specific configuration (load + build)
  python3 build.py build --config tests_x86_full_defconfig

  # Save current configuration
  python3 build.py savedefconfig my_custom_config

  # Clean build directory
  python3 build.py clean

  # Complete clean (build + .config)
  python3 build.py distclean

Configuration workflow:
  1. python3 build.py config <name>       # Load base config
  2. python3 build.py menuconfig          # Customize (optional)
  3. python3 build.py build               # Build
  4. python3 build.py savedefconfig <name> # Save changes (optional)
        """
    )

    parser.add_argument("cmd", nargs="?", default="build",
                        choices=["build", "menuconfig", "nconfig", "config",
                                "savedefconfig", "oldconfig", "clean", "distclean"],
                        help="Command to execute")
    parser.add_argument("config_name", nargs="?",
                        help="Configuration name (for config/savedefconfig commands)")
    parser.add_argument("--list", action="store_true",
                        help="List all available configurations with details")
    parser.add_argument("--config", "-c", type=str,
                        help="Configuration to load before building")
    parser.add_argument("--build-dir", "-b", type=str, default="_build",
                        help="Build directory (default: _build)")
    parser.add_argument("--jobs", "-j", type=int,
                        help="Number of parallel jobs (default: CPU count)")
    parser.add_argument("--verbose", "-v", action="store_true",
                        help="Verbose build output")

    args = parser.parse_args()

    # List configurations
    if args.list:
        list_all()
        return 0

    # Commands
    try:
        if args.cmd == "menuconfig":
            success = menuconfig(args.build_dir)
            return 0 if success else 1

        if args.cmd == "nconfig":
            success = nconfig(args.build_dir)
            return 0 if success else 1

        if args.cmd == "config":
            if not args.config_name:
                print("Error: Configuration name required")
                print("Usage: python3 build.py config <config_name>")
                print("\nAvailable configurations:")
                list_configs_detailed()
                return 1
            success = load_config_cmake(args.config_name, args.build_dir)
            return 0 if success else 1

        if args.cmd == "savedefconfig":
            success = savedefconfig(args.config_name, args.build_dir)
            return 0 if success else 1

        if args.cmd == "oldconfig":
            success = oldconfig(args.build_dir)
            return 0 if success else 1

        if args.cmd == "clean":
            success = clean(args.build_dir)
            return 0 if success else 1

        if args.cmd == "distclean":
            success = distclean(args.build_dir)
            return 0 if success else 1

        if args.cmd == "build":
            success = build(
                config=args.config,
                build_dir=args.build_dir,
                jobs=args.jobs,
                verbose=args.verbose
            )
            return 0 if success else 1

    except KeyboardInterrupt:
        print("\n\nInterrupted by user")
        return 130
    except Exception as e:
        print(f"\nUnexpected error: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        return 1

    return 0

if __name__ == "__main__":
    sys.exit(main())
