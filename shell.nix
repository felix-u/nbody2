# shell.nix

{ pkgs ? import <nixpkgs> { } }:
let
  pkgs-unstable = import <nixos-unstable> { config = { allowUnfree = true; }; };
in
with pkgs; mkShell {
  nativeBuildInputs = [
  ];
  buildInputs = [
    wayland
  ];
}
