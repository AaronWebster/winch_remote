#!/usr/bin/env bash
../emboss/embossc packets.emb
find . -name "*.h" -exec sed -i -e 's:runtime/cpp/::' '{}' \;
