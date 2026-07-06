package main
 
import (
    "fmt"
    "net/http"
//    "sync"
)
 
const (
    address = ":8080"
    message = "Hello from Go HTTP server!" // 26 bytes
)
 
var (
    messageBytes = []byte(message)
)
 
func main() {
    http.HandleFunc("/", handler)
    server := &http.Server{
	Addr:           address,
	Handler:        nil,
	MaxHeaderBytes: 1 << 20,
    }
    fmt.Printf("Listening on %s\n", address)
    if err := server.ListenAndServe(); err != nil {
	fmt.Printf("Failed to start server: %v\n", err)
    }
}
 
func handler(w http.ResponseWriter, r *http.Request) {
    // Preallocate buffer
    buf := make([]byte, 0, len(messageBytes))
    // Write the message to the buffer
    buf = append(buf, messageBytes...)
 
    // Set minimal headers
    w.Header().Set("Content-Type", "text/plain")
 
    // Write the buffer to the response
    w.Write(buf)
}
 

