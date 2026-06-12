.PHONY: build run clean pcd

BIN ?= ../pcd-blind-rs/target/release/pcd-blind
PORT ?= 8080
DATA ?= ./data

build:
	go build -o pcd-blind-web ./cmd/server

run: build
	./pcd-blind-web --port $(PORT) --bin $(BIN) --data $(DATA)

pcd:
	cd ../pcd-blind-rs && cargo build --release

clean:
	rm -f pcd-blind-web
	rm -rf data/output data/params.json data/runs.json data/selections.json
