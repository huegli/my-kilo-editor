#include "edit.h"
#include "row.h"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <string>

TEST_CASE("Insert", "[edit]")
{
  edit::editorConfig E;
  E.numrows = 0;// FIXME: should not need this
  E.dirty = 0;

  // First entry
  edit::Insert(E, 0, "Hello");
  CHECK(E.row[0].chars == "Hello");
  CHECK(E.row[0].size == 5);
  CHECK(E.row[0].idx == 0);
  CHECK(E.numrows == 1);
  CHECK(E.dirty > 0);

  // Second Entry, following first
  edit::Insert(E, 1, "World");
  CHECK(E.row[1].chars == "World");
  CHECK(E.row[1].idx == 1);
  CHECK(E.numrows == 2);

  // Third Entry, before first
  edit::Insert(E, 0, "Top");
  CHECK(E.row[0].chars == "Top");
  CHECK(E.row[0].idx == 0);
  CHECK(E.row[1].chars == "Hello");
  CHECK(E.row[1].idx == 1);
  CHECK(E.row[2].chars == "World");
  CHECK(E.row[2].idx == 2);
  CHECK(E.numrows == 3);

  // Fourth Entry, after last
  edit::Insert(E, 3, "Bottom");
  CHECK(E.row[3].chars == "Bottom");
  CHECK(E.row[3].idx == 3);
  CHECK(E.row[2].chars == "World");
  CHECK(E.row[2].idx == 2);
  CHECK(E.numrows == 4);

  // Fifth Entry, in the middle
  edit::Insert(E, 2, "Middle");
  CHECK(E.row[2].chars == "Middle");
  CHECK(E.row[2].idx == 2);
  CHECK(E.row[4].chars == "Bottom");
  CHECK(E.row[4].idx == 4);
  CHECK(E.numrows == 5);
}

TEST_CASE("Del", "[edit]")
{
  edit::editorConfig E;
  E.numrows = 0;// FIXME: should not need this
  E.dirty = 0;

  // build up rows
  edit::Insert(E, 0, "One");
  edit::Insert(E, 1, "Two");
  edit::Insert(E, 2, "Three");
  edit::Insert(E, 3, "Four");

  // Delete middle  ("Two")
  edit::Del(E, 1);
  CHECK(E.row[1].chars == "Three");
  CHECK(E.row[1].idx == 1);
  CHECK(E.row[2].chars == "Four");
  CHECK(E.row[2].idx == 2);
  CHECK(E.numrows == 3);
  CHECK(E.dirty > 0);


  // Delete last ("Four")
  edit::Del(E, 2);
  CHECK(E.row[1].chars == "Three");
  CHECK(E.row[1].idx == 1);
  CHECK(E.numrows == 2);

  // Delete first ("One")
  edit::Del(E, 0);
  CHECK(E.row[0].chars == "Three");
  CHECK(E.row[0].idx == 0);
  CHECK(E.numrows == 1);

  // Delete last remaining
  edit::Del(E, 0);
  CHECK(E.numrows == 0);
}
