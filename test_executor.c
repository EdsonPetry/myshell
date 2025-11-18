#define _POSIX_C_SOURCE 200809L
#include "parser.h"
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// REPLACE
int execute(ParsedCmd *cmd, int prev_status, int is_interactive,
            int *should_exit) {
  (void)cmd;
  (void)prev_status;
  (void)is_interactive;
  (void)should_exit;

  // just return failure
  return 99;
}

// assertion macros

static int tests_passed = 0;
static int tests_failed = 0;
static char current_test[256] = "";

#define COLOR_RED "\x1b[31m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_RESET "\x1b[0m"

#define TEST_START(name)                                                       \
  do {                                                                         \
    snprintf(current_test, sizeof(current_test), "%s", name);                  \
  } while (0)

#define ASSERT_EQUAL(actual, expected)                                         \
  do {                                                                         \
    if ((actual) != (expected)) {                                              \
      printf(COLOR_RED "FAIL" COLOR_RESET ": %s\n", current_test);             \
      printf("  Expected %d, got %d\n", (expected), (actual));                 \
      printf("  at %s:%d\n", __FILE__, __LINE__);                              \
      tests_failed++;                                                          \
      goto cleanup;                                                            \
    }                                                                          \
  } while (0)

#define ASSERT_NOT_EQUAL(actual, expected)                                     \
  do {                                                                         \
    if ((actual) == (expected)) {                                              \
      printf(COLOR_RED "FAIL" COLOR_RESET ": %s\n", current_test);             \
      printf("  Expected not %d, got %d\n", (expected), (actual));             \
      printf("  at %s:%d\n", __FILE__, __LINE__);                              \
      tests_failed++;                                                          \
      goto cleanup;                                                            \
    }                                                                          \
  } while (0)

#define ASSERT_TRUE(condition)                                                 \
  do {                                                                         \
    if (!(condition)) {                                                        \
      printf(COLOR_RED "FAIL" COLOR_RESET ": %s\n", current_test);             \
      printf("  Condition failed: %s\n", #condition);                          \
      printf("  at %s:%d\n", __FILE__, __LINE__);                              \
      tests_failed++;                                                          \
      goto cleanup;                                                            \
    }                                                                          \
  } while (0)

#define ASSERT_FALSE(condition) ASSERT_TRUE(!(condition))

#define ASSERT_STR_EQUAL(actual, expected)                                     \
  do {                                                                         \
    if (strcmp((actual), (expected)) != 0) {                                   \
      printf(COLOR_RED "FAIL" COLOR_RESET ": %s\n", current_test);             \
      printf("  Expected '%s', got '%s'\n", (expected), (actual));             \
      printf("  at %s:%d\n", __FILE__, __LINE__);                              \
      tests_failed++;                                                          \
      goto cleanup;                                                            \
    }                                                                          \
  } while (0)

#define TEST_PASS()                                                            \
  do {                                                                         \
    printf(COLOR_GREEN "PASS" COLOR_RESET ": %s\n", current_test);             \
    tests_passed++;                                                            \
  } while (0)

static char test_dir[1024];
static char original_cwd[1024];

ParsedCmd *make_cmd(int num_commands, int is_and, int is_or,
                    const char *input_file, const char *output_file) {
  ParsedCmd *cmd = malloc(sizeof(ParsedCmd));
  if (!cmd)
    return NULL;

  cmd->commands = calloc(num_commands, sizeof(Command));
  cmd->num_commands = num_commands;
  cmd->is_and = is_and;
  cmd->is_or = is_or;
  cmd->input_file = input_file ? strdup(input_file) : NULL;
  cmd->output_file = output_file ? strdup(output_file) : NULL;

  for (int i = 0; i < num_commands; i++) {
    cmd->commands[i].args = NULL;
    cmd->commands[i].num_args = 0;
  }

  return cmd;
}

void set_args(ParsedCmd *cmd, int cmd_idx, int num_args, ...) {
  if (!cmd || cmd_idx >= cmd->num_commands)
    return;

  va_list args;
  va_start(args, num_args);

  cmd->commands[cmd_idx].args = calloc(num_args + 1, sizeof(char *));
  cmd->commands[cmd_idx].num_args = num_args;

  for (int i = 0; i < num_args; i++) {
    const char *arg = va_arg(args, const char *);
    cmd->commands[cmd_idx].args[i] = strdup(arg);
  }
  cmd->commands[cmd_idx].args[num_args] = NULL;

  va_end(args);
}

void free_cmd(ParsedCmd *cmd) {
  if (!cmd)
    return;

  for (int i = 0; i < cmd->num_commands; i++) {
    if (cmd->commands[i].args) {
      for (int j = 0; j < cmd->commands[i].num_args; j++) {
        free(cmd->commands[i].args[j]);
      }
      free(cmd->commands[i].args);
    }
  }

  free(cmd->commands);
  free(cmd->input_file);
  free(cmd->output_file);
  free(cmd);
}

void make_file(const char *path, const char *content) {
  FILE *f = fopen(path, "w");
  if (f) {
    fprintf(f, "%s", content);
    fclose(f);
  }
}

char *read_file(const char *path) {
  FILE *f = fopen(path, "r");
  if (!f)
    return NULL;

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *content = malloc(size + 1);
  if (content) {
    size_t read = fread(content, 1, size, f);
    content[read] = '\0';
  }
  fclose(f);
  return content;
}

int exists(const char *path) { return access(path, F_OK) == 0; }

// Test built-ins

void test_cd_valid(void) {
  TEST_START("cd to /tmp");

  ParsedCmd *cmd = make_cmd(1, 0, 0, NULL, NULL);
  set_args(cmd, 0, 2, "cd", "/tmp");

  int should_exit = 0;
  int result = execute(cmd, 0, 1, &should_exit);

  ASSERT_EQUAL(result, 0);
  ASSERT_EQUAL(should_exit, 0);

  char cwd[1024];
  getcwd(cwd, sizeof(cwd));
  ASSERT_STR_EQUAL(cwd, "/tmp");

  TEST_PASS();

cleanup:
  chdir(original_cwd); // restore
  free_cmd(cmd);
}

void test_cd_invalid(void) {
  TEST_START("cd to invalid path");

  ParsedCmd *cmd = make_cmd(1, 0, 0, NULL, NULL);
  set_args(cmd, 0, 2, "cd", "/this_should_not_exist_xyz123");

  int should_exit = 0;
  int result = execute(cmd, 0, 1, &should_exit);

  ASSERT_EQUAL(result, 1);
  ASSERT_EQUAL(should_exit, 0);

  TEST_PASS();

cleanup:
  free_cmd(cmd);
}

void test_pwd_basic(void) {
  TEST_START("pwd command");

  char outfile[1024];
  snprintf(outfile, sizeof(outfile), "%s/pwd_out.txt", test_dir);

  ParsedCmd *cmd = make_cmd(1, 0, 0, NULL, outfile);
  set_args(cmd, 0, 1, "pwd");

  int should_exit = 0;
  int result = execute(cmd, 0, 1, &should_exit);

  ASSERT_EQUAL(result, 0);
  ASSERT_EQUAL(should_exit, 0);

  if (exists(outfile)) {
    char *content = read_file(outfile);
    if (content) {
      char cwd[1024];
      getcwd(cwd, sizeof(cwd));
      ASSERT_TRUE(strstr(content, cwd) != NULL);
      free(content);
    }
    unlink(outfile);
  }

  TEST_PASS();

cleanup:
  free_cmd(cmd);
}

void test_which_valid(void) {
  TEST_START("which ls");

  ParsedCmd *cmd = make_cmd(1, 0, 0, NULL, NULL);
  set_args(cmd, 0, 2, "which", "ls");

  int should_exit = 0;
  int result = execute(cmd, 0, 1, &should_exit);

  ASSERT_EQUAL(result, 0);
  ASSERT_EQUAL(should_exit, 0);

  TEST_PASS();

cleanup:
  free_cmd(cmd);
}

void test_which_builtin(void) {
  TEST_START("which on builtin command");

  ParsedCmd *cmd = make_cmd(1, 0, 0, NULL, NULL);
  set_args(cmd, 0, 2, "which", "cd");

  int should_exit = 0;
  int result = execute(cmd, 0, 1, &should_exit);

  ASSERT_EQUAL(result, 1);
  ASSERT_EQUAL(should_exit, 0);

  TEST_PASS();

cleanup:
  free_cmd(cmd);
}

void test_exit_basic(void) {
  TEST_START("exit command");

  ParsedCmd *cmd = make_cmd(1, 0, 0, NULL, NULL);
  set_args(cmd, 0, 1, "exit");

  int should_exit = 0;
  int result = execute(cmd, 0, 1, &should_exit);

  ASSERT_EQUAL(result, 0);
  ASSERT_EQUAL(should_exit, 1);

  TEST_PASS();

cleanup:
  free_cmd(cmd);
}

void test_die_basic(void) {
  TEST_START("die command");

  ParsedCmd *cmd = make_cmd(1, 0, 0, NULL, NULL);
  set_args(cmd, 0, 2, "die", "error");

  int should_exit = 0;
  int result = execute(cmd, 0, 1, &should_exit);
  (void)result;

  ASSERT_EQUAL(should_exit, 1);

  TEST_PASS();

cleanup:
  free_cmd(cmd);
}

// Conditional tests

void test_and_success(void) {
  TEST_START("AND after success");

  ParsedCmd *cmd = make_cmd(1, 1, 0, NULL, NULL);
  set_args(cmd, 0, 1, "pwd");

  int should_exit = 0;
  int result = execute(cmd, 0, 1, &should_exit); // prev_status = 0

  ASSERT_EQUAL(result, 0); // should execute
  ASSERT_EQUAL(should_exit, 0);

  TEST_PASS();

cleanup:
  free_cmd(cmd);
}

void test_and_failure(void) {
  TEST_START("AND after failure");

  ParsedCmd *cmd = make_cmd(1, 1, 0, NULL, NULL);
  set_args(cmd, 0, 1, "pwd");

  int should_exit = 0;
  int result = execute(cmd, 1, 1, &should_exit); // prev_status = 1

  ASSERT_EQUAL(result, 1); // should skip
  ASSERT_EQUAL(should_exit, 0);

  TEST_PASS();

cleanup:
  free_cmd(cmd);
}

void test_or_success(void) {
  TEST_START("OR after success");

  ParsedCmd *cmd = make_cmd(1, 0, 1, NULL, NULL);
  set_args(cmd, 0, 1, "pwd");

  int should_exit = 0;
  int result = execute(cmd, 0, 1, &should_exit);

  ASSERT_EQUAL(result, 0); // should skip
  ASSERT_EQUAL(should_exit, 0);

  TEST_PASS();

cleanup:
  free_cmd(cmd);
}

void test_or_failure(void) {
  TEST_START("OR after failure");

  ParsedCmd *cmd = make_cmd(1, 0, 1, NULL, NULL);
  set_args(cmd, 0, 1, "pwd");

  int should_exit = 0;
  int result = execute(cmd, 1, 1, &should_exit);

  ASSERT_EQUAL(result, 0); // should execute
  ASSERT_EQUAL(should_exit, 0);

  TEST_PASS();

cleanup:
  free_cmd(cmd);
}

// External commands

void test_echo_command(void) {
  TEST_START("echo command");

  char outfile[1024];
  snprintf(outfile, sizeof(outfile), "%s/echo_out.txt", test_dir);

  ParsedCmd *cmd = make_cmd(1, 0, 0, NULL, outfile);
  set_args(cmd, 0, 2, "echo", "hello");

  int should_exit = 0;
  int result = execute(cmd, 0, 1, &should_exit);

  ASSERT_EQUAL(result, 0);
  ASSERT_EQUAL(should_exit, 0);

  // output should contain "hello"
  if (exists(outfile)) {
    char *content = read_file(outfile);
    if (content) {
      ASSERT_TRUE(strstr(content, "hello") != NULL);
      free(content);
    }
    unlink(outfile);
  }

  TEST_PASS();

cleanup:
  free_cmd(cmd);
}

void test_true_command(void) {
  TEST_START("true command");

  ParsedCmd *cmd = make_cmd(1, 0, 0, NULL, NULL);
  set_args(cmd, 0, 1, "true");

  int should_exit = 0;
  int result = execute(cmd, 0, 1, &should_exit);

  ASSERT_EQUAL(result, 0);
  ASSERT_EQUAL(should_exit, 0);

  TEST_PASS();

cleanup:
  free_cmd(cmd);
}

void test_false_command(void) {
  TEST_START("false command");

  ParsedCmd *cmd = make_cmd(1, 0, 0, NULL, NULL);
  set_args(cmd, 0, 1, "false");

  int should_exit = 0;
  int result = execute(cmd, 0, 1, &should_exit);

  ASSERT_EQUAL(result, 1);
  ASSERT_EQUAL(should_exit, 0);

  TEST_PASS();

cleanup:
  free_cmd(cmd);
}

void test_nonexistent_command(void) {
  TEST_START("nonexistent command");

  ParsedCmd *cmd = make_cmd(1, 0, 0, NULL, NULL);
  set_args(cmd, 0, 1, "nonexistent_xyz_command");

  int should_exit = 0;
  int result = execute(cmd, 0, 1, &should_exit);

  ASSERT_EQUAL(result, 1);
  ASSERT_EQUAL(should_exit, 0);

  TEST_PASS();

cleanup:
  free_cmd(cmd);
}

// I/O redirection

void test_input_redirect(void) {
  TEST_START("input redirection");

  char infile[1024], outfile[1024];
  snprintf(infile, sizeof(infile), "%s/input.txt", test_dir);
  snprintf(outfile, sizeof(outfile), "%s/output.txt", test_dir);

  make_file(infile, "test input\n");

  ParsedCmd *cmd = make_cmd(1, 0, 0, infile, outfile);
  set_args(cmd, 0, 1, "cat");

  int should_exit = 0;
  int result = execute(cmd, 0, 1, &should_exit);

  ASSERT_EQUAL(result, 0);

  char *content = read_file(outfile);
  if (content) {
    ASSERT_STR_EQUAL(content, "test input\n");
    free(content);
  }

  TEST_PASS();

cleanup:
  unlink(infile);
  unlink(outfile);
  free_cmd(cmd);
}

void test_output_redirect(void) {
  TEST_START("output redirection");

  char outfile[1024];
  snprintf(outfile, sizeof(outfile), "%s/output.txt", test_dir);

  ParsedCmd *cmd = make_cmd(1, 0, 0, NULL, outfile);
  set_args(cmd, 0, 2, "echo", "test");

  int should_exit = 0;
  int result = execute(cmd, 0, 1, &should_exit);

  ASSERT_EQUAL(result, 0);
  ASSERT_TRUE(exists(outfile));

  char *content = read_file(outfile);
  if (content) {
    ASSERT_TRUE(strstr(content, "test") != NULL);
    free(content);
  }

  TEST_PASS();

cleanup:
  unlink(outfile);
  free_cmd(cmd);
}

void test_both_redirect(void) {
  TEST_START("both input and output redirection");

  char infile[1024], outfile[1024];
  snprintf(infile, sizeof(infile), "%s/in.txt", test_dir);
  snprintf(outfile, sizeof(outfile), "%s/out.txt", test_dir);

  make_file(infile, "data\n");

  ParsedCmd *cmd = make_cmd(1, 0, 0, infile, outfile);
  set_args(cmd, 0, 1, "cat");

  int should_exit = 0;
  int result = execute(cmd, 0, 1, &should_exit);

  ASSERT_EQUAL(result, 0);
  ASSERT_TRUE(exists(outfile));

  TEST_PASS();

cleanup:
  unlink(infile);
  unlink(outfile);
  free_cmd(cmd);
}

// Pipelines

void test_simple_pipeline(void) {
  TEST_START("simple pipeline: echo | cat");

  char outfile[1024];
  snprintf(outfile, sizeof(outfile), "%s/pipe_out.txt", test_dir);

  ParsedCmd *cmd = make_cmd(2, 0, 0, NULL, outfile);
  set_args(cmd, 0, 2, "echo", "piped");
  set_args(cmd, 1, 1, "cat");

  int should_exit = 0;
  int result = execute(cmd, 0, 1, &should_exit);

  ASSERT_EQUAL(result, 0);

  char *content = read_file(outfile);
  if (content) {
    ASSERT_TRUE(strstr(content, "piped") != NULL);
    free(content);
  }

  TEST_PASS();

cleanup:
  unlink(outfile);
  free_cmd(cmd);
}

void test_pipeline_status(void) {
  TEST_START("pipeline returns last command status");

  // "true | false" should return 1
  ParsedCmd *cmd = make_cmd(2, 0, 0, NULL, NULL);
  set_args(cmd, 0, 1, "true");
  set_args(cmd, 1, 1, "false");

  int should_exit = 0;
  int result = execute(cmd, 0, 1, &should_exit);

  ASSERT_EQUAL(result, 1);
  ASSERT_EQUAL(should_exit, 0);

  TEST_PASS();

cleanup:
  free_cmd(cmd);
}

// Errors

void test_null_command(void) {
  TEST_START("NULL command");

  int should_exit = 0;
  int result = execute(NULL, 0, 1, &should_exit);

  ASSERT_EQUAL(result, 0); // expected to return prev status
  ASSERT_EQUAL(should_exit, 0);

  TEST_PASS();

cleanup:
  return;
}

void test_empty_command(void) {
  TEST_START("empty command");

  ParsedCmd *cmd = make_cmd(0, 0, 0, NULL, NULL);

  int should_exit = 0;
  int result = execute(cmd, 0, 1, &should_exit);

  ASSERT_EQUAL(result, 0);
  ASSERT_EQUAL(should_exit, 0);

  TEST_PASS();

cleanup:
  free_cmd(cmd);
}

void setup_tests(void) {
  getcwd(original_cwd, sizeof(original_cwd));

  snprintf(test_dir, sizeof(test_dir), "/tmp/executor_manual_test_%d",
           getpid());
  mkdir(test_dir, 0755);
}

void cleanup_tests(void) {
  chdir(original_cwd);

  char cmd[1024];
  snprintf(cmd, sizeof(cmd), "rm -rf %s", test_dir);
  system(cmd);
}

int main(void) {
  printf("\n");
  printf("  Executor Tests\n");
  printf("\n");

  setup_tests();

  printf(COLOR_YELLOW "Built-in Commands:\n" COLOR_RESET);
  test_cd_valid();
  test_cd_invalid();
  test_pwd_basic();
  test_which_valid();
  test_which_builtin();
  test_exit_basic();
  test_die_basic();

  printf("\n" COLOR_YELLOW "Conditional Execution:\n" COLOR_RESET);
  test_and_success();
  test_and_failure();
  test_or_success();
  test_or_failure();

  printf("\n" COLOR_YELLOW "External Commands:\n" COLOR_RESET);
  test_echo_command();
  test_true_command();
  test_false_command();
  test_nonexistent_command();

  printf("\n" COLOR_YELLOW "I/O Redirection:\n" COLOR_RESET);
  test_input_redirect();
  test_output_redirect();
  test_both_redirect();

  printf("\n" COLOR_YELLOW "Pipelines:\n" COLOR_RESET);
  test_simple_pipeline();
  test_pipeline_status();

  printf("\n" COLOR_YELLOW "Error Cases:\n" COLOR_RESET);
  test_null_command();
  test_empty_command();

  cleanup_tests();

  printf("\n");
  printf("===============================================\n");
  printf("  Test Summary\n");
  printf("===============================================\n");
  printf(COLOR_GREEN "Passed: %d\n" COLOR_RESET, tests_passed);
  if (tests_failed > 0) {
    printf(COLOR_RED "Failed: %d\n" COLOR_RESET, tests_failed);
  } else {
    printf("Failed: %d\n", tests_failed);
  }
  printf("Total:  %d\n", tests_passed + tests_failed);
  printf("===============================================\n");
  printf("\n");

  if (tests_failed == 0) {
    printf(COLOR_GREEN "All tests passed!\n" COLOR_RESET);
    return EXIT_SUCCESS;
  } else {
    printf(COLOR_RED "Some tests failed.\n" COLOR_RESET);
    return EXIT_FAILURE;
  }
}
