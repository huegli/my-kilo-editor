#include "row.h"
#include <catch2/catch_test_macros.hpp>


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
