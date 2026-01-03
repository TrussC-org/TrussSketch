# tcScript Engine Roadmap

## Currently Implemented

See [REFERENCE.md](REFERENCE.md) for complete API documentation.

### Highlights
- Graphics: clear, setColor, setColorHSB/OKLCH/OKLab, shapes, text
- Transform: translate, rotate, scale, pushMatrix/popMatrix
- Time: getDeltaTime, getElapsedTimef, getFrameRate, system time, date/time
- Math: random, noise, lerp, clamp, map, trig, geometry (dist, distSquared)
- Input: mouse position, mouse pressed state
- Events: mousePressed, keyPressed, windowResized, etc.

---

## TODO

### Priority 1: Class Bindings (Core Types)

Add TrussC classes for richer scripting:

```javascript
// Vec2
var v = Vec2(10, 20)
v.x, v.y
v.length()
v.normalize()
v.dot(other)
v + other, v - other, v * scalar

// Vec3
var v = Vec3(10, 20, 30)
v.x, v.y, v.z
v.cross(other)

// Color
var c = Color(1.0, 0.5, 0.2)
var c = Color.fromHSB(hue, sat, bri)
c.r, c.g, c.b, c.a
```

### Priority 2: Path & StrokeMesh

```javascript
// Path (Polyline)
var path = Path()
path.addVertex(x, y)
path.close()
path.draw()

// StrokeMesh (variable width strokes)
var stroke = StrokeMesh()
stroke.setWidth(3.0)
stroke.setCapType("round")   // "butt", "round", "square"
stroke.setJoinType("miter")  // "miter", "round", "bevel"
stroke.addVertex(x, y)
stroke.draw()
```

### Priority 3: Tween (Animation)

```javascript
var tween = Tween()
tween.setup(0.0, 100.0, 1.0, "easeOutQuad")  // from, to, duration, easing
tween.start()
tween.getValue()
tween.isRunning()
```

### Priority 4: VideoGrabber (Camera)

WebRTC-based camera capture (already works in TrussC WASM).

```javascript
var camera = VideoGrabber()
camera.setup(640, 480)
camera.draw(x, y)
camera.draw(x, y, w, h)
camera.getWidth()
camera.getHeight()
```

### Priority 5: ChipSound

Add procedural sound generation for 8-bit style audio.

```javascript
var sound = buildSound({
    wave: "square",  // sin, square, triangle, sawtooth, noise
    hz: 440.0,
    duration: 0.3,
    volume: 0.4,
    attack: 0.01,
    decay: 0.05,
    sustain: 0.6,
    release: 0.1
})
sound.play()
sound.stop()
```

### Priority 6: VideoPlayer

Web video playback support.

```javascript
var video = loadVideo("https://example.com/video.mp4")
video.play()
video.draw(x, y, w, h)
video.getPosition()
video.setPosition(0.5)
```

### Priority 7: Image/Texture

```javascript
var img = loadImage("https://example.com/image.png")
img.draw(x, y)
img.draw(x, y, w, h)
img.getWidth()
img.getHeight()
```

### Priority 8: More Graphics

- `drawArc(x, y, r, startAngle, endAngle)`
- `beginShape()` / `vertex()` / `endShape()`
- `drawBezier(x1, y1, cx1, cy1, cx2, cy2, x2, y2)`
- `pushStyle()`, `popStyle()`

---

## Notes

- All features should work in WASM/Web environment
- ChaiScript requires exception handling (`-fexceptions` flag)
- Complex types need careful memory management in ChaiScript
- VideoGrabber uses WebRTC (requires HTTPS in production)
