#define _POSIX_C_SOURCE 200809L

#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void parser_init_command(struct Command *cmd) {
    memset(cmd, 0, sizeof(*cmd));
}

void parser_init_parsed_line(struct ParsedLine *parsed) {
    memset(parsed, 0, sizeof(*parsed));
    for (int i = 0; i < MAX_SEGMENTS; i++) {
        parser_init_command(&parsed->commands[i]);
    }
}

static bool is_special_char(char c) {
    return c == '|' || c == '<' || c == '>' || c == '&';
}

int parser_tokenize(const char *line, char **tokens, int max_tokens) {
    int count = 0;
    size_t i = 0;

    while (line[i] != '\0') {
        while (line[i] == ' ' || line[i] == '\t') {
            i++;
        }

        if (line[i] == '\0') {
            break;
        }

        if (count >= max_tokens - 1) {
            fprintf(stderr, "Too many tokens\n");
            return -1;
        }

        if (line[i] == '"') {
            size_t start = ++i;
            while (line[i] != '\0' && line[i] != '"') {
                i++;
            }
            if (line[i] != '"') {
                fprintf(stderr, "Unterminated quote\n");
                return -1;
            }
            tokens[count] = strndup(line + start, i - start);
            if (tokens[count] == NULL) {
                perror("strndup");
                return -1;
            }
            count++;
            i++;
            continue;
        }

        if (line[i] == '>' && line[i + 1] == '>') {
            tokens[count] = strdup(">>");
            if (tokens[count] == NULL) {
                perror("strdup");
                return -1;
            }
            count++;
            i += 2;
            continue;
        }

        if (is_special_char(line[i])) {
            char special[3] = {line[i], '\0', '\0'};
            tokens[count] = strdup(special);
            if (tokens[count] == NULL) {
                perror("strdup");
                return -1;
            }
            count++;
            i++;
            continue;
        }

        size_t start = i;
        while (line[i] != '\0' &&
               line[i] != ' ' &&
               line[i] != '\t' &&
               !is_special_char(line[i])) {
            i++;
        }
        tokens[count] = strndup(line + start, i - start);
        if (tokens[count] == NULL) {
            perror("strndup");
            return -1;
        }
        count++;
    }

    tokens[count] = NULL;
    return count;
}

void parser_free_tokens(char **tokens, int count) {
    for (int i = 0; i < count; i++) {
        free(tokens[i]);
    }
}

static int attach_redirection(struct Command *cmd, const char *operator, char *target) {
    if (strcmp(operator, "<") == 0) {
        if (cmd->input_file != NULL) {
            fprintf(stderr, "Multiple input redirects are not supported\n");
            return -1;
        }
        cmd->input_file = target;
        return 0;
    }

    if (cmd->output_file != NULL) {
        fprintf(stderr, "Multiple output redirects are not supported\n");
        return -1;
    }

    cmd->output_file = target;
    cmd->append_output = (strcmp(operator, ">>") == 0);
    return 0;
}

int parser_parse_tokens(char **tokens, int token_count, struct ParsedLine *parsed) {
    int cmd_index = 0;
    int arg_index = 0;

    if (token_count == 0) {
        return 0;
    }

    for (int i = 0; i < token_count; i++) {
        if (strcmp(tokens[i], "&") == 0) {
            if (i != token_count - 1) {
                fprintf(stderr, "Background operator must appear at end\n");
                return -1;
            }
            if (arg_index == 0) {
                fprintf(stderr, "Missing command before background operator\n");
                return -1;
            }
            parsed->background = true;
            continue;
        }

        if (strcmp(tokens[i], "|") == 0) {
            if (arg_index == 0) {
                fprintf(stderr, "Invalid null command in pipeline\n");
                return -1;
            }
            parsed->commands[cmd_index].argv[arg_index] = NULL;
            cmd_index++;
            if (cmd_index >= MAX_SEGMENTS) {
                fprintf(stderr, "Too many piped commands\n");
                return -1;
            }
            arg_index = 0;
            continue;
        }

        if (strcmp(tokens[i], "<") == 0 ||
            strcmp(tokens[i], ">") == 0 ||
            strcmp(tokens[i], ">>") == 0) {
            if (i + 1 >= token_count) {
                fprintf(stderr, "Missing filename after redirection operator\n");
                return -1;
            }
            char *target = tokens[i + 1];
            if (attach_redirection(&parsed->commands[cmd_index], tokens[i], target) != 0) {
                return -1;
            }
            i++;
            continue;
        }

        if (arg_index >= MAX_TOKENS - 1) {
            fprintf(stderr, "Too many arguments\n");
            return -1;
        }

        parsed->commands[cmd_index].argv[arg_index++] = tokens[i];
    }

    if (arg_index == 0) {
        fprintf(stderr, "Trailing pipe is not allowed\n");
        return -1;
    }

    parsed->commands[cmd_index].argv[arg_index] = NULL;
    parsed->command_count = cmd_index + 1;
    return 0;
}
