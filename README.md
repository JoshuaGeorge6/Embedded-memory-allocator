
# 🧠 Custom Contiguous Memory Allocator in C

This project implements a simple memory allocator using a contiguous memory block,
similar to `malloc`/`free`, but managed manually. It was built for an academic assignment
and is useful for learning about memory management, pointer arithmetic, and data structures in C.

## 📦 Features

- Manages a contiguous block of memory
- Supports allocation (`cmalloc`) and deallocation (`cfree`)
- Avoids using built-in memory functions internally (except `malloc` in `make_contiguous`)
- Prints debug info with `print_debug`

## 📂 Files

- `contiguous.c` — Implementation of memory allocator
- `contiguous.h` — Header file with function declarations and data structure interfaces
- `README.md` — This file


## 👨‍💻 Author

Joshua — Created for a systems programming assignment.

## ⚖️ License

This project is for educational purposes.
