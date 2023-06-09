#include "edit.h"
#include "row.h"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <string>

TEST_CASE("Cx -> Rx", "[row]")
{

  edit::erow r;
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

  edit::erow r;
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
  edit::erow r;
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
  edit::erow r;
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
  edit::erow r;
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
  edit::erow r;
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
