/*========================================
|               @includes                |
========================================*/
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "utils.h"

/*========================================
|             @forward defs              |
========================================*/
char editorReadKey();

/*========================================
|               @macros                  |
========================================*/
#define CTRL_KEY(k) ((k) & 0x1f)

/*========================================
|                 @data                  |
========================================*/
struct editorConfig {
  int screenrows;
  int screencols;
  struct termios origTermios;
};
struct editorConfig Editor;

/*========================================
|               @terminal                |
========================================*/
void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &Editor.origTermios) == -1)
    die("tcsetattr");
}

void enableRawMode() {
  if (tcgetattr(STDIN_FILENO, &Editor.origTermios) == -1)
    die("tcgetattr");
  atexit(disableRawMode); // resets terminal mode after exit() or main() exits

  struct termios termiosRaw = Editor.origTermios;
  // https://pubs.opengroup.org/onlinepubs/9699919799/
  termiosRaw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  // opost disables adding carrige return to newline, printf must include \r
  termiosRaw.c_oflag &= ~(OPOST);
  termiosRaw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

  termiosRaw.c_cc[VMIN] = 0;
  termiosRaw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &termiosRaw) == -1)
    die("tcsetattr");
}

int getCursorPosition(int *rows, int *cols) {
  char buf[32];
  unsigned int i = 0;
  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
    return -1;

  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1)
      break;
    if (buf[i] == 'R')
      break;
    i++;
  }
  buf[i] = '\0';

  if (buf[0] != '\x1b' || buf[1] != '[')
    return -1;
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2)
    return -1;

  return 0;
}

int getWindowSize(int *rows, int *cols) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
      return -1;
    return getCursorPosition(rows,
                             cols); // use escape codes if ioctl dosent work
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

/*========================================
|                @output                 |
========================================*/
void editorDrawRows() {
  int y;
  for (y = 0; y < Editor.screenrows; y++) {
    write(STDOUT_FILENO, "~", 1);
    if (y < Editor.screenrows - 1) {
      write(STDOUT_FILENO, "\r\n", 2);
    }
  }
}

void editorRefreshScreen() {
  write(STDOUT_FILENO, "\x1b[2J", 4); // 2j clears screen h gets cursor to top
  write(STDOUT_FILENO, "\x1b[H", 3);

  editorDrawRows();
  write(STDOUT_FILENO, "\x1b[H", 3); // reposition cursor
  // write(STDOUT_FILENO, "\x1b [2 q", 3);
}

/*========================================
|                @input                  |
========================================*/
char editorReadKey() {
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN)
      die("read");
  }

  return c;
}

void editorProccessKeypress() {
  char c = editorReadKey();

  switch (c) {
  case CTRL_KEY('q'):
    // refresh screen on exit
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    exit(0);
    break;
  }
}

/*========================================
|                 @init                  |
========================================*/
void setCursorShape(int shape) {
  printf("\x1b[%d q", shape);
  fflush(stdout);
}
void initEditor() {
  enableRawMode();
  if (getWindowSize(&Editor.screenrows, &Editor.screencols) == -1)
    die("getWindowSize");
  setCursorShape(2); // block cursor for normal mode
}

/*========================================
|               @entry                  |
========================================*/
int main() {
  initEditor();
  while (1) {
    editorRefreshScreen();
    editorProccessKeypress();
  }
  return 0;
}
