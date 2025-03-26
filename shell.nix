{ pkgs ? import <nixos> {} }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    gcc13
    clang-tools
    bear
    doxygen
    cppcheck
  ];
}
