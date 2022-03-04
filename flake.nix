{
  description = "Repository statistics and report tool";

  inputs.flake-utils.url = "github:numtide/flake-utils";

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in {
        defaultPackage = pkgs.stdenv.mkDerivation {
          pname = "pepper";
          version = "0.3.3";
          src = ./.;

          patchPhase = ''
            sed -i "/3rdparty\/Makefile/d" configure.ac
            sed -i "s/3rdparty//" Makefile.am
          '';

          nativeBuildInputs = [
            pkgs.autoconf
            pkgs.autoreconfHook
            pkgs.automake
            pkgs.leveldb
            pkgs.lua51Packages.lua
            pkgs.zlib
            pkgs.python
            pkgs.gnuplot
            pkgs.asciidoc
            pkgs.xmlto
            pkgs.git
          ];
        };
      });
}