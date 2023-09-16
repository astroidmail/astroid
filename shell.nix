{ ... }:

let
  pkgs = import <nixpkgs> { };

  shell-configure = pkgs.writeShellScriptBin "configure" ''
    mkdir -p build
    cd build
    cmake .. "$@"
  '';

  shell-build = pkgs.writeShellScriptBin "build" ''
    if [ ! -d "build" ]; then
      >&2 echo "First you have to run configure."
      exit 1
    fi
    cd build
    cmake --build . --parallel $NIX_BUILD_CORES "$@"
  '';

  shell-run = pkgs.writeShellScriptBin "run" ''
    if [ ! -f "build/astroid" ]; then
      >&2 echo "First you have to run build."
      exit 1
    fi
    build/astroid "@"
  '';

  shell-debug = pkgs.writeShellScriptBin "debug" ''
    if [ ! -f "build/astroid" ]; then
      >&2 echo "First you have to run build."
      exit 1
    fi
    gdb --args ./build/astroid "$@"
  '';

in
pkgs.mkShell {
  buildInputs = with pkgs; [
    shell-configure
    shell-build
    shell-run
    shell-debug

    boost
    cmake
    glib-networking protobuf
    gmime3
    gnome3.adwaita-icon-theme
    gsettings-desktop-schemas
    gtkmm3
    libpeas
    libsass
    ninja
    notmuch
    pkgconfig
    python3
    python3Packages.pygobject3
    ronn
    webkitgtk
    wrapGAppsHook
  ];

  LIBCLANG_PATH   = "${pkgs.llvmPackages.libclang}/lib";
}

