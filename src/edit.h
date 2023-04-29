#pragma once

#include <string>
#include <vector>

namespace edit {

const std::string KILO_VERSION{ "0.0.1" };
const std::size_t KILO_QUIT_TIMES{ 3 };

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


struct editorConfig
{
  std::size_t cx, cy;
  std::size_t rx;
  std::size_t rowoff;
  std::size_t coloff;
  int screenrows;
  int screencols;
  std::size_t numrows;
  std::vector<erow> row{};
  int dirty;
  std::string filename{};
  char statusmsg[80];
  time_t statusmsg_time;
  struct editorSyntax *syntax;
};

editorConfig &referenceToE();

edit::erow &Insert(edit::editorConfig &, const int, const std::string &);
void Del(edit::editorConfig &, const int);
void InsertChar(const char c);
void InsertNewLine();
void DelChar();
char *RowsToString(int *buflen);

void Scroll();

void Open(char *filename);

}// end namespace edit
