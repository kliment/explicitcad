{ stdenv
, lib
, mkDerivation
, fetchFromGitHub
, qmake
, qtbase
, qscintilla
, haskellPackages }:

mkDerivation rec {
  version = "master";
  pname = "explicitcad";

  src = ./.;
  #src = fetchFromGitHub {
  #  owner = "kliment";
  #  repo = "explicitcad";
  #  rev = version;
  #  sha256 = "0w5pidzhpwpggjn5la384fvjzkvprvrnidb06068whci11kgpbp7";
  #};

  buildInputs = [
    qtbase
    (qscintilla.override { withQt5 = true; })
  ];

  nativeBuildInputs = [ qmake ];

  qtWrapperArgs =
    [ "--prefix" "PATH" ":" (lib.makeBinPath [ haskellPackages.implicit ] ) ];

  preFixup = lib.optionalString stdenv.hostPlatform.isDarwin ''
    # wrapQtAppsHook is broken for macOS 😂 - do it manually
    wrapQtApp $out/bin/explicitcad.app/Contents/MacOS/explicitcad
  '';

  meta = with stdenv.lib; {
    description = "A graphical user interface for implicitcad";
    homepage = "https://github.com/kliment/explicitcad";
    license = licenses.gpl3;
    maintainers = [ maintainers.sorki ];
    platforms = platforms.unix;
    broken = builtins.compareVersions qtbase.version "5.9.0" < 0;
  };
}
