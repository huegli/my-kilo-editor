#include "row.h"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <string>

TEST_CASE("Cx -> Rx", "[row]")
{

  row::erow r;
  r.chars = "\t#";
  CHECK(9 == row::CxToRx(r, r.chars.length()));
  r.chars = "#\t#";
  CHECK(9 == row::CxToRx(r, r.chars.length()));
  r.chars = "##\t#";
  CHECK(9 == row::CxToRx(r, r.chars.length()));
  r.chars = "####\t#";
  CHECK(9 == row::CxToRx(r, r.chars.length()));
  r.chars = "#####\t#";
  CHECK(9 == row::CxToRx(r, r.chars.length()));
  r.chars = "######\t#";
  CHECK(9 == row::CxToRx(r, r.chars.length()));
  r.chars = "#######\t#";
  CHECK(9 == row::CxToRx(r, r.chars.length()));
  r.chars = "########\t#";
  CHECK(17 == row::CxToRx(r, r.chars.length()));
}

TEST_CASE("Rx -> Cx", "[row]")
{

  row::erow r;
  r.chars = "\t#";
  r.size = r.chars.length();
  CHECK(1 == row::RxToCx(r, 8));
  CHECK(0 == row::RxToCx(r, 7));
  CHECK(0 == row::RxToCx(r, 1));
  CHECK(0 == row::RxToCx(r, 0));

  r.chars = "#\t#";
  r.size = r.chars.length();
  CHECK(2 == row::RxToCx(r, 8));
  CHECK(1 == row::RxToCx(r, 7));
  CHECK(0 == row::RxToCx(r, 0));
}

bool checkUpdate(const std::string &chars, const std::string &render)
{
  row::erow r;
  r.chars = chars;
  r.size = chars.length();
  row::Update(r);
  if ((r.render == render) && (r.rsize == render.length())) {
    return true;
  } else {
    return false;
  }
}

std::array<std::pair<std::string, std::string>, 8> updateArray{ { { "\t", "        " },
  { "#\t#", "#       #" },
  { "##\t#", "##      #" },
  { "###\t#", "###     #" },
  { "####\t#", "####    #" },
  { "#####\t#", "#####   #" },
  { "######\t#", "######  #" },
  { "########\t#", "########        #" } } };


TEST_CASE("Update", "[row]")
{
  for (const auto &sp : updateArray) { CHECK(checkUpdate(sp.first, sp.second)); }
}

TEST_CASE("InsertChar", "[row]")
{
  row::erow r;
  r.chars = "0123456789";
  r.size = 10;
  row::InsertChar(r, 0, '*');
  CHECK("*0123456789" == r.chars);
  row::InsertChar(r, 1, '^');
  CHECK("*^0123456789" == r.chars);
  row::InsertChar(r, 11, '$');
  CHECK("*^012345678$9" == r.chars);
  row::InsertChar(r, 13, '#');
  CHECK("*^012345678$9#" == r.chars);
  row::InsertChar(r, -10, '@');
  CHECK("*^012345678$9#@" == r.chars);
  row::InsertChar(r, 9999, '!');
  CHECK("*^012345678$9#@!" == r.chars);
}

TEST_CASE("AppendString", "[row")
{
  row::erow r;
  r.chars = "0123456789";
  r.size = 10;
  row::AppendString(r, "ABC");
  CHECK("0123456789ABC" == r.chars);
  row::AppendString(r, "DEF");
  CHECK("0123456789ABCDEF" == r.chars);
  row::AppendString(r, "GHI");
  CHECK("0123456789ABCDEFGHI" == r.chars);
  CHECK(19 == r.size);
}

TEST_CASE("DelChar", "[row]")
{
  row::erow r;
  r.chars = "0123456789";
  r.size = 10;
  row::DelChar(r, 0);
  CHECK("123456789" == r.chars);
  row::DelChar(r, 1);
  CHECK("13456789" == r.chars);
  row::DelChar(r, 6);
  CHECK("1345679" == r.chars);
  row::DelChar(r, 6);
  CHECK("134567" == r.chars);
  row::DelChar(r, -10);
  CHECK("134567" == r.chars);
  row::DelChar(r, 10);
  CHECK("134567" == r.chars);
}

TEST_CASE("Insert", "[row]")
{
  editorConfig E;
  E.numrows = 0; // FIXME: should not need this
  E.dirty = 0;

  // First entry
  row::Insert(E, 0 , "Hello");
  CHECK(E.row[0].chars == "Hello");
  CHECK(E.row[0].size == 5);
  CHECK(E.row[0].idx == 0);
  CHECK(E.numrows == 1);
  CHECK(E.dirty > 0);

  // Second Entry, following first
  row::Insert(E, 1, "World");
  CHECK(E.row[1].chars == "World");
  CHECK(E.row[1].idx == 1);
  CHECK(E.numrows == 2);

  // Third Entry, before first
  row::Insert(E, 0, "Top");
  CHECK(E.row[0].chars == "Top");
  CHECK(E.row[0].idx == 0);
  CHECK(E.row[1].chars == "Hello");
  CHECK(E.row[1].idx == 1);
  CHECK(E.row[2].chars == "World");
  CHECK(E.row[2].idx == 2);
  CHECK(E.numrows == 3);

  // Fourth Entry, after last
  row::Insert(E, 3, "Bottom");
  CHECK(E.row[3].chars == "Bottom");
  CHECK(E.row[3].idx == 3);
  CHECK(E.row[2].chars == "World");
  CHECK(E.row[2].idx == 2);
  CHECK(E.numrows == 4);

  // Fifth Entry, in the middle
  row::Insert(E, 2, "Middle");
  CHECK(E.row[2].chars == "Middle");
  CHECK(E.row[2].idx == 2);
  CHECK(E.row[4].chars == "Bottom");
  CHECK(E.row[4].idx == 4);
  CHECK(E.numrows == 5);
}
