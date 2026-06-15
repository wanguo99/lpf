cmd_scripts/kconfig/lexer.lex.o := gcc -Wp,-MMD,scripts/kconfig/.lexer.lex.o.d -I/home/wanguo/CSPD/ES-Middleware/scripts/include  -I /home/wanguo/CSPD/ES-Middleware/scripts/kconfig -c -o scripts/kconfig/lexer.lex.o scripts/kconfig/lexer.lex.c

deps_scripts/kconfig/lexer.lex.o := \
  scripts/kconfig/lexer.lex.c \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/xalloc.h \
  scripts/kconfig/lkc.h \
    $(wildcard include/config/prefix.h) \
  scripts/kconfig/expr.h \
  /home/wanguo/CSPD/ES-Middleware/scripts/include/list_types.h \
  scripts/kconfig/lkc_proto.h \
  scripts/kconfig/preprocess.h \
  scripts/kconfig/parser.tab.h \

scripts/kconfig/lexer.lex.o: $(deps_scripts/kconfig/lexer.lex.o)

$(deps_scripts/kconfig/lexer.lex.o):
