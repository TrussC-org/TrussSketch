# TrussSketch CDN Configuration

## Overview

TrussSketch is available via CDN for easy embedding in any website.

```html
<script src="https://cdn.trussc.org/sketch.js"></script>
<script type="text/tcs">
void draw() {
    clear(0.1f);
    drawCircle(getMouseX(), getMouseY(), 50);
}
</script>
```

## CDN Structure

- **Domain**: `cdn.trussc.org`
- **R2 Bucket**: `trussc-sketch-cdn`
- **Files**:
  - `sketch.js` - Loader script (users include this)
  - `TrussSketch.js` - Emscripten-generated JS
  - `TrussSketch.wasm` - WebAssembly binary (~2.8MB)

## Versioning

- `sketch.js` contains `VERSION` constant (e.g., `'1'`)
- WASM files are loaded with `?v=1` query param for cache busting
- When updating WASM, bump VERSION in sketch.js and re-upload all files

## CORS Configuration

```json
{
  "rules": [
    {
      "allowed": {
        "origins": ["*"],
        "methods": ["GET", "HEAD"],
        "headers": ["*"]
      },
      "maxAgeSeconds": 86400
    }
  ]
}
```

## Deploy Commands

```bash
cd /path/to/testSketchSite  # or wherever the files are

# Upload all files
wrangler r2 object put trussc-sketch-cdn/sketch.js --file sketch.js --remote
wrangler r2 object put trussc-sketch-cdn/TrussSketch.js --file TrussSketch.js --remote
wrangler r2 object put trussc-sketch-cdn/TrussSketch.wasm --file TrussSketch.wasm --remote

# Update CORS (if needed)
wrangler r2 bucket cors set trussc-sketch-cdn --file cors.json --force
```

## Local Development

When `sketch.js` is loaded from a local path (not cdn.trussc.org), it automatically uses local files:

```
mysite/
  sketch.js
  TrussSketch.js
  TrussSketch.wasm
  index.html
```

## For Users Who Want Fixed Versions

Download all three files and host locally:

1. Download from `cdn.trussc.org/sketch.js`, `TrussSketch.js`, `TrussSketch.wasm`
2. Place in your project
3. Reference local `sketch.js`

## Related

- Test site: `testSketchSite/` (example LP with background animation)
- Main editor: `trussc.org/sketch/`
