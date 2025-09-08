#include <stdio.h>
#include <stdlib.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "cli.h"
#include "cli_commands.h"

char *command_generator(const char *text, int state) {
	static int list_index, len;
	if (!state) {
		list_index = 0;
		len = strlen(text);
	}

	char *name;
	while ((name = (char *)commands[list_index++].name)) {
		if (strncmp(name, text, len) == 0) return strdup(name);
	}
	return NULL;
}

char **cli_completion(const char *text, int start, int _) {
	(void)_;
	rl_attempted_completion_over = 1;

	if (start == 0) {
		return rl_completion_matches(text, command_generator);
	}

	return rl_completion_matches(text, rl_filename_completion_function);

	return NULL;
}

void cli_loop() {
	rl_attempted_completion_function = cli_completion;

	char *input;
	while ((input = readline("unmgk> ")) != NULL) {
		if (*input) add_history(input);

		int handled = 0;
		for (int i = 0; commands[i].name; i++) {
			int len = strlen(commands[i].name);
			if (strncmp(input, commands[i].name, len) == 0 &&
				(input[len] == ' ' || input[len] == '\0')) {
				commands[i].func(input + len);
				handled = 1;
				break;
			}
		}

		if (!handled) {
			printf("Comandos:\n");
			for (int i = 0; commands[i].name; i++) {
				printf("> %s\n", commands[i].help);
			}
		}

		free(input);
	}
}
