# MyShell

MyShell is a custom Unix-like shell implemented in C. It supports command execution, piping, input/output redirection, and wildcard expansion, providing a simplified but functional recreation of a traditional command-line interpreter.

This project demonstrates core operating system concepts such as process creation, inter-process communication, and command parsing using low-level system calls.

---

## Features

- Interactive shell with prompt (`mysh>`)
- Batch mode execution (run commands from a file)
- Built-in commands:
  - `cd` – change directory
  - `pwd` – print working directory
  - `which` – locate executables in `PATH`
  - `exit` – exit the shell
- Execution of external programs using `fork()` and `execv()`
- Input and output redirection:
  - `command < input.txt`
  - `command > output.txt`
- Pipelining between two commands:
  - `command1 | command2`
- Wildcard expansion (`*`) using directory scanning
- Custom tokenizer for parsing user input
- Dynamic array list implementation for argument storage

---

## Technologies Used

- C Programming Language
- POSIX system calls (`fork`, `execv`, `wait`, `pipe`, `dup2`)
- File I/O and directory handling (`open`, `readdir`)
- Makefile for build automation

---

## Installation & Compilation

Clone the repository and compile using `make`:

```bash
git clone https://github.com/jakedickman/MyShell.git
cd MyShell
make
