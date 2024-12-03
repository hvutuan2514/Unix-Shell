# Final Mini Project: The Unix Shell

## Overview
This project implements a simple Unix shell that can execute commands in both interactive and batch modes. The shell supports basic commands, built-in commands, and redirection.

## Features
- **Interactive Mode**: Allows the user to type commands directly into the shell.
- **Batch Mode**: Shell is given an input file of commands.
- **Built-in Commands**:
  - `exit`: Exit the shell.
  - `cd <directory>`: Change directories.
  - `path <dir1> <dir2> ...`: Modify directories to look for executable programs.
  - `loop <count> <command>`: Execute a command multiple times.
- Can run executable commands as well (ls, cat, echo, cp, clear, mv, etc.)
-  **Redirection**: Supports output redirection with `>`.

## How to Compile and Run
1. Connect to the university `ember` Unix system using SSH Client.
2. Transfer `hannah.c` and `batch.txt` files into your directory on the Unix system.
3. Compile the program using `gcc`:
```bash
gcc -std=c99 -o hannah hannah.c
```
4. To enter **interactive** mode, run the shell without arguments:
```bash
./hannah
```
Example Usage:
```bash
hannah> ls
hannah> cd ..
hannah> path /bin /usr/bin
hannah> loop 5 echo hello $loop
hannah> exit
```
5. To enter **batch** mode, provide a file containing commands as an argument:
```bash
./hannah batch.txt
```
