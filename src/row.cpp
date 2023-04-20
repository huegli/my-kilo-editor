#include "row.h"
#include "editorConfig.h"

#include <string>

const std::size_t KILO_TAB_STOP{ 8 };

namespace row {

/*** row operations ***/

std::size_t CxToRx(const erow &row, const std::size_t cx)
{
  std::size_t rx = 0;
  for (std::size_t j = 0; j < cx; j++) {
    if (row.chars.at(j) == '\t') rx += (KILO_TAB_STOP - 1) - (rx % KILO_TAB_STOP);
    rx++;
  }
  return rx;
}

std::size_t RxToCx(const erow &row, const std::size_t rx)
{
  std::size_t cur_rx = 0;
  std::size_t cx = 0;
  for (cx = 0; cx < row.size; cx++) {
    if (row.chars.at(cx) == '\t') cur_rx += (KILO_TAB_STOP - 1) - (cur_rx % KILO_TAB_STOP);
    cur_rx++;

    if (cur_rx > rx) return cx;
  }
  return cx;
}

void Update(erow &row)
{
  row.render.erase();
  std::size_t idx = 0;// FIXME: Try to get rid of idx
  for (std::size_t j = 0; j < row.size; j++) {
    if (row.chars.at(j) == '\t') {
      row.render.insert(idx, 1, ' ');
      idx++;
      while (idx % KILO_TAB_STOP != 0) {
        row.render.insert(idx, 1, ' ');
        idx++;
      }
    } else {
      row.render.insert(idx, 1, row.chars.at(j));
      idx++;
    }
  }
  row.rsize = row.render.length();

  // editorUpdateSyntax(row);
}

void Insert(const int at, const std::string &s)
{
  if (at < 0 || at > static_cast<int>(E.numrows)) return;

  auto idx = static_cast<std::size_t>(at);

  erow newRow{};
  E.row.insert(E.row.begin() + idx, newRow);

  for (auto j = idx + 1; j <= E.numrows; j++) E.row[j].idx++;

  E.row[at].idx = idx;

  E.row[at].size = s.length();
  E.row[at].chars = s;

  E.row[at].rsize = 0;
  E.row[at].render = "";
  E.row[at].hl = nullptr;
  E.row[at].hl_open_comment = 0;
  Update(E.row[idx]);

  E.numrows++;
  E.dirty++;
}

void editorFreeRow(erow &row) { free(row.hl); }

void Del(const int at)
{
  if (at < 0 || static_cast<std::size_t>(at) >= E.numrows) return;

  auto idx = static_cast<std::size_t>(at);
  editorFreeRow(E.row[idx]);
  E.row.erase(E.row.begin() + idx);
  for (auto j = idx; j < idx - 1; j++) E.row[j].idx--;
  E.numrows--;
  E.dirty++;
}

void InsertChar(erow &row, const int at, const char c)
{
  auto idx = static_cast<std::size_t>(at);
  if (at < 0 || static_cast<std::size_t>(at) > row.size) idx = row.size;
  row.chars.insert(idx, 1, c);
  row.size++;
  Update(row);
  E.dirty++;
}

void AppendString(erow &row, const std::string &s)
{
  row.chars.append(s);
  row.size += s.length();
  Update(row);
  E.dirty++;
}

void DelChar(erow &row, const int at)
{
  if (at < 0 || at >= static_cast<int>(row.size)) return;
  row.chars.erase(at, 1);
  row.size--;
  Update(row);
  E.dirty++;
}

}// end namespace row
