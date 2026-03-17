DIRS=$(filter-out Makefile appl.mk dot-vscode, $(wildcard *))
all clean clean_all:
	for dir in $(DIRS); do $(MAKE) $@ -C $$dir; done

