# LSH — A Tiny Unix Shell in C

A minimal, teaching-oriented shell that demonstrates core systems programming ideas:

* A **REPL** loop (read–evaluate–print)
* **Tokenizing** user input into an `argv` array
* **Built-in commands**: `cd`, `help`, `exit`
* Launching external programs with **fork/execvp** and **waitpid**

---

## Table of Contents

* [Project Structure](#project-structure)
* [Build & Run](#build--run)
* [Quick Demo](#quick-demo)
* [How It Works](#how-it-works)
* [Public API](#public-api)
* [Code Walkthrough](#code-walkthrough)
* [Extending the Shell (Add a Builtin)](#extending-the-shell-add-a-builtin)
* [Memory Model & Safety Notes](#memory-model--safety-notes)
* [Known Limitations](#known-limitations)
* [Troubleshooting](#troubleshooting)
* [Optional: Makefile](#optional-makefile)

---

## Project Structure

```
.
├── main.c          # REPL: read_line → split_line → execute
├── shell_funcs.c   # Builtins + external command launcher (fork/execvp/waitpid)
└── shell_funcs.h   # Public function declarations + system headers
```

---

## Build & Run

### Linux / macOS (clang or gcc)

```sh
gcc -Wall -Wextra -std=c11 -o lsh main.c shell_funcs.c
./lsh
```

**Recommended debug build:**

```sh
gcc -g -fsanitize=address -fno-omit-frame-pointer -O1 -Wall -Wextra -std=c11 -o lsh main.c shell_funcs.c
./lsh
```

### Windows

* Use **WSL** (Ubuntu) and follow the Linux steps, **or**
* Use **MSYS2/MinGW**. (Note: POSIX calls like `fork`, `execvp`, `waitpid` are not available in plain MSVC.)

---

## Quick Demo

```
> help
Omer Hayat's LSH
Type program names and arguments, and hit enter.
The following are built-in:
 cd
 help
 exit
Use the man command for information on other programs.

> pwd
/home/user

> ls -la
... (normal ls output)

> cd /tmp
> exit
```

---

## How It Works

1. **Prompt & Read**
   `lsh_loop()` prints `> `, then `lsh_read_line()` reads a full line from `stdin`.

2. **Tokenize**
   `lsh_split_line()` splits the input by whitespace (`" \t\r\n\a"`) into a dynamically sized `char **args` array terminated by `NULL`.

3. **Execute**
   `lsh_execute()` checks if `args[0]` matches a **builtin** (`cd`, `help`, `exit`).

   * If builtin → call the builtin function and return its status (`1` to continue, `0` to exit).
   * Otherwise → `lsh_launch(args)`:

     * `fork()` to create a child,
     * child runs `execvp(args[0], args)`,
     * parent waits with `waitpid()`.

The returned status controls whether the REPL loop continues.

---

## Public API

From `shell_funcs.h`:

```c
int lsh_cd(char **args);       // cd [path]
int lsh_help(char **args);     // prints builtins & basic help
int lsh_exit(char **args);     // returns 0 to stop REPL

int lsh_num_builtins(void);    // number of builtins (derived from builtin_str[])
int lsh_execute(char **args);  // route to builtin or external command
int lsh_launch(char **args);   // fork/execvp/wait wrapper
```

---

## Code Walkthrough

### `main.c`

* **Constants**

  * `LSH_RL_BUFSIZE` — starting size for line input buffer (1024).
  * `LSH_TOK_BUFSIZE` — initial token array capacity (64).
  * `LSH_TOK_DELIM` — delimiters passed to `strtok`.

* **`lsh_read_line()`**
  Reads characters with `getchar()` until newline/EOF, stores them in a growable buffer, and returns a NUL-terminated string.

* **`lsh_split_line(char *line)`**
  Uses `strtok` to split `line` into tokens. Grows the `char **tokens` array as needed via `realloc`. Returns a NULL-terminated `argv` array.

* **`lsh_loop()`**
  The REPL: prompt → read → split → execute → free. Continues while the last command returned `1`.

* **`main()`**
  Calls `lsh_loop()` and exits with `EXIT_SUCCESS`.

### `shell_funcs.c`

* **Builtins Table**

  ```c
  char *builtin_str[] = { "cd", "help", "exit" };
  int (*builtin_func[]) (char **) = { &lsh_cd, &lsh_help, &lsh_exit };
  ```

  `lsh_num_builtins()` returns the length of this table.

* **Builtins**

  * `lsh_cd(args)` — changes directory to `args[1]` (prints an error if missing or invalid).
  * `lsh_help(args)` — prints usage help and lists builtins.
  * `lsh_exit(args)` — returns `0` to signal the REPL to stop.

* **Dispatcher**

  * `lsh_execute(args)` — matches `args[0]` to a builtin; otherwise, launches an external command.

* **External launcher**

  * `lsh_launch(args)` — `fork()` → child `execvp()` → parent `waitpid()` until child exits or is signaled.

### `shell_funcs.h`

* System headers, include guard (`#ifndef LSH_H` … `#endif`), and all public function declarations.

---

## Extending the Shell (Add a Builtin)

Example: add a `pwd` builtin.

1. **Declare** in `shell_funcs.h`:

```c
int lsh_pwd(char **args);
```

2. **Implement** in `shell_funcs.c`:

```c
int lsh_pwd(char **args) {
    char buf[4096];
    if (getcwd(buf, sizeof buf) == NULL) {
        perror("lsh");
        return 1;
    }
    puts(buf);
    return 1;
}
```

3. **Register** it:

```c
char *builtin_str[] = { "cd", "help", "exit", "pwd" };
int (*builtin_func[]) (char **) = { &lsh_cd, &lsh_help, &lsh_exit, &lsh_pwd };
```

Rebuild and run:

```
> pwd
/home/user
```

---

## Memory Model & Safety Notes

* `lsh_split_line()` stores pointers to tokens **inside the original `line` buffer** (because `strtok` inserts `'\0'` terminators).
  After `lsh_execute(args)` returns, you should:

  ```c
  free(line);  // frees the underlying characters
  free(args);  // frees the token pointer array (not each token)
  ```

* **`realloc` safety pattern:**
  Prefer assigning to a temp pointer and checking for `NULL` to avoid leaks:

  ```c
  char *tmp = realloc(buffer, new_size);
  if (!tmp) { free(buffer); /* handle error */ }
  buffer = tmp;
  ```

* **Bug to be aware of (and easy fix):**
  In `lsh_read_line`, the buffer growth check is placed **after** the infinite loop, so it never runs. Move the growth check **inside** the loop right after incrementing `position` so very long input won’t overflow the buffer.

  ```c
  // after position++
  if (position >= bufsize) {
      bufsize += LSH_RL_BUFSIZE;
      char *tmp = realloc(buffer, bufsize);
      if (!tmp) { free(buffer); fprintf(stderr, "lsh: allocation error\n"); exit(EXIT_FAILURE); }
      buffer = tmp;
  }
  ```

---

## Known Limitations

* **Tokenization is simple.** Uses `strtok` with whitespace delimiters only:

  * No support for quotes (`"hello world"` as one token), escapes, pipes (`|`), or redirection (`>`, `<`).
* **No job control.** Background tasks (`&`), `fg`, `bg`, signal handling for interactive control are not implemented.
* **No search path customization.** Relies on environment `PATH` for external programs.

**Ideas to extend:**

* Implement a custom tokenizer that understands quotes and escapes.
* Add I/O redirection and pipelines.
* Add a command history and line editing via GNU Readline.
* Implement job control with process groups and signals.

---

## Troubleshooting

* **Compiler says “expected an expression” around allocation lines**
  Usually caused by a nearby typo (e.g., writing `sizeofchar(char )` instead of `sizeof(char *)`). Such errors can make the compiler misinterpret earlier lines.

* **`cd` prints an error**
  Ensure you provide a path: `cd /some/path`. Errors are reported with `perror("lsh")`.

* **External command not found**
  Ensure it exists in your `PATH` or provide an absolute path (e.g., `/bin/ls`).

* **Program crashes on very long lines**
  Apply the `lsh_read_line` growth fix described above.

---

## Optional: Makefile

You can drop this `Makefile` into the repo to build consistently:

```make
CC      := gcc
CFLAGS  := -Wall -Wextra -std=c11
DEBUG   := -g -fsanitize=address -fno-omit-frame-pointer -O1
TARGET  := lsh
SRC     := main.c shell_funcs.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC)

debug: $(SRC)
	$(CC) $(CFLAGS) $(DEBUG) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
```

Usage:

```sh
make        # builds lsh
./lsh
make clean  # removes the binary
```

