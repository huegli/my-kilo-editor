#pragma once

#include <string>
#include <vector>

namespace row {

typedef struct erow
{
  std::size_t idx;
  std::size_t size;
  std::size_t rsize;
  std::string chars{};
  std::string render{};
  unsigned char *hl;
  int hl_open_comment;
} erow;


}// namespace row


struct editorConfig
{
  std::size_t cx, cy;
  std::size_t rx;
  std::size_t rowoff;
  std::size_t coloff;
  int screenrows;
  int screencols;
  std::size_t numrows;
  std::vector<row::erow> row{};
  int dirty;
  std::string filename{};
  char statusmsg[80];
  time_t statusmsg_time;
  struct editorSyntax *syntax;
};

namespace row {

std::size_t CxToRx(const erow &, const std::size_t);
std::size_t RxToCx(const erow &, const std::size_t);
void Update(erow &);
erow &Insert(editorConfig &, const int, const std::string &);
void Del(editorConfig &, const int);
void InsertChar(erow &, const int, const char);
void AppendString(erow &, const std::string &);
void DelChar(erow &, const int);

}// end namespace row
