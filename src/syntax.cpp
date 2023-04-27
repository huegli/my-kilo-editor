#include "syntax.h"
#include "edit.h"

#include <cstring>

namespace syntax {


#define HL_HIGHLIGHT_NUMBERS (1 << 0)
#define HL_HIGHLIGHT_STRINGS (1 << 1)

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

struct edit::editorSyntax HLDB[] = {
  { "c", C_HL_extensions, C_HL_keywords, "//", "/*", "*/", HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS },
};

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

// /*** syntax highlighting ***/

bool is_separator(const char c) { return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != nullptr; }

void Update(edit::editorConfig &E, edit::erow &row)
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
  if (changed && row.idx + 1 < E.numrows) Update(E, E.row[row.idx + 1]);
}


void SelectHighlight(edit::editorConfig &E)
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

        for (std::size_t filerow = 0; filerow < E.numrows; filerow++) { syntax::Update(E, E.row[filerow]); }

        return;
      }
      i++;
    }
  }
}

}// end namespace syntax
