# Server Template (Current Scope)

The `server` template name is ahead of the runtime's current networking capabilities. The checked-in `examples/templates/server.npp` is a simple HTTP GET client demo using `std.net`; it is **not** an HTTP server and does not open a listening port. The current runtime does not expose TCP/UDP listeners, routing, HTTP request/response types or a server framework.

## Create and run

```bash
npp new MyServer --template server
cd MyServer
npp check
npp build
npp run
```

## Example

The template uses the current standard-library helper:

```npp
import std.net

fn main() {
    let body = http_get_text("https://example.com/")
    if body == "" {
        print("The request returned an empty body or failed")
        return
    }
    print(body)
}
```

This helper shells out to the host's `curl` command with a timeout, so `curl` must be available. It returns a plain string and does not expose status codes or headers. See [std.net](../std/net.md). A real server listener remains future work.
