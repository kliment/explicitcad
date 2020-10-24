{ stdenv
, lib
, mkDerivation
, fetchFromGitHub
, qmake
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
    (qscintilla.override { withQt5 = true; })
  ];

  nativeBuildInputs = [ qmake ];

  qtWrapperArgs =
    [ "--prefix" "PATH" ":" (lib.makeBinPath [ haskellPackages.implicit ] ) ];

  installPhase = ''
    install -d $out/bin
    install -m755 explicitcad $out/bin/
  '';

  meta = with stdenv.lib; {
    description = "A graphical user interface for implicitcad";
    homepage = "https://github.com/kliment/explicitcad";
    license = licenses.gpl3;
    maintainers = [ maintainers.sorki ];
    platforms = platforms.linux;
  };
}
