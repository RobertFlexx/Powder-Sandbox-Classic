# Powder Sandbox (C++ Classic Edition)

The original **C++17 + ncurses** version of the Powder Sandbox — a retro-style falling-sand simulator that kicked off the project later rebuilt in C#.

This edition delivers the raw, blazing-fast terminal experience with pixel-precise simulation logic, refined plant and seaweed growth, and classic minimalism — everything that made the sandbox chaotic, beautiful, and addictive.

### See [GameHub](https://github.com/RobertFlexx/Powder-Sandbox-GameHub) for more editions of this game.
---

## Features

* Written in modern **C++17** using `ncurses`
* Ultra-optimized CPU-based simulation loop
* Dozens of elements — sand, lava, acid, gases, plants, seaweed, etc.
* Realistic interactions — melting, burning, dissolving, condensing, shocking
* AI-driven actors — humans flee zombies, zombies hunt and infect
* Controlled plant & seaweed growth (improved over C# edition)
* Dynamic lightning behavior — conducts through metal, wire, and saltwater
* Fully colorized TUI display with minimal flicker
* Credits menu and in-game element browser
* Cross-platform (Linux, macOS, Windows via WSL)

---

## What’s New in This Edition

The C++ Classic is currently **one update ahead** of the C# Edition, introducing:

* Refined plant and seaweed logic — independent vertical growth, natural spacing, and more organic underwater generation
* Improved lava-water reaction — crisp cooling into stone or steam, not noisy block spam
* Cleaner smoke & steam condensation — produces fewer leftover particles
* More stable element transitions — no flickering ash storms or endless fire chains
* Better AI step timing — humans and zombies react more responsively
* Optimized simulation loop — smoother FPS on terminals

---

## Requirements

* `g++` (or `clang++`) with C++17 support
* `ncurses` library

To install on Debian/Ubuntu:

```bash
sudo apt install g++ libncurses5-dev libncursesw5-dev
```

---

## Building and Running

Clone the repo and build:

```bash
git clone https://github.com/RobertFlexx/Powder-Sandbox-Classic.git
cd Powder-Sandbox-Classic
g++ -std=c++17 -O2 -Wall main.cpp -lncurses -o powder
./powder
```

If you’re using Clang:

```bash
clang++ -std=c++17 -O2 -Wall main.cpp -lncurses -o powder
./powder
```

---

## Controls

| Key               | Action                 |
| ----------------- | ---------------------- |
| Arrow keys / WASD | Move cursor            |
| Space             | Place current element  |
| E                 | Erase with empty space |
| + / -             | Adjust brush size      |
| M / Tab           | Open element menu      |
| P                 | Pause simulation       |
| C / X             | Clear screen           |
| Q                 | Quit game              |

---

## Comparison to the C# Edition

| Aspect           | C++ Classic Edition                   | [C# Edition](https://github.com/RobertFlexx/Powder-Sandbox-CS-Edition) |
| ---------------- | ------------------------------------- | ---------------------------------------------------------------------- |
| Language Runtime | Native (no runtime dependency)        | Requires .NET 8                                                        |
| Performance      | Faster — raw memory & pointer control | Slightly slower due to managed runtime                                 |
| Stability        | High — manual memory, no GC pauses    | Very high — memory-safe                                                |
| Plant Growth     | Improved, natural spacing & balance   | Simpler, earlier logic                                                 |
| Seaweed Growth   | Controlled underwater strands         | More chaotic distribution                                              |
| Rendering        | Minimal flicker ncurses TUI           | Spectre.Console colored output                                         |
| Audio            | None (pure terminal)                  | Simple .wav playback                                                   |
| Extendability    | Manual editing, recompile required    | Easier modular expansion via classes                                   |

---

## License

Released under the BSD 3-Clause License.

---

## Author

**Robert (@RobertFlexx)**
Creator of FerriteOS, custom shells, editors, and countless experimental programming tools.

GitHub projects: [https://github.com/RobertFlexx](https://github.com/RobertFlexx)

---

**Note:**
This C++ Classic is the definitive version — tighter physics, sharper interactions, and the true retro sandbox charm that inspired the modern remake.

For the modernized and extensible remake, visit the definitive **C# Edition** here:
[https://github.com/RobertFlexx/Powder-Sandbox-CS-Edition](https://github.com/RobertFlexx/Powder-Sandbox-CS-Edition)
