#!/usr/bin/env bash
#
# deploy_cdn.sh — publish the TrussSketch engine + embed loader to the R2 CDN.
#
#   ./tools/deploy_cdn.sh <version>     # e.g. 0.6.5  -> a pinned, immutable release
#   ./tools/deploy_cdn.sh latest        # the unstable dev/testing alias
#
# Uploads to the `trussc-sketch-cdn` bucket ROOT (served as cdn.trussc.org):
#
#   cdn.trussc.org/<version>/TrussSketch.js|.wasm|.data   <- the engine
#   cdn.trussc.org/sketch@<version>.js                    <- the embed loader
#
# The loader is version-stamped on upload (its __TRUSSKETCH_VERSION__ placeholder
# is replaced with <version> so its default cdnBase points back at this release).
#
set -euo pipefail

VERSION="${1:?usage: deploy_cdn.sh <version>|latest}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BIN_DIR="$ROOT_DIR/bin"
LOADER="$ROOT_DIR/web/trussketch-loader.js"
BUCKET="trussc-sketch-cdn"

# --- preflight: refuse to run without built artifacts / loader ---------------
missing=0
for f in TrussSketch.js TrussSketch.wasm TrussSketch.data; do
    if [[ ! -f "$BIN_DIR/$f" ]]; then
        echo "error: missing engine artifact $BIN_DIR/$f — build the engine first" >&2
        missing=1
    fi
done
if [[ ! -f "$LOADER" ]]; then
    echo "error: missing loader $LOADER" >&2
    missing=1
fi
[[ "$missing" -eq 0 ]] || exit 1

# --- stamp the loader with this version --------------------------------------
STAMPED="$(mktemp)"
trap 'rm -f "$STAMPED"' EXIT
sed "s/__TRUSSKETCH_VERSION__/${VERSION}/g" "$LOADER" > "$STAMPED"

# --- cache policy -------------------------------------------------------------
# Pinned releases never change once uploaded -> browsers may cache them forever.
# 'latest' is a mutable alias -> keep its TTL short so updates propagate.
if [[ "$VERSION" == "latest" ]]; then
    CACHE="public, max-age=300"
else
    CACHE="public, max-age=31536000, immutable"
fi

echo "Deploying TrussSketch '$VERSION' to R2 bucket '$BUCKET'..."
echo

# --- engine artifacts --------------------------------------------------------
wrangler r2 object put "$BUCKET/${VERSION}/TrussSketch.js"   --file "$BIN_DIR/TrussSketch.js"   --cache-control "$CACHE" --remote
wrangler r2 object put "$BUCKET/${VERSION}/TrussSketch.wasm" --file "$BIN_DIR/TrussSketch.wasm" --cache-control "$CACHE" --remote
wrangler r2 object put "$BUCKET/${VERSION}/TrussSketch.data" --file "$BIN_DIR/TrussSketch.data" --cache-control "$CACHE" --remote

# --- version-stamped embed loader --------------------------------------------
wrangler r2 object put "$BUCKET/sketch@${VERSION}.js"        --file "$STAMPED"                  --cache-control "$CACHE" --remote

echo
echo "Done. Uploaded:"
echo "  engine  -> $BUCKET/${VERSION}/TrussSketch.js"
echo "          -> $BUCKET/${VERSION}/TrussSketch.wasm"
echo "          -> $BUCKET/${VERSION}/TrussSketch.data"
echo "  loader  -> $BUCKET/sketch@${VERSION}.js  (stamped VERSION=${VERSION})"
echo
echo "Embed URL: https://cdn.trussc.org/sketch@${VERSION}.js"
if [[ "$VERSION" == "latest" ]]; then
    echo "NOTE: 'latest' is the unstable alias — do not use it in production embeds."
fi
