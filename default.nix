{ pkgs ? import <nixpkgs> {} }:

pkgs.libsForQt5.callPackage ./explicitcad.nix {}
