CC=gcc
ECHO=echo -e

CFLAGS=-Wall -Werror -std=gnu99 -O0 -g
LIBS=-lcrypt

FILES=build/main.o
OUT=bin/beroot

all: $(FILES)
	@$(ECHO) "Linking program"
	@$(CC) $(CFLAGS) $(FILES) -o $(OUT) $(LIBS)
	@$(ECHO) "Linking finished"

build/main.o: src/main.c src/config.h
	@$(ECHO) "CC\t\t"$<
	@$(CC) $(CFLAGS) $< -c -o $@ $(LIBS)

install: all
	@$(ECHO) "Installing beroot (you MUST be root to install beroot)"
	@cp $(OUT) /usr/bin
	@chown root:root /usr/bin/beroot # set permissions
	@chmod u+s /usr/bin/beroot
	@$(ECHO) "Installation successful"

uninstall: all
	@$(ECHO) "Uninstalling beroot (you MUST be root to uninstall beroot)"
	@rm -f /usr/bin/beroot
	@$(ECHO) "Uninstallation successful"

clean:
	@$(ECHO) "Cleaning files..."
	@rm -f $(FILES) $(OUT)
	@$(ECHO) "Cleaning complete :)"
