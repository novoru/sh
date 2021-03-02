#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <readline/readline.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <signal.h>

typedef struct Vector {
  void **data;
  int capacity;
  int len;
} Vector;

Vector *new_vec() {
  Vector *vec = malloc(sizeof(Vector));
  vec->data = malloc(sizeof(void *) * 16);
  vec->capacity = 16;
  vec->len = 0;

  return vec;
}

void vec_push(Vector *vec, void *elem) {
  if (vec->capacity == vec->len) {
    vec->capacity *= 2;
    vec->data = realloc(vec->data, sizeof(void *) * vec->capacity);
  }

  vec->data[vec->len++] = elem;    
}

void free_vec(Vector *vec) {
  for (int i = 0; i < vec->len; i++)
    free(vec->data[i]);
  free(vec->data);
  free(vec);
}

char **parse_args(char *input) {
  char **args = malloc(8 * sizeof(char *));
  if(args == NULL) {
    perror("malloc failed");
    exit(1);
  }
  
  char *delim = " ";
  char *token;
  char *buf;
  int index = 0;

  token = strtok_r(input, delim, &buf);
  for (; token != NULL; token = strtok_r(NULL, delim, &buf)) {
    args[index] = token;
    index++;
  }

  args[index] = NULL;

  return args;
}

bool is_builtin(char *cmd) {
  if (!strcmp(cmd, "cd"))
    return true;
  if (!strcmp(cmd, "exit"))
    return true;

  return false;
}

void exec_builtin(char **args) {
  if (!strcmp(args[0], "cd")) {
    if (chdir(args[1]) < 0)
      perror(args[1]);

    return;
  }

  if (!strcmp(args[0], "exit")) {
    exit(0);
  }
}

void exec_cmds(Vector *cmds, int idx, int out[]) {
  char **args = (char **) cmds->data[idx];
  if (is_builtin(args[0])) {
    exec_builtin(args);
    return;
  }

  int *statlock = 0;
  pid_t pid = fork();

  if (pid == -1) {
    perror("fork failed");
    exit(1);
  } else if (pid == 0) {

    signal(SIGINT, SIG_DFL);

    int in[2];
    pipe(in);
    
    if (idx > 0) {
      exec_cmds(cmds, idx-1, in);
    }
    if (idx == 0 && (idx == cmds->len -1));  // SINGLE
    else if (idx == 0) {
      // HEAD
      if (dup2(out[1], STDOUT_FILENO) == -1) {
	perror("dup failed");
	exit(1);
      }
      close(STDIN_FILENO);
    } else if (idx == cmds->len - 1){
      // LAST
      if (dup2(in[0], STDIN_FILENO) == -1) {
	perror("dup failed");
	exit(1);
      }
    } else {
      // BODY
      if (dup2(in[0], STDIN_FILENO) == -1) {
	perror("dup failed");
	exit(1);
      }	
      if (dup2(out[1], STDOUT_FILENO) == -1) {
	perror("dup failed");
	exit(1);
      }	
    }
      close(in[0]);
      close(in[1]);
      close(out[0]);
      close(out[1]);
    if (execvp(args[0], args) < 0){
      perror(args[0]);
      exit(1);
    }
  } else {
    waitpid(pid, statlock, WUNTRACED);
  }
}

int main() {
  char *delim = "|";
  char *token;
  char *buf;

  signal(SIGINT, SIG_IGN);
  
  while (1) {
    char *input = readline("> ");
    Vector *cmds = new_vec();

    int fd[2];
    if (pipe(fd) == -1) {
      perror("pipe failed");
      exit(1);
    }
    
    if (!strcmp(input, ""))
      continue;

    token = strtok_r(input, delim, &buf);
    for (; token != NULL; token = strtok_r(NULL, delim, &buf))
      vec_push(cmds, (void *) parse_args(token));

    exec_cmds(cmds, cmds->len-1, fd);
    
    free(input);
    free_vec(cmds);
  }

  return 0;
}
