#include "tcScriptHost.h"

tcScriptHost::tcScriptHost() {
    chai_ = make_unique<chaiscript::ChaiScript>();
    bindTrussCFunctions();
}

tcScriptHost::~tcScriptHost() = default;

void tcScriptHost::bindTrussCFunctions() {
    using namespace chaiscript;

    // ==========================================================================
    // Graphics - Drawing
    // ==========================================================================

    // clear(gray) or clear(r, g, b) or clear(r, g, b, a)
    chai_->add(fun([](float gray) { clear(gray); }), "clear");
    chai_->add(fun([](float r, float g, float b) { clear(r, g, b); }), "clear");
    chai_->add(fun([](float r, float g, float b, float a) { clear(r, g, b, a); }), "clear");

    // setColor - multiple overloads
    chai_->add(fun([](float r, float g, float b) { setColor(r, g, b); }), "setColor");
    chai_->add(fun([](float r, float g, float b, float a) { setColor(r, g, b, a); }), "setColor");
    chai_->add(fun([](float gray) { setColor(gray, gray, gray); }), "setColor");

    // setColor variants for different color spaces
    chai_->add(fun([](float h, float s, float b) { setColorHSB(h, s, b); }), "setColorHSB");
    chai_->add(fun([](float h, float s, float b, float a) { setColorHSB(h, s, b, a); }), "setColorHSB");
    chai_->add(fun([](float L, float C, float H) { setColorOKLCH(L, C, H); }), "setColorOKLCH");
    chai_->add(fun([](float L, float C, float H, float a) { setColorOKLCH(L, C, H, a); }), "setColorOKLCH");
    chai_->add(fun([](float L, float a_lab, float b_lab) { setColorOKLab(L, a_lab, b_lab); }), "setColorOKLab");
    chai_->add(fun([](float L, float a_lab, float b_lab, float a) { setColorOKLab(L, a_lab, b_lab, a); }), "setColorOKLab");

    // Basic shapes
    chai_->add(fun([](float x, float y, float w, float h) { drawRect(x, y, w, h); }), "drawRect");
    chai_->add(fun([](float x, float y, float r) { drawCircle(x, y, r); }), "drawCircle");
    chai_->add(fun([](float x, float y, float w, float h) { drawEllipse(x, y, w, h); }), "drawEllipse");
    chai_->add(fun([](float x1, float y1, float x2, float y2) { drawLine(x1, y1, x2, y2); }), "drawLine");
    chai_->add(fun([](float x1, float y1, float x2, float y2, float x3, float y3) {
        drawTriangle(x1, y1, x2, y2, x3, y3);
    }), "drawTriangle");

    // Fill/Stroke control
    chai_->add(fun([]() { fill(); }), "fill");
    chai_->add(fun([]() { noFill(); }), "noFill");
    chai_->add(fun([]() { stroke(); }), "stroke");
    chai_->add(fun([]() { noStroke(); }), "noStroke");
    chai_->add(fun([](float w) { setStrokeWeight(w); }), "setStrokeWeight");

    // Text
    chai_->add(fun([](const string& text, float x, float y) {
        drawBitmapString(text, x, y);
    }), "drawBitmapString");

    // ==========================================================================
    // Transform
    // ==========================================================================
    chai_->add(fun([](float x, float y) { translate(x, y); }), "translate");
    chai_->add(fun([](float x, float y, float z) { translate(x, y, z); }), "translate");
    chai_->add(fun([](float rad) { rotate(rad); }), "rotate");
    chai_->add(fun([](float deg) { rotateDeg(deg); }), "rotateDeg");
    chai_->add(fun([](float s) { scale(s, s); }), "scale");
    chai_->add(fun([](float sx, float sy) { scale(sx, sy); }), "scale");
    chai_->add(fun([]() { pushMatrix(); }), "pushMatrix");
    chai_->add(fun([]() { popMatrix(); }), "popMatrix");

    // ==========================================================================
    // Window
    // ==========================================================================
    chai_->add(fun([]() { return getWindowWidth(); }), "getWindowWidth");
    chai_->add(fun([]() { return getWindowHeight(); }), "getWindowHeight");

    // ==========================================================================
    // Input - Mouse
    // ==========================================================================
    chai_->add(fun([]() { return getMouseX(); }), "getMouseX");
    chai_->add(fun([]() { return getMouseY(); }), "getMouseY");
    chai_->add(fun([]() { return isMousePressed(); }), "isMousePressed");

    // ==========================================================================
    // Time - Frame
    // ==========================================================================
    chai_->add(fun([]() { return getDeltaTime(); }), "getDeltaTime");
    chai_->add(fun([]() { return getFrameRate(); }), "getFrameRate");
    chai_->add(fun([]() { return static_cast<int>(getFrameCount()); }), "getFrameCount");

    // ==========================================================================
    // Time - Elapsed
    // ==========================================================================
    chai_->add(fun([]() { return getElapsedTimef(); }), "getElapsedTimef");
    chai_->add(fun([]() { return static_cast<int64_t>(getElapsedTimeMillis()); }), "getElapsedTimeMillis");
    chai_->add(fun([]() { return static_cast<int64_t>(getElapsedTimeMicros()); }), "getElapsedTimeMicros");
    chai_->add(fun([]() { resetElapsedTimeCounter(); }), "resetElapsedTimeCounter");

    // ==========================================================================
    // Time - System
    // ==========================================================================
    chai_->add(fun([]() { return static_cast<int64_t>(getSystemTimeMillis()); }), "getSystemTimeMillis");
    chai_->add(fun([]() { return static_cast<int64_t>(getSystemTimeMicros()); }), "getSystemTimeMicros");
    chai_->add(fun([]() { return getTimestampString(); }), "getTimestampString");
    chai_->add(fun([](const string& fmt) { return getTimestampString(fmt); }), "getTimestampString");

    // ==========================================================================
    // Time - Current time/date
    // ==========================================================================
    chai_->add(fun([]() { return getSeconds(); }), "getSeconds");
    chai_->add(fun([]() { return getMinutes(); }), "getMinutes");
    chai_->add(fun([]() { return getHours(); }), "getHours");
    chai_->add(fun([]() { return getYear(); }), "getYear");
    chai_->add(fun([]() { return getMonth(); }), "getMonth");
    chai_->add(fun([]() { return getDay(); }), "getDay");
    chai_->add(fun([]() { return getWeekday(); }), "getWeekday");

    // ==========================================================================
    // Math
    // ==========================================================================
    chai_->add(fun([](float min, float max) { return random(min, max); }), "random");
    chai_->add(fun([](float max) { return random(0.0f, max); }), "random");
    chai_->add(fun([]() { return random(0.0f, 1.0f); }), "random");

    chai_->add(fun([](float a, float b, float t) { return tc::lerp(a, b, t); }), "lerp");
    chai_->add(fun([](float v, float min, float max) { return tc::clamp(v, min, max); }), "clamp");
    chai_->add(fun([](float v, float inMin, float inMax, float outMin, float outMax) {
        return tc::map(v, inMin, inMax, outMin, outMax);
    }), "map");

    chai_->add(fun([](float x) { return noise(x); }), "noise");
    chai_->add(fun([](float x, float y) { return noise(x, y); }), "noise");
    chai_->add(fun([](float x, float y, float z) { return noise(x, y, z); }), "noise");

    chai_->add(fun([](float deg) -> float { return deg2rad(deg); }), "deg2rad");
    chai_->add(fun([](float rad) -> float { return rad2deg(rad); }), "rad2deg");

    // Math - Trigonometry
    chai_->add(fun([](float x) { return std::sin(x); }), "sin");
    chai_->add(fun([](float x) { return std::cos(x); }), "cos");
    chai_->add(fun([](float x) { return std::tan(x); }), "tan");
    chai_->add(fun([](float x) { return std::asin(x); }), "asin");
    chai_->add(fun([](float x) { return std::acos(x); }), "acos");
    chai_->add(fun([](float x) { return std::atan(x); }), "atan");
    chai_->add(fun([](float y, float x) { return std::atan2(y, x); }), "atan2");

    // Math - General
    chai_->add(fun([](float x) { return std::abs(x); }), "abs");
    chai_->add(fun([](float x) { return std::sqrt(x); }), "sqrt");
    chai_->add(fun([](float x, float y) { return std::pow(x, y); }), "pow");
    chai_->add(fun([](float x, float y) { return std::min(x, y); }), "min");
    chai_->add(fun([](float x, float y) { return std::max(x, y); }), "max");
    chai_->add(fun([](float x) { return std::floor(x); }), "floor");
    chai_->add(fun([](float x) { return std::ceil(x); }), "ceil");
    chai_->add(fun([](float x) { return std::round(x); }), "round");
    chai_->add(fun([](float x, float y) { return std::fmod(x, y); }), "fmod");
    chai_->add(fun([](float x) { return std::log(x); }), "log");
    chai_->add(fun([](float x) { return std::exp(x); }), "exp");
    chai_->add(fun([](float x) { return tc::sq(x); }), "sq");
    chai_->add(fun([](float x) { return tc::sign(x); }), "sign");
    chai_->add(fun([](float x) { return tc::fract(x); }), "fract");

    // Math - Geometry
    chai_->add(fun([](float x1, float y1, float x2, float y2) {
        return tc::dist(x1, y1, x2, y2);
    }), "dist");
    chai_->add(fun([](float x1, float y1, float x2, float y2) {
        return tc::distSquared(x1, y1, x2, y2);
    }), "distSquared");

    // ==========================================================================
    // Constants
    // ==========================================================================
    chai_->add_global_const(chaiscript::const_var(TAU), "TAU");
    chai_->add_global_const(chaiscript::const_var(HALF_TAU), "HALF_TAU");
    chai_->add_global_const(chaiscript::const_var(QUARTER_TAU), "QUARTER_TAU");
    chai_->add_global_const(chaiscript::const_var(static_cast<float>(M_PI)), "PI");

    // ==========================================================================
    // Utility
    // ==========================================================================
    chai_->add(fun([](const string& msg) {
        logNotice("tcScript") << msg;
    }), "logNotice");

    chai_->add(fun([]() { beep(); }), "beep");
    chai_->add(fun([](float f) { beep(f); }), "beep");

    // ==========================================================================
    // Class Bindings - Vec2
    // ==========================================================================
    chai_->add(user_type<Vec2>(), "Vec2");

    // Constructors
    chai_->add(constructor<Vec2()>(), "Vec2");
    chai_->add(constructor<Vec2(float, float)>(), "Vec2");
    chai_->add(constructor<Vec2(float)>(), "Vec2");

    // Member variables
    chai_->add(fun(&Vec2::x), "x");
    chai_->add(fun(&Vec2::y), "y");

    // Methods
    chai_->add(fun(static_cast<Vec2& (Vec2::*)(float, float)>(&Vec2::set)), "set");
    chai_->add(fun(static_cast<Vec2& (Vec2::*)(const Vec2&)>(&Vec2::set)), "set");
    chai_->add(fun(&Vec2::length), "length");
    chai_->add(fun(&Vec2::lengthSquared), "lengthSquared");
    chai_->add(fun(&Vec2::normalized), "normalized");
    chai_->add(fun(static_cast<Vec2& (Vec2::*)()>(&Vec2::normalize)), "normalize");
    chai_->add(fun(&Vec2::dot), "dot");
    chai_->add(fun(static_cast<float (Vec2::*)(const Vec2&) const>(&Vec2::cross)), "cross");
    chai_->add(fun(&Vec2::distance), "distance");
    chai_->add(fun(&Vec2::distanceSquared), "distanceSquared");
    chai_->add(fun(static_cast<float (Vec2::*)() const>(&Vec2::angle)), "angle");
    chai_->add(fun(&Vec2::rotated), "rotated");
    chai_->add(fun(static_cast<Vec2& (Vec2::*)(float)>(&Vec2::rotate)), "rotate");
    chai_->add(fun(&Vec2::perpendicular), "perpendicular");
    chai_->add(fun(static_cast<Vec2 (Vec2::*)(const Vec2&, float) const>(&Vec2::lerp)), "lerp");

    // Operators
    chai_->add(fun([](Vec2& a, const Vec2& b) -> Vec2& { a = b; return a; }), "=");
    chai_->add(fun([](const Vec2& a, const Vec2& b) { return a + b; }), "+");
    chai_->add(fun([](const Vec2& a, const Vec2& b) { return a - b; }), "-");
    chai_->add(fun([](const Vec2& v, float s) { return v * s; }), "*");
    chai_->add(fun([](float s, const Vec2& v) { return v * s; }), "*");
    chai_->add(fun([](const Vec2& v, float s) { return v / s; }), "/");
    chai_->add(fun([](const Vec2& v) { return -v; }), "-");

    // Static methods
    chai_->add(fun([](float radians) { return Vec2::fromAngle(radians); }), "Vec2_fromAngle");
    chai_->add(fun([](float radians, float length) { return Vec2::fromAngle(radians, length); }), "Vec2_fromAngle");

    // ==========================================================================
    // Class Bindings - Vec3
    // ==========================================================================
    chai_->add(user_type<Vec3>(), "Vec3");

    // Constructors
    chai_->add(constructor<Vec3()>(), "Vec3");
    chai_->add(constructor<Vec3(float, float, float)>(), "Vec3");
    chai_->add(constructor<Vec3(float)>(), "Vec3");
    chai_->add(constructor<Vec3(const Vec2&, float)>(), "Vec3");

    // Member variables
    chai_->add(fun(&Vec3::x), "x");
    chai_->add(fun(&Vec3::y), "y");
    chai_->add(fun(&Vec3::z), "z");

    // Methods
    chai_->add(fun(static_cast<Vec3& (Vec3::*)(float, float, float)>(&Vec3::set)), "set");
    chai_->add(fun(static_cast<Vec3& (Vec3::*)(const Vec3&)>(&Vec3::set)), "set");
    chai_->add(fun(&Vec3::length), "length");
    chai_->add(fun(&Vec3::lengthSquared), "lengthSquared");
    chai_->add(fun(&Vec3::normalized), "normalized");
    chai_->add(fun(static_cast<Vec3& (Vec3::*)()>(&Vec3::normalize)), "normalize");
    chai_->add(fun(&Vec3::dot), "dot");
    chai_->add(fun(&Vec3::cross), "cross");
    chai_->add(fun(&Vec3::distance), "distance");
    chai_->add(fun(&Vec3::distanceSquared), "distanceSquared");
    chai_->add(fun(static_cast<Vec3 (Vec3::*)(const Vec3&, float) const>(&Vec3::lerp)), "lerp");
    chai_->add(fun(&Vec3::xy), "xy");

    // Operators
    chai_->add(fun([](Vec3& a, const Vec3& b) -> Vec3& { a = b; return a; }), "=");
    chai_->add(fun([](const Vec3& a, const Vec3& b) { return a + b; }), "+");
    chai_->add(fun([](const Vec3& a, const Vec3& b) { return a - b; }), "-");
    chai_->add(fun([](const Vec3& v, float s) { return v * s; }), "*");
    chai_->add(fun([](float s, const Vec3& v) { return v * s; }), "*");
    chai_->add(fun([](const Vec3& v, float s) { return v / s; }), "/");
    chai_->add(fun([](const Vec3& v) { return -v; }), "-");

    // ==========================================================================
    // Class Bindings - Color
    // ==========================================================================
    chai_->add(user_type<Color>(), "Color");

    // Constructors
    chai_->add(constructor<Color()>(), "Color");
    chai_->add(constructor<Color(float, float, float)>(), "Color");
    chai_->add(constructor<Color(float, float, float, float)>(), "Color");

    // Member variables
    chai_->add(fun(&Color::r), "r");
    chai_->add(fun(&Color::g), "g");
    chai_->add(fun(&Color::b), "b");
    chai_->add(fun(&Color::a), "a");

    // Methods
    chai_->add(fun([](Color& c, float r, float g, float b) -> Color& { return c.set(r, g, b); }), "set");
    chai_->add(fun(static_cast<Color& (Color::*)(float, float, float, float)>(&Color::set)), "set");
    chai_->add(fun([](Color& c, float gray) -> Color& { return c.set(gray); }), "set");
    chai_->add(fun(static_cast<Color& (Color::*)(float, float)>(&Color::set)), "set");
    chai_->add(fun(static_cast<Color& (Color::*)(const Color&)>(&Color::set)), "set");

    chai_->add(fun(&Color::clamped), "clamped");
    chai_->add(fun([](float h, float s, float b) { return Color::fromHSB(h, s, b); }), "Color_fromHSB");
    chai_->add(fun([](float h, float s, float b, float a) { return Color::fromHSB(h, s, b, a); }), "Color_fromHSB");
    chai_->add(fun([](float L, float C, float H) { return Color::fromOKLCH(L, C, H); }), "Color_fromOKLCH");
    chai_->add(fun([](float L, float C, float H, float a) { return Color::fromOKLCH(L, C, H, a); }), "Color_fromOKLCH");
    chai_->add(fun([](float L, float a_lab, float b_lab) { return Color::fromOKLab(L, a_lab, b_lab); }), "Color_fromOKLab");
    chai_->add(fun([](float L, float a_lab, float b_lab, float alpha) { return Color::fromOKLab(L, a_lab, b_lab, alpha); }), "Color_fromOKLab");

    // Methods
    chai_->add(fun(&Color::clamped), "clamped");
    chai_->add(fun(&Color::lerpRGB), "lerpRGB");

    // Operators
    chai_->add(fun([](Color& a, const Color& b) -> Color& { a = b; return a; }), "=");
    chai_->add(fun([](const Color& a, const Color& b) { return a + b; }), "+");
    chai_->add(fun([](const Color& a, const Color& b) { return a - b; }), "-");
    chai_->add(fun([](const Color& c, float s) { return c * s; }), "*");
    chai_->add(fun([](const Color& c, float s) { return c / s; }), "/");

    // setColor with Color object
    chai_->add(fun([](const Color& c) { setColor(c.r, c.g, c.b, c.a); }), "setColor");

    // ==========================================================================
    // Class Bindings - Rect
    // ==========================================================================
    chai_->add(user_type<Rect>(), "Rect");

    // Constructors
    chai_->add(constructor<Rect()>(), "Rect");
    chai_->add(constructor<Rect(float, float, float, float)>(), "Rect");

    // Member variables
    chai_->add(fun(&Rect::x), "x");
    chai_->add(fun(&Rect::y), "y");
    chai_->add(fun(&Rect::width), "width");
    chai_->add(fun(&Rect::height), "height");

    // Methods
    chai_->add(fun(static_cast<Rect& (Rect::*)(float, float, float, float)>(&Rect::set)), "set");
    chai_->add(fun(static_cast<Rect& (Rect::*)(const Vec2&, float, float)>(&Rect::set)), "set");
    chai_->add(fun(&Rect::contains), "contains");
    chai_->add(fun(&Rect::intersects), "intersects");
    chai_->add(fun(&Rect::getRight), "getRight");
    chai_->add(fun(&Rect::getBottom), "getBottom");
    chai_->add(fun(&Rect::getCenterX), "getCenterX");
    chai_->add(fun(&Rect::getCenterY), "getCenterY");

    // ==========================================================================
    // Class Bindings - Mesh
    // ==========================================================================
    chai_->add(user_type<Mesh>(), "Mesh");
    chai_->add(constructor<Mesh()>(), "Mesh");

    // Mesh Enums
    chai_->add_global_const(const_var(PrimitiveMode::Triangles), "MESH_TRIANGLES");
    chai_->add_global_const(const_var(PrimitiveMode::TriangleStrip), "MESH_TRIANGLE_STRIP");
    chai_->add_global_const(const_var(PrimitiveMode::TriangleFan), "MESH_TRIANGLE_FAN");
    chai_->add_global_const(const_var(PrimitiveMode::Lines), "MESH_LINES");
    chai_->add_global_const(const_var(PrimitiveMode::LineStrip), "MESH_LINE_STRIP");
    chai_->add_global_const(const_var(PrimitiveMode::LineLoop), "MESH_LINE_LOOP");
    chai_->add_global_const(const_var(PrimitiveMode::Points), "MESH_POINTS");

    chai_->add(fun(&Mesh::setMode), "setMode");
    chai_->add(fun(&Mesh::getMode), "getMode");

    chai_->add(fun(static_cast<void(Mesh::*)(float, float, float)>(&Mesh::addVertex)), "addVertex");
    chai_->add(fun(static_cast<void(Mesh::*)(const Vec3&)>(&Mesh::addVertex)), "addVertex");
    chai_->add(fun(static_cast<void(Mesh::*)(const Vec2&)>(&Mesh::addVertex)), "addVertex");

    chai_->add(fun(static_cast<void(Mesh::*)(float, float, float, float)>(&Mesh::addColor)), "addColor");
    chai_->add(fun(static_cast<void(Mesh::*)(const Color&)>(&Mesh::addColor)), "addColor");

    chai_->add(fun(&Mesh::addIndex), "addIndex");
    chai_->add(fun(&Mesh::addTriangle), "addTriangle");

    chai_->add(fun(static_cast<void(Mesh::*)(float, float, float)>(&Mesh::addNormal)), "addNormal");
    chai_->add(fun(static_cast<void(Mesh::*)(const Vec3&)>(&Mesh::addNormal)), "addNormal");

    chai_->add(fun(static_cast<void(Mesh::*)(float, float)>(&Mesh::addTexCoord)), "addTexCoord");
    chai_->add(fun(static_cast<void(Mesh::*)(const Vec2&)>(&Mesh::addTexCoord)), "addTexCoord");

    chai_->add(fun(&Mesh::clear), "clear");
    chai_->add(fun(static_cast<void(Mesh::*)() const>(&Mesh::draw)), "draw");
    chai_->add(fun(static_cast<void(Mesh::*)(const Texture&) const>(&Mesh::draw)), "draw");

    chai_->add(fun([](const Mesh& m) { m.draw(); }), "drawMesh");

    // ==========================================================================
    // Class Bindings - Path (Polyline)
    // ==========================================================================
    chai_->add(user_type<Path>(), "Path");
    chai_->add(user_type<Path>(), "Polyline"); // Alias for Path
    chai_->add(constructor<Path()>(), "Path");
    chai_->add(constructor<Path()>(), "Polyline");

    chai_->add(fun(static_cast<void(Path::*)(const Vec3&)>(&Path::addVertex)), "addVertex");

    chai_->add(fun([](Path& p, float x, float y) { p.lineTo(x, y); }), "lineTo");
    chai_->add(fun([](Path& p, float x, float y, float z) { p.lineTo(x, y, z); }), "lineTo");
    chai_->add(fun([](Path& p, const Vec2& v) { p.lineTo(v); }), "lineTo");
    chai_->add(fun([](Path& p, const Vec3& v) { p.lineTo(v); }), "lineTo");

    // Curves
    chai_->add(fun(static_cast<void(Path::*)(float, float, float, float, float, float, int)>(&Path::bezierTo)), "bezierTo");
    chai_->add(fun(static_cast<void(Path::*)(const Vec2&, const Vec2&, const Vec2&, int)>(&Path::bezierTo)), "bezierTo");
    
    chai_->add(fun(static_cast<void(Path::*)(float, float, float, float, int)>(&Path::quadBezierTo)), "quadBezierTo");
    chai_->add(fun(static_cast<void(Path::*)(const Vec2&, const Vec2&, int)>(&Path::quadBezierTo)), "quadBezierTo");
    
    chai_->add(fun(static_cast<void(Path::*)(float, float, float, int)>(&Path::curveTo)), "curveTo");
    chai_->add(fun(static_cast<void(Path::*)(const Vec2&, int)>(&Path::curveTo)), "curveTo");

    chai_->add(fun(static_cast<void(Path::*)(float, float, float, float, float, float, int)>(&Path::arc)), "arc");
    chai_->add(fun(static_cast<void(Path::*)(const Vec2&, float, float, float, float, int)>(&Path::arc)), "arc");

    chai_->add(fun(&Path::close), "close");
    chai_->add(fun(&Path::setClosed), "setClosed");
    chai_->add(fun(&Path::isClosed), "isClosed");
    chai_->add(fun(&Path::clear), "clear");
    chai_->add(fun(&Path::draw), "draw");

    chai_->add(fun([](const Path& p) { p.draw(); }), "drawPath");
    chai_->add(fun([](const Path& p) { p.draw(); }), "drawPolyline");

    // ==========================================================================
    // Class Bindings - StrokeMesh
    // ==========================================================================
    chai_->add(user_type<StrokeMesh>(), "StrokeMesh");
    chai_->add(constructor<StrokeMesh()>(), "StrokeMesh");
    chai_->add(constructor<StrokeMesh(const Path&)>(), "StrokeMesh");

    // StrokeMesh Enums
    chai_->add_global_const(const_var(StrokeMesh::CAP_BUTT), "CAP_BUTT");
    chai_->add_global_const(const_var(StrokeMesh::CAP_ROUND), "CAP_ROUND");
    chai_->add_global_const(const_var(StrokeMesh::CAP_SQUARE), "CAP_SQUARE");
    
    chai_->add_global_const(const_var(StrokeMesh::JOIN_MITER), "JOIN_MITER");
    chai_->add_global_const(const_var(StrokeMesh::JOIN_ROUND), "JOIN_ROUND");
    chai_->add_global_const(const_var(StrokeMesh::JOIN_BEVEL), "JOIN_BEVEL");

    chai_->add(fun(&StrokeMesh::setWidth), "setWidth");
    chai_->add(fun(&StrokeMesh::setColor), "setColor");
    chai_->add(fun(&StrokeMesh::setCapType), "setCapType");
    chai_->add(fun(&StrokeMesh::setJoinType), "setJoinType");
    chai_->add(fun(&StrokeMesh::setMiterLimit), "setMiterLimit");

    chai_->add(fun(static_cast<void(StrokeMesh::*)(float, float, float)>(&StrokeMesh::addVertex)), "addVertex");
    chai_->add(fun(static_cast<void(StrokeMesh::*)(const Vec3&)>(&StrokeMesh::addVertex)), "addVertex");
    chai_->add(fun(static_cast<void(StrokeMesh::*)(const Vec2&)>(&StrokeMesh::addVertex)), "addVertex");

    chai_->add(fun(static_cast<void(StrokeMesh::*)(float, float, float)>(&StrokeMesh::addVertexWithWidth)), "addVertexWithWidth");
    chai_->add(fun(static_cast<void(StrokeMesh::*)(const Vec3&, float)>(&StrokeMesh::addVertexWithWidth)), "addVertexWithWidth");

    chai_->add(fun(&StrokeMesh::setShape), "setShape");
    chai_->add(fun(&StrokeMesh::setClosed), "setClosed");
    chai_->add(fun(&StrokeMesh::clear), "clear");
    chai_->add(fun(&StrokeMesh::update), "update");
    chai_->add(fun(&StrokeMesh::draw), "draw");
}

bool tcScriptHost::loadScript(const string& code) {
    lastError_.clear();

    // Reset function flags
    hasSetup_ = hasUpdate_ = hasDraw_ = false;
    hasMousePressed_ = hasMouseReleased_ = hasMouseMoved_ = hasMouseDragged_ = false;
    hasKeyPressed_ = hasKeyReleased_ = hasWindowResized_ = false;

    try {
        // Create fresh ChaiScript instance
        chai_ = make_unique<chaiscript::ChaiScript>();
        bindTrussCFunctions();

        // Evaluate user code
        chai_->eval(code);

        // Check which functions are defined
        try { chai_->eval<function<void()>>("setup"); hasSetup_ = true; } catch (...) {}
        try { chai_->eval<function<void()>>("update"); hasUpdate_ = true; } catch (...) {}
        try { chai_->eval<function<void()>>("draw"); hasDraw_ = true; } catch (...) {}
        try { chai_->eval<function<void(float, float, int)>>("mousePressed"); hasMousePressed_ = true; } catch (...) {}
        try { chai_->eval<function<void(float, float, int)>>("mouseReleased"); hasMouseReleased_ = true; } catch (...) {}
        try { chai_->eval<function<void(float, float)>>("mouseMoved"); hasMouseMoved_ = true; } catch (...) {}
        try { chai_->eval<function<void(float, float, int)>>("mouseDragged"); hasMouseDragged_ = true; } catch (...) {}
        try { chai_->eval<function<void(int)>>("keyPressed"); hasKeyPressed_ = true; } catch (...) {}
        try { chai_->eval<function<void(int)>>("keyReleased"); hasKeyReleased_ = true; } catch (...) {}
        try { chai_->eval<function<void(int, int)>>("windowResized"); hasWindowResized_ = true; } catch (...) {}

        return true;
    } catch (const chaiscript::exception::eval_error& e) {
        lastError_ = e.pretty_print();
        logError("tcScript") << lastError_;
        return false;
    } catch (const exception& e) {
        lastError_ = e.what();
        logError("tcScript") << lastError_;
        return false;
    }
}

template<typename Func>
void tcScriptHost::tryCall(const string& funcName, Func&& func) {
    try {
        func();
    } catch (const exception& e) {
        lastError_ = string(funcName) + ": " + e.what();
        logError("tcScript") << lastError_;
    }
}

void tcScriptHost::callSetup() {
    if (!hasSetup_) return;
    tryCall("setup", [this]() {
        auto func = chai_->eval<function<void()>>("setup");
        func();
    });
}

void tcScriptHost::callUpdate() {
    if (!hasUpdate_) return;
    tryCall("update", [this]() {
        auto func = chai_->eval<function<void()>>("update");
        func();
    });
}

void tcScriptHost::callDraw() {
    if (!hasDraw_) return;
    tryCall("draw", [this]() {
        auto func = chai_->eval<function<void()>>("draw");
        func();
    });
}

void tcScriptHost::callMousePressed(float x, float y, int button) {
    if (!hasMousePressed_) return;
    tryCall("mousePressed", [this, x, y, button]() {
        auto func = chai_->eval<function<void(float, float, int)>>("mousePressed");
        func(x, y, button);
    });
}

void tcScriptHost::callMouseReleased(float x, float y, int button) {
    if (!hasMouseReleased_) return;
    tryCall("mouseReleased", [this, x, y, button]() {
        auto func = chai_->eval<function<void(float, float, int)>>("mouseReleased");
        func(x, y, button);
    });
}

void tcScriptHost::callMouseMoved(float x, float y) {
    if (!hasMouseMoved_) return;
    tryCall("mouseMoved", [this, x, y]() {
        auto func = chai_->eval<function<void(float, float)>>("mouseMoved");
        func(x, y);
    });
}

void tcScriptHost::callMouseDragged(float x, float y, int button) {
    if (!hasMouseDragged_) return;
    tryCall("mouseDragged", [this, x, y, button]() {
        auto func = chai_->eval<function<void(float, float, int)>>("mouseDragged");
        func(x, y, button);
    });
}

void tcScriptHost::callKeyPressed(int key) {
    if (!hasKeyPressed_) return;
    tryCall("keyPressed", [this, key]() {
        auto func = chai_->eval<function<void(int)>>("keyPressed");
        func(key);
    });
}

void tcScriptHost::callKeyReleased(int key) {
    if (!hasKeyReleased_) return;
    tryCall("keyReleased", [this, key]() {
        auto func = chai_->eval<function<void(int)>>("keyReleased");
        func(key);
    });
}

void tcScriptHost::callWindowResized(int width, int height) {
    if (!hasWindowResized_) return;
    tryCall("windowResized", [this, width, height]() {
        auto func = chai_->eval<function<void(int, int)>>("windowResized");
        func(width, height);
    });
}
