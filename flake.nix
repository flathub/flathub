{
  description = "all dependencies for generating dependencies for deltachat tauri";

  inputs = {
    nixpkgs.url = "https://flakehub.com/f/NixOS/nixpkgs/0.1.*.tar.gz";
  };

  outputs = {
    self,
    nixpkgs,
  }: let
    supportedSystems = ["x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin"];
    forEachSupportedSystem = f:
        nixpkgs.lib.genAttrs supportedSystems (system:
            f {
                pkgs = import nixpkgs {
                    inherit system;
                };
            });
  in {
    devShells = forEachSupportedSystem ({pkgs}: {
      default = pkgs.mkShell rec {
        nativeBuildInputs = with pkgs; [
          
        ];

        buildInputs = with pkgs; [
          bash
          jq
          nodejs
          flatpak
          flatpak-builder
          pnpm
          python3
          appstream
          librsvg # for flatpak-validate-icon
        ];
        env = {};
      };
    });
  };
}