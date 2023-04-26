#pragma once

#include <string>
#include <vector>

namespace edit {

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

}// end namespace edit
