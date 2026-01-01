# tcScript Engine Roadmap

## Currently Implemented

### Graphics
- `clear(gray)`, `clear(r, g, b)`, `clear(r, g, b, a)`
- `setColor(gray)`, `setColor(r, g, b)`, `setColor(r, g, b, a)`
- `drawRect(x, y, w, h)`
- `drawCircle(x, y, r)`
- `drawEllipse(x, y, w, h)`
- `drawLine(x1, y1, x2, y2)`
- `drawTriangle(x1, y1, x2, y2, x3, y3)`
- `drawText(text, x, y)`
- `fill()`, `noFill()`, `stroke()`, `noStroke()`, `setStrokeWeight(w)`

### Transform
- `translate(x, y)`, `translate(x, y, z)`
- `rotate(rad)`, `rotateDeg(deg)`
- `scale(s)`, `scale(sx, sy)`
- `pushMatrix()`, `popMatrix()`

### Window
- `getWindowWidth()`, `getWindowHeight()`

### Input
- `getMouseX()`, `getMouseY()`, `isMousePressed()`

### Time
- `getElapsedTime()`, `getDeltaTime()`, `getFrameRate()`, `getFrameCount()`

### Math
- `random()`, `random(max)`, `random(min, max)`
- `lerp(a, b, t)`, `clamp(v, min, max)`, `map(v, inMin, inMax, outMin, outMax)`
- `noise(x)`, `noise(x, y)`, `noise(x, y, z)`
- `sin`, `cos`, `tan`, `abs`, `sqrt`, `pow`, `min`, `max`, `floor`, `ceil`, `fmod`
- `radians(deg)`, `degrees(rad)`

### Constants
- `TAU`, `HALF_TAU`, `QUARTER_TAU`, `PI`

### Utility
- `print(msg)`, `log(msg)`

---

## TODO

### Priority 1: ChipSound (from chipSoundExample)

Add procedural sound generation for 8-bit style audio.

```javascript
// Target API
var sound = buildSound({
    wave: "square",  // sin, square, triangle, sawtooth, noise, pinkNoise
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
sound.setLoop(true)

// Or simpler API
playTone(hz, duration)
playTone(hz, duration, wave)
```

Required bindings:
- `ChipSoundNote` struct
- `ChipSoundBundle` for layered sounds
- `Sound` class with `play()`, `stop()`, `setLoop()`
- `Wave` enum constants

### Priority 2: VideoPlayer (from videoPlayerWebExample)

Add web video playback support.

```javascript
// Target API
var video = loadVideo("https://example.com/video.mp4")
video.play()
video.stop()
video.setPaused(true)
video.setPosition(0.5)  // 0.0 - 1.0
video.setVolume(0.8)    // 0.0 - 1.0
video.draw(x, y, w, h)

// Getters
video.isLoaded()
video.isPlaying()
video.isPaused()
video.getWidth()
video.getHeight()
video.getDuration()
video.getPosition()
video.getVolume()
video.getCurrentFrame()
video.getTotalFrames()
```

### Priority 3: More Graphics

- `drawArc(x, y, r, startAngle, endAngle)`
- `drawPolygon(points)` or `beginShape()` / `vertex()` / `endShape()`
- `drawBezier(x1, y1, cx1, cy1, cx2, cy2, x2, y2)`
- `setTextAlign(h, v)`
- `pushStyle()`, `popStyle()`

### Priority 4: Image/Texture

```javascript
var img = loadImage("https://example.com/image.png")
img.draw(x, y)
img.draw(x, y, w, h)
img.getWidth()
img.getHeight()
```

### Priority 5: Keyboard Input

```javascript
isKeyPressed()
isKeyPressed(keyCode)
getKeyCode()

// Key constants
KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN
KEY_SPACE, KEY_ENTER, KEY_ESCAPE
```

### Priority 6: Additional Math

- `atan2(y, x)`
- `asin(x)`, `acos(x)`
- `dist(x1, y1, x2, y2)`
- `constrain(v, min, max)` (alias for clamp)

---

## Notes

- All features should work in WASM/Web environment
- ChaiScript requires exception handling (`-fexceptions` flag)
- Complex types (Sound, VideoPlayer, Image) need careful memory management in ChaiScript
