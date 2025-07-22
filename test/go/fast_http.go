package main

import (
    "fmt"
    "github.com/valyala/fasthttp"
)

const (
    address = ":8080"
    message = "Hello from Go HTTP server!" // 26 bytes
)

var (
    messageBytes = []byte(message)
)

func main() {
    // Define the request handler
    handler := func(ctx *fasthttp.RequestCtx) {
	// Set minimal headers
	ctx.Response.Header.Set("Content-Type", "text/plain")
	// Write the message to the response
	ctx.Write(messageBytes)
    }

    // Start the Fasthttp server
    fmt.Printf("Listening on %s\n", address)
    if err := fasthttp.ListenAndServe(address, handler); err != nil {
	fmt.Printf("Failed to start server: %v\n", err)
    }
}