{ channel ? <nixpkgs>
, pkgs ? import channel { }
, ... }:

let
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
    adwaita-icon-theme
    gsettings-desktop-schemas
    gtkmm3
    libpeas # should be updated to peas2
    libsass
    libsysprof-capture
    ninja
    notmuch
    pkg-config
    python3
    python3Packages.pygobject3
    scdoc # or ronn
    vte
    webkitgtk_4_1
    wrapGAppsHook3
    xorg.libXdmcp
  ];

  LIBCLANG_PATH   = "${pkgs.llvmPackages.libclang}/lib";
}

