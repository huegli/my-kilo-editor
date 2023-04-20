#include "row.h"
#include <catch2/catch_test_macros.hpp>

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
  CHECK(1 == row::RxToCx(r, 8));
  CHECK(0 == row::RxToCx(r, 7));
  CHECK(0 == row::RxToCx(r, 1));
  CHECK(0 == row::RxToCx(r, 0));

  r.chars = "#\t#";
  CHECK(2 == row::RxToCx(r, 8));
  CHECK(1 == row::RxToCx(r, 7));
  CHECK(0 == row::RxToCx(r, 0));
}

TEST_CASE("Update", "[row]")
{
  CHECK(checkUpdate("\t", "        "));
  CHECK(checkUpdate("#\t#", "#       #"));
  CHECK(checkUpdate("##\t#", "##      #"));
  CHECK(checkUpdate("###\t#", "###     #"));
  CHECK(checkUpdate("####\t#", "####    #"));
  CHECK(checkUpdate("#####\t#", "#####   #"));
  CHECK(checkUpdate("######\t#", "######  #"));
  CHECK(checkUpdate("########\t#", "########        #"));
}
