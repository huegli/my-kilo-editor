/*** includes ***/

#include "terminal.h"

#include <stdexcept>
#include <iostream>
#include <fstream>
#include <memory>

#include <cctype>
#include <cerrno>
#include <fcntl.h>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <ctime>

/*** defines ***/

const std::string KILO_VERSION{"0.0.1"};
const auto KILO_TAB_STOP{8};
const auto KILO_QUIT_TIMES{3};

using Term::Terminal;
using Term::cursor_on;
using Term::cursor_off;
using Term::move_cursor;
using Term::erase_to_eol;
using Term::fg;
using Term::style;
using Term::Key;

enum editorHighlight {
  HL_NORMAL = 0,
  HL_COMMENT,
  HL_MLCOMMENT,
  HL_KEYWORD1,
  HL_KEYWORD2,
  HL_STRING,
  HL_NUMBER,
  HL_MATCH
};

#define HL_HIGHLIGHT_NUMBERS (1 << 0)
#define HL_HIGHLIGHT_STRINGS (1 << 1)

/*** data ***/

struct editorSyntax {
  const char *filetype;
  const char **filematch;
  const char **keywords;
  const char *singleline_comment_start;
  const char *multiline_comment_start;
  const char *multiline_comment_end;
  int flags;
};

typedef struct erow {
  std::size_t idx;
  std::size_t  size;
  std::size_t  rsize;
  char *chars;
  char *render;
  unsigned char *hl;
  int hl_open_comment;
} erow;

struct editorConfig {
  std::size_t cx, cy;
  std::size_t rx;
  std::size_t rowoff;
  std::size_t coloff;
  int screenrows;
  int screencols;
  std::size_t numrows;
  erow *row;
  int dirty;
  char *filename;
  char statusmsg[80];
  time_t statusmsg_time;
  struct editorSyntax *syntax;
};

struct editorConfig E;

// /*** filetypes ***/

const char *C_HL_extensions[] = {".c", ".h", ".cpp", nullptr};
const char *C_HL_keywords[] = {
    "switch", "if",    "while",     "for",     "break",   "continue",
    "return", "else",  "struct",    "union",   "typedef", "static",
    "enum",   "class", "case",      "int|",    "long|",   "double|",
    "float|", "char|", "unsigned|", "signed|", "void|",   nullptr};

struct editorSyntax HLDB[] = {
    {"c", C_HL_extensions, C_HL_keywords, "//", "/*", "*/",
     HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS},
};

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

// /*** prototypes ***/

char *editorPrompt(const Terminal &term, const char *prompt1, const char *prompt2, void (*callback)(char *, int));
void editorSetStatusMessage();
void editorSetStatusMessage(const char *msg);
void editorSetStatusMessage(const char *fmt, const char *buf);

// /*** syntax highlighting ***/

int is_separator(int c) {
  return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != nullptr;
}

void editorUpdateSyntax(erow *row) {
  row->hl = static_cast<unsigned char *>(realloc(row->hl, row->rsize));
  memset(row->hl, HL_NORMAL, row->rsize);

  if (E.syntax == nullptr)
    return;

  const char **keywords = E.syntax->keywords;

  const char *scs = E.syntax->singleline_comment_start;
  const char *mcs = E.syntax->multiline_comment_start;
  const char *mce = E.syntax->multiline_comment_end;

  size_t scs_len = scs ? strlen(scs) : 0;
  size_t mcs_len = mcs ? strlen(mcs) : 0;
  size_t mce_len = mce ? strlen(mce) : 0;

  int prev_sep = 1;
  int in_string = 0;
  int in_comment = (row->idx > 0 && E.row[row->idx - 1].hl_open_comment);

  size_t i = 0;
  while (i < row->rsize) {
    char c = row->render[i];
    unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : static_cast<char>(HL_NORMAL);

    if (scs_len && !in_string && !in_comment) {
      if (!strncmp(&row->render[i], scs, scs_len)) {
        memset(&row->hl[i], HL_COMMENT, row->rsize - i);
        break;
      }
    }

    if (mcs_len && mce_len && !in_string) {
      if (in_comment) {
        row->hl[i] = HL_MLCOMMENT;
        if (!strncmp(&row->render[i], mce, mce_len)) {
          memset(&row->hl[i], HL_MLCOMMENT, mce_len);
          i += mce_len;
          in_comment = 0;
          prev_sep = 1;
          continue;
        } else {
          i++;
          continue;
        }
      } else if (!strncmp(&row->render[i], mcs, mcs_len)) {
        memset(&row->hl[i], HL_MLCOMMENT, mcs_len);
        i += mcs_len;
        in_comment = 1;
        continue;
      }
    }

    if (E.syntax->flags & HL_HIGHLIGHT_STRINGS) {
      if (in_string) {
        row->hl[i] = HL_STRING;
        if (c == '\\' && i + 1 < row->rsize) {
          row->hl[i + 1] = HL_STRING;
          i += 2;
          continue;
        }
        if (c == in_string)
          in_string = 0;
        i++;
        prev_sep = 1;
        continue;
      } else {
        if (c == '"' || c == '\'') {
          in_string = static_cast<unsigned char>(c);
          row->hl[i] = HL_STRING;
          i++;
          continue;
        }
      }
    }

    if (E.syntax->flags & HL_HIGHLIGHT_NUMBERS) {
      if ((isdigit(c) && (prev_sep || prev_hl == HL_NUMBER)) ||
          (c == '.' && prev_hl == HL_NUMBER)) {
        row->hl[i] = HL_NUMBER;
        i++;
        prev_sep = 0;
        continue;
      }
    }

    if (prev_sep) {
      int j;
      for (j = 0; keywords[j]; j++) {
        size_t klen = strlen(keywords[j]);
        size_t kw2 = keywords[j][klen - 1] == '|';
        if (kw2)
          klen--;

        if (!strncmp(&row->render[i], keywords[j], klen) &&
            is_separator(row->render[i + klen])) {
          memset(&row->hl[i], kw2 ? HL_KEYWORD2 : HL_KEYWORD1, klen);
          i += klen;
          break;
        }
      }
      if (keywords[j] != nullptr) {
        prev_sep = 0;
        continue;
      }
    }

    prev_sep = is_separator(c);
    i++;
  }

  int changed = (row->hl_open_comment != in_comment);
  row->hl_open_comment = in_comment;
  if (changed && row->idx + 1 < E.numrows)
    editorUpdateSyntax(&E.row[row->idx + 1]);
}

fg editorSyntaxToColor(int hl) {
  switch (hl) {
  case HL_COMMENT:
  case HL_MLCOMMENT:
    return fg::cyan;
  case HL_KEYWORD1:
    return fg::yellow;
  case HL_KEYWORD2:
    return fg::green;
  case HL_STRING:
    return fg::magenta;
  case HL_NUMBER:
    return fg::red;
  case HL_MATCH:
    return fg::blue;
  default:
    return fg::gray;
  }
}

void editorSelectSyntaxHighlight() {
  E.syntax = nullptr;
  if (E.filename == nullptr)
    return;

  char *ext = strrchr(E.filename, '.');

  for (std::size_t j = 0; j != HLDB_ENTRIES; ++j) {
    struct editorSyntax *s = &HLDB[j];
    unsigned int i = 0;
    while (s->filematch[i]) {
      int is_ext = (s->filematch[i][0] == '.');
      if ((is_ext && ext && !strcmp(ext, s->filematch[i])) ||
          (!is_ext && strstr(E.filename, s->filematch[i]))) {
        E.syntax = s;

        std::size_t filerow;
        for (filerow = 0; filerow < E.numrows; filerow++) {
          editorUpdateSyntax(&E.row[filerow]);
        }

        return;
      }
      i++;
    }
  }
}

/*** row operations ***/

auto editorRowCxToRx(erow *row, int cx) {
  auto rx = 0;
  for (auto j = 0; j < cx; j++) {
    if (row->chars[j] == '\t')
      rx += (KILO_TAB_STOP - 1) - (rx % KILO_TAB_STOP);
    rx++;
  }
  return rx;
}

size_t editorRowRxToCx(erow *row, int rx) {
  int cur_rx = 0;
  size_t  cx{};
  for (cx = 0; cx < row->size; cx++) {
    if (row->chars[cx] == '\t')
      cur_rx += (KILO_TAB_STOP - 1) - (cur_rx % KILO_TAB_STOP);
    cur_rx++;

    if (cur_rx > rx)
      return cx;
  }
  return cx;
}

void editorUpdateRow(erow *row) {
  auto tabs = 0;
  for (auto j = 0; j < static_cast<int>(row->size); j++)
    if (row->chars[j] == '\t')
      tabs++;

  free(row->render);
  row->render = static_cast<char *>(malloc(static_cast<std::size_t>(static_cast<int>(row->size) + tabs * (KILO_TAB_STOP - 1) + 1)));

  auto idx = 0;
  for (std::size_t j = 0; j < row->size; j++) {
    if (row->chars[j] == '\t') {
      row->render[idx++] = ' ';
      while (idx % KILO_TAB_STOP != 0)
        row->render[idx++] = ' ';
    } else {
      row->render[idx++] = row->chars[j];
    }
  }
  row->render[idx] = '\0';
  row->rsize = static_cast<std::size_t>(idx);

  editorUpdateSyntax(row);
}

void editorInsertRow(int at, const char *s, std::size_t len) {
  if (at < 0 || at > static_cast<int>(E.numrows))
    return;

  E.row = static_cast<erow *>(realloc(E.row, sizeof(erow) * (E.numrows + 1)));
  memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - static_cast<std::size_t>(at)));
  for (auto j = static_cast<std::size_t>(at) + 1; j <= E.numrows; j++)
    E.row[j].idx++;

  E.row[at].idx = static_cast<std::size_t>(at);

  E.row[at].size = len;
  E.row[at].chars = static_cast<char *>(malloc(len + 1));
  memcpy(E.row[at].chars, s, len);
  E.row[at].chars[len] = '\0';

  E.row[at].rsize = 0;
  E.row[at].render = nullptr;
  E.row[at].hl = nullptr;
  E.row[at].hl_open_comment = 0;
  editorUpdateRow(&E.row[at]);

  E.numrows++;
  E.dirty++;
}

void editorFreeRow(erow *row) {
  free(row->render);
  free(row->chars);
  free(row->hl);
}

void editorDelRow(int at) {
  if (at < 0 || static_cast<std::size_t>(at) >= E.numrows)
    return;
  editorFreeRow(&E.row[at]);
  memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - static_cast<std::size_t>(at) - 1));
  for (auto j = at; j < static_cast<int>(E.numrows) - 1; j++)
    E.row[j].idx--;
  E.numrows--;
  E.dirty++;
}

void editorRowInsertChar(erow *row, int at, int c) {
  if (at < 0 || static_cast<std::size_t>(at) > row->size)
    at = static_cast<int>(row->size);
  row->chars = static_cast<char *>(realloc(row->chars, row->size + 2));
  memmove(&row->chars[at + 1], &row->chars[at], row->size - static_cast<std::size_t>(at) + 1);
  row->size++;
  row->chars[at] = static_cast<char>(c);
  editorUpdateRow(row);
  E.dirty++;
}

void editorRowAppendString(erow *row, char *s, size_t len) {
  row->chars = static_cast<char *>(realloc(row->chars, row->size + len + 1));
  memcpy(&row->chars[row->size], s, len);
  row->size += len;
  row->chars[row->size] = '\0';
  editorUpdateRow(row);
  E.dirty++;
}

void editorRowDelChar(erow *row, int at) {
  if (at < 0 || at >= static_cast<int>(row->size))
    return;
  memmove(&row->chars[at], &row->chars[at + 1], row->size - static_cast<std::size_t>(at));
  row->size--;
  editorUpdateRow(row);
  E.dirty++;
}

/*** editor operations ***/

void editorInsertChar(int c) {
  if (E.cy == E.numrows) {
    editorInsertRow(static_cast<int>(E.numrows), "", 0);
  }
  editorRowInsertChar(&E.row[E.cy], static_cast<int>(E.cx), c);
  E.cx++;
}

void editorInsertNewLine() {
  if (E.cx == 0) {
    editorInsertRow(static_cast<int>(E.cy), "", 0);
  } else {
    erow *row = &E.row[E.cy];
    editorInsertRow(static_cast<int>(E.cy) + 1, &row->chars[E.cx], row->size - E.cx);
    row = &E.row[E.cy];
    row->size = E.cx;
    row->chars[row->size] = '\0';
    editorUpdateRow(row);
  }
  E.cy++;
  E.cx = 0;
}

void editorDelChar() {
  if (E.cy == E.numrows)
    return;
  if (E.cx == 0 && E.cy == 0)
    return;

  erow *row = &E.row[E.cy];
  if (E.cx > 0) {
    editorRowDelChar(row, static_cast<int>(E.cx) - 1);
    E.cx--;
  } else {
    E.cx = E.row[E.cy - 1].size;
    editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
    editorDelRow(static_cast<int>(E.cy));
    E.cy--;
  }
}

// /*** file i/o ***/

char *editorRowsToString(int *buflen) {
  auto totlen = 0;
  for (auto j = 0; j < static_cast<int>(E.numrows); j++)
    totlen += E.row[j].size + 1;
  *buflen = totlen;

  if (totlen > 0) {
    char *buf = static_cast<char *>(malloc(static_cast<std::size_t>(totlen)));
    char *p = buf;
    for (auto j = 0; j < static_cast<int>(E.numrows); j++) {
      memcpy(p, E.row[j].chars, E.row[j].size);
      p += E.row[j].size;
      *p = '\n';
      p++;
    }
    return buf;
  }
  else 
    return nullptr; 
}

void editorOpen(char *filename) {
  free(E.filename);
#ifdef _WIN32
  E.filename = _strdup(filename);
#else
  E.filename = strdup(filename);
#endif
  editorSelectSyntaxHighlight();

  std::ifstream f(filename);
  if (f.fail())
    throw std::runtime_error("File failed to open.");
  std::string line;
  std::getline(f, line);
  while (f.rdstate() == std::ios_base::goodbit) {
    std::size_t linelen = line.size();
    while (linelen > 0 &&
           (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
      linelen--;
    editorInsertRow(static_cast<int>(E.numrows), line.c_str(), linelen);
    std::getline(f, line);
  }
  E.dirty = 0;
}

void editorSave(const Terminal &term) {
  if (E.filename == nullptr) {
    E.filename = editorPrompt(term, "Save as: ", " (ESC to cancel)", nullptr);
    if (E.filename == nullptr) {
      editorSetStatusMessage("Save aborted");
      return;
    }
    editorSelectSyntaxHighlight();
  }

  int len;
  char *buf = editorRowsToString(&len);
  std::string s = std::string(buf, static_cast<std::size_t>(len));
  free(buf);

  std::ofstream out;
  out.open(E.filename);
  out << s;
  out.close();
  E.dirty = 0;
  /* editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno)); */
}

// /*** find ***/

void editorFindCallback(char *query, int key) {
  static int last_match = -1;
  static int direction = 1;

  static int saved_hl_line;
  static char *saved_hl = nullptr;

  if (saved_hl) {
    memcpy(E.row[saved_hl_line].hl, saved_hl, E.row[saved_hl_line].rsize);
    free(saved_hl);
    saved_hl = nullptr;
  }
  if (key == Term::Key::ENTER || key == Term::Key::ESC) {
    last_match = -1;
    direction = 1;
    return;
  } else if (key == Term::Key::ARROW_RIGHT || key == Term::Key::ARROW_DOWN) {
    direction = 1;
  } else if (key == Term::Key::ARROW_LEFT || key == Term::Key::ARROW_UP) {
    direction = -1;
  } else {
    last_match = -1;
    direction = 1;
  }

  if (last_match == -1)
    direction = 1;
  int current = last_match;
  int i;
  for (i = 0; i < static_cast<int>(E.numrows); i++) {
    current += direction;
    if (current == -1)
      current = static_cast<int>(E.numrows) - 1;
    else if (current == static_cast<int>(E.numrows))
      current = 0;

    erow *row = &E.row[current];
    char *match = strstr(row->render, query);
    if (match) {
      last_match = current;
      E.cy = static_cast<std::size_t>(current);
      E.cx = editorRowRxToCx(row, static_cast<int>(match - row->render));
      E.rowoff = E.numrows;

      saved_hl_line = current;
      saved_hl = static_cast<char *>(malloc(row->size));
      memcpy(saved_hl, row->hl, row->rsize);
      memset(&row->hl[match - row->render], HL_MATCH, strlen(query));
      break;
    }
  }
}

void editorFind(const Terminal &term) {
  auto saved_cx = E.cx;
  auto saved_cy = E.cy;
  auto saved_coloff = E.coloff;
  auto saved_rowoff = E.rowoff;

  char *query =
      editorPrompt(term, "Search: ", " (ESC/Arrows/Enter)", editorFindCallback);

  if (query) {
    free(query);
  } else {
    E.cx = saved_cx;
    E.cy = saved_cy;
    E.coloff = saved_coloff;
    E.rowoff = saved_rowoff;
  }
}

/*** output ***/

void editorScroll() {
  E.rx = 0;
  if (E.cy < E.numrows) {
    E.rx = static_cast<std::size_t>(editorRowCxToRx(&E.row[E.cy], static_cast<int>(E.cx)));
  }
  if (E.cy < E.rowoff) {
    E.rowoff = E.cy;
  }
  if (E.cy >= E.rowoff + static_cast<std::size_t>(E.screenrows)) {
    E.rowoff = E.cy - static_cast<std::size_t>(E.screenrows)+ 1;
  }
  if (E.cx < E.coloff) {
    E.coloff = E.rx;
  }
  if (E.rx >= E.coloff + static_cast<std::size_t>(E.screencols)) {
    E.coloff = E.rx - static_cast<std::size_t>(E.screencols) + 1;
  }
}

void editorDrawRows(std::string &ab) {
  int y;
  for (y = 0; y < E.screenrows; y++) {
    int filerow = y + static_cast<int>(E.rowoff);
    if (filerow >= static_cast<int>(E.numrows)) {
      if (E.numrows == 0 && y == E.screenrows / 3) {
        char welcome[80];
        int welcomelen =
            snprintf(welcome, sizeof(welcome), "Kilo editor -- version %s",
                     KILO_VERSION.c_str());
        if (welcomelen > E.screencols)
          welcomelen = E.screencols;
        int padding = (E.screencols - welcomelen) / 2;
        if (padding) {
          ab.append("~");
          padding--;
        }
        while (padding--)
          ab.append(" ");
        ab.append(welcome);
      } else {
        ab.append("~");
      }
    } else {
      int len = static_cast<int>(E.row[filerow].rsize) - 
        static_cast<int>(E.coloff);
      if (len < 0)
        len = 0;
      if (len > E.screencols)
        len = E.screencols;
      char *c = &E.row[filerow].render[E.coloff];
      unsigned char *hl = &E.row[filerow].hl[E.coloff];
      fg current_color = fg::black; // black is not used in editorSyntaxToColor
      int j;
      for (j = 0; j < len; j++) {
        if (iscntrl(c[j])) {
          char sym = (c[j] <= 26) ? '@' + c[j] : '?';
          ab.append(color(style::reversed));
          ab.append(std::string(&sym, 1));
          ab.append(color(style::reset));
          if (current_color != fg::black) {
            ab.append(color(current_color));
          }
        } else if (hl[j] == HL_NORMAL) {
          if (current_color != fg::black) {
            ab.append(color(fg::reset));
            current_color = fg::black;
          }
          ab.append(std::string(&c[j], 1));
        } else {
          fg color = editorSyntaxToColor(hl[j]);
          if (color != current_color) {
            current_color = color;
            ab.append(Term::color(color));
          }
          ab.append(std::string(&c[j], 1));
        }
      }
      ab.append(color(fg::reset));
    }

    ab.append(erase_to_eol());
    ab.append("\r\n");
  }
}

void editorDrawStatusBar(std::string &ab) {
  ab.append(color(style::reversed));
  char status[80], rstatus[80];
  int len = snprintf(status, sizeof(status), "%.20s - %u lines %s",
                     E.filename ? E.filename : "[No Name]", static_cast<unsigned int>(E.numrows),
                     E.dirty ? "(modified)" : "");
  int rlen =
      snprintf(rstatus, sizeof(rstatus), "%s | %u/%u",
               E.syntax ? E.syntax->filetype : "no ft", static_cast<unsigned int>(E.cy) + 1, static_cast<unsigned int>(E.numrows));
  if (len > E.screencols)
    len = E.screencols;
  ab.append(std::string(status, static_cast<std::size_t>(len)));
  while (len < E.screencols) {
    if (E.screencols - len == rlen) {
      ab.append(std::string(rstatus, static_cast<std::size_t>(rlen)));
      break;
    } else {
      ab.append(" ");
      len++;
    }
  }
  ab.append(color(style::reset));
  ab.append("\r\n");
}

void editorDrawMessageBar(std::string &ab) {
  ab.append(erase_to_eol());
  int msglen = static_cast<int>(strlen(E.statusmsg));
  if (msglen > E.screencols)
    msglen = E.screencols;
  if (msglen && time(NULL) - E.statusmsg_time < 5)
    ab.append(std::string(E.statusmsg, static_cast<std::size_t>(msglen)));
}

void editorRefreshScreen(const Terminal &term) {
  editorScroll();

  std::string ab;
  ab.reserve(16 * 1024);

  ab.append(cursor_off());
  ab.append(move_cursor(1, 1));

  editorDrawRows(ab);
  editorDrawStatusBar(ab);
  editorDrawMessageBar(ab);

  ab.append(move_cursor((E.cy - E.rowoff) + 1, (E.rx - E.coloff) + 1));

  ab.append(cursor_on());

  term.write(ab);
}

void editorSetStatusMessage() {
  E.statusmsg[0] = '\0';
  E.statusmsg_time = time(NULL);
}

void editorSetStatusMessage(const char *msg) {
  strncpy(E.statusmsg, msg, sizeof(E.statusmsg));
  E.statusmsg_time = time(NULL);
}

/* void editorSetStatusMessage(const char *fmt, const char *msg) { */
/*   snprintf(E.statusmsg, sizeof(E.statusmsg), fmt,  msg); */ 
/*   E.statusmsg_time = time(NULL); */
/* } */

// /*** input ***/

char *editorPrompt(const Terminal &term, const char *prompt1, const char *prompt2, void (*callback)(char *, int)) {
  std::size_t bufsize = 128;
  char *buf = static_cast<char *>(malloc(bufsize));

  std::size_t buflen = 0;
  buf[0] = '\0';
  char outbuf[256];

  while (true) {
    snprintf(outbuf, sizeof(outbuf), "%s%s%s", prompt1, buf, prompt2);
    editorSetStatusMessage(outbuf);
    editorRefreshScreen(term);

    int c = term.read_key();
    if (c == Key::DEL || c == CTRL('h') ||
        c == Key::BACKSPACE) {
      if (buflen != 0)
        buf[--buflen] = '\0';
    } else if (c == Key::ESC) {
      editorSetStatusMessage();
      if (callback)
        callback(buf, c);
      free(buf);
      return nullptr;
    } else if (c == Key::ENTER) {
      if (buflen != 0) {
        editorSetStatusMessage();
        if (callback)
          callback(buf, c);
        return buf;
      }
    } else if (!iscntrl(c) && c < 128) {
      if (buflen == bufsize - 1) {
        bufsize *= 2;
        buf = static_cast<char *>(realloc(buf, bufsize));
      }
      buf[buflen++] = static_cast<char>(c);
      buf[buflen] = '\0';
    }

    if (callback)
      callback(buf, c);
  }
}

void editorMoveCursor(int key) {
  erow *row = (E.cy >= E.numrows) ? nullptr : &E.row[E.cy];

  switch (key) {
  case Key::ARROW_LEFT:
    if (E.cx != 0) {
      E.cx--;
    } else if (E.cy > 0) {
      E.cy--;
      E.cx = E.row[E.cy].size;
    }
    break;
  case Key::ARROW_RIGHT:
    if (row && E.cx < row->size) {
      E.cx++;
    } else if (row && E.cx == row->size) {
      E.cy++;
      E.cx = 0;
    }
    break;
  case Key::ARROW_UP:
    if (E.cy != 0) {
      E.cy--;
    }
    break;
  case Key::ARROW_DOWN:
    if (E.cy < E.numrows) {
      E.cy++;
    }
    break;
  }

  row = (E.cy >= E.numrows) ? nullptr : &E.row[E.cy];
  int rowlen = row ? static_cast<int>(row->size) : 0;
  if (static_cast<int>(E.cx) > rowlen) {
    E.cx = static_cast<std::size_t>(rowlen);
  }
}

bool editorProcessKeypress(const Terminal &term) {
  static int quit_times = KILO_QUIT_TIMES;

  int c = term.read_key();

  switch (c) {
  case Key::ENTER:
    editorInsertNewLine();
    break;

  case CTRL_KEY('q'):
    if (E.dirty && quit_times > 0) {
      char buf[256];
      snprintf(buf, sizeof(buf), 
          "WARNING!!! File has unsaved changes. "
          "Press Ctrl-Q %d more times to quit.", quit_times);
      editorSetStatusMessage(buf);
      quit_times--;
      return true;
    }
    return false;
    break;

  case CTRL_KEY('s'):
    editorSave(term);
    break;

  case Key::HOME:
    E.cx = 0;
    break;

  case Key::END:
    if (E.cy < E.numrows)
      E.cx = E.row[E.cy].size;
    break;

  case CTRL_KEY('f'):
    editorFind(term);
    break;

  case Key::BACKSPACE:
  case CTRL_KEY('h'):
  case Key::DEL:
    if (c == Key::DEL)
      editorMoveCursor(Key::ARROW_RIGHT);
    editorDelChar();
    break;

  case Key::PAGE_UP:
  case Key::PAGE_DOWN: {
    if (c == Key::PAGE_UP) {
      E.cy = E.rowoff;
    } else if (c == Key::PAGE_DOWN) {
      E.cy = E.rowoff + static_cast<std::size_t>(E.screenrows) - 1;
      if (E.cy > E.numrows)
        E.cy = E.numrows;
    }
    int times = E.screenrows;
    while (times--)
      editorMoveCursor(c == Key::PAGE_UP ? Key::ARROW_UP
                                         : Key::ARROW_DOWN);
  } break;

  case Key::ARROW_UP:
  case Key::ARROW_DOWN:
  case Key::ARROW_LEFT:
  case Key::ARROW_RIGHT:
    editorMoveCursor(c);
    break;

  case CTRL_KEY('l'):
  case Key::ESC:
    break;

  case Key::TAB:
      editorInsertChar('\t');
      break;

  default:
    editorInsertChar(c);
    break;
  }

  quit_times = KILO_QUIT_TIMES;
  return true;
}

// /*** init ***/

void initEditor(const Terminal &term) {
  E.cx = 0;
  E.cy = 0;
  E.rx = 0;
  E.rowoff = 0;
  E.coloff = 0;
  E.numrows = 0;
  E.row = nullptr;
  E.dirty = 0;
  E.filename = nullptr;
  E.statusmsg[0] = '\0';
  E.statusmsg_time = 0;
  E.syntax = nullptr;
  term.get_term_size(E.screenrows, E.screencols);
  E.screenrows -= 2;
}

int main(int argc, char *argv[]) {
  // We must put all code in try/catch block, otherwise destructors are not
  // being called when exception happens and the terminal is not put into
  // correct state.
  try {
    Terminal term(true, false);
    term.save_screen();
    initEditor(term);
    if (argc >= 2) {
      editorOpen(argv[1]);
    }
    editorSetStatusMessage(
        "HELP: Ctrl-S = save | Ctrl - Q = quit | Ctrl-F = find");

    editorRefreshScreen(term);
    while (editorProcessKeypress(term)) {
      editorRefreshScreen(term);
    }
  } catch (const std::runtime_error& re) {
    std::cerr << "Runtime error: " << re.what() << std::endl;
    return 2;
  } catch (...) {
    std::cerr << "Unknown error." << std::endl;
    return 1;
  }
  return 0;
}

/* #include <functional> */
/* #include <iostream> */
/* #include <optional> */

/* #include <CLI/CLI.hpp> */
/* #include <spdlog/spdlog.h> */

// This file will be generated automatically when you run the CMake configuration step.
// It creates a namespace called `myproject`.
// You can modify the source template at `configured_files/config.hpp.in`.
/* #include <internal_use_only/config.hpp> */


// NOLINTNEXTLINE(bugprone-exception-escape)
/* int main(int argc, const char **argv) */
/* { */
/*   try { */
/*     CLI::App app{ fmt::format("{} version {}", myproject::cmake::project_name, myproject::cmake::project_version) }; */

/*     std::optional<std::string> message; */
/*     app.add_option("-m,--message", message, "A message to print back out"); */
/*     bool show_version = false; */
/*     app.add_flag("--version", show_version, "Show version information"); */

/*     CLI11_PARSE(app, argc, argv); */

/*     if (show_version) { */
/*       fmt::print("{}\n", myproject::cmake::project_version); */
/*       return EXIT_SUCCESS; */
/*     } */

/*     // Use the default logger (stdout, multi-threaded, colored) */
/*     spdlog::info("Hello, {}!", "World"); */

/*     if (message) { */
/*       fmt::print("Message: '{}'\n", *message); */
/*     } else { */
/*       fmt::print("No Message Provided :(\n"); */
/*     } */
/*   } catch (const std::exception &e) { */
/*     spdlog::error("Unhandled exception in main: {}", e.what()); */
/*   } */
/* } */


