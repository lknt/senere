/* -*- C++ -*-
 * Serene Programming Language
 *
 * Copyright (c) 2019-2023 Sameer Rahmani <lxsameer@gnu.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "commands/commands.h" // for cc, run
#include "serene/config.h"     // for SERENE_VERSION
                               //
#include <__fwd/string.h>      // for string

#include <llvm/ADT/StringRef.h>                 // for StringRef
#include <llvm/Support/CommandLine.h>           // for SubCommand, ParseCom...
#include <llvm/Support/FormatVariadic.h>        // for formatv, formatv_object
#include <llvm/Support/FormatVariadicDetails.h> // for provider_format_adapter

#include <cstring> // for strcmp
#include <string>  // for basic_string
#include <tuple>   // for tuple

namespace cl = llvm::cl;

static std::string banner =
    llvm::formatv("\n\nSerene Compiler Version {0}"
                  "\nCopyright (C) 2019-2023 "
                  "Sameer Rahmani <lxsameer@gnu.org>\n"
                  "Serene comes with ABSOLUTELY NO WARRANTY;\n"
                  "This is free software, and you are welcome\n"
                  "to redistribute it under certain conditions; \n"
                  "for details take a look at the LICENSE file.\n",
                  SERENE_VERSION);

namespace serene::opts {
// Global options ===========================================================
static cl::opt<bool> verbose("v", cl::desc("Use verbose output"),
                             cl::cat(cl::getGeneralCategory()),
                             cl::sub(cl::SubCommand::getAll()));

// Subcommands ==============================================================
// We don't use this subcommand directly but we need it for the CLI interface
static cl::SubCommand CC("cc", "Serene's C compiler interface");

static cl::SubCommand Run("run", "Run a Serene file");
} // namespace serene::opts

int main(int argc, char **argv) {

  // We don't use llvm::cl here cause we want to let
  // the clang take care of the argument parsing.
  if ((argc >= 2) && (strcmp(argv[1], "cc") == 0)) {
    return serene::commands::cc(--argc, ++argv);
  }

  // We start using llvm::cl from here onward which
  // enforces our rules even for the subcommands.
  cl::ParseCommandLineOptions(argc, argv, banner);

  if (serene::opts::Run) {
    serene::commands::run();
  }

  return 0;
}
