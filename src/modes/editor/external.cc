# include "external.hh"

# include "modes/edit_message.hh"

using std::endl;

namespace Astroid {
  External::External (EditMessage * _em) {
    em = _em;

    /* editor settings */
    editor_cmd  = em->editor_config.get <std::string>("cmd");

  }

  External::~External () {

  }


}

