/*** includes ***/

#include "edit.h"
#include "terminal.h"
#include "tui.h"


/*** defines ***/


using Term::Terminal;
using Term::cursor_on;
using Term::cursor_off;
using Term::move_cursor;
using Term::erase_to_eol;
using Term::fg;
using Term::style;
using Term::Key;


int main(int argc, char *argv[])
{
  // We must put all code in try/catch block, otherwise destructors are not
  // being called when exception happens and the terminal is not put into
  // correct state.
  try {
    Terminal term(true, false);
    term.save_screen();
    tui::init(edit::referenceToE(), term);
    if (argc >= 2) { edit::Open(argv[1]); }
    tui::SetStatusMessage(edit::referenceToE(), "HELP: Ctrl-S = save | Ctrl - Q = quit | Ctrl-F = find");

    std::string ab;
    ab.reserve(16 * 1024);

    tui::RefreshScreen(edit::referenceToE(), ab);
    term.write(ab);
    while (tui::ProcessKeypress(edit::referenceToE(), term)) {
      tui::RefreshScreen(edit::referenceToE(), ab);
      term.write(ab);
    }
  } catch (const std::runtime_error &re) {
    std::cerr << "Runtime error: " << re.what() << std::endl;
    return 2;
  } catch (...) {
    std::cerr << "Unknown error." << std::endl;
    return 1;
  }
  return 0;
}
