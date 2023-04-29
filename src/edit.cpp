#include "edit.h"
#include "row.h"
#include "syntax.h"

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

namespace edit {

/*** data ***/
struct editorConfig E;

editorConfig &referenceToE() { return E; }

/*** editor operations ***/
edit::erow &Insert(edit::editorConfig &E, const int at, const std::string &s)
{
  auto idx = static_cast<std::size_t>(at);

  edit::erow newRow{};
  E.row.insert(E.row.begin() + idx, newRow);

  for (auto j = idx + 1; j <= E.numrows; j++) E.row[j].idx++;

  E.row[at].idx = idx;

  E.row[at].size = s.length();
  E.row[at].chars = s;

  E.row[at].rsize = 0;
  E.row[at].render = "";
  E.row[at].hl = nullptr;
  E.row[at].hl_open_comment = 0;
  row::Update(E.row[idx]);

  E.numrows++;
  E.dirty++;

  return E.row[idx];
}

void editorFreeRow(edit::erow &r) { free(r.hl); }

void Del(edit::editorConfig &E, const int at)
{
  if (at < 0 || static_cast<std::size_t>(at) >= E.numrows) return;

  auto idx = static_cast<std::size_t>(at);
  editorFreeRow(E.row[idx]);
  E.row.erase(E.row.begin() + idx);
  for (auto j = idx; j < E.numrows - 1; j++) E.row[j].idx--;
  E.numrows--;
  E.dirty++;
}

void InsertChar(const char c)
{
  if (E.cy == E.numrows) { syntax::Update(E, edit::Insert(E, static_cast<int>(E.numrows), "")); }
  row::InsertChar(E.row[E.cy], static_cast<int>(E.cx), c);
  syntax::Update(E, E.row[E.cy]);
  E.dirty++;
  E.cx++;
}

void InsertNewLine()
{
  if (E.cx == 0) {
    syntax::Update(E, edit::Insert(E, static_cast<int>(E.cy), ""));
  } else {
    if (E.cy <= E.numrows) {
      syntax::Update(
        E, edit::Insert(E, static_cast<int>(E.cy) + 1, E.row[E.cy].chars.substr(E.cx, E.row[E.cy].size - E.cx)));
    }
    E.row[E.cy].chars.erase(E.cx, E.row[E.cy].size - E.cx);
    E.row[E.cy].size = E.cx;
    row::Update(E.row[E.cy]);
    syntax::Update(E, E.row[E.cy]);
  }
  E.dirty++;
  E.cy++;
  E.cx = 0;
}

void DelChar()
{
  if (E.cy == E.numrows) return;
  if (E.cx == 0 && E.cy == 0) return;

  edit::erow &row = E.row[E.cy];
  if (E.cx > 0) {
    row::DelChar(row, static_cast<int>(E.cx) - 1);
    syntax::Update(E, E.row[E.cy - 1]);
    E.dirty++;
    E.cx--;
  } else {
    E.cx = E.row[E.cy - 1].size;
    row::AppendString(E.row[E.cy - 1], row.chars);
    syntax::Update(E, E.row[E.cy - 1]);
    E.dirty++;
    edit::Del(E, static_cast<int>(E.cy));
    E.cy--;
  }
}

// /*** file i/o ***/

char *RowsToString(int *buflen)
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

void Scroll()
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

void Open(char *filename)
{
#ifdef _WIN32
  E.filename = _strdup(filename);
#else
  E.filename = filename;
#endif
  syntax::SelectHighlight(E);

  std::ifstream f(filename);
  if (f.fail()) throw std::runtime_error("File failed to open.");
  std::string line;
  std::getline(f, line);
  while (f.rdstate() == std::ios_base::goodbit) {
    std::size_t linelen = line.size();
    while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) linelen--;
    syntax::Update(E, edit::Insert(E, static_cast<int>(E.numrows), line));
    std::getline(f, line);
  }
  E.dirty = 0;
}

}// namespace edit
