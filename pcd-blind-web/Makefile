.PHONY: build run clean build-linux build-windows

GOOS   ?= $(shell go env GOOS)
GOARCH ?= $(shell go env GOARCH)
# For macOS arm64 building Linux: override these per target
BIN    ?= ../pcd-blind-rs/target/release/pcd-blind
PORT   ?= 8080
DATA   ?= ./data

# Auto-detect extension
ifeq ($(GOOS),windows)
  EXT = .exe
else
  EXT =
endif

# Copy Rust binary for current platform. For cross-compile, build Rust first with:
#   cd ../pcd-blind-rs && cargo build --release --target <target>
# then set BIN=../pcd-blind-rs/target/<target>/release/pcd-blind
prep:
	@mkdir -p cmd/server/bin cmd/server/web/static cmd/server/web/templates
	@if [ -f $(BIN) ]; then cp $(BIN) cmd/server/bin/pcd-blind; \
	else echo "WARN: pcd-blind not at $(BIN) — build Rust first"; fi
	@if [ ! -f cmd/server/web/static/htmx.min.js ]; then \
		curl -sL -o cmd/server/web/static/htmx.min.js https://unpkg.com/htmx.org@2.0.4/dist/htmx.min.js; \
	fi

build: prep
	GOOS=$(GOOS) GOARCH=$(GOARCH) go build -o pcd-blind-web$(EXT) ./cmd/server

run: build
	./pcd-blind-web --port $(PORT) --data $(DATA)

# Cross-compile targets (run on macOS with proper Rust targets installed)
#   rustup target add x86_64-unknown-linux-gnu
#   rustup target add x86_64-pc-windows-gnu

build-linux:
	cd ../pcd-blind-rs && cargo build --release --target x86_64-unknown-linux-gnu
	$(MAKE) build GOOS=linux GOARCH=amd64 BIN=../pcd-blind-rs/target/x86_64-unknown-linux-gnu/release/pcd-blind

build-windows:
	cd ../pcd-blind-rs && cargo build --release --target x86_64-pc-windows-gnu
	$(MAKE) build GOOS=windows GOARCH=amd64 BIN=../pcd-blind-rs/target/x86_64-pc-windows-gnu/release/pcd-blind.exe

clean:
	rm -f pcd-blind-web pcd-blind-web.exe
	rm -rf data/output
