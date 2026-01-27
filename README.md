# simple‑reverse‑proxy

A simple reverse proxy implementation in C.

## Overview

This project implements a basic reverse proxy written in C.

## Getting Started

### Prerequisites

- Linux (uses epoll syscall for event loop)

### Build

```bash
git clone https://github.com/kumang‑subba/simple‑reverse‑proxy‑c.git
cd simple‑reverse‑proxy‑c
mkdir build && cd build
cmake ..
make
```

### Run

```bash
./srp -l <listen_port> -p <upstream_port> <upstream_port>
```
