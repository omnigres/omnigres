{
  inputs = {
    # Pending cmake 3.25.1 merge and release: https://github.com/NixOS/nixpkgs/pull/206799 
    cmake_nixpkgs.url = "github:yrashk/nixpkgs?ref=cmake-3.25.1";
    nixpkgs.url = "nixpkgs/nixos-22.11";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, cmake_nixpkgs, nixpkgs, flake-utils }:
  flake-utils.lib.eachDefaultSystem
  (system:
      let cmake = cmake_nixpkgs.legacyPackages.${system}.cmake; in
      let pkgs = nixpkgs.legacyPackages.${system};
      in {
        devShell = pkgs.mkShell { buildInputs = [ pkgs.pkgsStatic.openssl pkgs.pkgconfig cmake pkgs.flex pkgs.readline
                                                  pkgs.zlib]; };
      });
}
