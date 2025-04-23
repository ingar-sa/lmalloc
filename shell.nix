{ pkgs ? import <nixos> {} }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    gdb
    gcc13
    clang-tools
    bear
    doxygen
    cppcheck
    postgresql
    docker-compose
  ];
}
