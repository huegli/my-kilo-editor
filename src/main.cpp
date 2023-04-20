/*** includes ***/

#include "editorConfig.h"
#include "row.h"
#include "terminal.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

#include <cctype>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <sys/types.h>

/*** defines ***/

const std::string KILO_VERSION{ "0.0.1" };
const std::size_t KILO_QUIT_TIMES{ 3 };

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

struct editorSyntax
{
  const char *filetype;
  const char **filematch;
  const char **keywords;
  const char *singleline_comment_start;
  const char *multiline_comment_start;
  const char *multiline_comment_end;
  int flags;
};

struct editorConfig E;

// /*** filetypes ***/

const char *C_HL_extensions[] = { ".c", ".h", ".cpp", nullptr };
const char *C_HL_keywords[] = { "switch",
  "if",
  "while",
  "for",
  "break",
  "continue",
  "return",
  "else",
  "struct",
  "union",
  "typedef",
  "static",
  "enum",
  "class",
  "case",
  "int|",
  "long|",
  "double|",
  "float|",
  "char|",
  "unsigned|",
  "signed|",
  "void|",
  nullptr };

struct editorSyntax HLDB[] = {
  { "c", C_HL_extensions, C_HL_keywords, "//", "/*", "*/", HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS },
};

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

// /*** prototypes ***/

char *editorPrompt(const Terminal &term, const char *prompt1, const char *prompt2, void (*callback)(char *, int));
void editorSetStatusMessage();
void editorSetStatusMessage(const char *msg);
void editorSetStatusMessage(const char *fmt, const char *buf);

// /*** syntax highlighting ***/

bool is_separator(const char c) { return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != nullptr; }

void editorUpdateSyntax(row::erow &row)
{
  row.hl = static_cast<unsigned char *>(realloc(row.hl, row.rsize));
  memset(row.hl, HL_NORMAL, row.rsize);

  if (E.syntax == nullptr) return;

  const auto **keywords = E.syntax->keywords;

  const auto *scs = E.syntax->singleline_comment_start;
  const auto *mcs = E.syntax->multiline_comment_start;
  const auto *mce = E.syntax->multiline_comment_end;

  auto scs_len = scs ? strlen(scs) : 0;
  auto mcs_len = mcs ? strlen(mcs) : 0;
  auto mce_len = mce ? strlen(mce) : 0;

  auto prev_sep = true;
  auto in_string = 0;
  auto in_comment = (row.idx > 0 && E.row[row.idx - 1].hl_open_comment);

  size_t i = 0;
  while (i < row.rsize) {
    auto c = row.render.at(i);
    auto prev_hl = (i > 0) ? row.hl[i - 1] : HL_NORMAL;

    if (scs_len && !in_string && !in_comment) {
      if (!strncmp(&row.render.at(i), scs, scs_len)) {
        memset(&row.hl[i], HL_COMMENT, row.rsize - i);
        break;
      }
    }

    if (mcs_len && mce_len && !in_string) {
      if (in_comment) {
        row.hl[i] = HL_MLCOMMENT;
        if (!strncmp(&row.render.at(i), mce, mce_len)) {
          memset(&row.hl[i], HL_MLCOMMENT, mce_len);
          i += mce_len;
          in_comment = 0;
          prev_sep = true;
          continue;
        } else {
          i++;
          continue;
        }
      } else if (!strncmp(&row.render.at(i), mcs, mcs_len)) {
        memset(&row.hl[i], HL_MLCOMMENT, mcs_len);
        i += mcs_len;
        in_comment = 1;
        continue;
      }
    }

    if (E.syntax->flags & HL_HIGHLIGHT_STRINGS) {
      if (in_string) {
        row.hl[i] = HL_STRING;
        if (c == '\\' && i + 1 < row.rsize) {
          row.hl[i + 1] = HL_STRING;
          i += 2;
          continue;
        }
        if (c == in_string) in_string = 0;
        i++;
        prev_sep = true;
        continue;
      } else {
        if (c == '"' || c == '\'') {
          in_string = static_cast<unsigned char>(c);
          row.hl[i] = HL_STRING;
          i++;
          continue;
        }
      }
    }

    if (E.syntax->flags & HL_HIGHLIGHT_NUMBERS) {
      if ((isdigit(c) && (prev_sep || prev_hl == HL_NUMBER)) || (c == '.' && prev_hl == HL_NUMBER)) {
        row.hl[i] = HL_NUMBER;
        i++;
        prev_sep = false;
        continue;
      }
    }

    if (prev_sep) {
      int j;
      for (j = 0; keywords[j]; j++) {
        auto klen = strlen(keywords[j]);
        auto kw2 = keywords[j][klen - 1] == '|';
        if (kw2) klen--;

        if (!strncmp(&row.render[i], keywords[j], klen) && is_separator(row.render[i + klen])) {
          memset(&row.hl[i], kw2 ? HL_KEYWORD2 : HL_KEYWORD1, klen);
          i += klen;
          break;
        }
      }
      if (keywords[j] != nullptr) {
        prev_sep = false;
        continue;
      }
    }

    prev_sep = is_separator(c);
    i++;
  }

  auto changed = (row.hl_open_comment != in_comment);
  row.hl_open_comment = in_comment;
  if (changed && row.idx + 1 < E.numrows) editorUpdateSyntax(E.row[row.idx + 1]);
}

auto editorSyntaxToColor(const int hl)
{
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

void editorSelectSyntaxHighlight()
{
  E.syntax = nullptr;
  if (E.filename.empty()) return;

  for (auto j = 0; j != HLDB_ENTRIES; ++j) {
    auto *s = &HLDB[j];
    unsigned int i = 0;
    while (s->filematch[i]) {
      auto is_ext = (s->filematch[i][0] == '.');
      if ((is_ext && E.filename.ends_with(s->filematch[i])) || (!is_ext && E.filename.starts_with(s->filematch[i]))) {
        E.syntax = s;

        for (std::size_t filerow = 0; filerow < E.numrows; filerow++) { editorUpdateSyntax(E.row[filerow]); }

        return;
      }
      i++;
    }
  }
}

/*** editor operations ***/

void editorInsertChar(const char c)
{
  if (E.cy == E.numrows) { row::Insert(static_cast<int>(E.numrows), ""); }
  row::InsertChar(E.row[E.cy], static_cast<int>(E.cx), c);
  E.cx++;
}

void editorInsertNewLine()
{
  if (E.cx == 0) {
    row::Insert(static_cast<int>(E.cy), "");
  } else {
    row::Insert(static_cast<int>(E.cy) + 1, E.row[E.cy].chars.substr(E.cx, E.row[E.cy].size - E.cx));
    E.row[E.cy].chars.erase(E.cx, E.row[E.cy].size - E.cx);
    E.row[E.cy].size = E.cx;
    row::Update(E.row[E.cy]);
    editorUpdateSyntax(E.row[E.cy]);
  }
  E.cy++;
  E.cx = 0;
}

void editorDelChar()
{
  if (E.cy == E.numrows) return;
  if (E.cx == 0 && E.cy == 0) return;

  row::erow &row = E.row[E.cy];
  if (E.cx > 0) {
    row::DelChar(row, static_cast<int>(E.cx) - 1);
    E.cx--;
  } else {
    E.cx = E.row[E.cy - 1].size;
    row::AppendString(E.row[E.cy - 1], row.chars);
    row::Del(static_cast<int>(E.cy));
    E.cy--;
  }
}

// /*** file i/o ***/

char *editorRowsToString(int *buflen)
{
  auto totlen = 0;
  for (auto j = 0; j < static_cast<int>(E.numrows); j++) totlen += E.row[j].size + 1;
  *buflen = totlen;

  if (totlen > 0) {
    char *buf = static_cast<char *>(malloc(static_cast<std::size_t>(totlen)));
    char *p = buf;
    for (auto j = 0; j < static_cast<int>(E.numrows); j++) {
      memcpy(p, E.row[j].chars.c_str(), E.row[j].size);
      p += E.row[j].size;
      *p = '\n';
      p++;
    }
    return buf;
  } else
    return nullptr;
}

void editorOpen(char *filename)
{
#ifdef _WIN32
  E.filename = _strdup(filename);
#else
  E.filename = filename;
#endif
  editorSelectSyntaxHighlight();

  std::ifstream f(filename);
  if (f.fail()) throw std::runtime_error("File failed to open.");
  std::string line;
  std::getline(f, line);
  while (f.rdstate() == std::ios_base::goodbit) {
    std::size_t linelen = line.size();
    while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) linelen--;
    row::Insert(static_cast<int>(E.numrows), line);
    std::getline(f, line);
  }
  E.dirty = 0;
}

void editorSave(const Terminal &term)
{
  if (E.filename.empty()) {
    E.filename = editorPrompt(term, "Save as: ", " (ESC to cancel)", nullptr);
    if (E.filename.empty()) {
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

void editorFindCallback(char *query, int key)
{
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

  if (last_match == -1) direction = 1;
  int current = last_match;
  int i;
  for (i = 0; i < static_cast<int>(E.numrows); i++) {
    current += direction;
    if (current == -1)
      current = static_cast<int>(E.numrows) - 1;
    else if (current == static_cast<int>(E.numrows))
      current = 0;

    row::erow &row = E.row[current];
    auto pos = row.render.find(query);

    if (std::string::npos != pos) {
      char *match = &row.render.at(pos);
      last_match = current;
      E.cy = static_cast<std::size_t>(current);
      E.cx = row::RxToCx(row, static_cast<std::size_t>(match - row.render.c_str()));
      E.rowoff = E.numrows;

      saved_hl_line = current;
      saved_hl = static_cast<char *>(malloc(row.size));
      memcpy(saved_hl, row.hl, row.rsize);
      memset(&row.hl[match - row.render.c_str()], HL_MATCH, strlen(query));
      break;
    }
  }
}

void editorFind(const Terminal &term)
{
  auto saved_cx = E.cx;
  auto saved_cy = E.cy;
  auto saved_coloff = E.coloff;
  auto saved_rowoff = E.rowoff;

  char *query = editorPrompt(term, "Search: ", " (ESC/Arrows/Enter)", editorFindCallback);

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

void editorScroll()
{
  E.rx = 0;
  if (E.cy < E.numrows) { E.rx = row::CxToRx(E.row[E.cy], E.cx); }
  if (E.cy < E.rowoff) { E.rowoff = E.cy; }
  if (E.cy >= E.rowoff + static_cast<std::size_t>(E.screenrows)) {
    E.rowoff = E.cy - static_cast<std::size_t>(E.screenrows) + 1;
  }
  if (E.cx < E.coloff) { E.coloff = E.rx; }
  if (E.rx >= E.coloff + static_cast<std::size_t>(E.screencols)) {
    E.coloff = E.rx - static_cast<std::size_t>(E.screencols) + 1;
  }
}

void editorDrawRows(std::string &ab)
{
  int y;
  for (y = 0; y < E.screenrows; y++) {
    int filerow = y + static_cast<int>(E.rowoff);
    if (filerow >= static_cast<int>(E.numrows)) {
      if (E.numrows == 0 && y == E.screenrows / 3) {
        char welcome[80];
        int welcomelen = snprintf(welcome, sizeof(welcome), "Kilo editor -- version %s", KILO_VERSION.c_str());
        if (welcomelen > E.screencols) welcomelen = E.screencols;
        int padding = (E.screencols - welcomelen) / 2;
        if (padding) {
          ab.append("~");
          padding--;
        }
        while (padding--) ab.append(" ");
        ab.append(welcome);
      } else {
        ab.append("~");
      }
    } else {
      int len = static_cast<int>(E.row[filerow].rsize) - static_cast<int>(E.coloff);
      if (len < 0) len = 0;
      if (len > E.screencols) len = E.screencols;
      if (len > 0) {// FIXME: Do we need this condition?
        char *c = &E.row[filerow].render.at(E.coloff);
        unsigned char *hl = &E.row[filerow].hl[E.coloff];
        fg current_color = fg::black;// black is not used in editorSyntaxToColor
        int j;
        for (j = 0; j < len; j++) {
          if (iscntrl(c[j])) {
            char sym = (c[j] <= 26) ? '@' + c[j] : '?';
            ab.append(color(style::reversed));
            ab.append(std::string(&sym, 1));
            ab.append(color(style::reset));
            if (current_color != fg::black) { ab.append(color(current_color)); }
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
    }

    ab.append(erase_to_eol());
    ab.append("\r\n");
  }
}

void editorDrawStatusBar(std::string &ab)
{
  ab.append(color(style::reversed));
  char status[80], rstatus[80];
  int len = snprintf(status,
    sizeof(status),
    "%.20s - %u lines %s",
    !E.filename.empty() ? E.filename.c_str() : "[No Name]",
    static_cast<unsigned int>(E.numrows),
    E.dirty ? "(modified)" : "");
  int rlen = snprintf(rstatus,
    sizeof(rstatus),
    "%s | %u/%u",
    E.syntax ? E.syntax->filetype : "no ft",
    static_cast<unsigned int>(E.cy) + 1,
    static_cast<unsigned int>(E.numrows));
  if (len > E.screencols) len = E.screencols;
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

void editorDrawMessageBar(std::string &ab)
{
  ab.append(erase_to_eol());
  int msglen = static_cast<int>(strlen(E.statusmsg));
  if (msglen > E.screencols) msglen = E.screencols;
  if (msglen && time(NULL) - E.statusmsg_time < 5)
    ab.append(std::string(E.statusmsg, static_cast<std::size_t>(msglen)));
}

void editorRefreshScreen(std::string &ab)
{
  editorScroll();

  ab.clear();

  ab.append(cursor_off());
  ab.append(move_cursor(1, 1));

  editorDrawRows(ab);
  editorDrawStatusBar(ab);
  editorDrawMessageBar(ab);

  ab.append(move_cursor((E.cy - E.rowoff) + 1, (E.rx - E.coloff) + 1));

  ab.append(cursor_on());
}

void editorSetStatusMessage()
{
  E.statusmsg[0] = '\0';
  E.statusmsg_time = time(NULL);
}

void editorSetStatusMessage(const char *msg)
{
  strncpy(E.statusmsg, msg, sizeof(E.statusmsg));
  E.statusmsg_time = time(NULL);
}

/* void editorSetStatusMessage(const char *fmt, const char *msg) { */
/*   snprintf(E.statusmsg, sizeof(E.statusmsg), fmt,  msg); */
/*   E.statusmsg_time = time(NULL); */
/* } */

// /*** input ***/

char *editorPrompt(const Terminal &term, const char *prompt1, const char *prompt2, void (*callback)(char *, int))
{
  std::size_t bufsize = 128;
  char *buf = static_cast<char *>(malloc(bufsize));

  std::size_t buflen = 0;
  buf[0] = '\0';
  char outbuf[256];

  std::string ab;
  ab.reserve(16 * 1024);

  while (true) {
    snprintf(outbuf, sizeof(outbuf), "%s%s%s", prompt1, buf, prompt2);
    editorSetStatusMessage(outbuf);
    editorRefreshScreen(ab);
    term.write(ab);

    int c = term.read_key();
    if (c == Key::DEL || c == CTRL_KEY('h') || c == Key::BACKSPACE) {
      if (buflen != 0) buf[--buflen] = '\0';
    } else if (c == Key::ESC) {
      editorSetStatusMessage();
      if (callback) callback(buf, c);
      free(buf);
      return nullptr;
    } else if (c == Key::ENTER) {
      if (buflen != 0) {
        editorSetStatusMessage();
        if (callback) callback(buf, c);
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

    if (callback) callback(buf, c);
  }
}

void editorMoveCursor(int key)
{
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
    if ((E.cy < E.numrows) && E.cx < E.row[E.cy].size) {
      E.cx++;
    } else if ((E.cy < E.numrows) && E.cx == E.row[E.cy].size) {
      E.cy++;
      E.cx = 0;
    }
    break;
  case Key::ARROW_UP:
    if (E.cy != 0) { E.cy--; }
    break;
  case Key::ARROW_DOWN:
    if (E.cy < E.numrows) { E.cy++; }
    break;
  }

  int rowlen = (E.cy < E.numrows) ? static_cast<int>(E.row[E.cy].size) : 0;
  if (static_cast<int>(E.cx) > rowlen) { E.cx = static_cast<std::size_t>(rowlen); }
}

bool editorProcessKeypress(const Terminal &term)
{
  static int quit_times = KILO_QUIT_TIMES;

  int c = term.read_key();

  switch (c) {
  case Key::ENTER:
    editorInsertNewLine();
    break;

  case CTRL_KEY('q'):
    if (E.dirty && quit_times > 0) {
      char buf[256];
      snprintf(buf,
        sizeof(buf),
        "WARNING!!! File has unsaved changes. "
        "Press Ctrl-Q %d more times to quit.",
        quit_times);
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
    if (E.cy < E.numrows) E.cx = E.row[E.cy].size;
    break;

  case CTRL_KEY('f'):
    editorFind(term);
    break;

  case Key::BACKSPACE:
  case CTRL_KEY('h'):
  case Key::DEL:
    if (c == Key::DEL) editorMoveCursor(Key::ARROW_RIGHT);
    editorDelChar();
    break;

  case Key::PAGE_UP:
  case Key::PAGE_DOWN: {
    if (c == Key::PAGE_UP) {
      E.cy = E.rowoff;
    } else if (c == Key::PAGE_DOWN) {
      E.cy = E.rowoff + static_cast<std::size_t>(E.screenrows) - 1;
      if (E.cy > E.numrows) E.cy = E.numrows;
    }
    int times = E.screenrows;
    while (times--) editorMoveCursor(c == Key::PAGE_UP ? Key::ARROW_UP : Key::ARROW_DOWN);
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
    editorInsertChar(static_cast<char>(c));
    break;
  }

  quit_times = KILO_QUIT_TIMES;
  return true;
}

// /*** init ***/

void initEditor(const Terminal &term)
{
  E.cx = 0;
  E.cy = 0;
  E.rx = 0;
  E.rowoff = 0;
  E.coloff = 0;
  E.numrows = 0;
  E.dirty = 0;
  E.statusmsg[0] = '\0';
  E.statusmsg_time = 0;
  E.syntax = nullptr;
  term.get_term_size(E.screenrows, E.screencols);
  E.screenrows -= 2;
}

int main(int argc, char *argv[])
{
  // We must put all code in try/catch block, otherwise destructors are not
  // being called when exception happens and the terminal is not put into
  // correct state.
  try {
    Terminal term(true, false);
    term.save_screen();
    initEditor(term);
    if (argc >= 2) { editorOpen(argv[1]); }
    editorSetStatusMessage("HELP: Ctrl-S = save | Ctrl - Q = quit | Ctrl-F = find");

    std::string ab;
    ab.reserve(16 * 1024);

    editorRefreshScreen(ab);
    term.write(ab);
    while (editorProcessKeypress(term)) {
      editorRefreshScreen(ab);
      term.write(ab);
    }
  } catch (const std::runtime_error &re) {
    std::cerr << "Runtime error: " << re.what() << std::endl;
    return 2;
  } catch (...) {
    std::cerr << "Unknown error." << std::endl;
    return 1;
  }
  return 0;
}
