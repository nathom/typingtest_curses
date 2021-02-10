CC = clang
CFLAGS = -lncurses

typingtest: typingtest.c
	$(CC) $(CFLAGS) $< -o $@
