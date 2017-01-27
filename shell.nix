with import <nixpkgs> {};

stdenv.mkDerivation {
  name = "esp-env";
  
  buildInputs = [ gnumake esp-open-sdk python35Packages.pyserial esptool ];
}