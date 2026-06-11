#!/bin/bash
# Extract token definitions from zconf.tab.c_shipped to create zconf.tab.h

INPUT="$1"
OUTPUT="$2"

cat > "$OUTPUT" << 'EOF'
/* Automatically generated from zconf.tab.c_shipped */
#ifndef ZCONF_TAB_H
#define ZCONF_TAB_H

EOF

# Extract token enum
sed -n '/^  enum yytokentype/,/^  };/p' "$INPUT" >> "$OUTPUT"

# Extract YYSTYPE
cat >> "$OUTPUT" << 'EOF'

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
	char *string;
	struct file *file;
	struct symbol *symbol;
	struct expr *expr;
	struct menu *menu;
	const struct kconf_id *id;
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;
extern struct menu *current_menu, *current_entry;

int yyparse (void);

#endif /* ZCONF_TAB_H */
EOF
