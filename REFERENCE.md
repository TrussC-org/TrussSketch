# TrussSketch API Reference

Complete API reference for TrussSketch. All functions are directly mapped from TrussC.

## Lifecycle

```javascript
setup()                          // Called once at start
update()                         // Called every frame before draw
draw()                           // Called every frame after update
```

## Events

```javascript
mousePressed(x, y, button)       // Mouse button pressed
mouseReleased(x, y, button)      // Mouse button released
mouseMoved(x, y)                 // Mouse moved
mouseDragged(x, y, button)       // Mouse dragged
keyPressed(key)                  // Key pressed
keyReleased(key)                 // Key released
windowResized(width, height)     // Window resized
```

## Graphics - Color

```javascript
clear(gray)                      // Clear screen
clear(r, g, b)                   // Clear screen
setColor(gray)                   // Set drawing color (0.0-1.0)
setColor(r, g, b)                // Set drawing color (0.0-1.0)
setColor(r, g, b, a)             // Set drawing color (0.0-1.0)
setColorHSB(h, s, b)             // Set color from HSB (H: 0-TAU)
setColorOKLCH(L, C, H)           // Set color from OKLCH
setColorOKLab(L, a, b)           // Set color from OKLab
```

## Graphics - Shapes

```javascript
drawRect(x, y, w, h)             // Draw rectangle
drawCircle(x, y, radius)         // Draw circle
drawEllipse(x, y, w, h)          // Draw ellipse
drawLine(x1, y1, x2, y2)         // Draw line
drawTriangle(x1, y1, x2, y2, x3, y3) // Draw triangle
drawBitmapString(text, x, y)     // Draw text
```

## Graphics - Style

```javascript
fill()                           // Enable fill
noFill()                         // Disable fill
stroke()                         // Enable stroke
noStroke()                       // Disable stroke
setStrokeWeight(weight)          // Set stroke width
```

## Transform

```javascript
translate(x, y)                  // Move origin
translate(x, y, z)               // Move origin
rotate(radians)                  // Rotate by radians
rotateDeg(degrees)               // Rotate by degrees
scale(s)                         // Scale
scale(sx, sy)                    // Scale
pushMatrix()                     // Save transform state
popMatrix()                      // Restore transform state
```

## Window & Input

```javascript
getWindowWidth()                 // Get canvas width
getWindowHeight()                // Get canvas height
getMouseX()                      // Get mouse X position
getMouseY()                      // Get mouse Y position
isMousePressed()                 // Is mouse button pressed
```

## Time - Frame

```javascript
getDeltaTime()                   // Seconds since last frame
getFrameRate()                   // Current FPS
getFrameCount()                  // Total frames rendered
```

## Time - Elapsed

```javascript
getElapsedTimef()                // Elapsed seconds (float)
getElapsedTimeMillis()           // Elapsed milliseconds (int64)
getElapsedTimeMicros()           // Elapsed microseconds (int64)
resetElapsedTimeCounter()        // Reset elapsed time
```

## Time - System

```javascript
getSystemTimeMillis()            // Unix time in milliseconds
getSystemTimeMicros()            // Unix time in microseconds
getTimestampString()             // Formatted timestamp
getTimestampString(format)       // Formatted timestamp
```

## Time - Current

```javascript
getSeconds()                     // Current seconds (0-59)
getMinutes()                     // Current minutes (0-59)
getHours()                       // Current hours (0-23)
getYear()                        // Current year
getMonth()                       // Current month (1-12)
getDay()                         // Current day (1-31)
getWeekday()                     // Weekday (0=Sun, 6=Sat)
```

## Math - Random & Noise

```javascript
random()                         // Random number
random(max)                      // Random number
random(min, max)                 // Random number
noise(x)                         // Perlin noise
noise(x, y)                      // Perlin noise
noise(x, y, z)                   // Perlin noise
```

## Math - Interpolation

```javascript
lerp(a, b, t)                    // Linear interpolation
clamp(v, min, max)               // Clamp value to range
map(v, inMin, inMax, outMin, outMax) // Map value between ranges
```

## Math - Trigonometry

```javascript
sin(x)                           // Sine
cos(x)                           // Cosine
tan(x)                           // Tangent
asin(x)                          // Arc sine
acos(x)                          // Arc cosine
atan(x)                          // Arc tangent
atan2(y, x)                      // Arc tangent of y/x
deg2rad(degrees)                 // Degrees to radians
rad2deg(radians)                 // Radians to degrees
```

## Math - General

```javascript
abs(x)                           // Absolute value
sqrt(x)                          // Square root
sq(x)                            // Square (x*x)
pow(x, y)                        // Power (x^y)
log(x)                           // Natural logarithm
exp(x)                           // Exponential (e^x)
min(a, b)                        // Minimum
max(a, b)                        // Maximum
floor(x)                         // Round down
ceil(x)                          // Round up
round(x)                         // Round to nearest
fmod(x, y)                       // Floating-point modulo
sign(x)                          // Sign (-1, 0, 1)
fract(x)                         // Fractional part
```

## Math - Geometry

```javascript
dist(x1, y1, x2, y2)             // Distance between points
distSquared(x1, y1, x2, y2)      // Squared distance
```

## Utility

```javascript
logNotice(message)               // Print to console
to_string(value)                 // Convert to string
```

## Types - Vec2

```javascript
Vec2()                           // Create 2D vector
Vec2(x, y)                       // Create 2D vector
Vec2(v)                          // Create 2D vector
Vec2_fromAngle(radians)          // Create Vec2 from angle
Vec2_fromAngle(radians, length)  // Create Vec2 from angle
```

## Types - Vec3

```javascript
Vec3()                           // Create 3D vector
Vec3(x, y, z)                    // Create 3D vector
Vec3(v)                          // Create 3D vector
```

## Types - Color

```javascript
Color()                          // Create color (0.0-1.0)
Color(r, g, b)                   // Create color (0.0-1.0)
Color(r, g, b, a)                // Create color (0.0-1.0)
Color_fromHSB(h, s, b)           // Create Color from HSB
Color_fromHSB(h, s, b, a)        // Create Color from HSB
Color_fromOKLCH(L, C, H)         // Create Color from OKLCH
Color_fromOKLCH(L, C, H, a)      // Create Color from OKLCH
Color_fromOKLab(L, a, b)         // Create Color from OKLab
Color_fromOKLab(L, a, b, alpha)  // Create Color from OKLab
```

## Constants

```javascript
TAU                          // 6.283... (Full circle (2*PI))
HALF_TAU                     // 3.141... (Half circle (PI))
QUARTER_TAU                  // 1.570... (Quarter circle (PI/2))
PI                           // 3.141... (Pi (use TAU instead))
```

## Variables

```javascript
global myVar = 0         // Global variable (persists across frames)
var localVar = 0         // Local variable (scope-limited)
```

## Example

```javascript
global angle = 0.0

def setup() {
    logNotice("Starting!")
}

def update() {
    angle = angle + getDeltaTime()
}

def draw() {
    clear(0.1)

    pushMatrix()
    translate(getWindowWidth() / 2.0, getWindowHeight() / 2.0)
    rotate(angle)

    setColor(1.0, 0.5, 0.2)
    drawRect(-50.0, -50.0, 100.0, 100.0)

    popMatrix()
}

def keyPressed(key) {
    logNotice("Key: " + to_string(key))
}
```
