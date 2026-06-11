#!/bin/bash
# Extract zconf.tab.h from zconf.tab.c_shipped
# Usage: extract_header.sh <input_file> <output_file>

if [ $# -ne 2 ]; then
    echo "Usage: $0 <input_file> <output_file>"
    exit 1
fi

INPUT="$1"
OUTPUT="$2"

# Extract from #ifndef YYTOKENTYPE to extern YYSTYPE yylval;
sed -n '/^#ifndef YYTOKENTYPE/,/^extern YYSTYPE yylval;/p' "$INPUT" > "$OUTPUT"

if [ ! -s "$OUTPUT" ]; then
    echo "Error: Failed to extract header from $INPUT"
    exit 1
fi

echo "Successfully extracted header to $OUTPUT"
exit 0
