# Repository Guidelines

## Project Structure & Module Organization
Reusable framework layers live at the repository top level: `kernel/pdm/` holds the kernel-side PDM module, bus, registries, diagnostics, logging, and peripheral drivers; `kernel/include/pdm/` holds kernel-side public headers; `user/pdi/` holds the userspace PDI library; `uapi/pdm/` holds shared userspace/kernel ABI headers. Common userspace tools live under `apps/`, configuration presets under `configs/`, build support scripts under `scripts/`, documentation under `docs/`, and generated artifacts under `_build/`.

## Build, Test, and Development Commands
- `make list` - show available configuration and build targets.
- `make ubuntu_x86_modules_defconfig` - load a baseline kernel-module configuration.
- `make ubuntu_x86_mock_modules_defconfig` - load the x86 mock-device smoke configuration.
- `make menuconfig` - open the interactive Kconfig editor.
- `make all` - configure and build the selected target.
- `make modules` - build enabled kernel modules such as `pdm.ko`.
- `make mock-modules-smoke` - load/unload the x86 mock module build and verify the manager plus mock device nodes.
- `make clean` - remove build outputs.

## Coding Style & Naming Conventions
Follow the existing C style in the tree: 4-space indentation in newer code, tabs in legacy Kconfig/C sources where already used, braces on the next line for functions, and short `static` helpers for file-local logic. Keep names uppercase for public APIs and macros (`PDM_*`, `PDI_*`, `LOG_*`), lowercase snake_case for internal files and helpers, and match the module prefix already in use (`pdm_`, `pdi_`). The `LOG_*` interfaces are intentionally unprefixed compatibility APIs owned by PDM in kernel space and PDI in userspace.

## Testing Guidelines
The previous `examples/` and `tests/` products have been removed. Current smoke coverage is provided by `make mock-modules-smoke` with `ubuntu_x86_mock_modules_defconfig`. New tests should be added with a fresh test layout and matching Kconfig/CMake integration.

## Commit & Pull Request Guidelines
Recent history uses concise prefixes such as `refactor:` and `fix:`, often with a module scope like `refactor(pdm): ...` or `refactor(pdi): ...`. Follow that pattern: short, imperative, and specific. Pull requests should explain the functional change, list the defconfig or smoke target used for verification, and mention any configuration impact or skipped coverage.

## Configuration Notes
Do not edit generated files under `_build/`. When adding a module, app, or test, update the relevant `Config.in` and CMake files together so Kconfig and CMake stay aligned.
