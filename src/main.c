/*** includes ***/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

/*** defines ***/

#define CTRL_KEY(k) ((k)&0x1f)
// endblk

/*** data ***/
struct editorConfig {
  int screenrows;
  int screencols;
  struct termios orig_termios;
};
struct editorConfig Editor;

// endblk

/*** terminal ***/

void die(const char *s) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  perror(s);
  exit(1);
}

void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &Editor.orig_termios) == -1)
    die("tcsetattr");
}
void enableRawMode() {
  if (tcgetattr(STDIN_FILENO, &Editor.orig_termios) == -1)
    die("tcgetattr");
  atexit(disableRawMode);

  struct termios raw = Editor.orig_termios;

  raw.c_iflag &= ~(ICRNL | IXON | BRKINT | INPCK | ISTRIP);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    die("tcsetattr");
}

char editorReadKey() {
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN)
      die("read");
  }

  return c;
}

int getWindowSize(int *rows, int *cols) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    return -1;
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

/*** output ***/

// draw 24 tildes across the screen
void editorDrawRows() {
  int y;
  for (y = 0; y < Editor.screenrows; y++) {
    write(STDOUT_FILENO, "~\r\n", 3);
  }
}

void editorRefreshScreen() {
  // j2 clears screen h gets cursor to top
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  editorDrawRows();
  write(STDOUT_FILENO, "\x1b[H", 3);
}
// endblk

/*** input ***/

void editorProccessKeypress() {
  char c = editorReadKey();

  switch (c) {
  case CTRL_KEY('q'):
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    exit(0);
    break;
  case CTRL_KEY('r'):
    editorRefreshScreen();
  }
}
// endblk

/*** init ***/

void initEditor() {
  if (getWindowSize(&Editor.screenrows, &Editor.screencols) == -1)
    die("getWindowSize");
}

int main() {
  enableRawMode();
  initEditor();

  while (1) {
    editorRefreshScreen();
    editorProccessKeypress();
  }
  return 0;
}
// endblk