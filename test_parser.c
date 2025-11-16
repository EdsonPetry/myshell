#include "parser.h"
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char **args;
  int num_args;
} Command;

typedef struct ParsedCmd {
  Command *commands;
  int num_commands;
  char *input_file;
  char *output_file;
  int is_and;
  int is_or;
} ParsedCmd;

// helper to check if a command has the expected arguments
int verify_command_args(Command *cmd, int expected_num_args,
                        char **expected_args) {
  if (cmd->num_args != expected_num_args) {
    printf("Expected %d args, got %d\n", expected_num_args, cmd->num_args);
    return 0;
  }

  for (int i = 0; i < expected_num_args; i++) {
    if (strcmp(cmd->args[i], expected_args[i]) != 0) {
      printf("Arg %d: expected '%s', got '%s'\n", i, expected_args[i],
             cmd->args[i]);
      return 0;
    }
  }

  if (cmd->args[expected_num_args] != NULL) {
    printf("Args array not NULL-terminated\n");
    return 0;
  }

  return 1;
}

void print_parsed_cmd(ParsedCmd *cmd) {
  if (!cmd) {
    printf("ParsedCmd is NULL\n");
    return;
  }

  printf("ParsedCmd {\n");
  printf("  num_commands: %d\n", cmd->num_commands);
  printf("  is_and: %d, is_or: %d\n", cmd->is_and, cmd->is_or);
  printf("  input_file: %s\n", cmd->input_file ? cmd->input_file : "NULL");
  printf("  output_file: %s\n", cmd->output_file ? cmd->output_file : "NULL");

  for (int i = 0; i < cmd->num_commands; i++) {
    printf("  Command %d (%d args):\n", i, cmd->commands[i].num_args);
    for (int j = 0; j < cmd->commands[i].num_args; j++) {
      printf("    [%d]: '%s'\n", j, cmd->commands[i].args[j]);
    }
  }
  printf("}\n");
}

/* Test Suite 1: Comment Handling */

void test_full_line_comment(void) {
  ParsedCmd *cmd = parse("# this is a comment");
  CU_ASSERT_PTR_NULL(cmd);
  free_parsed_cmd(cmd);
}

void test_inline_comment(void) {
  ParsedCmd *cmd = parse("ls -la # list files");
  CU_ASSERT_PTR_NOT_NULL(cmd);
  if (cmd) {
    CU_ASSERT_EQUAL(cmd->num_commands, 1);
    CU_ASSERT_EQUAL(cmd->commands[0].num_args, 2);
    char *expected[] = {"ls", "-la"};
    CU_ASSERT_TRUE(verify_command_args(&cmd->commands[0], 2, expected));
    free_parsed_cmd(cmd);
  }
}

void test_empty_line(void) {
  ParsedCmd *cmd = parse("");
  CU_ASSERT_PTR_NULL(cmd);
  free_parsed_cmd(cmd);
}

void test_whitespace_only(void) {
  ParsedCmd *cmd = parse("   \t  ");
  CU_ASSERT_PTR_NULL(cmd);
  free_parsed_cmd(cmd);
}

void test_comment_after_whitespace(void) {
  ParsedCmd *cmd = parse("  # comment");
  CU_ASSERT_PTR_NULL(cmd);
  free_parsed_cmd(cmd);
}

/* Test Suite 2: Basic Tokenization */

void test_simple_command(void) {
  ParsedCmd *cmd = parse("pwd");
  CU_ASSERT_PTR_NOT_NULL(cmd);
  if (cmd) {
    CU_ASSERT_EQUAL(cmd->num_commands, 1);
    CU_ASSERT_EQUAL(cmd->commands[0].num_args, 1);
    char *expected[] = {"pwd"};
    CU_ASSERT_TRUE(verify_command_args(&cmd->commands[0], 1, expected));
    CU_ASSERT_EQUAL(cmd->is_and, 0);
    CU_ASSERT_EQUAL(cmd->is_or, 0);
    free_parsed_cmd(cmd);
  }
}

void test_command_with_args(void) {
  ParsedCmd *cmd = parse("ls -la /home");
  CU_ASSERT_PTR_NOT_NULL(cmd);
  if (cmd) {
    CU_ASSERT_EQUAL(cmd->num_commands, 1);
    CU_ASSERT_EQUAL(cmd->commands[0].num_args, 3);
    char *expected[] = {"ls", "-la", "/home"};
    CU_ASSERT_TRUE(verify_command_args(&cmd->commands[0], 3, expected));
    free_parsed_cmd(cmd);
  }
}

void test_command_with_multiple_spaces(void) {
  ParsedCmd *cmd = parse("echo    hello     world");
  CU_ASSERT_PTR_NOT_NULL(cmd);
  if (cmd) {
    CU_ASSERT_EQUAL(cmd->num_commands, 1);
    CU_ASSERT_EQUAL(cmd->commands[0].num_args, 3);
    char *expected[] = {"echo", "hello", "world"};
    CU_ASSERT_TRUE(verify_command_args(&cmd->commands[0], 3, expected));
    free_parsed_cmd(cmd);
  }
}

void test_command_with_leading_spaces(void) {
  ParsedCmd *cmd = parse("   ls   ");
  CU_ASSERT_PTR_NOT_NULL(cmd);
  if (cmd) {
    CU_ASSERT_EQUAL(cmd->num_commands, 1);
    CU_ASSERT_EQUAL(cmd->commands[0].num_args, 1);
    char *expected[] = {"ls"};
    CU_ASSERT_TRUE(verify_command_args(&cmd->commands[0], 1, expected));
    free_parsed_cmd(cmd);
  }
}

/* Test Suite 3: Conditional Commands */

void test_and_conditional(void) {
  ParsedCmd *cmd = parse("and ls");
  CU_ASSERT_PTR_NOT_NULL(cmd);
  if (cmd) {
    CU_ASSERT_EQUAL(cmd->is_and, 1);
    CU_ASSERT_EQUAL(cmd->is_or, 0);
    CU_ASSERT_EQUAL(cmd->num_commands, 1);
    CU_ASSERT_EQUAL(cmd->commands[0].num_args, 1);
    char *expected[] = {"ls"};
    CU_ASSERT_TRUE(verify_command_args(&cmd->commands[0], 1, expected));
    free_parsed_cmd(cmd);
  }
}

void test_or_conditional(void) {
  ParsedCmd *cmd = parse("or pwd");
  CU_ASSERT_PTR_NOT_NULL(cmd);
  if (cmd) {
    CU_ASSERT_EQUAL(cmd->is_and, 0);
    CU_ASSERT_EQUAL(cmd->is_or, 1);
    CU_ASSERT_EQUAL(cmd->num_commands, 1);
    CU_ASSERT_EQUAL(cmd->commands[0].num_args, 1);
    char *expected[] = {"pwd"};
    CU_ASSERT_TRUE(verify_command_args(&cmd->commands[0], 1, expected));
    free_parsed_cmd(cmd);
  }
}

void test_and_with_args(void) {
  ParsedCmd *cmd = parse("and ls -la /tmp");
  CU_ASSERT_PTR_NOT_NULL(cmd);
  if (cmd) {
    CU_ASSERT_EQUAL(cmd->is_and, 1);
    CU_ASSERT_EQUAL(cmd->is_or, 0);
    CU_ASSERT_EQUAL(cmd->commands[0].num_args, 3);
    char *expected[] = {"ls", "-la", "/tmp"};
    CU_ASSERT_TRUE(verify_command_args(&cmd->commands[0], 3, expected));
    free_parsed_cmd(cmd);
  }
}

void test_only_and_keyword(void) {
  ParsedCmd *cmd = parse("and");
  CU_ASSERT_PTR_NULL(cmd); // return NULL for empty command
  free_parsed_cmd(cmd);
}

void test_only_or_keyword(void) {
  ParsedCmd *cmd = parse("or");
  CU_ASSERT_PTR_NULL(cmd);
  free_parsed_cmd(cmd);
}

/* Test Suite 4: Input Redirection */

void test_input_redirection_simple(void) {
  ParsedCmd *cmd = parse("cat < input.txt");
  CU_ASSERT_PTR_NOT_NULL(cmd);
  if (cmd) {
    CU_ASSERT_EQUAL(cmd->num_commands, 1);
    char *expected[] = {"cat"};
    CU_ASSERT_TRUE(verify_command_args(&cmd->commands[0], 1, expected));
    CU_ASSERT_PTR_NOT_NULL(cmd->input_file);
    CU_ASSERT_STRING_EQUAL(cmd->input_file, "input.txt");
    CU_ASSERT_PTR_NULL(cmd->output_file);
    free_parsed_cmd(cmd);
  }
}

void test_input_redirection_with_args(void) {
  ParsedCmd *cmd = parse("grep pattern < file.txt");
  CU_ASSERT_PTR_NOT_NULL(cmd);
  if (cmd) {
    CU_ASSERT_EQUAL(cmd->num_commands, 1);
    char *expected[] = {"grep", "pattern"};
    CU_ASSERT_TRUE(verify_command_args(&cmd->commands[0], 2, expected));
    CU_ASSERT_PTR_NOT_NULL(cmd->input_file);
    CU_ASSERT_STRING_EQUAL(cmd->input_file, "file.txt");
    free_parsed_cmd(cmd);
  }
}

/* Test Suite 5: Output Redirection */

void test_output_redirection_simple(void) {
  ParsedCmd *cmd = parse("ls > output.txt");
  CU_ASSERT_PTR_NOT_NULL(cmd);
  if (cmd) {
    CU_ASSERT_EQUAL(cmd->num_commands, 1);
    char *expected[] = {"ls"};
    CU_ASSERT_TRUE(verify_command_args(&cmd->commands[0], 1, expected));
    CU_ASSERT_PTR_NULL(cmd->input_file);
    CU_ASSERT_PTR_NOT_NULL(cmd->output_file);
    CU_ASSERT_STRING_EQUAL(cmd->output_file, "output.txt");
    free_parsed_cmd(cmd);
  }
}

void test_output_redirection_with_args(void) {
  ParsedCmd *cmd = parse("ls -la /tmp > files.txt");
  CU_ASSERT_PTR_NOT_NULL(cmd);
  if (cmd) {
    CU_ASSERT_EQUAL(cmd->num_commands, 1);
    char *expected[] = {"ls", "-la", "/tmp"};
    CU_ASSERT_TRUE(verify_command_args(&cmd->commands[0], 3, expected));
    CU_ASSERT_PTR_NOT_NULL(cmd->output_file);
    CU_ASSERT_STRING_EQUAL(cmd->output_file, "files.txt");
    free_parsed_cmd(cmd);
  }
}

/* Test Suite 6: Input and Output Redirection */

void test_both_redirections(void) {
  ParsedCmd *cmd = parse("sort < input.txt > output.txt");
  CU_ASSERT_PTR_NOT_NULL(cmd);
  if (cmd) {
    CU_ASSERT_EQUAL(cmd->num_commands, 1);
    char *expected[] = {"sort"};
    CU_ASSERT_TRUE(verify_command_args(&cmd->commands[0], 1, expected));
    CU_ASSERT_PTR_NOT_NULL(cmd->input_file);
    CU_ASSERT_STRING_EQUAL(cmd->input_file, "input.txt");
    CU_ASSERT_PTR_NOT_NULL(cmd->output_file);
    CU_ASSERT_STRING_EQUAL(cmd->output_file, "output.txt");
    free_parsed_cmd(cmd);
  }
}

void test_both_redirections_reverse_order(void) {
  ParsedCmd *cmd = parse("sort > output.txt < input.txt");
  CU_ASSERT_PTR_NOT_NULL(cmd);
  if (cmd) {
    CU_ASSERT_PTR_NOT_NULL(cmd->input_file);
    CU_ASSERT_STRING_EQUAL(cmd->input_file, "input.txt");
    CU_ASSERT_PTR_NOT_NULL(cmd->output_file);
    CU_ASSERT_STRING_EQUAL(cmd->output_file, "output.txt");
    free_parsed_cmd(cmd);
  }
}

/* Test Suite 7: Pipeline Commands */

void test_simple_pipeline(void) {
  ParsedCmd *cmd = parse("ls | grep test");
  CU_ASSERT_PTR_NOT_NULL(cmd);
  if (cmd) {
    CU_ASSERT_EQUAL(cmd->num_commands, 2);

    // First command
    char *expected1[] = {"ls"};
    CU_ASSERT_TRUE(verify_command_args(&cmd->commands[0], 1, expected1));

    // Second command
    char *expected2[] = {"grep", "test"};
    CU_ASSERT_TRUE(verify_command_args(&cmd->commands[1], 2, expected2));

    free_parsed_cmd(cmd);
  }
}

void test_pipeline_three_commands(void) {
  ParsedCmd *cmd = parse("cat file.txt | grep error | wc -l");
  CU_ASSERT_PTR_NOT_NULL(cmd);
  if (cmd) {
    CU_ASSERT_EQUAL(cmd->num_commands, 3);

    char *expected1[] = {"cat", "file.txt"};
    CU_ASSERT_TRUE(verify_command_args(&cmd->commands[0], 2, expected1));

    char *expected2[] = {"grep", "error"};
    CU_ASSERT_TRUE(verify_command_args(&cmd->commands[1], 2, expected2));

    char *expected3[] = {"wc", "-l"};
    CU_ASSERT_TRUE(verify_command_args(&cmd->commands[2], 2, expected3));

    free_parsed_cmd(cmd);
  }
}

void test_pipeline_four_commands(void) {
  ParsedCmd *cmd = parse("ps aux | grep bash | awk '{print $2}' | head -1");
  CU_ASSERT_PTR_NOT_NULL(cmd);
  if (cmd) {
    CU_ASSERT_EQUAL(cmd->num_commands, 4);

    char *expected1[] = {"ps", "aux"};
    CU_ASSERT_TRUE(verify_command_args(&cmd->commands[0], 2, expected1));

    char *expected2[] = {"grep", "bash"};
    CU_ASSERT_TRUE(verify_command_args(&cmd->commands[1], 2, expected2));

    char *expected3[] = {"awk", "'{print", "$2}'"};
    CU_ASSERT_TRUE(verify_command_args(&cmd->commands[2], 3, expected3));

    char *expected4[] = {"head", "-1"};
    CU_ASSERT_TRUE(verify_command_args(&cmd->commands[3], 2, expected4));

    free_parsed_cmd(cmd);
  }
}

void test_pipeline_with_spaces(void) {
  ParsedCmd *cmd = parse("ls  |  grep  test");
  CU_ASSERT_PTR_NOT_NULL(cmd);
  if (cmd) {
    CU_ASSERT_EQUAL(cmd->num_commands, 2);
    free_parsed_cmd(cmd);
  }
}

/* Test Suite 8: More Tests */

void test_conditional_with_redirection(void) {
  ParsedCmd *cmd = parse("and ls > output.txt");
  CU_ASSERT_PTR_NOT_NULL(cmd);
  if (cmd) {
    CU_ASSERT_EQUAL(cmd->is_and, 1);
    CU_ASSERT_EQUAL(cmd->num_commands, 1);
    CU_ASSERT_PTR_NOT_NULL(cmd->output_file);
    CU_ASSERT_STRING_EQUAL(cmd->output_file, "output.txt");
    free_parsed_cmd(cmd);
  }
}

void test_conditional_with_pipeline(void) {
  ParsedCmd *cmd = parse("or ls | grep test");
  CU_ASSERT_PTR_NOT_NULL(cmd);
  if (cmd) {
    CU_ASSERT_EQUAL(cmd->is_or, 1);
    CU_ASSERT_EQUAL(cmd->num_commands, 2);
    free_parsed_cmd(cmd);
  }
}

void test_pipeline_with_input_redirection(void) {
  ParsedCmd *cmd = parse("cat < input.txt | grep pattern");
  CU_ASSERT_PTR_NOT_NULL(cmd);
  if (cmd) {
    CU_ASSERT_EQUAL(cmd->num_commands, 2);
    CU_ASSERT_PTR_NOT_NULL(cmd->input_file);
    CU_ASSERT_STRING_EQUAL(cmd->input_file, "input.txt");
    free_parsed_cmd(cmd);
  }
}

void test_pipeline_with_output_redirection(void) {
  ParsedCmd *cmd = parse("ls | grep test > results.txt");
  CU_ASSERT_PTR_NOT_NULL(cmd);
  if (cmd) {
    CU_ASSERT_EQUAL(cmd->num_commands, 2);
    CU_ASSERT_PTR_NOT_NULL(cmd->output_file);
    CU_ASSERT_STRING_EQUAL(cmd->output_file, "results.txt");
    free_parsed_cmd(cmd);
  }
}

void test_complex_pipeline_with_both_redirections(void) {
  ParsedCmd *cmd = parse("cat < input.txt | sort | uniq > output.txt");
  CU_ASSERT_PTR_NOT_NULL(cmd);
  if (cmd) {
    CU_ASSERT_EQUAL(cmd->num_commands, 3);
    CU_ASSERT_PTR_NOT_NULL(cmd->input_file);
    CU_ASSERT_STRING_EQUAL(cmd->input_file, "input.txt");
    CU_ASSERT_PTR_NOT_NULL(cmd->output_file);
    CU_ASSERT_STRING_EQUAL(cmd->output_file, "output.txt");
    free_parsed_cmd(cmd);
  }
}

void test_conditional_pipeline_redirections(void) {
  ParsedCmd *cmd = parse("and cat < in.txt | grep pattern | sort > out.txt");
  CU_ASSERT_PTR_NOT_NULL(cmd);
  if (cmd) {
    CU_ASSERT_EQUAL(cmd->is_and, 1);
    CU_ASSERT_EQUAL(cmd->num_commands, 3);
    CU_ASSERT_PTR_NOT_NULL(cmd->input_file);
    CU_ASSERT_PTR_NOT_NULL(cmd->output_file);
    free_parsed_cmd(cmd);
  }
}

/* Test Suite 9: Memory and Cleanup */

void test_multiple_parses(void) {
  for (int i = 0; i < 100; i++) {
    ParsedCmd *cmd = parse("ls -la");
    CU_ASSERT_PTR_NOT_NULL(cmd);
    free_parsed_cmd(cmd);
  }
}

void test_free_null_cmd(void) {
  free_parsed_cmd(NULL);
  CU_PASS("free_parsed_cmd(NULL) did not crash");
}

/* Suite Initialization */

int init_suite(void) { return 0; }
int clean_suite(void) { return 0; }

/* Main function to run all tests */

int main() {
  CU_pSuite suite1 = NULL;
  CU_pSuite suite2 = NULL;
  CU_pSuite suite3 = NULL;
  CU_pSuite suite4 = NULL;
  CU_pSuite suite5 = NULL;
  CU_pSuite suite6 = NULL;
  CU_pSuite suite7 = NULL;
  CU_pSuite suite8 = NULL;
  CU_pSuite suite9 = NULL;

  // Initialize CUnit registry
  if (CUE_SUCCESS != CU_initialize_registry()) {
    return CU_get_error();
  }

  // Suite 1: Comment Handling
  suite1 = CU_add_suite("Comment Handling", init_suite, clean_suite);
  if (NULL == suite1) {
    CU_cleanup_registry();
    return CU_get_error();
  }
  CU_add_test(suite1, "Full line comment", test_full_line_comment);
  CU_add_test(suite1, "Inline comment", test_inline_comment);
  CU_add_test(suite1, "Empty line", test_empty_line);
  CU_add_test(suite1, "Whitespace only", test_whitespace_only);
  CU_add_test(suite1, "Comment after whitespace",
              test_comment_after_whitespace);

  suite2 = CU_add_suite("Basic Tokenization", init_suite, clean_suite);
  if (NULL == suite2) {
    CU_cleanup_registry();
    return CU_get_error();
  }
  CU_add_test(suite2, "Simple command", test_simple_command);
  CU_add_test(suite2, "Command with args", test_command_with_args);
  CU_add_test(suite2, "Multiple spaces", test_command_with_multiple_spaces);
  CU_add_test(suite2, "Leading spaces", test_command_with_leading_spaces);

  suite3 = CU_add_suite("Conditional Commands", init_suite, clean_suite);
  if (NULL == suite3) {
    CU_cleanup_registry();
    return CU_get_error();
  }
  CU_add_test(suite3, "AND conditional", test_and_conditional);
  CU_add_test(suite3, "OR conditional", test_or_conditional);
  CU_add_test(suite3, "AND with args", test_and_with_args);
  CU_add_test(suite3, "Only AND keyword", test_only_and_keyword);
  CU_add_test(suite3, "Only OR keyword", test_only_or_keyword);

  suite4 = CU_add_suite("Input Redirection", init_suite, clean_suite);
  if (NULL == suite4) {
    CU_cleanup_registry();
    return CU_get_error();
  }
  CU_add_test(suite4, "Simple input redirect", test_input_redirection_simple);
  CU_add_test(suite4, "Input redirect with args",
              test_input_redirection_with_args);

  suite5 = CU_add_suite("Output Redirection", init_suite, clean_suite);
  if (NULL == suite5) {
    CU_cleanup_registry();
    return CU_get_error();
  }
  CU_add_test(suite5, "Simple output redirect", test_output_redirection_simple);
  CU_add_test(suite5, "Output redirect with args",
              test_output_redirection_with_args);

  suite6 = CU_add_suite("Both Redirections", init_suite, clean_suite);
  if (NULL == suite6) {
    CU_cleanup_registry();
    return CU_get_error();
  }
  CU_add_test(suite6, "Input and output", test_both_redirections);
  CU_add_test(suite6, "Reverse order", test_both_redirections_reverse_order);

  suite7 = CU_add_suite("Pipeline Commands", init_suite, clean_suite);
  if (NULL == suite7) {
    CU_cleanup_registry();
    return CU_get_error();
  }
  CU_add_test(suite7, "Simple pipeline", test_simple_pipeline);
  CU_add_test(suite7, "Three command pipeline", test_pipeline_three_commands);
  CU_add_test(suite7, "Four command pipeline", test_pipeline_four_commands);
  CU_add_test(suite7, "Pipeline with spaces", test_pipeline_with_spaces);

  // Suite 8: More Tests
  suite8 = CU_add_suite("More Tests", init_suite, clean_suite);
  if (NULL == suite8) {
    CU_cleanup_registry();
    return CU_get_error();
  }
  CU_add_test(suite8, "Conditional with redirection",
              test_conditional_with_redirection);
  CU_add_test(suite8, "Conditional with pipeline",
              test_conditional_with_pipeline);
  CU_add_test(suite8, "Pipeline with input",
              test_pipeline_with_input_redirection);
  CU_add_test(suite8, "Pipeline with output",
              test_pipeline_with_output_redirection);
  CU_add_test(suite8, "Pipeline both redirections",
              test_complex_pipeline_with_both_redirections);
  CU_add_test(suite8, "All features combined",
              test_conditional_pipeline_redirections);

  suite9 = CU_add_suite("Memory and Cleanup", init_suite, clean_suite);
  if (NULL == suite9) {
    CU_cleanup_registry();
    return CU_get_error();
  }
  CU_add_test(suite9, "Multiple parses", test_multiple_parses);
  CU_add_test(suite9, "Free NULL cmd", test_free_null_cmd);

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();

  printf("\n");
  CU_basic_show_failures(CU_get_failure_list());
  printf("\n\n");

  int num_failed = CU_get_number_of_tests_failed();

  CU_cleanup_registry();

  return num_failed;
}
