module Gulp

  git_version   = `git describe --tags --always`
  VERSION = "git-#{git_version}"

end
