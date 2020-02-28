#include <sys/select.h>
#include <stdio.h>
#include <sys/types.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <stdbool.h>

#define PROMPT_BUFSIZ BUFSIZ
static char prompt_buffer[PROMPT_BUFSIZ];
static int prompt_buffer_end = 0;

static char prompt[PROMPT_BUFSIZ + 1];

static pid_t prompt_pid;

int read_prompt_chunk(int fd) {
  int space_left_before_read = PROMPT_BUFSIZ - prompt_buffer_end;
  int chunk_size = read(
      fd,
      prompt_buffer + prompt_buffer_end, 
      space_left_before_read
  );
  if (!chunk_size) return chunk_size;
  prompt_buffer_end += chunk_size;
  int space_left_after_read = space_left_before_read - chunk_size;

  int newline_index = -1;
  for (int search = prompt_buffer_end - 1; search >= 0; search--) {
    if (prompt_buffer[search] == '\n') {
      newline_index = search;
      break;
    }
  }

  if (newline_index != -1) {
    int new_prompt_start = newline_index;
    while (new_prompt_start > 0 && prompt_buffer[new_prompt_start - 1] != '\n') {
      new_prompt_start--;
    }
    int new_prompt_length = newline_index - new_prompt_start;
    memcpy(prompt, prompt_buffer + new_prompt_start, new_prompt_length);
    prompt[new_prompt_length + 1] = '\0';
  }

  if (!space_left_after_read) {
    if (newline_index != -1) {
      int remaining_size = PROMPT_BUFSIZ - newline_index - 1;
      memcpy(
          prompt_buffer,
          prompt_buffer + newline_index,
          remaining_size
      );
      prompt_buffer_end = remaining_size + 1;
    } else {
      // No newlines and out of space? Prompt was too large so truncate.
      memcpy(prompt, prompt_buffer, PROMPT_BUFSIZ);
      prompt[PROMPT_BUFSIZ + 1] = '\0';
    }
  }
  return chunk_size;
}

static int last_prompt_size = 0;
void show_prompt() {
  for (int i = 0; i < last_prompt_size; i++) {
    fputs("\b \b", stdout);
  }
  fputs(prompt, stdout);
  fflush(stdout);
  last_prompt_size = strlen(prompt);
}

void cleanup() {
  if (prompt_pid != 0)
    kill(prompt_pid, SIGTERM);
}

int main(void) {
  int prompt_pipes[2];
  pipe(prompt_pipes);

  // Dying processes do not become zombies.
  struct sigaction child_action = {
    .sa_handler = SIG_DFL,
    .sa_flags = SA_NOCLDWAIT | SA_RESTART
  };
  sigaction(SIGCHLD, &child_action, NULL);

  prompt_pid = fork();
  if (!prompt_pid) {
    close(prompt_pipes[0]);
    dup2(prompt_pipes[1], STDOUT_FILENO);
    execl("./prompt", (char *) NULL);
  }
  atexit(&cleanup);
  close(prompt_pipes[1]);

  fd_set fds;
  bool prompt_alive = true;
  while (prompt_alive) {
    FD_ZERO(&fds);
    FD_SET(prompt_pipes[0], &fds);
    select(prompt_pipes[0]+1, &fds, NULL, NULL, NULL);

    prompt_alive = read_prompt_chunk(prompt_pipes[0]) > 0;
    show_prompt();
  }

  wait(NULL);

  return 0;
}
