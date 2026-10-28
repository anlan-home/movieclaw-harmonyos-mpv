#!/bin/bash
set -eux
if ! command -v rustup >/dev/null 2>&1; then
  wget -qO - https://sh.rustup.rs | sh -s -- -y
fi
. "/root/.cargo/env"
rustup target add aarch64-unknown-linux-ohos
rustup target add x86_64-unknown-linux-ohos
if ! command -v cargo-cinstall >/dev/null 2>&1; then
  cargo install cargo-c --features=vendored-openssl
fi
