package main

import (
	"embed"
	"flag"
	"fmt"
	"io/fs"
	"log"
	"net/http"
	"os"
	"path/filepath"

	"pcd-blind-web/internal/handler"
	"pcd-blind-web/internal/store"
)

//go:embed bin/*
var binFS embed.FS

//go:embed web/templates/*.html
var templateFS embed.FS

//go:embed web/static/*
var staticFS embed.FS

func main() {
	port := flag.Int("port", 8080, "HTTP server port")
	dataDir := flag.String("data", "./data", "Data directory path")
	flag.Parse()

	// Extract embedded pcd-blind binary
	binPath, err := extractBin()
	if err != nil {
		log.Fatalf("解压管线程序失败: %v", err)
	}
	defer os.Remove(binPath)

	// Init templates from embedded FS
	if err := handler.InitTemplatesFS(templateFS, "web/templates/*.html"); err != nil {
		log.Fatalf("加载模板失败: %v", err)
	}

	s := store.New(*dataDir)
	h := handler.New(s, binPath)

	addr := fmt.Sprintf(":%d", *port)
	log.Printf("标靶球检测 Web 启动: http://localhost%s", addr)
	log.Printf("  数据目录: %s", *dataDir)

	if err := http.ListenAndServe(addr, h.BuildRoutes(staticFS)); err != nil {
		log.Fatalf("服务器错误: %v", err)
	}
}

// extractBin writes the embedded pcd-blind binary to a temp file and returns its path.
func extractBin() (string, error) {
	data, err := fs.ReadFile(binFS, "bin/pcd-blind")
	if err != nil {
		return "", fmt.Errorf("read embedded binary: %w", err)
	}

	tmpDir, err := os.MkdirTemp("", "pcd-blind-web-*")
	if err != nil {
		return "", err
	}

	binPath := filepath.Join(tmpDir, "pcd-blind")
	if err := os.WriteFile(binPath, data, 0755); err != nil {
		return "", err
	}

	return binPath, nil
}
