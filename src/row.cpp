#include "row.h"

#include <string>

const std::size_t KILO_TAB_STOP{ 8 };


namespace row {

/*** row operations ***/

std::size_t CxToRx(const edit::erow &r, const std::size_t cx)
{
  std::size_t rx = 0;
  for (std::size_t j = 0; j < cx; j++) {
    if (r.chars.at(j) == '\t') rx += (KILO_TAB_STOP - 1) - (rx % KILO_TAB_STOP);
    rx++;
  }
  return rx;
}

std::size_t RxToCx(const edit::erow &r, const std::size_t rx)
{
  std::size_t cur_rx = 0;
  std::size_t cx = 0;
  for (cx = 0; cx < r.size; cx++) {
    if (r.chars.at(cx) == '\t') cur_rx += (KILO_TAB_STOP - 1) - (cur_rx % KILO_TAB_STOP);
    cur_rx++;

    if (cur_rx > rx) return cx;
  }
  return cx;
}

void Update(edit::erow &r)
{
  r.render.erase();
  std::size_t idx = 0;// FIXME: Try to get rid of idx
  for (std::size_t j = 0; j < r.size; j++) {
    if (r.chars.at(j) == '\t') {
      r.render.insert(idx, 1, ' ');
      idx++;
      while (idx % KILO_TAB_STOP != 0) {
        r.render.insert(idx, 1, ' ');
        idx++;
      }
    } else {
      r.render.insert(idx, 1, r.chars.at(j));
      idx++;
    }
  }
  r.rsize = r.render.length();
}

void InsertChar(edit::erow &r, const int at, const char c)
{
  auto idx = static_cast<std::size_t>(at);
  if (at < 0 || static_cast<std::size_t>(at) > r.size) idx = r.size;
  r.chars.insert(idx, 1, c);
  r.size++;
  Update(r);
}

void AppendString(edit::erow &r, const std::string &s)
{
  r.chars.append(s);
  r.size += s.length();
  Update(r);
}

void DelChar(edit::erow &r, const int at)
{
  if (at < 0 || at >= static_cast<int>(r.size)) return;
  r.chars.erase(at, 1);
  r.size--;
  Update(r);
}

}// end namespace row
