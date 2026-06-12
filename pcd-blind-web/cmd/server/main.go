package main

import (
	"flag"
	"fmt"
	"log"
	"net/http"
	"os"

	"pcd-blind-web/internal/handler"
	"pcd-blind-web/internal/store"
)

func main() {
	port := flag.Int("port", 8080, "HTTP server port")
	binPath := flag.String("bin", "./pcd-blind", "Path to pcd-blind binary")
	dataDir := flag.String("data", "./data", "Data directory path")
	tmplDir := flag.String("templates", "", "Templates directory (auto-detected if empty)")
	flag.Parse()

	// Find templates directory
	td := *tmplDir
	if td == "" {
		for _, d := range []string{"web/templates", "../web/templates", "../../web/templates"} {
			if fi, err := os.Stat(d); err == nil && fi.IsDir() {
				td = d
				break
			}
		}
	}
	if td == "" {
		log.Fatal("找不到模板目录，请使用 --templates 参数指定")
	}

	if err := handler.InitTemplates(td); err != nil {
		log.Fatalf("加载模板失败: %v", err)
	}

	s := store.New(*dataDir)
	h := handler.New(s, *binPath)

	addr := fmt.Sprintf(":%d", *port)
	log.Printf("标靶球检测 Web 启动: http://localhost%s", addr)
	log.Printf("  数据目录: %s", *dataDir)
	log.Printf("  管线程序: %s", *binPath)

	if err := http.ListenAndServe(addr, h.BuildRoutes()); err != nil {
		log.Fatalf("服务器错误: %v", err)
	}
}
