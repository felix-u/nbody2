NAME=nbody2
VERSION=0.1-dev

CFLAGS=\
	-Wall -Wextra -pedantic -Werror -Wshadow \
	-fno-strict-aliasing -Wstrict-overflow
DEBUGFLAGS=-g3 -ggdb -fsanitize=address,undefined
RELEASEFLAGS=-O3 -s
LIBS=-lwayland-client -lrt

CROSSCC=zig cc -DUNITY_BUILD

debug: src/main.c
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(LIBS) -o $(NAME) $^

release: src/main.c
	$(CC) $(CFLAGS) -march=native $(RELEASEFLAGS) $(LIBS) -o $(NAME) $^

cross: src/main.c
	mkdir -p release
	$(CROSSCC) -static -target x86_64-windows     $(CFLAGS) $(RELEASEFLAGS) $(LIBS) -o release/$(NAME)-v$(VERSION)-x86_64-win.exe  $^
	$(CROSSCC) -static -target aarch64-windows    $(CFLAGS) $(RELEASEFLAGS) $(LIBS) -o release/$(NAME)-v$(VERSION)-aarch64-win.exe $^
	$(CROSSCC) -static -target x86_64-linux-musl  $(CFLAGS) $(RELEASEFLAGS) $(LIBS) -o release/$(NAME)-v$(VERSION)-x86_64-linux    $^
	$(CROSSCC) -static -target aarch64-linux-musl $(CFLAGS) $(RELEASEFLAGS) $(LIBS) -o release/$(NAME)-v$(VERSION)-aarch64-linux   $^
	$(CROSSCC) -static -target x86_64-macos       $(CFLAGS) $(RELEASEFLAGS) $(LIBS) -o release/$(NAME)-v$(VERSION)-x86_64-macos    $^
	$(CROSSCC) -static -target aarch64-macos      $(CFLAGS) $(RELEASEFLAGS) $(LIBS) -o release/$(NAME)-v$(VERSION)-aarch64-macos   $^

.PHONY: clean
clean:
	rm -f $(obj) debug
