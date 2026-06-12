#!/bin/bash
# Generate autoconf.h from .config
# This script is called by Makefile after Kconfig operations

set -e

if [ $# -ne 2 ]; then
    echo "Usage: $0 <config_file> <output_h>"
    exit 1
fi

CONFIG_FILE="$1"
OUTPUT_H="$2"

if [ ! -f "$CONFIG_FILE" ]; then
    echo "Error: Configuration file not found: $CONFIG_FILE"
    exit 1
fi

# Create output directory
mkdir -p "$(dirname "$OUTPUT_H")"

# Generate header
cat > "$OUTPUT_H" << 'EOF'
/*
 * Automatically generated file - DO NOT EDIT
 * Generated from .config by scripts/gen_autoconf.sh
 */
#ifndef AUTOCONF_H
#define AUTOCONF_H

EOF

# Process .config line by line
while IFS= read -r line; do
    # Skip empty lines and comments
    if [ -z "$line" ] || [[ "$line" =~ ^[[:space:]]*# ]]; then
        # Check for "# CONFIG_XXX is not set" pattern
        if [[ "$line" =~ ^#[[:space:]]*CONFIG_([A-Za-z0-9_]+)[[:space:]]+is[[:space:]]+not[[:space:]]+set ]]; then
            name="${BASH_REMATCH[1]}"
            # Don't define anything for disabled options
            # CMake/C code checks with #ifdef CONFIG_XXX
            continue
        fi
        continue
    fi

    # Parse CONFIG_XXX=value
    if [[ "$line" =~ ^CONFIG_([A-Za-z0-9_]+)=(.*)$ ]]; then
        name="${BASH_REMATCH[1]}"
        value="${BASH_REMATCH[2]}"

        case "$value" in
            y)
                # Boolean enabled
                echo "#define CONFIG_${name} 1" >> "$OUTPUT_H"
                ;;
            \"*\")
                # String value (keep quotes)
                echo "#define CONFIG_${name} ${value}" >> "$OUTPUT_H"
                ;;
            [0-9]*)
                # Numeric value
                echo "#define CONFIG_${name} ${value}" >> "$OUTPUT_H"
                ;;
            0x*)
                # Hex value
                echo "#define CONFIG_${name} ${value}" >> "$OUTPUT_H"
                ;;
            *)
                # Other values
                echo "#define CONFIG_${name} ${value}" >> "$OUTPUT_H"
                ;;
        esac
    fi
done < "$CONFIG_FILE"

# Close header guard
cat >> "$OUTPUT_H" << 'EOF'

#endif /* AUTOCONF_H */
EOF

echo "Generated: $OUTPUT_H"
