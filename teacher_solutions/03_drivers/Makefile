DIRS=$(filter-out Makefile, $(wildcard *))

all clean install:
	for dir in $(DIRS); do $(MAKE) $@ -C $$dir; done

