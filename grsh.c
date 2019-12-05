#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>


#define DELIMETERS " \t\r\n\a"

extern char **environ;
char *env_args[4] = {"PATH=/bin", "USER=me", NULL, NULL};
char *args[5] = {NULL, NULL, NULL, NULL, NULL};
char c[1024] = "0";
int andC;

FILE *file;

bool input = true;
bool built = true;
bool and = false;
bool redir = false;

int main(int argc, char *argv[]) {
	if (argc == 2) {
		input = false;
		file = fopen(argv[1], "r");
		batch(file);
		fclose(file);
	}
	else if (argc > 2) {
		printf("error: too many arguments");
	}
	else {
		batch(stdin);
	}
}

void batch(FILE *stream) {
	char *buffer;
	size_t sizeb = 1024;
	buffer = (char *)malloc(sizeb * sizeof(char));
	if (input) {
		printf("grsh> ");
	}
	while (1) {
		int check = getline(&buffer, &sizeb, stream);
		andC = 1;
		if (check <= 1 && !input) {
			fclose(stream);
			exit(0);
		}
		//printf("input: %s\n", buffer);
		char *token = strtok(buffer, DELIMETERS);
		int j = 0;
		for (j; j < 5; j++) {
			args[j] = NULL;
		}
		int count = 0;
		while (token != NULL) {
			if (strcmp(token, "exit") == 0) {
				exit(0);
			}
			else if (strcmp(token, "cd") == 0) {
				token = strtok(NULL, DELIMETERS);
				cd(token);
				built = true;
			}
			else if (strcmp(token, "&") == 0) {
				count = 0;
				and = true;
				andC++;
				command();
				//built = true;
				int i = 0;
				for (i; i < 5; i++) {
					args[i] = NULL;
				}
			}
			else {
				built = false;
				args[count++] = token;
			}
			token = strtok(NULL, DELIMETERS);
		}
		if (built) {
			built = false;
			if (input) {
				printf("grsh> ");
			}
		}
		else if (check <= 1) {
			built = true;
			if (input) {
				printf("grsh> ");
			}
		}
		else {
			command();
		}
		if (check <= 1 && !input) {
			fclose(stream);
			exit(0);
		}
	}
}

void command() {
	int w = 0;
	int fd;
	FILE *write;
	for (w; w < sizeof args && args[w] != NULL; w++) {
		if (strcmp(args[w], ">") == 0) {
			//printf("found > character\n");
			fd = open(args[w+1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
			//printf("opened file\n");
			redir = true;
			args[w] = NULL;
			args[w+1] = NULL;
			break;
		}
	}
	if (strcmp(args[0], "path") == 0) {
		int k = 1;
		memset(c, 0, sizeof(c));
		strcat(c, "PATH=");
		while (args[k] != NULL) {
			strcat(c, args[k++]);
			if (args[k] != NULL) {
				strcat(c, ":");
			}
		}
		env_args[0] = c;
		if (input) {
			printf("grsh> ");
		}
		//printf("%s\n", c);
	}
	else if (fork() == 0) {
		if (redir) {
			//printf("now redirecting\n");
			dup2(fd, 1);
			dup2(fd, 2);
		}
		int err = execve(args[0], args, env_args);
		environ = env_args;
		execvp(args[0], args);
		if (err == -1) {
			//printf("there was error\n");
			if (!input) {
				fclose(file);
			}
			exit(0);
		}
		if (redir) {
			close(fd);
		}
		//exit(0);
	}
	else if (and) {
		and = false;
	}
	else {
		if (redir) {
			close(fd);
		}
		int p = 0;
		for (p; p < andC; p++) {
			//printf("waiting\n");
			wait(NULL);
			//printf("done waiting\n");
		}
		if (input) {
			printf("grsh> ");
		}
	}
	if (redir) {
		redir = false;
	}
}

void cd(char *token) {
	memset(c, 0, sizeof(c));
	getcwd(c, sizeof(c));
	if (token != NULL) {
		strcat(c, token);
		if (chdir(c) != 0) {
			chdir(token);
		}
	}
	else {
		chdir("..");
	}
}
