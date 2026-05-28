#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
EMS SDK Root Build Script
Unified build entry point for all products
"""

import argparse
import os
import sys
import subprocess
from pathlib import Path
from multiprocessing import cpu_count

def get_products():
    """获取所有可用的产品"""
    products_dir = Path(__file__).parent / "products"
    if not products_dir.exists():
        return []

    products = []
    for item in products_dir.iterdir():
        if item.is_dir() and (item / "CMakeLists.txt").exists():
            products.append(item.name)
    return sorted(products)

def get_configs():
    """获取所有可用的配置"""
    configs_dir = Path(__file__).parent / "configs"
    if not configs_dir.exists():
        return []

    configs = []
    for item in configs_dir.iterdir():
        if item.is_file() and item.name.endswith("_defconfig"):
            config_name = item.name.replace("_defconfig", "")
            configs.append(config_name)
    return sorted(configs)

def menuconfig():
    """打开 menuconfig 配置界面"""
    root_dir = Path(__file__).parent

    print("Opening menuconfig for EMS SDK")
    print(f"SDK directory: {root_dir}")
    print()

    # 确保 build 目录存在
    build_dir = root_dir / "build"
    build_dir.mkdir(exist_ok=True)

    # 调用 kconfig 工具
    tool_path = root_dir / "tools/kconfig/genconfig.py"
    if not tool_path.exists():
        print(f"Error: kconfig tool not found: {tool_path}")
        return False

    # 获取默认配置文件
    config_files = []
    config_mk = root_dir / ".config.mk"
    if config_mk.exists():
        config_files.append(str(config_mk))

    config_out_path = build_dir / "config"
    config_out_path.mkdir(exist_ok=True)

    cmd = [
        sys.executable, str(tool_path),
        "--kconfig", str(root_dir / "Kconfig"),
        "--menuconfig", "True",
        "--env", f"SDK_PATH={root_dir}",
        "--env", f"PROJECT_PATH={root_dir}",
        "--env", "BUILD_TYPE=Debug",
        "--output", "makefile", str(config_out_path / "global_config.mk"),
        "--output", "cmake", str(config_out_path / "global_config.cmake"),
        "--output", "header", str(config_out_path / "global_config.h")
    ]

    for path in config_files:
        cmd.extend(["--defaults", path])

    result = subprocess.run(cmd, cwd=root_dir)

    if result.returncode != 0:
        print("\nMenuconfig exited with error!")
        return False

    print("\nConfiguration saved.")
    return True

def load_config(config_name, build_dir="_build"):
    """加载配置文件（不编译）"""
    root_dir = Path(__file__).parent
    build_path = root_dir / build_dir

    config_file = root_dir / "configs" / f"{config_name}_defconfig"
    if not config_file.exists():
        print(f"Error: Config file not found: {config_file}")
        return False

    print(f"Loading config: {config_name}")

    # 使用 kconfig 工具加载配置
    tool_path = root_dir / "tools/kconfig/genconfig.py"
    if not tool_path.exists():
        print(f"Error: kconfig tool not found: {tool_path}")
        return False

    build_path.mkdir(exist_ok=True)
    config_out_path = build_path / "config"
    config_out_path.mkdir(exist_ok=True)

    cmd = [
        sys.executable, str(tool_path),
        "--kconfig", str(root_dir / "Kconfig"),
        "--defaults", str(config_file),
        "--env", f"SDK_PATH={root_dir}",
        "--env", f"PROJECT_PATH={root_dir}",
        "--env", "BUILD_TYPE=Debug",
        "--output", "makefile", str(config_out_path / "global_config.mk"),
        "--output", "cmake", str(config_out_path / "global_config.cmake"),
        "--output", "header", str(config_out_path / "global_config.h")
    ]

    result = subprocess.run(cmd, cwd=root_dir)
    if result.returncode != 0:
        print("Failed to load configuration!")
        return False

    print(f"Configuration loaded: {config_name}")
    print("Run 'python3 build.py build' to compile")
    return True

def build(config=None, build_dir="_build", clean=False, jobs=None, verbose=False):
    """构建项目"""
    root_dir = Path(__file__).parent
    build_path = root_dir / build_dir

    # 如果只是清理，执行清理后返回
    if clean:
        if build_path.exists():
            print(f"Cleaning build directory: {build_path}")
            import shutil
            shutil.rmtree(build_path)
        else:
            print(f"Build directory does not exist: {build_path}")

        print("Clean complete.")
        return True

    # 如果指定了配置，加载配置文件
    if config:
        if not load_config(config, build_dir):
            return False

    # 检查是否有配置文件
    config_cmake = build_path / "config" / "global_config.cmake"
    if not config_cmake.exists():
        print("No configuration found.")
        print("\nAvailable configurations:")

        configs = get_configs()
        if not configs:
            print("  No configurations available")
            return False

        for i, cfg in enumerate(configs, 1):
            print(f"  {i}. {cfg}")

        print("\nPlease select a configuration:")
        print("  Enter number (1-{}), or 'q' to quit".format(len(configs)))

        try:
            choice = input("Your choice: ").strip()
            if choice.lower() == 'q':
                print("Build cancelled.")
                return False

            idx = int(choice) - 1
            if idx < 0 or idx >= len(configs):
                print(f"Error: Invalid choice. Please enter 1-{len(configs)}")
                return False

            selected_config = configs[idx]
            print(f"\nSelected: {selected_config}")

            # 加载选中的配置
            if not load_config(selected_config, build_dir):
                return False

            print()  # 空行分隔

        except ValueError:
            print("Error: Please enter a valid number")
            return False
        except KeyboardInterrupt:
            print("\nBuild cancelled.")
            return False

    print("Building EMS SDK...")

    # 配置 CMake
    build_path.mkdir(exist_ok=True)

    if not (build_path / "Makefile").exists():
        print("Configuring CMake...")
        cmake_cmd = [
            "cmake",
            "-G", "Unix Makefiles",
            "-DCMAKE_BUILD_TYPE=Debug",
            str(root_dir)
        ]

        result = subprocess.run(cmake_cmd, cwd=build_path)
        if result.returncode != 0:
            print("CMake configuration failed!")
            return False

    # 编译
    print("Compiling...")
    if jobs is None:
        jobs = cpu_count()

    build_cmd = ["cmake", "--build", ".", "--", f"-j{jobs}"]
    if verbose:
        build_cmd.append("VERBOSE=1")

    result = subprocess.run(build_cmd, cwd=build_path)
    if result.returncode != 0:
        print("Build failed!")
        return False

    print(f"\nBuild successful!")
    print(f"Output directory: {build_path}")

    return True

def list_all():
    """列出所有产品和配置"""
    products = get_products()
    configs = get_configs()

    print("Available products:")
    for product in products:
        print(f"  - {product}")

    print("\nAvailable configurations:")
    for config in configs:
        print(f"  - {config}")

def main():
    parser = argparse.ArgumentParser(
        description="EMS SDK Build Tool",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # List all products and configs
  python3 build.py --list

  # Open menuconfig
  python3 build.py menuconfig

  # Load a specific config (without building)
  python3 build.py config ccm_h200_100p_v1

  # Build with specific config
  python3 build.py build --config ccm_h200_100p_v1

  # Clean build directory
  python3 build.py clean

  # Build with existing configuration
  python3 build.py build
        """
    )

    parser.add_argument("cmd", nargs="?", default="build",
                        choices=["build", "menuconfig", "config", "clean", "distclean"],
                        help="Command to execute")
    parser.add_argument("config_name", nargs="?",
                        help="Configuration name (for 'config' command)")
    parser.add_argument("--list", action="store_true",
                        help="List all available products and configs")
    parser.add_argument("--config", "-c", type=str,
                        help="Configuration to load (defconfig name without _defconfig suffix)")
    parser.add_argument("--build-dir", "-b", type=str, default="_build",
                        help="Build directory (default: _build)")
    parser.add_argument("--jobs", "-j", type=int,
                        help="Number of parallel jobs")
    parser.add_argument("--verbose", "-v", action="store_true",
                        help="Verbose build output")

    args = parser.parse_args()

    # 列出产品和配置
    if args.list:
        list_all()
        return 0

    # menuconfig
    if args.cmd == "menuconfig":
        success = menuconfig()
        return 0 if success else 1

    # config
    if args.cmd == "config":
        if not args.config_name:
            print("Error: config name is required")
            print("Usage: python3 build.py config <config_name>")
            print("\nAvailable configurations:")
            for cfg in get_configs():
                print(f"  - {cfg}")
            return 1
        success = load_config(args.config_name, args.build_dir)
        return 0 if success else 1

    # clean / distclean
    if args.cmd in ["clean", "distclean"]:
        success = build(build_dir=args.build_dir, clean=True)
        return 0 if success else 1

    # build
    if args.cmd == "build":
        success = build(
            config=args.config,
            build_dir=args.build_dir,
            clean=False,
            jobs=args.jobs,
            verbose=args.verbose
        )
        return 0 if success else 1

    return 0

if __name__ == "__main__":
    sys.exit(main())
