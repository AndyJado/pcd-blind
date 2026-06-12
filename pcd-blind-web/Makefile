.PHONY: build run clean

BIN ?= ../pcd-blind-rs/target/release/pcd-blind
PORT ?= 8080
DATA ?= ./data

# Copy Rust binary and HTMX before building
prep:
	@mkdir -p cmd/server/bin cmd/server/web/static cmd/server/web/templates
	@cp $(BIN) cmd/server/bin/pcd-blind 2>/dev/null || echo "WARN: pcd-blind binary not found at $(BIN)"
	@if [ ! -f cmd/server/web/static/htmx.min.js ]; then \
		curl -sL -o cmd/server/web/static/htmx.min.js https://unpkg.com/htmx.org@2.0.4/dist/htmx.min.js; \
	fi

build: prep
	go build -o pcd-blind-web ./cmd/server

run: build
	./pcd-blind-web --port $(PORT) --data $(DATA)

clean:
	rm -f pcd-blind-web
	rm -rf data/output
