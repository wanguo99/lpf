# Kconfig Tools

This directory contains the Kconfig configuration system tools extracted from Buildroot.

## Tools Built

- **conf**: Command-line configuration tool
- **mconf**: Menuconfig (ncurses-based interactive configuration)
- **nconf**: Nconfig (alternative ncurses-based configuration)

## Building

```bash
cd scripts/kconfig
cmake -B build -S .
cmake --build build -j$(nproc)
```

The built executables will be in `build/`:
- `build/conf`
- `build/mconf`
- `build/nconf`

## Dependencies

Required:
- CMake 3.16+
- ncurses development libraries (`libncurses-dev`)
- pkg-config

Optional (for regenerating parsers):
- flex
- bison

## Usage

From ES-Middleware root directory:

```bash
# Interactive menuconfig
./scripts/kconfig/build/mconf Kconfig

# Command-line configuration
./scripts/kconfig/build/conf Kconfig

# Alternative ncurses interface
./scripts/kconfig/build/nconf Kconfig
```

## Source

Extracted from Buildroot v2026.02 `support/kconfig/` directory.

## Modifications

To integrate with ES-Middleware, the following modifications were made:

1. **CMakeLists.txt**: Created CMake build system to compile the Kconfig tools
2. **zconf.tab.h_shipped**: Extracted token definitions from `zconf.tab.c_shipped`
3. **zconf.tab.c_shipped**: Changed `static struct menu *current_menu` to non-static to allow access from `menu.c` and `symbol.c`
4. **zconf.lex.c_shipped**: Added `#include "zconf.tab.h"` to access token definitions
5. **lkc.h**: No changes (avoided conflicts with mconf/nconf's own `current_menu` variables)
6. **menu.c**: Added extern declarations for parser state variables
7. **symbol.c**: Added extern declarations for parser state variables

## Notes

- Uses pre-generated (shipped) parser files by default
- Set `KCONFIG_REGENERATE_PARSERS=ON` to regenerate with flex/bison
- GTK2-based `gconf` is not built (GTK2 not available on this system)
