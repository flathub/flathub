#!/usr/bin/env bats

setup() {
  TMPROOT="$(mktemp -d)"
  export XDG_CONFIG_HOME="$TMPROOT/config"
  export XDG_DATA_HOME="$TMPROOT/data"
  export XDG_CACHE_HOME="$TMPROOT/cache"
  export XDG_RUNTIME_DIR="$TMPROOT/run"
  export FLATPAK_ID="ai.lemonade_server.Lemonade"
  unset LEMONADE_DATA_DIR LEMONADE_FLATPAK_FORCE_BUNDLED
  SUPERVISOR="$BATS_TEST_DIRNAME/../../lemonade-supervisor.sh"
}

teardown() {
  rm -rf "$TMPROOT"
}

@test "env LEMONADE_DATA_DIR overrides everything" {
  mkdir -p "$TMPROOT/custom"
  LEMONADE_DATA_DIR="$TMPROOT/custom" run "$SUPERVISOR" --print-data-resolution
  [ "$status" -eq 0 ]
  [[ "$output" == *"LEMONADE_DATA_SOURCE=env"* ]]
  [[ "$output" == *"LEMONADE_DATA_ROOT=$TMPROOT/custom"* ]]
}

@test "no env override -> bundled (\$XDG_CACHE_HOME/lemonade)" {
  run "$SUPERVISOR" --print-data-resolution
  [ "$status" -eq 0 ]
  [[ "$output" == *"LEMONADE_DATA_SOURCE=bundled"* ]]
  [[ "$output" == *"LEMONADE_DATA_ROOT=$TMPROOT/cache/lemonade"* ]]
}

@test "unknown arg -> exit 64 with usage on stderr" {
  run "$SUPERVISOR" --bogus
  [ "$status" -eq 64 ]
}
