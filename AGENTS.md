# Repository Guidelines

## Project Structure & Module Organization
Reusable framework layers live at the repository top level: `kernel/` holds kernel-side modules such as `osal`, `hal`, and `lpf`; `kernel/pconfig` currently holds transitional runtime config sources linked into `lpf_peripheral_runtime.ko`; `user/` holds userspace libraries such as `osal`, `aconfig`, and `pdi`; `uapi/` holds shared userspace/kernel ABI headers. Configuration presets live under `configs/`, build support scripts are under `scripts/`, and generated artifacts are written to `_build/`.

## Build, Test, and Development Commands
- `make list` - show available configuration and build targets.
- `make kernel_x86_modules_defconfig` - load a baseline kernel-module configuration.
- `make menuconfig` - open the interactive Kconfig editor.
- `make all` - configure and build the selected target.
- `make modules` - build enabled kernel modules such as `osal.ko`, `hal.ko`, and `lpf_peripheral_runtime.ko`.
- `make clean` - remove build outputs.

## Coding Style & Naming Conventions
Follow the existing C style in the tree: 4-space indentation in newer code, tabs in legacy Kconfig/C sources where already used, braces on the next line for functions, and short `static` helpers for file-local logic. Keep names uppercase for public APIs and macros (`OSAL_*`, `PDI_*`, `LPF_*`), lowercase snake_case for internal files and helpers, and match the module prefix already in use (`osal_`, `hal_`, `pdi_`, `lpf_`).

## Testing Guidelines
The previous test product has been removed. New tests should be added with a fresh test layout and matching Kconfig/CMake integration.

## Commit & Pull Request Guidelines
Recent history uses concise prefixes such as `refactor:` and `fix:`, often with a module scope like `refactor(pdi): ...` or `refactor(lpf): ...`. Follow that pattern: short, imperative, and specific. Pull requests should explain the functional change, list the defconfig or test binary used for verification, and mention any configuration impact or skipped coverage.

## Configuration Notes
Do not edit generated files under `_build/`. When adding a module or test, update the relevant `Config.in` and CMake files together so Kconfig and CMake stay aligned.
