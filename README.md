# myshell

A shell implementation with support for command parsing, pipelines, I/O
redirection, and conditional execution.

## Parser

The parser module (`parser.c`, `parser.h`) tokenizes and parses shell
command input into a structured format that can be executed.

### Data Structures

#### Command

```c
typedef struct {
  char **args;      // NULL terminated array of command arguments
  int num_args;     // number of arguments (not counting NULL terminator)
} Command;
```

#### ParsedCmd

```c
typedef struct ParsedCmd {
  Command *commands;      // array of commands
  int num_commands;       // number of commands in pipeline

  char *input_file;       // input redirection filename or NULL
  char *output_file;      // output redirection filename or NULL

  int is_and;            // 1 if command starts with "and" conditional
  int is_or;             // 1 if command starts with "or" conditional
} ParsedCmd;
```

### Functions

#### `ParsedCmd *parse(const char *line)`

Parses a shell command line into a ParsedCmd structure.

**Parameters:**

- `line`: The input command line string

**Returns:**

- Pointer to a ParsedCmd structure on success
- NULL if the line is empty, contains only whitespace/comments, or has syntax errors

**Features:**

- **Comment handling**: Lines starting with `#` or content after `#` are treated as comments
- **Tokenization**: Splits input by whitespace while treating `<`, `>`, and `|` as separate tokens
- **Conditional execution**: Recognizes `and` and `or` keywords at the start of commands
- **Input redirection**: Parses `< filename` syntax
- **Output redirection**: Parses `> filename` syntax
- **Pipelines**: Supports multiple commands separated by `|`

**Example:**

```c
ParsedCmd *cmd = parse("cat < input.txt | grep error > output.txt");
// Creates a ParsedCmd with:
//   - num_commands: 2
//   - commands[0]: {"cat"}
//   - commands[1]: {"grep", "error"}
//   - input_file: "input.txt"
//   - output_file: "output.txt"
```

#### `void free_parsed_cmd(ParsedCmd *cmd)`

Frees all memory allocated for a ParsedCmd structure.

**Parameters:**

- `cmd`: Pointer to ParsedCmd to free (safe to pass NULL)

### Syntax Rules

1. **Comments**: `#` introduces a comment; everything after it is ignored

   ```
   ls -la  # list all files
   # this entire line is a comment
   ```

2. **Whitespace**: Multiple spaces and tabs are treated as single separators

   ```
   ls    -la      /home     # equivalent to: ls -la /home
   ```

3. **Conditional Execution**: Commands can start with `and` or `or` keywords

   ```
   and ls -la    # execute only if previous command succeeded
   or pwd        # execute only if previous command failed
   ```

4. **I/O Redirection**:

   ```
   cat < input.txt          # read from input.txt
   ls > output.txt          # write to output.txt
   sort < in.txt > out.txt  # both input and output redirection
   ```

5. **Pipelines**: Commands can be chained with `|`

   ```
   ls | grep test                              # 2-stage pipeline
   cat file.txt | grep error | wc -l          # 3-stage pipeline
   ps aux | grep bash | awk '{print $2}'      # complex pipeline
   ```

6. **Error Cases**: The parser returns NULL for:
   - Empty lines or whitespace-only lines
   - Lines with only comments
   - Lines with only conditional keywords (`and` or `or` alone)
   - Missing filenames after `<` or `>`
   - Redirection operators used as filenames
   - Empty commands in a pipeline (e.g., `ls | | grep`)

## Parser Tests

The parser is thoroughly tested using CUnit framework (`test_parser.c`).
Run tests with:

```bash
make test_parser
```

### Test Suites

#### 1. Comment Handling

- Full line comments
- Inline comments
- Empty lines
- Whitespace-only lines
- Comments after whitespace

#### 2. Basic Tokenization

- Simple commands (`pwd`)
- Commands with arguments (`ls -la /home`)
- Multiple spaces between tokens
- Leading/trailing whitespace

#### 3. Conditional Commands

- `and` keyword with commands
- `or` keyword with commands
- Conditionals with arguments
- Invalid cases (keyword only, no command)

#### 4. Input Redirection

- Simple input redirection (`cat < file.txt`)
- Input redirection with command arguments

#### 5. Output Redirection

- Simple output redirection (`ls > file.txt`)
- Output redirection with command arguments

#### 6. Input and Output Redirection

- Both redirections together
- Both redirections in reverse order

#### 7. Pipeline Commands

- Two-command pipelines (`ls | grep test`)
- Three-command pipelines
- Four-command pipelines
- Pipelines with extra whitespace

#### 8. Combined Features

- Conditionals with redirection
- Conditionals with pipelines
- Pipelines with input redirection
- Pipelines with output redirection
- Complex combinations (conditionals + pipelines + both redirections)

#### 9. Memory and Cleanup

- Multiple parse operations
- NULL pointer handling in free_parsed_cmd

### Test Output

The test suite provides verbose output showing:

- Number of test suites run
- Number of tests run
- Number of tests passed/failed
- Detailed failure information (if any)

