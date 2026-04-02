# Nexus

Nexus is a C++ networking library for multiplayer games, built on top of [ASIO](https://think-async.com/Asio/). It handles the boilerplate of TCP connections, typed message routing, and thread management so you can focus on your game logic.

It started as a graduation project at Howest (DAE, 2024-2025) and has since been hardened into something worth actually using.

---

## What it does

You define your message types as an enum, register handlers, and connect or start listening. The library takes care of the rest: async I/O, connection lifecycle, client ID assignment, thread pooling, and safe message dispatch.

```cpp
enum class MsgType : uint32_t { Ping, Pong };

// Server
nxs::Server<MsgType> server{60000};

server
    .OnConnect([](auto conn) {
        std::cout << "Client " << conn->GetID() << " connected\n";
    })
    .OnMessage(MsgType::Ping, [](auto conn, auto &) {
        conn->Send(nxs::Message<MsgType>{MsgType::Pong});
    })
    .RunBlocking();

// Client
nxs::Client<MsgType> client;

client
    .OnConnect([&]() {
        client.Send(nxs::Message<MsgType>{MsgType::Ping});
    })
    .OnMessage(MsgType::Pong, [](auto &) {
        std::cout << "Pong!\n";
    })
    .ConnectTo("localhost", 60000);
```

That's the whole setup. No subclassing, no virtual dispatch, no manual thread management.

---

## Building

Nexus uses CMake and fetches its dependencies automatically on first configure. You need a C++20 compiler and an internet connection for that first build.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Tested on Windows, Linux, and macOS via CI. The GitHub Actions workflow builds Debug and Release for all three on every push to main.

---

## Concepts

### Initialisation

Call `nxs::Nexus::Init()` once at the start of your program. This starts a thread pool sized to your CPU's hardware concurrency. Call `nxs::Nexus::Shutdown()` before exiting.

```cpp
nxs::Nexus::Init();       // spins up worker threads
// ... your program
nxs::Nexus::Shutdown();   // joins threads cleanly
```

### Message types

Your message type must be an `enum class` with `uint32_t` as its underlying type. This is enforced at compile time.

```cpp
enum class GameMsg : uint32_t {
    PlayerMove,
    PlayerJoined,
    PlayerLeft,
    ChatMessage,
};
```

### Messages

`nxs::Message<T>` is a typed, variable-length message. Data is pushed and popped using `<<` and `>>` — think of it as a stack, so read back in reverse order of writes.

```cpp
// Writing
nxs::Message<GameMsg> msg{GameMsg::PlayerMove};
msg << pos.x << pos.y;

// Reading
float x, y;
msg >> y >> x;  // reverse order
```

Strings and standard-layout types are supported. The header carries the message type, the sender's ID, and the body size.

### Server

```cpp
nxs::Server<MyMsgType> server{60000};  // port

server
    .OnConnect([](auto conn) { /* new client */ })
    .OnDisconnect([](auto conn) { /* client left */ })
    .OnMessage(MyMsgType::Something, [](auto conn, auto &msg) { /* handle */ })
    .Run();          // non-blocking, use with a game loop
    // or
    .RunBlocking();  // parks the calling thread, use for headless servers
```

From anywhere (including your game loop on the main thread):

```cpp
server.SendToClient(clientID, message);
server.Broadcast(message);
server.Broadcast(message, /*skipID=*/senderID);  // broadcast to all except one
```

Each connected client is assigned a unique `uint32_t` ID by the server, starting at 1001.

### Client

```cpp
nxs::Client<MyMsgType> client;

client
    .OnConnect([&]() { /* connected, client.GetID() is valid here */ })
    .OnDisconnect([&]() { /* lost connection */ })
    .OnMessage(MyMsgType::Something, [](auto &msg) { /* handle */ })
    .ConnectTo("192.168.1.10", 60000);
```

Register all your handlers before calling `ConnectTo`. The connection is async — your `OnConnect` handler fires once the server has accepted and sent back your assigned ID.

---

## Threading

Nexus runs a pool of ASIO worker threads internally. You don't manage them directly.

Each connection has its own strand, so its read and write handlers are always serialised — no races on per-connection state without any external locking. Connections from different clients run concurrently across the thread pool.

The server has a separate strand protecting its connection map and ID counter, so `SendToClient`, `Broadcast`, and `OnConnectionDisconnected` are all safe to call from any thread, including your main game loop.

If your message handlers access shared state (a player list, a game world), you still need to protect that yourself with a mutex. The library serialises its own internals, not yours.

---

## Performance

Measured locally on the same machine over loopback:

| Library | Avg latency | Avg throughput |
|---------|-------------|----------------|
| Raw ASIO | 254 µs | ~5.1 MB/s* |
| ENet | 373 µs | ~5.3 MB/s |
| **Nexus** | **151 µs** | **~11 MB/s** |

\* Figures from the original graduation paper. Nexus throughput has since improved significantly after fixing a dual-write bug in the client and adding a thread pool. Current measurements with 1024 × 1 KB messages hover around 11 MB/s.

Latency is measured as round-trip time for a 1 KB message. Throughput is measured by sending 1024 × 1 KB messages and tracking replies. Outliers (min and max) are trimmed before averaging.

---

## Targets

The repo includes several small programs alongside the library itself:

- **nexus_connect_disconnect** — a client connects and the server immediately disconnects it
- **nexus_ping** — client sends Ping, server replies with Pong, client prints RTT
- **nexus_send_message** — two clients exchange a chat message via the server
- **nexus_latency** — measures average round-trip time over 50 messages
- **nexus_throughput** — measures throughput over 50 batches of 1024 messages
- **nexus_game** — a small SDL demo where connected clients move squares around a shared world

The ENet and raw ASIO versions of the same tests are included for comparison.

---

## Limitations

- TCP only. UDP support is not implemented.
- Client-server topology only. No peer-to-peer.
- The message serialisation uses a stack model (LIFO), so read order is reversed from write order. This is intentional and consistent but takes a moment to get used to.
- `nxs::Nexus` is a singleton. One io_context per process.

---

## Dependencies

All fetched automatically by CMake via FetchContent:

- [ASIO](https://github.com/chriskohlhoff/asio) 1.38.0 (standalone, no Boost)
- [SDL3](https://github.com/libsdl-org/SDL) 3.4.2 (nexus_game only)
- [SDL3_ttf](https://github.com/libsdl-org/SDL_ttf) 3.2.2 (nexus_game only)
- [ENet](https://github.com/lsalzman/enet) 1.3.18 (comparison targets only)

---

## License

See LICENSE.md.
