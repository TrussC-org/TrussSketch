#include "tcScriptHost.h"
#include <scriptstdstring/scriptstdstring.h>
#include <scriptarray/scriptarray.h>
#include <cmath>
#include <vector>
#include <memory>

// Global containers for reference types (cleaned up on script reload)
static vector<unique_ptr<Texture>> g_textures;
static vector<unique_ptr<Fbo>> g_fbos;
static vector<unique_ptr<Pixels>> g_pixels;
static vector<unique_ptr<Sound>> g_sounds;
static vector<unique_ptr<Font>> g_fonts;
static vector<unique_ptr<Tween<float>>> g_tweens;
static vector<unique_ptr<ChipSoundBundle>> g_chipBundles;
static vector<unique_ptr<Mesh>> g_meshes;
static vector<unique_ptr<Path>> g_paths;
static vector<unique_ptr<StrokeMesh>> g_strokeMeshes;
static vector<unique_ptr<Image>> g_images;
static vector<unique_ptr<EasyCam>> g_easyCams;

static void clearScriptResources() {
    g_textures.clear();
    g_fbos.clear();
    g_pixels.clear();
    g_sounds.clear();
    g_fonts.clear();
    g_tweens.clear();
    g_chipBundles.clear();
    g_meshes.clear();
    g_paths.clear();
    g_strokeMeshes.clear();
    g_images.clear();
    g_easyCams.clear();
}

// Font path constants for script access
static const string fontSans = TC_FONT_SANS;
static const string fontSerif = TC_FONT_SERIF;
static const string fontMono = TC_FONT_MONO;

// Message callback for AngelScript errors
static void messageCallbackStatic(const asSMessageInfo* msg, void* param) {
    tcScriptHost* host = static_cast<tcScriptHost*>(param);
    const char* type = "ERR ";
    if (msg->type == asMSGTYPE_WARNING) type = "WARN";
    else if (msg->type == asMSGTYPE_INFORMATION) type = "INFO";

    tc::logNotice() << "[AngelScript] " << type << " (" << msg->section << ", " << msg->row << ") : " << msg->col << " : " << msg->message;

    // Store error info for JS to parse (only errors, not warnings/info)
    if (msg->type == asMSGTYPE_ERROR) {
        host->appendError(msg->section, msg->row, msg->col, msg->message);
    }
}

// =============================================================================
// Helper macros for generic calling convention
// =============================================================================

#define AS_VOID_0(func) \
    static void as_##func(asIScriptGeneric* gen) { func(); }

#define AS_VOID_1F(func) \
    static void as_##func##_1f(asIScriptGeneric* gen) { func(gen->GetArgFloat(0)); }

#define AS_VOID_2F(func) \
    static void as_##func##_2f(asIScriptGeneric* gen) { func(gen->GetArgFloat(0), gen->GetArgFloat(1)); }

#define AS_VOID_3F(func) \
    static void as_##func##_3f(asIScriptGeneric* gen) { func(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2)); }

#define AS_VOID_4F(func) \
    static void as_##func##_4f(asIScriptGeneric* gen) { func(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2), gen->GetArgFloat(3)); }

#define AS_VOID_5F(func) \
    static void as_##func##_5f(asIScriptGeneric* gen) { func(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2), gen->GetArgFloat(3), gen->GetArgFloat(4)); }

#define AS_VOID_6F(func) \
    static void as_##func##_6f(asIScriptGeneric* gen) { func(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2), gen->GetArgFloat(3), gen->GetArgFloat(4), gen->GetArgFloat(5)); }

#define AS_FLOAT_0(func) \
    static void as_##func(asIScriptGeneric* gen) { gen->SetReturnFloat(func()); }

#define AS_FLOAT_1F(func) \
    static void as_##func##_1f(asIScriptGeneric* gen) { gen->SetReturnFloat(func(gen->GetArgFloat(0))); }

#define AS_FLOAT_2F(func) \
    static void as_##func##_2f(asIScriptGeneric* gen) { gen->SetReturnFloat(func(gen->GetArgFloat(0), gen->GetArgFloat(1))); }

#define AS_INT_0(func) \
    static void as_##func(asIScriptGeneric* gen) { gen->SetReturnDWord(func()); }

#define AS_VOID_1I(func) \
    static void as_##func##_1i(asIScriptGeneric* gen) { func(gen->GetArgDWord(0)); }

#define AS_BOOL_0(func) \
    static void as_##func(asIScriptGeneric* gen) { gen->SetReturnByte(func() ? 1 : 0); }

// =============================================================================
// Graphics - Clear & Color
// =============================================================================
AS_VOID_1F(clear)
AS_VOID_3F(clear)
AS_VOID_1F(setColor)
AS_VOID_3F(setColor)
AS_VOID_4F(setColor)
AS_VOID_3F(setColorHSB)
AS_VOID_3F(setColorOKLCH)
AS_VOID_3F(setColorOKLab)

// =============================================================================
// Graphics - Shapes
// =============================================================================
AS_VOID_4F(drawRect)
AS_VOID_3F(drawCircle)
AS_VOID_2F(drawPoint)
AS_VOID_4F(drawEllipse)
AS_VOID_4F(drawLine)
AS_VOID_6F(drawTriangle)
AS_VOID_4F(drawStroke)

// 3D shapes
static void as_drawBox_1f(asIScriptGeneric* gen) { drawBox(gen->GetArgFloat(0)); }
static void as_drawBox_3f(asIScriptGeneric* gen) { drawBox(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2)); }
static void as_drawBox_4f(asIScriptGeneric* gen) { drawBox(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2), gen->GetArgFloat(3)); }
static void as_drawBox_6f(asIScriptGeneric* gen) { drawBox(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2), gen->GetArgFloat(3), gen->GetArgFloat(4), gen->GetArgFloat(5)); }

static void as_drawSphere_1f(asIScriptGeneric* gen) { drawSphere(gen->GetArgFloat(0)); }
static void as_drawSphere_4f(asIScriptGeneric* gen) { drawSphere(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2), gen->GetArgFloat(3)); }

static void as_drawCone_2f(asIScriptGeneric* gen) { drawCone(gen->GetArgFloat(0), gen->GetArgFloat(1)); }
static void as_drawCone_5f(asIScriptGeneric* gen) { drawCone(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2), gen->GetArgFloat(3), gen->GetArgFloat(4)); }

// Text
static void as_drawBitmapString(asIScriptGeneric* gen) {
    string* text = static_cast<string*>(gen->GetArgObject(0));
    drawBitmapString(*text, gen->GetArgFloat(1), gen->GetArgFloat(2));
}

// =============================================================================
// Graphics - Style
// =============================================================================
AS_VOID_0(fill)
AS_VOID_0(noFill)
AS_VOID_1F(setStrokeWeight)
AS_FLOAT_0(getStrokeWeight)

static void as_setStrokeCap(asIScriptGeneric* gen) {
    setStrokeCap(static_cast<StrokeCap>(gen->GetArgDWord(0)));
}
static void as_getStrokeCap(asIScriptGeneric* gen) {
    gen->SetReturnDWord(static_cast<int>(getStrokeCap()));
}
static void as_setStrokeJoin(asIScriptGeneric* gen) {
    setStrokeJoin(static_cast<StrokeJoin>(gen->GetArgDWord(0)));
}
static void as_getStrokeJoin(asIScriptGeneric* gen) {
    gen->SetReturnDWord(static_cast<int>(getStrokeJoin()));
}

AS_VOID_1I(setCircleResolution)
AS_INT_0(getCircleResolution)
AS_BOOL_0(isFillEnabled)
AS_BOOL_0(isStrokeEnabled)
AS_VOID_0(pushStyle)
AS_VOID_0(popStyle)

static void as_getColor(asIScriptGeneric* gen) {
    Color c = getColor();
    gen->SetReturnObject(&c);
}

// =============================================================================
// Shape & Stroke construction
// =============================================================================
AS_VOID_0(beginShape)
AS_VOID_0(endShape)
static void as_endShape_bool(asIScriptGeneric* gen) { endShape(gen->GetArgByte(0) != 0); }
AS_VOID_2F(vertex)
AS_VOID_3F(vertex)
AS_VOID_0(beginStroke)
AS_VOID_0(endStroke)
static void as_endStroke_bool(asIScriptGeneric* gen) { endStroke(gen->GetArgByte(0) != 0); }

// =============================================================================
// Transform
// =============================================================================
AS_VOID_0(pushMatrix)
AS_VOID_0(popMatrix)
AS_VOID_2F(translate)
AS_VOID_3F(translate)
AS_VOID_1F(rotate)
AS_VOID_3F(rotate)

static void as_rotateDeg_1f(asIScriptGeneric* gen) { rotateDeg(gen->GetArgFloat(0)); }
static void as_rotateDeg_3f(asIScriptGeneric* gen) { rotateDeg(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2)); }

AS_VOID_1F(rotateX)
AS_VOID_1F(rotateY)
AS_VOID_1F(rotateZ)
AS_VOID_1F(rotateXDeg)
AS_VOID_1F(rotateYDeg)
AS_VOID_1F(rotateZDeg)
AS_VOID_0(resetMatrix)

static void as_scale_1f(asIScriptGeneric* gen) { scale(gen->GetArgFloat(0), gen->GetArgFloat(0)); }
AS_VOID_2F(scale)

// =============================================================================
// Window & Input
// =============================================================================
AS_INT_0(getWindowWidth)
AS_INT_0(getWindowHeight)
AS_FLOAT_0(getMouseX)
AS_FLOAT_0(getMouseY)
static void as_isMousePressed(asIScriptGeneric* gen) { gen->SetReturnByte(isMousePressed() ? 1 : 0); }

// =============================================================================
// Time - Frame
// =============================================================================
AS_FLOAT_0(getDeltaTime)
AS_FLOAT_0(getFrameRate)
static void as_getFrameCount(asIScriptGeneric* gen) { gen->SetReturnQWord(getFrameCount()); }

// =============================================================================
// Time - Elapsed
// =============================================================================
AS_FLOAT_0(getElapsedTimef)
static void as_getElapsedTimeMillis(asIScriptGeneric* gen) { gen->SetReturnQWord(getElapsedTimeMillis()); }
static void as_getElapsedTimeMicros(asIScriptGeneric* gen) { gen->SetReturnQWord(getElapsedTimeMicros()); }
AS_VOID_0(resetElapsedTimeCounter)

// =============================================================================
// Time - System
// =============================================================================
static void as_getSystemTimeMillis(asIScriptGeneric* gen) { gen->SetReturnQWord(getSystemTimeMillis()); }
static void as_getSystemTimeMicros(asIScriptGeneric* gen) { gen->SetReturnQWord(getSystemTimeMicros()); }
static void as_getTimestampString_0(asIScriptGeneric* gen) {
    new(gen->GetAddressOfReturnLocation()) string(getTimestampString());
}
static void as_getTimestampString_1(asIScriptGeneric* gen) {
    string* fmt = static_cast<string*>(gen->GetArgObject(0));
    new(gen->GetAddressOfReturnLocation()) string(getTimestampString(*fmt));
}

// =============================================================================
// Time - Current
// =============================================================================
AS_INT_0(getSeconds)
AS_INT_0(getMinutes)
AS_INT_0(getHours)
AS_INT_0(getYear)
AS_INT_0(getMonth)
AS_INT_0(getDay)
AS_INT_0(getWeekday)

// =============================================================================
// Math - Random
// =============================================================================
static void as_random_0(asIScriptGeneric* gen) { gen->SetReturnFloat(random(1.0f)); }
AS_FLOAT_1F(random)
AS_FLOAT_2F(random)
static void as_randomInt_1(asIScriptGeneric* gen) { gen->SetReturnDWord(randomInt(gen->GetArgDWord(0))); }
static void as_randomInt_2(asIScriptGeneric* gen) { gen->SetReturnDWord(randomInt(gen->GetArgDWord(0), gen->GetArgDWord(1))); }
static void as_randomSeed(asIScriptGeneric* gen) { randomSeed(gen->GetArgDWord(0)); }

// =============================================================================
// Math - Noise
// =============================================================================
AS_FLOAT_1F(noise)
AS_FLOAT_2F(noise)
static void as_noise_3f(asIScriptGeneric* gen) { gen->SetReturnFloat(noise(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2))); }
AS_FLOAT_1F(signedNoise)
AS_FLOAT_2F(signedNoise)
static void as_signedNoise_3f(asIScriptGeneric* gen) { gen->SetReturnFloat(signedNoise(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2))); }

// fbm - with default parameters
static void as_fbm_2f(asIScriptGeneric* gen) { gen->SetReturnFloat(fbm(gen->GetArgFloat(0), gen->GetArgFloat(1))); }
static void as_fbm_5f(asIScriptGeneric* gen) { gen->SetReturnFloat(fbm(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgDWord(2), gen->GetArgFloat(3), gen->GetArgFloat(4))); }

// =============================================================================
// Math - Interpolation
// =============================================================================
static void as_lerp(asIScriptGeneric* gen) { gen->SetReturnFloat(tc::lerp(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2))); }
static void as_clamp(asIScriptGeneric* gen) { gen->SetReturnFloat(clamp(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2))); }
static void as_map(asIScriptGeneric* gen) { gen->SetReturnFloat(tc::map(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2), gen->GetArgFloat(3), gen->GetArgFloat(4))); }

// =============================================================================
// Math - Trigonometry
// =============================================================================
static void as_sin(asIScriptGeneric* gen) { gen->SetReturnFloat(std::sin(gen->GetArgFloat(0))); }
static void as_cos(asIScriptGeneric* gen) { gen->SetReturnFloat(std::cos(gen->GetArgFloat(0))); }
static void as_tan(asIScriptGeneric* gen) { gen->SetReturnFloat(std::tan(gen->GetArgFloat(0))); }
static void as_asin(asIScriptGeneric* gen) { gen->SetReturnFloat(std::asin(gen->GetArgFloat(0))); }
static void as_acos(asIScriptGeneric* gen) { gen->SetReturnFloat(std::acos(gen->GetArgFloat(0))); }
static void as_atan(asIScriptGeneric* gen) { gen->SetReturnFloat(std::atan(gen->GetArgFloat(0))); }
static void as_atan2(asIScriptGeneric* gen) { gen->SetReturnFloat(std::atan2(gen->GetArgFloat(0), gen->GetArgFloat(1))); }
static void as_deg2rad(asIScriptGeneric* gen) { gen->SetReturnFloat(deg2rad(gen->GetArgFloat(0))); }
static void as_rad2deg(asIScriptGeneric* gen) { gen->SetReturnFloat(rad2deg(gen->GetArgFloat(0))); }

// =============================================================================
// Math - General
// =============================================================================
static void as_abs(asIScriptGeneric* gen) { gen->SetReturnFloat(std::fabs(gen->GetArgFloat(0))); }
static void as_sqrt(asIScriptGeneric* gen) { gen->SetReturnFloat(std::sqrt(gen->GetArgFloat(0))); }
static void as_sq(asIScriptGeneric* gen) { float x = gen->GetArgFloat(0); gen->SetReturnFloat(x * x); }
static void as_pow(asIScriptGeneric* gen) { gen->SetReturnFloat(std::pow(gen->GetArgFloat(0), gen->GetArgFloat(1))); }
static void as_log(asIScriptGeneric* gen) { gen->SetReturnFloat(std::log(gen->GetArgFloat(0))); }
static void as_exp(asIScriptGeneric* gen) { gen->SetReturnFloat(std::exp(gen->GetArgFloat(0))); }
static void as_min(asIScriptGeneric* gen) { gen->SetReturnFloat(std::min(gen->GetArgFloat(0), gen->GetArgFloat(1))); }
static void as_max(asIScriptGeneric* gen) { gen->SetReturnFloat(std::max(gen->GetArgFloat(0), gen->GetArgFloat(1))); }
static void as_floor(asIScriptGeneric* gen) { gen->SetReturnFloat(std::floor(gen->GetArgFloat(0))); }
static void as_ceil(asIScriptGeneric* gen) { gen->SetReturnFloat(std::ceil(gen->GetArgFloat(0))); }
static void as_round(asIScriptGeneric* gen) { gen->SetReturnFloat(std::round(gen->GetArgFloat(0))); }
static void as_fmod(asIScriptGeneric* gen) { gen->SetReturnFloat(std::fmod(gen->GetArgFloat(0), gen->GetArgFloat(1))); }
static void as_sign(asIScriptGeneric* gen) { gen->SetReturnFloat(sign(gen->GetArgFloat(0))); }
static void as_fract(asIScriptGeneric* gen) { gen->SetReturnFloat(fract(gen->GetArgFloat(0))); }

// =============================================================================
// Math - Geometry
// =============================================================================
static void as_dist(asIScriptGeneric* gen) { gen->SetReturnFloat(dist(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2), gen->GetArgFloat(3))); }
static void as_distSquared(asIScriptGeneric* gen) { gen->SetReturnFloat(distSquared(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2), gen->GetArgFloat(3))); }

// =============================================================================
// System
// =============================================================================
AS_VOID_0(toggleFullscreen)
static void as_setClipboardString(asIScriptGeneric* gen) {
    string* text = static_cast<string*>(gen->GetArgObject(0));
    setClipboardString(*text);
}
static void as_getClipboardString(asIScriptGeneric* gen) {
    new(gen->GetAddressOfReturnLocation()) string(getClipboardString());
}

// =============================================================================
// Utility
// =============================================================================
static void as_logNotice(asIScriptGeneric* gen) {
    string* str = static_cast<string*>(gen->GetArgObject(0));
    logNotice(*str);
}
static void as_toString_int(asIScriptGeneric* gen) {
    new(gen->GetAddressOfReturnLocation()) string(to_string(gen->GetArgDWord(0)));
}
static void as_toString_float(asIScriptGeneric* gen) {
    new(gen->GetAddressOfReturnLocation()) string(to_string(gen->GetArgFloat(0)));
}
AS_VOID_0(beep)
static void as_beep_1f(asIScriptGeneric* gen) { beep(gen->GetArgFloat(0)); }

// =============================================================================
// 3D Projection
// =============================================================================
AS_VOID_0(setupScreenPerspective)
static void as_setupScreenPerspective_1f(asIScriptGeneric* gen) { setupScreenPerspective(gen->GetArgFloat(0)); }
static void as_setupScreenPerspective_3f(asIScriptGeneric* gen) { setupScreenPerspective(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2)); }
AS_VOID_0(setupScreenOrtho)
static void as_setupScreenFov_1f(asIScriptGeneric* gen) { setupScreenFov(gen->GetArgFloat(0)); }
static void as_setupScreenFov_3f(asIScriptGeneric* gen) { setupScreenFov(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2)); }
static void as_setDefaultScreenFov(asIScriptGeneric* gen) { setDefaultScreenFov(gen->GetArgFloat(0)); }
static void as_getDefaultScreenFov(asIScriptGeneric* gen) { gen->SetReturnFloat(getDefaultScreenFov()); }

// =============================================================================
// Constants
// =============================================================================
static const int kStrokeCapButt = static_cast<int>(StrokeCap::Butt);
static const int kStrokeCapRound = static_cast<int>(StrokeCap::Round);
static const int kStrokeCapSquare = static_cast<int>(StrokeCap::Square);
static const int kStrokeJoinMiter = static_cast<int>(StrokeJoin::Miter);
static const int kStrokeJoinRound = static_cast<int>(StrokeJoin::Round);
static const int kStrokeJoinBevel = static_cast<int>(StrokeJoin::Bevel);

// EaseType constants
static const int kEaseLinear = static_cast<int>(EaseType::Linear);
static const int kEaseQuad = static_cast<int>(EaseType::Quad);
static const int kEaseCubic = static_cast<int>(EaseType::Cubic);
static const int kEaseQuart = static_cast<int>(EaseType::Quart);
static const int kEaseQuint = static_cast<int>(EaseType::Quint);
static const int kEaseSine = static_cast<int>(EaseType::Sine);
static const int kEaseExpo = static_cast<int>(EaseType::Expo);
static const int kEaseCirc = static_cast<int>(EaseType::Circ);
static const int kEaseBack = static_cast<int>(EaseType::Back);
static const int kEaseElastic = static_cast<int>(EaseType::Elastic);
static const int kEaseBounce = static_cast<int>(EaseType::Bounce);

// EaseMode constants
static const int kEaseModeIn = static_cast<int>(EaseMode::In);
static const int kEaseModeOut = static_cast<int>(EaseMode::Out);
static const int kEaseModeInOut = static_cast<int>(EaseMode::InOut);
static const float kHalfTau = TAU / 2.0f;
static const float kQuarterTau = TAU / 4.0f;

// =============================================================================
// Vec2 type for AngelScript
// =============================================================================
static void Vec2_Construct(asIScriptGeneric* gen) {
    new(gen->GetObject()) Vec2();
}
static void Vec2_Construct_2f(asIScriptGeneric* gen) {
    new(gen->GetObject()) Vec2(gen->GetArgFloat(0), gen->GetArgFloat(1));
}
static void Vec2_Construct_1f(asIScriptGeneric* gen) {
    float v = gen->GetArgFloat(0);
    new(gen->GetObject()) Vec2(v, v);
}
static void Vec2_CopyConstruct(asIScriptGeneric* gen) {
    Vec2* other = static_cast<Vec2*>(gen->GetArgObject(0));
    new(gen->GetObject()) Vec2(*other);
}
static void Vec2_Set(asIScriptGeneric* gen) {
    Vec2* self = static_cast<Vec2*>(gen->GetObject());
    self->x = gen->GetArgFloat(0);
    self->y = gen->GetArgFloat(1);
    gen->SetReturnObject(self);
}
static void Vec2_Length(asIScriptGeneric* gen) {
    Vec2* self = static_cast<Vec2*>(gen->GetObject());
    gen->SetReturnFloat(self->length());
}
static void Vec2_LengthSquared(asIScriptGeneric* gen) {
    Vec2* self = static_cast<Vec2*>(gen->GetObject());
    gen->SetReturnFloat(self->lengthSquared());
}
static void Vec2_Normalize(asIScriptGeneric* gen) {
    Vec2* self = static_cast<Vec2*>(gen->GetObject());
    gen->SetReturnObject(&self->normalize());
}
static void Vec2_Normalized(asIScriptGeneric* gen) {
    Vec2* self = static_cast<Vec2*>(gen->GetObject());
    new(gen->GetAddressOfReturnLocation()) Vec2(self->normalized());
}
static void Vec2_Dot(asIScriptGeneric* gen) {
    Vec2* self = static_cast<Vec2*>(gen->GetObject());
    Vec2* other = static_cast<Vec2*>(gen->GetArgObject(0));
    gen->SetReturnFloat(self->dot(*other));
}
static void Vec2_Distance(asIScriptGeneric* gen) {
    Vec2* self = static_cast<Vec2*>(gen->GetObject());
    Vec2* other = static_cast<Vec2*>(gen->GetArgObject(0));
    gen->SetReturnFloat(self->distance(*other));
}
static void Vec2_Angle(asIScriptGeneric* gen) {
    Vec2* self = static_cast<Vec2*>(gen->GetObject());
    gen->SetReturnFloat(self->angle());
}
static void Vec2_Rotate(asIScriptGeneric* gen) {
    Vec2* self = static_cast<Vec2*>(gen->GetObject());
    gen->SetReturnObject(&self->rotate(gen->GetArgFloat(0)));
}
static void Vec2_Rotated(asIScriptGeneric* gen) {
    Vec2* self = static_cast<Vec2*>(gen->GetObject());
    new(gen->GetAddressOfReturnLocation()) Vec2(self->rotated(gen->GetArgFloat(0)));
}
static void Vec2_fromAngle_1f(asIScriptGeneric* gen) {
    new(gen->GetAddressOfReturnLocation()) Vec2(Vec2::fromAngle(gen->GetArgFloat(0)));
}
static void Vec2_fromAngle_2f(asIScriptGeneric* gen) {
    new(gen->GetAddressOfReturnLocation()) Vec2(Vec2::fromAngle(gen->GetArgFloat(0), gen->GetArgFloat(1)));
}
// Operators
static void Vec2_OpAdd(asIScriptGeneric* gen) {
    Vec2* a = static_cast<Vec2*>(gen->GetObject());
    Vec2* b = static_cast<Vec2*>(gen->GetArgObject(0));
    new(gen->GetAddressOfReturnLocation()) Vec2(*a + *b);
}
static void Vec2_OpSub(asIScriptGeneric* gen) {
    Vec2* a = static_cast<Vec2*>(gen->GetObject());
    Vec2* b = static_cast<Vec2*>(gen->GetArgObject(0));
    new(gen->GetAddressOfReturnLocation()) Vec2(*a - *b);
}
static void Vec2_OpMulScalar(asIScriptGeneric* gen) {
    Vec2* a = static_cast<Vec2*>(gen->GetObject());
    float s = gen->GetArgFloat(0);
    new(gen->GetAddressOfReturnLocation()) Vec2(*a * s);
}
static void Vec2_OpDivScalar(asIScriptGeneric* gen) {
    Vec2* a = static_cast<Vec2*>(gen->GetObject());
    float s = gen->GetArgFloat(0);
    new(gen->GetAddressOfReturnLocation()) Vec2(*a / s);
}
static void Vec2_OpNeg(asIScriptGeneric* gen) {
    Vec2* a = static_cast<Vec2*>(gen->GetObject());
    new(gen->GetAddressOfReturnLocation()) Vec2(-*a);
}
static void Vec2_Limit(asIScriptGeneric* gen) {
    Vec2* self = static_cast<Vec2*>(gen->GetObject());
    float maxLen = gen->GetArgFloat(0);
    self->limit(maxLen);
    gen->SetReturnObject(self);
}
static void Vec2_Cross(asIScriptGeneric* gen) {
    Vec2* self = static_cast<Vec2*>(gen->GetObject());
    Vec2* other = static_cast<Vec2*>(gen->GetArgObject(0));
    gen->SetReturnFloat(self->cross(*other));
}
static void Vec2_DistanceSquared(asIScriptGeneric* gen) {
    Vec2* self = static_cast<Vec2*>(gen->GetObject());
    Vec2* other = static_cast<Vec2*>(gen->GetArgObject(0));
    gen->SetReturnFloat(self->distanceSquared(*other));
}
static void Vec2_Lerp(asIScriptGeneric* gen) {
    Vec2* self = static_cast<Vec2*>(gen->GetObject());
    Vec2* other = static_cast<Vec2*>(gen->GetArgObject(0));
    float t = gen->GetArgFloat(1);
    new(gen->GetAddressOfReturnLocation()) Vec2(self->lerp(*other, t));
}
static void Vec2_Perpendicular(asIScriptGeneric* gen) {
    Vec2* self = static_cast<Vec2*>(gen->GetObject());
    new(gen->GetAddressOfReturnLocation()) Vec2(self->perpendicular());
}
static void Vec2_Reflected(asIScriptGeneric* gen) {
    Vec2* self = static_cast<Vec2*>(gen->GetObject());
    Vec2* normal = static_cast<Vec2*>(gen->GetArgObject(0));
    new(gen->GetAddressOfReturnLocation()) Vec2(self->reflected(*normal));
}
static void Vec2_AngleWith(asIScriptGeneric* gen) {
    Vec2* self = static_cast<Vec2*>(gen->GetObject());
    Vec2* other = static_cast<Vec2*>(gen->GetArgObject(0));
    gen->SetReturnFloat(self->angle(*other));
}

// =============================================================================
// Vec3 type for AngelScript
// =============================================================================
static void Vec3_Construct(asIScriptGeneric* gen) {
    new(gen->GetObject()) Vec3();
}
static void Vec3_Construct_3f(asIScriptGeneric* gen) {
    new(gen->GetObject()) Vec3(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2));
}
static void Vec3_Construct_1f(asIScriptGeneric* gen) {
    float v = gen->GetArgFloat(0);
    new(gen->GetObject()) Vec3(v, v, v);
}
static void Vec3_CopyConstruct(asIScriptGeneric* gen) {
    Vec3* other = static_cast<Vec3*>(gen->GetArgObject(0));
    new(gen->GetObject()) Vec3(*other);
}
static void Vec3_Set(asIScriptGeneric* gen) {
    Vec3* self = static_cast<Vec3*>(gen->GetObject());
    self->x = gen->GetArgFloat(0);
    self->y = gen->GetArgFloat(1);
    self->z = gen->GetArgFloat(2);
    gen->SetReturnObject(self);
}
static void Vec3_Length(asIScriptGeneric* gen) {
    Vec3* self = static_cast<Vec3*>(gen->GetObject());
    gen->SetReturnFloat(self->length());
}
static void Vec3_LengthSquared(asIScriptGeneric* gen) {
    Vec3* self = static_cast<Vec3*>(gen->GetObject());
    gen->SetReturnFloat(self->lengthSquared());
}
static void Vec3_Normalize(asIScriptGeneric* gen) {
    Vec3* self = static_cast<Vec3*>(gen->GetObject());
    gen->SetReturnObject(&self->normalize());
}
static void Vec3_Normalized(asIScriptGeneric* gen) {
    Vec3* self = static_cast<Vec3*>(gen->GetObject());
    new(gen->GetAddressOfReturnLocation()) Vec3(self->normalized());
}
static void Vec3_Dot(asIScriptGeneric* gen) {
    Vec3* self = static_cast<Vec3*>(gen->GetObject());
    Vec3* other = static_cast<Vec3*>(gen->GetArgObject(0));
    gen->SetReturnFloat(self->dot(*other));
}
static void Vec3_Cross(asIScriptGeneric* gen) {
    Vec3* self = static_cast<Vec3*>(gen->GetObject());
    Vec3* other = static_cast<Vec3*>(gen->GetArgObject(0));
    new(gen->GetAddressOfReturnLocation()) Vec3(self->cross(*other));
}
// Operators
static void Vec3_OpAdd(asIScriptGeneric* gen) {
    Vec3* a = static_cast<Vec3*>(gen->GetObject());
    Vec3* b = static_cast<Vec3*>(gen->GetArgObject(0));
    new(gen->GetAddressOfReturnLocation()) Vec3(*a + *b);
}
static void Vec3_OpSub(asIScriptGeneric* gen) {
    Vec3* a = static_cast<Vec3*>(gen->GetObject());
    Vec3* b = static_cast<Vec3*>(gen->GetArgObject(0));
    new(gen->GetAddressOfReturnLocation()) Vec3(*a - *b);
}
static void Vec3_OpMulScalar(asIScriptGeneric* gen) {
    Vec3* a = static_cast<Vec3*>(gen->GetObject());
    float s = gen->GetArgFloat(0);
    new(gen->GetAddressOfReturnLocation()) Vec3(*a * s);
}
static void Vec3_OpDivScalar(asIScriptGeneric* gen) {
    Vec3* a = static_cast<Vec3*>(gen->GetObject());
    float s = gen->GetArgFloat(0);
    new(gen->GetAddressOfReturnLocation()) Vec3(*a / s);
}
static void Vec3_OpNeg(asIScriptGeneric* gen) {
    Vec3* a = static_cast<Vec3*>(gen->GetObject());
    new(gen->GetAddressOfReturnLocation()) Vec3(-*a);
}
static void Vec3_Limit(asIScriptGeneric* gen) {
    Vec3* self = static_cast<Vec3*>(gen->GetObject());
    float maxLen = gen->GetArgFloat(0);
    self->limit(maxLen);
    gen->SetReturnObject(self);
}
static void Vec3_Distance(asIScriptGeneric* gen) {
    Vec3* self = static_cast<Vec3*>(gen->GetObject());
    Vec3* other = static_cast<Vec3*>(gen->GetArgObject(0));
    gen->SetReturnFloat(self->distance(*other));
}
static void Vec3_DistanceSquared(asIScriptGeneric* gen) {
    Vec3* self = static_cast<Vec3*>(gen->GetObject());
    Vec3* other = static_cast<Vec3*>(gen->GetArgObject(0));
    gen->SetReturnFloat(self->distanceSquared(*other));
}
static void Vec3_Lerp(asIScriptGeneric* gen) {
    Vec3* self = static_cast<Vec3*>(gen->GetObject());
    Vec3* other = static_cast<Vec3*>(gen->GetArgObject(0));
    float t = gen->GetArgFloat(1);
    new(gen->GetAddressOfReturnLocation()) Vec3(self->lerp(*other, t));
}
static void Vec3_Reflected(asIScriptGeneric* gen) {
    Vec3* self = static_cast<Vec3*>(gen->GetObject());
    Vec3* normal = static_cast<Vec3*>(gen->GetArgObject(0));
    new(gen->GetAddressOfReturnLocation()) Vec3(self->reflected(*normal));
}
static void Vec3_XY(asIScriptGeneric* gen) {
    Vec3* self = static_cast<Vec3*>(gen->GetObject());
    new(gen->GetAddressOfReturnLocation()) Vec2(self->xy());
}

// =============================================================================
// Color type for AngelScript
// =============================================================================
static void Color_Construct(asIScriptGeneric* gen) {
    new(gen->GetObject()) Color();
}
static void Color_Construct_3f(asIScriptGeneric* gen) {
    new(gen->GetObject()) Color(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2));
}
static void Color_Construct_4f(asIScriptGeneric* gen) {
    new(gen->GetObject()) Color(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2), gen->GetArgFloat(3));
}
static void Color_Construct_1f(asIScriptGeneric* gen) {
    float v = gen->GetArgFloat(0);
    new(gen->GetObject()) Color(v, v, v);
}
static void Color_CopyConstruct(asIScriptGeneric* gen) {
    Color* other = static_cast<Color*>(gen->GetArgObject(0));
    new(gen->GetObject()) Color(*other);
}
static void Color_Set_3f(asIScriptGeneric* gen) {
    Color* self = static_cast<Color*>(gen->GetObject());
    self->r = gen->GetArgFloat(0);
    self->g = gen->GetArgFloat(1);
    self->b = gen->GetArgFloat(2);
    gen->SetReturnObject(self);
}
static void Color_Set_4f(asIScriptGeneric* gen) {
    Color* self = static_cast<Color*>(gen->GetObject());
    self->r = gen->GetArgFloat(0);
    self->g = gen->GetArgFloat(1);
    self->b = gen->GetArgFloat(2);
    self->a = gen->GetArgFloat(3);
    gen->SetReturnObject(self);
}
static void Color_Lerp(asIScriptGeneric* gen) {
    Color* self = static_cast<Color*>(gen->GetObject());
    Color* target = static_cast<Color*>(gen->GetArgObject(0));
    float t = gen->GetArgFloat(1);
    new(gen->GetAddressOfReturnLocation()) Color(self->lerp(*target, t));
}
static void Color_fromHSB_3f(asIScriptGeneric* gen) {
    new(gen->GetAddressOfReturnLocation()) Color(Color::fromHSB(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2)));
}
static void Color_fromHSB_4f(asIScriptGeneric* gen) {
    new(gen->GetAddressOfReturnLocation()) Color(Color::fromHSB(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2), gen->GetArgFloat(3)));
}
static void Color_fromOKLCH_3f(asIScriptGeneric* gen) {
    new(gen->GetAddressOfReturnLocation()) Color(Color::fromOKLCH(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2)));
}
static void Color_fromOKLCH_4f(asIScriptGeneric* gen) {
    new(gen->GetAddressOfReturnLocation()) Color(Color::fromOKLCH(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2), gen->GetArgFloat(3)));
}
static void Color_fromOKLab_3f(asIScriptGeneric* gen) {
    new(gen->GetAddressOfReturnLocation()) Color(Color::fromOKLab(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2)));
}
static void Color_fromOKLab_4f(asIScriptGeneric* gen) {
    new(gen->GetAddressOfReturnLocation()) Color(Color::fromOKLab(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2), gen->GetArgFloat(3)));
}
static void Color_ToHex_0(asIScriptGeneric* gen) {
    Color* self = static_cast<Color*>(gen->GetObject());
    gen->SetReturnDWord(self->toHex(false));
}
static void Color_ToHex_1b(asIScriptGeneric* gen) {
    Color* self = static_cast<Color*>(gen->GetObject());
    bool includeAlpha = gen->GetArgByte(0) != 0;
    gen->SetReturnDWord(self->toHex(includeAlpha));
}
static void Color_LerpRGB(asIScriptGeneric* gen) {
    Color* self = static_cast<Color*>(gen->GetObject());
    Color* target = static_cast<Color*>(gen->GetArgObject(0));
    float t = gen->GetArgFloat(1);
    new(gen->GetAddressOfReturnLocation()) Color(self->lerpRGB(*target, t));
}
static void Color_Clamped(asIScriptGeneric* gen) {
    Color* self = static_cast<Color*>(gen->GetObject());
    new(gen->GetAddressOfReturnLocation()) Color(self->clamped());
}

// =============================================================================
// Rect type for AngelScript
// =============================================================================
static void Rect_Construct(asIScriptGeneric* gen) {
    new(gen->GetObject()) Rect();
}
static void Rect_Construct_4f(asIScriptGeneric* gen) {
    new(gen->GetObject()) Rect(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2), gen->GetArgFloat(3));
}
static void Rect_CopyConstruct(asIScriptGeneric* gen) {
    Rect* other = static_cast<Rect*>(gen->GetArgObject(0));
    new(gen->GetObject()) Rect(*other);
}
static void Rect_Set(asIScriptGeneric* gen) {
    Rect* self = static_cast<Rect*>(gen->GetObject());
    self->x = gen->GetArgFloat(0);
    self->y = gen->GetArgFloat(1);
    self->width = gen->GetArgFloat(2);
    self->height = gen->GetArgFloat(3);
    gen->SetReturnObject(self);
}
static void Rect_Contains(asIScriptGeneric* gen) {
    Rect* self = static_cast<Rect*>(gen->GetObject());
    gen->SetReturnByte(self->contains(gen->GetArgFloat(0), gen->GetArgFloat(1)) ? 1 : 0);
}
static void Rect_Intersects(asIScriptGeneric* gen) {
    Rect* self = static_cast<Rect*>(gen->GetObject());
    Rect* other = static_cast<Rect*>(gen->GetArgObject(0));
    gen->SetReturnByte(self->intersects(*other) ? 1 : 0);
}
static void Rect_GetCenterX(asIScriptGeneric* gen) {
    Rect* self = static_cast<Rect*>(gen->GetObject());
    gen->SetReturnFloat(self->getCenterX());
}
static void Rect_GetCenterY(asIScriptGeneric* gen) {
    Rect* self = static_cast<Rect*>(gen->GetObject());
    gen->SetReturnFloat(self->getCenterY());
}
static void Rect_GetRight(asIScriptGeneric* gen) {
    Rect* self = static_cast<Rect*>(gen->GetObject());
    gen->SetReturnFloat(self->getRight());
}
static void Rect_GetBottom(asIScriptGeneric* gen) {
    Rect* self = static_cast<Rect*>(gen->GetObject());
    gen->SetReturnFloat(self->getBottom());
}

// =============================================================================
// Mat4 value type wrappers
// =============================================================================
static void Mat4_Construct(asIScriptGeneric* gen) {
    new (gen->GetAddressOfReturnLocation()) Mat4();
}
static void Mat4_CopyConstruct(asIScriptGeneric* gen) {
    Mat4* other = static_cast<Mat4*>(gen->GetArgObject(0));
    new (gen->GetAddressOfReturnLocation()) Mat4(*other);
}
static void Mat4_OpMul_Mat4(asIScriptGeneric* gen) {
    Mat4* self = static_cast<Mat4*>(gen->GetObject());
    Mat4* other = static_cast<Mat4*>(gen->GetArgObject(0));
    new (gen->GetAddressOfReturnLocation()) Mat4(*self * *other);
}
static void Mat4_OpMul_Vec3(asIScriptGeneric* gen) {
    Mat4* self = static_cast<Mat4*>(gen->GetObject());
    Vec3* v = static_cast<Vec3*>(gen->GetArgObject(0));
    new (gen->GetAddressOfReturnLocation()) Vec3(*self * *v);
}
static void Mat4_Transposed(asIScriptGeneric* gen) {
    Mat4* self = static_cast<Mat4*>(gen->GetObject());
    new (gen->GetAddressOfReturnLocation()) Mat4(self->transposed());
}
static void Mat4_Inverted(asIScriptGeneric* gen) {
    Mat4* self = static_cast<Mat4*>(gen->GetObject());
    new (gen->GetAddressOfReturnLocation()) Mat4(self->inverted());
}
// Static factory functions for Mat4
static void Mat4_Identity(asIScriptGeneric* gen) {
    new (gen->GetAddressOfReturnLocation()) Mat4(Mat4::identity());
}
static void Mat4_Translate_3f(asIScriptGeneric* gen) {
    new (gen->GetAddressOfReturnLocation()) Mat4(Mat4::translate(
        gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2)));
}
static void Mat4_Translate_Vec3(asIScriptGeneric* gen) {
    Vec3* v = static_cast<Vec3*>(gen->GetArgObject(0));
    new (gen->GetAddressOfReturnLocation()) Mat4(Mat4::translate(*v));
}
static void Mat4_RotateX(asIScriptGeneric* gen) {
    new (gen->GetAddressOfReturnLocation()) Mat4(Mat4::rotateX(gen->GetArgFloat(0)));
}
static void Mat4_RotateY(asIScriptGeneric* gen) {
    new (gen->GetAddressOfReturnLocation()) Mat4(Mat4::rotateY(gen->GetArgFloat(0)));
}
static void Mat4_RotateZ(asIScriptGeneric* gen) {
    new (gen->GetAddressOfReturnLocation()) Mat4(Mat4::rotateZ(gen->GetArgFloat(0)));
}
static void Mat4_Scale_1f(asIScriptGeneric* gen) {
    new (gen->GetAddressOfReturnLocation()) Mat4(Mat4::scale(gen->GetArgFloat(0)));
}
static void Mat4_Scale_3f(asIScriptGeneric* gen) {
    new (gen->GetAddressOfReturnLocation()) Mat4(Mat4::scale(
        gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2)));
}
static void Mat4_LookAt(asIScriptGeneric* gen) {
    Vec3* eye = static_cast<Vec3*>(gen->GetArgObject(0));
    Vec3* target = static_cast<Vec3*>(gen->GetArgObject(1));
    Vec3* up = static_cast<Vec3*>(gen->GetArgObject(2));
    new (gen->GetAddressOfReturnLocation()) Mat4(Mat4::lookAt(*eye, *target, *up));
}
static void Mat4_Ortho(asIScriptGeneric* gen) {
    new (gen->GetAddressOfReturnLocation()) Mat4(Mat4::ortho(
        gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2),
        gen->GetArgFloat(3), gen->GetArgFloat(4), gen->GetArgFloat(5)));
}
static void Mat4_Perspective(asIScriptGeneric* gen) {
    new (gen->GetAddressOfReturnLocation()) Mat4(Mat4::perspective(
        gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2), gen->GetArgFloat(3)));
}

// =============================================================================
// Quaternion value type wrappers
// =============================================================================
static void Quaternion_Construct(asIScriptGeneric* gen) {
    new (gen->GetAddressOfReturnLocation()) Quaternion();
}
static void Quaternion_Construct_4f(asIScriptGeneric* gen) {
    new (gen->GetAddressOfReturnLocation()) Quaternion(
        gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2), gen->GetArgFloat(3));
}
static void Quaternion_CopyConstruct(asIScriptGeneric* gen) {
    Quaternion* other = static_cast<Quaternion*>(gen->GetArgObject(0));
    new (gen->GetAddressOfReturnLocation()) Quaternion(*other);
}
static void Quaternion_OpMul(asIScriptGeneric* gen) {
    Quaternion* self = static_cast<Quaternion*>(gen->GetObject());
    Quaternion* other = static_cast<Quaternion*>(gen->GetArgObject(0));
    new (gen->GetAddressOfReturnLocation()) Quaternion(*self * *other);
}
static void Quaternion_Rotate(asIScriptGeneric* gen) {
    Quaternion* self = static_cast<Quaternion*>(gen->GetObject());
    Vec3* v = static_cast<Vec3*>(gen->GetArgObject(0));
    new (gen->GetAddressOfReturnLocation()) Vec3(self->rotate(*v));
}
static void Quaternion_ToEuler(asIScriptGeneric* gen) {
    Quaternion* self = static_cast<Quaternion*>(gen->GetObject());
    new (gen->GetAddressOfReturnLocation()) Vec3(self->toEuler());
}
static void Quaternion_ToMatrix(asIScriptGeneric* gen) {
    Quaternion* self = static_cast<Quaternion*>(gen->GetObject());
    new (gen->GetAddressOfReturnLocation()) Mat4(self->toMatrix());
}
static void Quaternion_Normalized(asIScriptGeneric* gen) {
    Quaternion* self = static_cast<Quaternion*>(gen->GetObject());
    new (gen->GetAddressOfReturnLocation()) Quaternion(self->normalized());
}
static void Quaternion_Length(asIScriptGeneric* gen) {
    Quaternion* self = static_cast<Quaternion*>(gen->GetObject());
    gen->SetReturnFloat(self->length());
}
static void Quaternion_Conjugate(asIScriptGeneric* gen) {
    Quaternion* self = static_cast<Quaternion*>(gen->GetObject());
    new (gen->GetAddressOfReturnLocation()) Quaternion(self->conjugate());
}
// Static factory functions for Quaternion
static void Quaternion_Identity(asIScriptGeneric* gen) {
    new (gen->GetAddressOfReturnLocation()) Quaternion(Quaternion::identity());
}
static void Quaternion_FromAxisAngle(asIScriptGeneric* gen) {
    Vec3* axis = static_cast<Vec3*>(gen->GetArgObject(0));
    float radians = gen->GetArgFloat(1);
    new (gen->GetAddressOfReturnLocation()) Quaternion(Quaternion::fromAxisAngle(*axis, radians));
}
static void Quaternion_FromEuler_3f(asIScriptGeneric* gen) {
    new (gen->GetAddressOfReturnLocation()) Quaternion(Quaternion::fromEuler(
        gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2)));
}
static void Quaternion_FromEuler_Vec3(asIScriptGeneric* gen) {
    Vec3* euler = static_cast<Vec3*>(gen->GetArgObject(0));
    new (gen->GetAddressOfReturnLocation()) Quaternion(Quaternion::fromEuler(*euler));
}
static void Quaternion_Slerp(asIScriptGeneric* gen) {
    Quaternion* a = static_cast<Quaternion*>(gen->GetArgObject(0));
    Quaternion* b = static_cast<Quaternion*>(gen->GetArgObject(1));
    float t = gen->GetArgFloat(2);
    new (gen->GetAddressOfReturnLocation()) Quaternion(Quaternion::slerp(*a, *b, t));
}

// =============================================================================
// Window functions
// =============================================================================
static void as_setWindowTitle(asIScriptGeneric* gen) {
    string* title = static_cast<string*>(gen->GetArgObject(0));
    setWindowTitle(*title);
}
static void as_setWindowSize(asIScriptGeneric* gen) {
    setWindowSize(gen->GetArgDWord(0), gen->GetArgDWord(1));
}
static void as_getWindowSize(asIScriptGeneric* gen) {
    new (gen->GetAddressOfReturnLocation()) Vec2(getWindowSize());
}
static void as_getMousePos(asIScriptGeneric* gen) {
    new (gen->GetAddressOfReturnLocation()) Vec2(getMousePos());
}
static void as_getGlobalMousePos(asIScriptGeneric* gen) {
    new (gen->GetAddressOfReturnLocation()) Vec2(getGlobalMousePos());
}

// Transform matrix functions
static void as_getCurrentMatrix(asIScriptGeneric* gen) {
    new (gen->GetAddressOfReturnLocation()) Mat4(getCurrentMatrix());
}
static void as_setMatrix(asIScriptGeneric* gen) {
    Mat4* mat = static_cast<Mat4*>(gen->GetArgObject(0));
    setMatrix(*mat);
}

// Text alignment functions
static void as_setTextAlign(asIScriptGeneric* gen) {
    int h = gen->GetArgDWord(0);
    int v = gen->GetArgDWord(1);
    setTextAlign(static_cast<Direction>(h), static_cast<Direction>(v));
}
static void as_getTextAlignH(asIScriptGeneric* gen) {
    gen->SetReturnDWord(static_cast<int>(getTextAlignH()));
}
static void as_getTextAlignV(asIScriptGeneric* gen) {
    gen->SetReturnDWord(static_cast<int>(getTextAlignV()));
}
static void as_getBitmapFontHeight(asIScriptGeneric* gen) {
    gen->SetReturnFloat(getBitmapFontHeight());
}
static void as_getBitmapStringWidth(asIScriptGeneric* gen) {
    string* text = static_cast<string*>(gen->GetArgObject(0));
    gen->SetReturnFloat(getBitmapStringWidth(*text));
}
static void as_getBitmapStringHeight(asIScriptGeneric* gen) {
    string* text = static_cast<string*>(gen->GetArgObject(0));
    gen->SetReturnFloat(getBitmapStringHeight(*text));
}
static void as_getBitmapStringBBox(asIScriptGeneric* gen) {
    string* text = static_cast<string*>(gen->GetArgObject(0));
    new (gen->GetAddressOfReturnLocation()) Rect(getBitmapStringBBox(*text));
}

// Graphics advanced functions
static void as_drawMesh(asIScriptGeneric* gen) {
    Mesh* mesh = static_cast<Mesh*>(gen->GetArgObject(0));
    mesh->draw();
}
static void as_drawPolyline(asIScriptGeneric* gen) {
    Path* path = static_cast<Path*>(gen->GetArgObject(0));
    path->draw();
}
static void as_drawTexture_3f(asIScriptGeneric* gen) {
    Texture* tex = static_cast<Texture*>(gen->GetArgObject(0));
    tex->draw(gen->GetArgFloat(1), gen->GetArgFloat(2));
}
static void as_drawTexture_5f(asIScriptGeneric* gen) {
    Texture* tex = static_cast<Texture*>(gen->GetArgObject(0));
    tex->draw(gen->GetArgFloat(1), gen->GetArgFloat(2), gen->GetArgFloat(3), gen->GetArgFloat(4));
}
static void as_createBox_1f(asIScriptGeneric* gen) {
    g_meshes.push_back(make_unique<Mesh>(createBox(gen->GetArgFloat(0))));
    gen->SetReturnObject(g_meshes.back().get());
}
static void as_createBox_3f(asIScriptGeneric* gen) {
    g_meshes.push_back(make_unique<Mesh>(createBox(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2))));
    gen->SetReturnObject(g_meshes.back().get());
}
static void as_createSphere_1f(asIScriptGeneric* gen) {
    g_meshes.push_back(make_unique<Mesh>(createSphere(gen->GetArgFloat(0))));
    gen->SetReturnObject(g_meshes.back().get());
}
static void as_createSphere_2(asIScriptGeneric* gen) {
    g_meshes.push_back(make_unique<Mesh>(createSphere(gen->GetArgFloat(0), gen->GetArgDWord(1))));
    gen->SetReturnObject(g_meshes.back().get());
}

// Vec2 static factory functions
static void Vec2_FromAngle_1f(asIScriptGeneric* gen) {
    new (gen->GetAddressOfReturnLocation()) Vec2(Vec2::fromAngle(gen->GetArgFloat(0)));
}
static void Vec2_FromAngle_2f(asIScriptGeneric* gen) {
    new (gen->GetAddressOfReturnLocation()) Vec2(Vec2::fromAngle(gen->GetArgFloat(0), gen->GetArgFloat(1)));
}

// Color static factory functions
static void Color_FromHex_1u(asIScriptGeneric* gen) {
    new (gen->GetAddressOfReturnLocation()) Color(Color::fromHex(gen->GetArgDWord(0)));
}
static void Color_FromHex_1u1b(asIScriptGeneric* gen) {
    new (gen->GetAddressOfReturnLocation()) Color(Color::fromHex(gen->GetArgDWord(0), gen->GetArgByte(1) != 0));
}
static void Color_FromBytes_3i(asIScriptGeneric* gen) {
    new (gen->GetAddressOfReturnLocation()) Color(Color::fromBytes(gen->GetArgDWord(0), gen->GetArgDWord(1), gen->GetArgDWord(2)));
}
static void Color_FromBytes_4i(asIScriptGeneric* gen) {
    new (gen->GetAddressOfReturnLocation()) Color(Color::fromBytes(gen->GetArgDWord(0), gen->GetArgDWord(1), gen->GetArgDWord(2), gen->GetArgDWord(3)));
}

// =============================================================================
// Texture type for AngelScript (reference type)
// =============================================================================
static void Texture_Factory(asIScriptGeneric* gen) {
    g_textures.push_back(make_unique<Texture>());
    gen->SetReturnObject(g_textures.back().get());
}
static void Texture_AddRef(asIScriptGeneric*) { /* no-op, managed by global container */ }
static void Texture_Release(asIScriptGeneric*) { /* no-op, cleaned up on script reload */ }

static void Texture_Allocate_2i(asIScriptGeneric* gen) {
    Texture* self = static_cast<Texture*>(gen->GetObject());
    self->allocate(gen->GetArgDWord(0), gen->GetArgDWord(1));
}
static void Texture_Allocate_Pixels(asIScriptGeneric* gen) {
    Texture* self = static_cast<Texture*>(gen->GetObject());
    Pixels* pixels = static_cast<Pixels*>(gen->GetArgObject(0));
    self->allocate(*pixels);
}
static void Texture_Bind(asIScriptGeneric* gen) {
    Texture* self = static_cast<Texture*>(gen->GetObject());
    self->bind();
}
static void Texture_Unbind(asIScriptGeneric* gen) {
    Texture* self = static_cast<Texture*>(gen->GetObject());
    self->unbind();
}
static void Texture_GetWidth(asIScriptGeneric* gen) {
    Texture* self = static_cast<Texture*>(gen->GetObject());
    gen->SetReturnDWord(self->getWidth());
}
static void Texture_GetHeight(asIScriptGeneric* gen) {
    Texture* self = static_cast<Texture*>(gen->GetObject());
    gen->SetReturnDWord(self->getHeight());
}
static void Texture_IsAllocated(asIScriptGeneric* gen) {
    Texture* self = static_cast<Texture*>(gen->GetObject());
    gen->SetReturnByte(self->isAllocated() ? 1 : 0);
}
static void Texture_Draw_2f(asIScriptGeneric* gen) {
    Texture* self = static_cast<Texture*>(gen->GetObject());
    self->draw(gen->GetArgFloat(0), gen->GetArgFloat(1));
}
static void Texture_Draw_4f(asIScriptGeneric* gen) {
    Texture* self = static_cast<Texture*>(gen->GetObject());
    self->draw(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2), gen->GetArgFloat(3));
}
static void Texture_LoadData(asIScriptGeneric* gen) {
    Texture* self = static_cast<Texture*>(gen->GetObject());
    Pixels* pixels = static_cast<Pixels*>(gen->GetArgObject(0));
    self->loadData(*pixels);
}

// =============================================================================
// Fbo type for AngelScript (reference type)
// =============================================================================
static void Fbo_Factory(asIScriptGeneric* gen) {
    g_fbos.push_back(make_unique<Fbo>());
    gen->SetReturnObject(g_fbos.back().get());
}
static void Fbo_AddRef(asIScriptGeneric*) { /* no-op */ }
static void Fbo_Release(asIScriptGeneric*) { /* no-op */ }

static void Fbo_Allocate_2i(asIScriptGeneric* gen) {
    Fbo* self = static_cast<Fbo*>(gen->GetObject());
    self->allocate(gen->GetArgDWord(0), gen->GetArgDWord(1));
}
static void Fbo_Begin(asIScriptGeneric* gen) {
    Fbo* self = static_cast<Fbo*>(gen->GetObject());
    self->begin();
}
static void Fbo_Begin_4f(asIScriptGeneric* gen) {
    Fbo* self = static_cast<Fbo*>(gen->GetObject());
    self->begin(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2), gen->GetArgFloat(3));
}
static void Fbo_End(asIScriptGeneric* gen) {
    Fbo* self = static_cast<Fbo*>(gen->GetObject());
    self->end();
}
static void Fbo_GetTexture(asIScriptGeneric* gen) {
    Fbo* self = static_cast<Fbo*>(gen->GetObject());
    gen->SetReturnObject(&self->getTexture());
}
static void Fbo_GetWidth(asIScriptGeneric* gen) {
    Fbo* self = static_cast<Fbo*>(gen->GetObject());
    gen->SetReturnDWord(self->getWidth());
}
static void Fbo_GetHeight(asIScriptGeneric* gen) {
    Fbo* self = static_cast<Fbo*>(gen->GetObject());
    gen->SetReturnDWord(self->getHeight());
}
static void Fbo_IsAllocated(asIScriptGeneric* gen) {
    Fbo* self = static_cast<Fbo*>(gen->GetObject());
    gen->SetReturnByte(self->isAllocated() ? 1 : 0);
}
static void Fbo_Draw_2f(asIScriptGeneric* gen) {
    Fbo* self = static_cast<Fbo*>(gen->GetObject());
    self->draw(gen->GetArgFloat(0), gen->GetArgFloat(1));
}
static void Fbo_Draw_4f(asIScriptGeneric* gen) {
    Fbo* self = static_cast<Fbo*>(gen->GetObject());
    self->draw(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2), gen->GetArgFloat(3));
}

// =============================================================================
// Mesh type for AngelScript (reference type)
// =============================================================================
static void Mesh_Factory(asIScriptGeneric* gen) {
    g_meshes.push_back(make_unique<Mesh>());
    gen->SetReturnObject(g_meshes.back().get());
}
static void Mesh_AddRef(asIScriptGeneric*) { /* no-op */ }
static void Mesh_Release(asIScriptGeneric*) { /* no-op */ }

static void Mesh_SetMode(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    self->setMode(static_cast<PrimitiveMode>(gen->GetArgDWord(0)));
    gen->SetReturnObject(self);
}
static void Mesh_GetMode(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    gen->SetReturnDWord(static_cast<int>(self->getMode()));
}
static void Mesh_AddVertex_3f(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    self->addVertex(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2));
    gen->SetReturnObject(self);
}
static void Mesh_AddVertex_2f(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    self->addVertex(gen->GetArgFloat(0), gen->GetArgFloat(1), 0.0f);
    gen->SetReturnObject(self);
}
static void Mesh_AddVertex_Vec3(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    Vec3* v = static_cast<Vec3*>(gen->GetArgObject(0));
    self->addVertex(*v);
    gen->SetReturnObject(self);
}
static void Mesh_AddVertex_Vec2(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    Vec2* v = static_cast<Vec2*>(gen->GetArgObject(0));
    self->addVertex(*v);
    gen->SetReturnObject(self);
}
static void Mesh_AddColor_Color(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    Color* c = static_cast<Color*>(gen->GetArgObject(0));
    self->addColor(*c);
    gen->SetReturnObject(self);
}
static void Mesh_AddColor_4f(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    self->addColor(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2), gen->GetArgFloat(3));
    gen->SetReturnObject(self);
}
static void Mesh_AddColor_3f(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    self->addColor(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2), 1.0f);
    gen->SetReturnObject(self);
}
static void Mesh_AddTexCoord_2f(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    self->addTexCoord(gen->GetArgFloat(0), gen->GetArgFloat(1));
    gen->SetReturnObject(self);
}
static void Mesh_AddTexCoord_Vec2(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    Vec2* t = static_cast<Vec2*>(gen->GetArgObject(0));
    self->addTexCoord(*t);
    gen->SetReturnObject(self);
}
static void Mesh_AddNormal_3f(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    self->addNormal(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2));
    gen->SetReturnObject(self);
}
static void Mesh_AddNormal_Vec3(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    Vec3* n = static_cast<Vec3*>(gen->GetArgObject(0));
    self->addNormal(*n);
    gen->SetReturnObject(self);
}
static void Mesh_AddIndex(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    self->addIndex(gen->GetArgDWord(0));
    gen->SetReturnObject(self);
}
static void Mesh_AddTriangle(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    self->addTriangle(gen->GetArgDWord(0), gen->GetArgDWord(1), gen->GetArgDWord(2));
    gen->SetReturnObject(self);
}
static void Mesh_Clear(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    self->clear();
    gen->SetReturnObject(self);
}
static void Mesh_Draw(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    self->draw();
}
static void Mesh_DrawWireframe(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    self->drawWireframe();
}
static void Mesh_GetNumVertices(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    gen->SetReturnDWord(self->getNumVertices());
}
static void Mesh_GetNumIndices(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    gen->SetReturnDWord(self->getNumIndices());
}
static void Mesh_GetNumColors(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    gen->SetReturnDWord(self->getNumColors());
}
static void Mesh_GetNumNormals(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    gen->SetReturnDWord(self->getNumNormals());
}
static void Mesh_HasColors(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    gen->SetReturnByte(self->hasColors() ? 1 : 0);
}
static void Mesh_HasIndices(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    gen->SetReturnByte(self->hasIndices() ? 1 : 0);
}
static void Mesh_HasNormals(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    gen->SetReturnByte(self->hasNormals() ? 1 : 0);
}
static void Mesh_HasTexCoords(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    gen->SetReturnByte(self->hasTexCoords() ? 1 : 0);
}
static void Mesh_Translate_3f(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    self->translate(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2));
    gen->SetReturnObject(self);
}
static void Mesh_Translate_Vec3(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    Vec3* v = static_cast<Vec3*>(gen->GetArgObject(0));
    self->translate(*v);
    gen->SetReturnObject(self);
}
static void Mesh_RotateX(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    self->rotateX(gen->GetArgFloat(0));
    gen->SetReturnObject(self);
}
static void Mesh_RotateY(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    self->rotateY(gen->GetArgFloat(0));
    gen->SetReturnObject(self);
}
static void Mesh_RotateZ(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    self->rotateZ(gen->GetArgFloat(0));
    gen->SetReturnObject(self);
}
static void Mesh_Scale_1f(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    self->scale(gen->GetArgFloat(0));
    gen->SetReturnObject(self);
}
static void Mesh_Scale_3f(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    self->scale(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2));
    gen->SetReturnObject(self);
}
static void Mesh_AddVertices_Vec3Array(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    CScriptArray* arr = static_cast<CScriptArray*>(gen->GetArgObject(0));
    for (asUINT i = 0; i < arr->GetSize(); i++) {
        Vec3* v = static_cast<Vec3*>(arr->At(i));
        self->addVertex(*v);
    }
    gen->SetReturnObject(self);
}
static void Mesh_AddVertices_Vec2Array(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    CScriptArray* arr = static_cast<CScriptArray*>(gen->GetArgObject(0));
    for (asUINT i = 0; i < arr->GetSize(); i++) {
        Vec2* v = static_cast<Vec2*>(arr->At(i));
        self->addVertex(v->x, v->y, 0);
    }
    gen->SetReturnObject(self);
}
static void Mesh_AddColors_Array(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    CScriptArray* arr = static_cast<CScriptArray*>(gen->GetArgObject(0));
    for (asUINT i = 0; i < arr->GetSize(); i++) {
        Color* c = static_cast<Color*>(arr->At(i));
        self->addColor(*c);
    }
    gen->SetReturnObject(self);
}
static void Mesh_AddIndices_Array(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    CScriptArray* arr = static_cast<CScriptArray*>(gen->GetArgObject(0));
    for (asUINT i = 0; i < arr->GetSize(); i++) {
        uint32_t* idx = static_cast<uint32_t*>(arr->At(i));
        self->addIndex(*idx);
    }
    gen->SetReturnObject(self);
}
static void Mesh_AddNormals_Array(asIScriptGeneric* gen) {
    Mesh* self = static_cast<Mesh*>(gen->GetObject());
    CScriptArray* arr = static_cast<CScriptArray*>(gen->GetArgObject(0));
    for (asUINT i = 0; i < arr->GetSize(); i++) {
        Vec3* n = static_cast<Vec3*>(arr->At(i));
        self->addNormal(*n);
    }
    gen->SetReturnObject(self);
}

// =============================================================================
// Path (Polyline) type for AngelScript (reference type)
// =============================================================================
static void Path_Factory(asIScriptGeneric* gen) {
    g_paths.push_back(make_unique<Path>());
    gen->SetReturnObject(g_paths.back().get());
}
static void Path_AddRef(asIScriptGeneric*) { /* no-op */ }
static void Path_Release(asIScriptGeneric*) { /* no-op */ }

static void Path_AddVertex_2f(asIScriptGeneric* gen) {
    Path* self = static_cast<Path*>(gen->GetObject());
    self->addVertex(gen->GetArgFloat(0), gen->GetArgFloat(1));
    gen->SetReturnObject(self);
}
static void Path_AddVertex_3f(asIScriptGeneric* gen) {
    Path* self = static_cast<Path*>(gen->GetObject());
    self->addVertex(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2));
    gen->SetReturnObject(self);
}
static void Path_AddVertex_Vec2(asIScriptGeneric* gen) {
    Path* self = static_cast<Path*>(gen->GetObject());
    Vec2* v = static_cast<Vec2*>(gen->GetArgObject(0));
    self->addVertex(*v);
    gen->SetReturnObject(self);
}
static void Path_AddVertex_Vec3(asIScriptGeneric* gen) {
    Path* self = static_cast<Path*>(gen->GetObject());
    Vec3* v = static_cast<Vec3*>(gen->GetArgObject(0));
    self->addVertex(*v);
    gen->SetReturnObject(self);
}
static void Path_LineTo_2f(asIScriptGeneric* gen) {
    Path* self = static_cast<Path*>(gen->GetObject());
    self->lineTo(gen->GetArgFloat(0), gen->GetArgFloat(1));
    gen->SetReturnObject(self);
}
static void Path_LineTo_Vec2(asIScriptGeneric* gen) {
    Path* self = static_cast<Path*>(gen->GetObject());
    Vec2* v = static_cast<Vec2*>(gen->GetArgObject(0));
    self->lineTo(*v);
    gen->SetReturnObject(self);
}
static void Path_BezierTo_6f(asIScriptGeneric* gen) {
    Path* self = static_cast<Path*>(gen->GetObject());
    self->bezierTo(gen->GetArgFloat(0), gen->GetArgFloat(1),
                   gen->GetArgFloat(2), gen->GetArgFloat(3),
                   gen->GetArgFloat(4), gen->GetArgFloat(5));
    gen->SetReturnObject(self);
}
static void Path_QuadBezierTo_4f(asIScriptGeneric* gen) {
    Path* self = static_cast<Path*>(gen->GetObject());
    self->quadBezierTo(gen->GetArgFloat(0), gen->GetArgFloat(1),
                       gen->GetArgFloat(2), gen->GetArgFloat(3));
    gen->SetReturnObject(self);
}
static void Path_CurveTo_2f(asIScriptGeneric* gen) {
    Path* self = static_cast<Path*>(gen->GetObject());
    self->curveTo(gen->GetArgFloat(0), gen->GetArgFloat(1));
    gen->SetReturnObject(self);
}
static void Path_CurveTo_3f(asIScriptGeneric* gen) {
    Path* self = static_cast<Path*>(gen->GetObject());
    self->curveTo(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2));
    gen->SetReturnObject(self);
}
static void Path_Arc_6f(asIScriptGeneric* gen) {
    Path* self = static_cast<Path*>(gen->GetObject());
    self->arc(gen->GetArgFloat(0), gen->GetArgFloat(1),
              gen->GetArgFloat(2), gen->GetArgFloat(3),
              gen->GetArgFloat(4), gen->GetArgFloat(5));
    gen->SetReturnObject(self);
}
static void Path_Close(asIScriptGeneric* gen) {
    Path* self = static_cast<Path*>(gen->GetObject());
    self->close();
    gen->SetReturnObject(self);
}
static void Path_SetClosed(asIScriptGeneric* gen) {
    Path* self = static_cast<Path*>(gen->GetObject());
    self->setClosed(gen->GetArgByte(0) != 0);
    gen->SetReturnObject(self);
}
static void Path_IsClosed(asIScriptGeneric* gen) {
    Path* self = static_cast<Path*>(gen->GetObject());
    gen->SetReturnByte(self->isClosed() ? 1 : 0);
}
static void Path_Clear(asIScriptGeneric* gen) {
    Path* self = static_cast<Path*>(gen->GetObject());
    self->clear();
    gen->SetReturnObject(self);
}
static void Path_Draw(asIScriptGeneric* gen) {
    Path* self = static_cast<Path*>(gen->GetObject());
    self->draw();
}
static void Path_Size(asIScriptGeneric* gen) {
    Path* self = static_cast<Path*>(gen->GetObject());
    gen->SetReturnDWord(self->size());
}
static void Path_Empty(asIScriptGeneric* gen) {
    Path* self = static_cast<Path*>(gen->GetObject());
    gen->SetReturnByte(self->empty() ? 1 : 0);
}
static void Path_GetPerimeter(asIScriptGeneric* gen) {
    Path* self = static_cast<Path*>(gen->GetObject());
    gen->SetReturnFloat(self->getPerimeter());
}
static void Path_GetBounds(asIScriptGeneric* gen) {
    Path* self = static_cast<Path*>(gen->GetObject());
    Rect r = self->getBounds();
    new(gen->GetAddressOfReturnLocation()) Rect(r);
}
static void Path_AddVertices_Vec3Array(asIScriptGeneric* gen) {
    Path* self = static_cast<Path*>(gen->GetObject());
    CScriptArray* arr = static_cast<CScriptArray*>(gen->GetArgObject(0));
    for (asUINT i = 0; i < arr->GetSize(); i++) {
        Vec3* v = static_cast<Vec3*>(arr->At(i));
        self->addVertex(*v);
    }
    gen->SetReturnObject(self);
}
static void Path_AddVertices_Vec2Array(asIScriptGeneric* gen) {
    Path* self = static_cast<Path*>(gen->GetObject());
    CScriptArray* arr = static_cast<CScriptArray*>(gen->GetArgObject(0));
    for (asUINT i = 0; i < arr->GetSize(); i++) {
        Vec2* v = static_cast<Vec2*>(arr->At(i));
        self->addVertex(*v);
    }
    gen->SetReturnObject(self);
}

// =============================================================================
// StrokeMesh type for AngelScript (reference type)
// =============================================================================
static void StrokeMesh_Factory(asIScriptGeneric* gen) {
    g_strokeMeshes.push_back(make_unique<StrokeMesh>());
    gen->SetReturnObject(g_strokeMeshes.back().get());
}
static void StrokeMesh_AddRef(asIScriptGeneric*) { /* no-op */ }
static void StrokeMesh_Release(asIScriptGeneric*) { /* no-op */ }

static void StrokeMesh_SetWidth(asIScriptGeneric* gen) {
    StrokeMesh* self = static_cast<StrokeMesh*>(gen->GetObject());
    self->setWidth(gen->GetArgFloat(0));
    gen->SetReturnObject(self);
}
static void StrokeMesh_SetColor(asIScriptGeneric* gen) {
    StrokeMesh* self = static_cast<StrokeMesh*>(gen->GetObject());
    Color* c = static_cast<Color*>(gen->GetArgObject(0));
    self->setColor(*c);
    gen->SetReturnObject(self);
}
static void StrokeMesh_SetCapType(asIScriptGeneric* gen) {
    StrokeMesh* self = static_cast<StrokeMesh*>(gen->GetObject());
    int type = gen->GetArgDWord(0);
    self->setCapType(static_cast<StrokeMesh::CapType>(type));
    gen->SetReturnObject(self);
}
static void StrokeMesh_SetJoinType(asIScriptGeneric* gen) {
    StrokeMesh* self = static_cast<StrokeMesh*>(gen->GetObject());
    int type = gen->GetArgDWord(0);
    self->setJoinType(static_cast<StrokeMesh::JoinType>(type));
    gen->SetReturnObject(self);
}
static void StrokeMesh_SetMiterLimit(asIScriptGeneric* gen) {
    StrokeMesh* self = static_cast<StrokeMesh*>(gen->GetObject());
    self->setMiterLimit(gen->GetArgFloat(0));
    gen->SetReturnObject(self);
}
static void StrokeMesh_AddVertex_2f(asIScriptGeneric* gen) {
    StrokeMesh* self = static_cast<StrokeMesh*>(gen->GetObject());
    self->addVertex(gen->GetArgFloat(0), gen->GetArgFloat(1));
    gen->SetReturnObject(self);
}
static void StrokeMesh_AddVertex_3f(asIScriptGeneric* gen) {
    StrokeMesh* self = static_cast<StrokeMesh*>(gen->GetObject());
    self->addVertex(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2));
    gen->SetReturnObject(self);
}
static void StrokeMesh_AddVertex_Vec2(asIScriptGeneric* gen) {
    StrokeMesh* self = static_cast<StrokeMesh*>(gen->GetObject());
    Vec2* v = static_cast<Vec2*>(gen->GetArgObject(0));
    self->addVertex(*v);
    gen->SetReturnObject(self);
}
static void StrokeMesh_AddVertex_Vec3(asIScriptGeneric* gen) {
    StrokeMesh* self = static_cast<StrokeMesh*>(gen->GetObject());
    Vec3* v = static_cast<Vec3*>(gen->GetArgObject(0));
    self->addVertex(*v);
    gen->SetReturnObject(self);
}
static void StrokeMesh_AddVertexWithWidth_3f(asIScriptGeneric* gen) {
    StrokeMesh* self = static_cast<StrokeMesh*>(gen->GetObject());
    self->addVertexWithWidth(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2));
    gen->SetReturnObject(self);
}
static void StrokeMesh_SetShape(asIScriptGeneric* gen) {
    StrokeMesh* self = static_cast<StrokeMesh*>(gen->GetObject());
    Path* path = static_cast<Path*>(gen->GetArgObject(0));
    self->setShape(*path);
    gen->SetReturnObject(self);
}
static void StrokeMesh_SetClosed(asIScriptGeneric* gen) {
    StrokeMesh* self = static_cast<StrokeMesh*>(gen->GetObject());
    self->setClosed(gen->GetArgByte(0) != 0);
    gen->SetReturnObject(self);
}
static void StrokeMesh_Clear(asIScriptGeneric* gen) {
    StrokeMesh* self = static_cast<StrokeMesh*>(gen->GetObject());
    self->clear();
    gen->SetReturnObject(self);
}
static void StrokeMesh_Update(asIScriptGeneric* gen) {
    StrokeMesh* self = static_cast<StrokeMesh*>(gen->GetObject());
    self->update();
}
static void StrokeMesh_Draw(asIScriptGeneric* gen) {
    StrokeMesh* self = static_cast<StrokeMesh*>(gen->GetObject());
    self->draw();
}

// =============================================================================
// Image type for AngelScript (reference type)
// =============================================================================
static void Image_Factory(asIScriptGeneric* gen) {
    g_images.push_back(make_unique<Image>());
    gen->SetReturnObject(g_images.back().get());
}
static void Image_AddRef(asIScriptGeneric*) { /* no-op */ }
static void Image_Release(asIScriptGeneric*) { /* no-op */ }

static void Image_Load(asIScriptGeneric* gen) {
    Image* self = static_cast<Image*>(gen->GetObject());
    string* path = static_cast<string*>(gen->GetArgObject(0));
    gen->SetReturnByte(self->load(*path) ? 1 : 0);
}
static void Image_Save(asIScriptGeneric* gen) {
    Image* self = static_cast<Image*>(gen->GetObject());
    string* path = static_cast<string*>(gen->GetArgObject(0));
    gen->SetReturnByte(self->save(*path) ? 1 : 0);
}
static void Image_Allocate_2i(asIScriptGeneric* gen) {
    Image* self = static_cast<Image*>(gen->GetObject());
    self->allocate(gen->GetArgDWord(0), gen->GetArgDWord(1));
}
static void Image_Allocate_3i(asIScriptGeneric* gen) {
    Image* self = static_cast<Image*>(gen->GetObject());
    self->allocate(gen->GetArgDWord(0), gen->GetArgDWord(1), gen->GetArgDWord(2));
}
static void Image_Clear(asIScriptGeneric* gen) {
    Image* self = static_cast<Image*>(gen->GetObject());
    self->clear();
}
static void Image_IsAllocated(asIScriptGeneric* gen) {
    Image* self = static_cast<Image*>(gen->GetObject());
    gen->SetReturnByte(self->isAllocated() ? 1 : 0);
}
static void Image_GetWidth(asIScriptGeneric* gen) {
    Image* self = static_cast<Image*>(gen->GetObject());
    gen->SetReturnDWord(self->getWidth());
}
static void Image_GetHeight(asIScriptGeneric* gen) {
    Image* self = static_cast<Image*>(gen->GetObject());
    gen->SetReturnDWord(self->getHeight());
}
static void Image_GetChannels(asIScriptGeneric* gen) {
    Image* self = static_cast<Image*>(gen->GetObject());
    gen->SetReturnDWord(self->getChannels());
}
static void Image_GetPixels(asIScriptGeneric* gen) {
    Image* self = static_cast<Image*>(gen->GetObject());
    gen->SetReturnObject(&self->getPixels());
}
static void Image_GetColor(asIScriptGeneric* gen) {
    Image* self = static_cast<Image*>(gen->GetObject());
    Color c = self->getColor(gen->GetArgDWord(0), gen->GetArgDWord(1));
    new(gen->GetAddressOfReturnLocation()) Color(c);
}
static void Image_SetColor(asIScriptGeneric* gen) {
    Image* self = static_cast<Image*>(gen->GetObject());
    Color* c = static_cast<Color*>(gen->GetArgObject(2));
    self->setColor(gen->GetArgDWord(0), gen->GetArgDWord(1), *c);
}
static void Image_Update(asIScriptGeneric* gen) {
    Image* self = static_cast<Image*>(gen->GetObject());
    self->update();
}
static void Image_SetDirty(asIScriptGeneric* gen) {
    Image* self = static_cast<Image*>(gen->GetObject());
    self->setDirty();
}
static void Image_GetTexture(asIScriptGeneric* gen) {
    Image* self = static_cast<Image*>(gen->GetObject());
    gen->SetReturnObject(&self->getTexture());
}
static void Image_Draw_0(asIScriptGeneric* gen) {
    Image* self = static_cast<Image*>(gen->GetObject());
    self->draw(0, 0);
}
static void Image_Draw_2f(asIScriptGeneric* gen) {
    Image* self = static_cast<Image*>(gen->GetObject());
    self->draw(gen->GetArgFloat(0), gen->GetArgFloat(1));
}
static void Image_Draw_4f(asIScriptGeneric* gen) {
    Image* self = static_cast<Image*>(gen->GetObject());
    self->draw(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2), gen->GetArgFloat(3));
}

// =============================================================================
// EasyCam type for AngelScript (reference type)
// =============================================================================
static void EasyCam_Factory(asIScriptGeneric* gen) {
    g_easyCams.push_back(make_unique<EasyCam>());
    gen->SetReturnObject(g_easyCams.back().get());
}
static void EasyCam_AddRef(asIScriptGeneric*) { /* no-op */ }
static void EasyCam_Release(asIScriptGeneric*) { /* no-op */ }

static void EasyCam_Begin(asIScriptGeneric* gen) {
    EasyCam* self = static_cast<EasyCam*>(gen->GetObject());
    self->begin();
}
static void EasyCam_End(asIScriptGeneric* gen) {
    EasyCam* self = static_cast<EasyCam*>(gen->GetObject());
    self->end();
}
static void EasyCam_Reset(asIScriptGeneric* gen) {
    EasyCam* self = static_cast<EasyCam*>(gen->GetObject());
    self->reset();
}
static void EasyCam_SetTarget_3f(asIScriptGeneric* gen) {
    EasyCam* self = static_cast<EasyCam*>(gen->GetObject());
    self->setTarget(gen->GetArgFloat(0), gen->GetArgFloat(1), gen->GetArgFloat(2));
}
static void EasyCam_SetTarget_Vec3(asIScriptGeneric* gen) {
    EasyCam* self = static_cast<EasyCam*>(gen->GetObject());
    Vec3* v = static_cast<Vec3*>(gen->GetArgObject(0));
    self->setTarget(*v);
}
static void EasyCam_GetTarget(asIScriptGeneric* gen) {
    EasyCam* self = static_cast<EasyCam*>(gen->GetObject());
    Vec3 t = self->getTarget();
    new(gen->GetAddressOfReturnLocation()) Vec3(t);
}
static void EasyCam_SetDistance(asIScriptGeneric* gen) {
    EasyCam* self = static_cast<EasyCam*>(gen->GetObject());
    self->setDistance(gen->GetArgFloat(0));
}
static void EasyCam_GetDistance(asIScriptGeneric* gen) {
    EasyCam* self = static_cast<EasyCam*>(gen->GetObject());
    gen->SetReturnFloat(self->getDistance());
}
static void EasyCam_SetFov(asIScriptGeneric* gen) {
    EasyCam* self = static_cast<EasyCam*>(gen->GetObject());
    self->setFov(gen->GetArgFloat(0));
}
static void EasyCam_GetFov(asIScriptGeneric* gen) {
    EasyCam* self = static_cast<EasyCam*>(gen->GetObject());
    gen->SetReturnFloat(self->getFov());
}
static void EasyCam_SetFovDeg(asIScriptGeneric* gen) {
    EasyCam* self = static_cast<EasyCam*>(gen->GetObject());
    self->setFovDeg(gen->GetArgFloat(0));
}
static void EasyCam_SetNearClip(asIScriptGeneric* gen) {
    EasyCam* self = static_cast<EasyCam*>(gen->GetObject());
    self->setNearClip(gen->GetArgFloat(0));
}
static void EasyCam_SetFarClip(asIScriptGeneric* gen) {
    EasyCam* self = static_cast<EasyCam*>(gen->GetObject());
    self->setFarClip(gen->GetArgFloat(0));
}
static void EasyCam_EnableMouseInput(asIScriptGeneric* gen) {
    EasyCam* self = static_cast<EasyCam*>(gen->GetObject());
    self->enableMouseInput();
}
static void EasyCam_DisableMouseInput(asIScriptGeneric* gen) {
    EasyCam* self = static_cast<EasyCam*>(gen->GetObject());
    self->disableMouseInput();
}
static void EasyCam_IsMouseInputEnabled(asIScriptGeneric* gen) {
    EasyCam* self = static_cast<EasyCam*>(gen->GetObject());
    gen->SetReturnByte(self->isMouseInputEnabled() ? 1 : 0);
}
static void EasyCam_MousePressed(asIScriptGeneric* gen) {
    EasyCam* self = static_cast<EasyCam*>(gen->GetObject());
    self->mousePressed(gen->GetArgDWord(0), gen->GetArgDWord(1), gen->GetArgDWord(2));
}
static void EasyCam_MouseReleased(asIScriptGeneric* gen) {
    EasyCam* self = static_cast<EasyCam*>(gen->GetObject());
    self->mouseReleased(gen->GetArgDWord(0), gen->GetArgDWord(1), gen->GetArgDWord(2));
}
static void EasyCam_MouseDragged(asIScriptGeneric* gen) {
    EasyCam* self = static_cast<EasyCam*>(gen->GetObject());
    self->mouseDragged(gen->GetArgDWord(0), gen->GetArgDWord(1), gen->GetArgDWord(2));
}
static void EasyCam_MouseScrolled(asIScriptGeneric* gen) {
    EasyCam* self = static_cast<EasyCam*>(gen->GetObject());
    self->mouseScrolled(gen->GetArgFloat(0), gen->GetArgFloat(1));
}
static void EasyCam_GetPosition(asIScriptGeneric* gen) {
    EasyCam* self = static_cast<EasyCam*>(gen->GetObject());
    Vec3 p = self->getPosition();
    new(gen->GetAddressOfReturnLocation()) Vec3(p);
}
static void EasyCam_SetSensitivity(asIScriptGeneric* gen) {
    EasyCam* self = static_cast<EasyCam*>(gen->GetObject());
    self->setSensitivity(gen->GetArgFloat(0));
}
static void EasyCam_SetZoomSensitivity(asIScriptGeneric* gen) {
    EasyCam* self = static_cast<EasyCam*>(gen->GetObject());
    self->setZoomSensitivity(gen->GetArgFloat(0));
}
static void EasyCam_SetPanSensitivity(asIScriptGeneric* gen) {
    EasyCam* self = static_cast<EasyCam*>(gen->GetObject());
    self->setPanSensitivity(gen->GetArgFloat(0));
}

// =============================================================================
// Pixels type for AngelScript (reference type)
// =============================================================================
static void Pixels_Factory(asIScriptGeneric* gen) {
    g_pixels.push_back(make_unique<Pixels>());
    gen->SetReturnObject(g_pixels.back().get());
}
static void Pixels_AddRef(asIScriptGeneric*) { /* no-op */ }
static void Pixels_Release(asIScriptGeneric*) { /* no-op */ }

static void Pixels_Allocate_2i(asIScriptGeneric* gen) {
    Pixels* self = static_cast<Pixels*>(gen->GetObject());
    self->allocate(gen->GetArgDWord(0), gen->GetArgDWord(1));
}
static void Pixels_Allocate_3i(asIScriptGeneric* gen) {
    Pixels* self = static_cast<Pixels*>(gen->GetObject());
    self->allocate(gen->GetArgDWord(0), gen->GetArgDWord(1), gen->GetArgDWord(2));
}
static void Pixels_GetColor(asIScriptGeneric* gen) {
    Pixels* self = static_cast<Pixels*>(gen->GetObject());
    Color c = self->getColor(gen->GetArgDWord(0), gen->GetArgDWord(1));
    new(gen->GetAddressOfReturnLocation()) Color(c);
}
static void Pixels_SetColor(asIScriptGeneric* gen) {
    Pixels* self = static_cast<Pixels*>(gen->GetObject());
    Color* c = static_cast<Color*>(gen->GetArgObject(2));
    self->setColor(gen->GetArgDWord(0), gen->GetArgDWord(1), *c);
}
static void Pixels_Load(asIScriptGeneric* gen) {
    Pixels* self = static_cast<Pixels*>(gen->GetObject());
    string* path = static_cast<string*>(gen->GetArgObject(0));
    gen->SetReturnByte(self->load(*path) ? 1 : 0);
}
static void Pixels_GetWidth(asIScriptGeneric* gen) {
    Pixels* self = static_cast<Pixels*>(gen->GetObject());
    gen->SetReturnDWord(self->getWidth());
}
static void Pixels_GetHeight(asIScriptGeneric* gen) {
    Pixels* self = static_cast<Pixels*>(gen->GetObject());
    gen->SetReturnDWord(self->getHeight());
}
static void Pixels_IsAllocated(asIScriptGeneric* gen) {
    Pixels* self = static_cast<Pixels*>(gen->GetObject());
    gen->SetReturnByte(self->isAllocated() ? 1 : 0);
}
static void Pixels_Save(asIScriptGeneric* gen) {
    Pixels* self = static_cast<Pixels*>(gen->GetObject());
    string* path = static_cast<string*>(gen->GetArgObject(0));
    gen->SetReturnByte(self->save(*path) ? 1 : 0);
}

// =============================================================================
// Sound type for AngelScript (reference type)
// =============================================================================
static void Sound_Factory(asIScriptGeneric* gen) {
    g_sounds.push_back(make_unique<Sound>());
    gen->SetReturnObject(g_sounds.back().get());
}
static void Sound_AddRef(asIScriptGeneric*) { /* no-op */ }
static void Sound_Release(asIScriptGeneric*) { /* no-op */ }

static void Sound_Load(asIScriptGeneric* gen) {
    Sound* self = static_cast<Sound*>(gen->GetObject());
    string* path = static_cast<string*>(gen->GetArgObject(0));
    gen->SetReturnByte(self->load(*path) ? 1 : 0);
}
static void Sound_Play(asIScriptGeneric* gen) {
    Sound* self = static_cast<Sound*>(gen->GetObject());
    self->play();
}
static void Sound_Stop(asIScriptGeneric* gen) {
    Sound* self = static_cast<Sound*>(gen->GetObject());
    self->stop();
}
static void Sound_IsLoaded(asIScriptGeneric* gen) {
    Sound* self = static_cast<Sound*>(gen->GetObject());
    gen->SetReturnByte(self->isLoaded() ? 1 : 0);
}
static void Sound_IsPlaying(asIScriptGeneric* gen) {
    Sound* self = static_cast<Sound*>(gen->GetObject());
    gen->SetReturnByte(self->isPlaying() ? 1 : 0);
}
static void Sound_SetVolume(asIScriptGeneric* gen) {
    Sound* self = static_cast<Sound*>(gen->GetObject());
    self->setVolume(gen->GetArgFloat(0));
}
static void Sound_SetLoop(asIScriptGeneric* gen) {
    Sound* self = static_cast<Sound*>(gen->GetObject());
    self->setLoop(gen->GetArgByte(0) != 0);
}
static void Sound_IsLoop(asIScriptGeneric* gen) {
    Sound* self = static_cast<Sound*>(gen->GetObject());
    gen->SetReturnByte(self->isLoop() ? 1 : 0);
}
static void Sound_SetPan(asIScriptGeneric* gen) {
    Sound* self = static_cast<Sound*>(gen->GetObject());
    self->setPan(gen->GetArgFloat(0));
}
static void Sound_GetPan(asIScriptGeneric* gen) {
    Sound* self = static_cast<Sound*>(gen->GetObject());
    gen->SetReturnFloat(self->getPan());
}
static void Sound_SetSpeed(asIScriptGeneric* gen) {
    Sound* self = static_cast<Sound*>(gen->GetObject());
    self->setSpeed(gen->GetArgFloat(0));
}
static void Sound_GetSpeed(asIScriptGeneric* gen) {
    Sound* self = static_cast<Sound*>(gen->GetObject());
    gen->SetReturnFloat(self->getSpeed());
}
static void Sound_Pause(asIScriptGeneric* gen) {
    Sound* self = static_cast<Sound*>(gen->GetObject());
    self->pause();
}
static void Sound_Resume(asIScriptGeneric* gen) {
    Sound* self = static_cast<Sound*>(gen->GetObject());
    self->resume();
}
static void Sound_IsPaused(asIScriptGeneric* gen) {
    Sound* self = static_cast<Sound*>(gen->GetObject());
    gen->SetReturnByte(self->isPaused() ? 1 : 0);
}
static void Sound_GetPosition(asIScriptGeneric* gen) {
    Sound* self = static_cast<Sound*>(gen->GetObject());
    gen->SetReturnFloat(self->getPosition());
}
static void Sound_GetDuration(asIScriptGeneric* gen) {
    Sound* self = static_cast<Sound*>(gen->GetObject());
    gen->SetReturnFloat(self->getDuration());
}

// =============================================================================
// ChipSoundNote type for AngelScript (value type)
// =============================================================================
// Wave enum constants
static const int kWaveSin = static_cast<int>(Wave::Sin);
static const int kWaveSquare = static_cast<int>(Wave::Square);
static const int kWaveTriangle = static_cast<int>(Wave::Triangle);
static const int kWaveSawtooth = static_cast<int>(Wave::Sawtooth);
static const int kWaveNoise = static_cast<int>(Wave::Noise);
static const int kWavePinkNoise = static_cast<int>(Wave::PinkNoise);
static const int kWaveSilent = static_cast<int>(Wave::Silent);

static void ChipNote_Construct(asIScriptGeneric* gen) {
    ChipSoundNote* note = new(gen->GetObject()) ChipSoundNote();
    note->wave = Wave::Square;
    note->hz = 440.0f;
    note->volume = 0.5f;
    note->duration = 0.2f;
}
static void ChipNote_Construct_4(asIScriptGeneric* gen) {
    int w = gen->GetArgDWord(0);
    float hz = gen->GetArgFloat(1);
    float dur = gen->GetArgFloat(2);
    float vol = gen->GetArgFloat(3);
    new(gen->GetObject()) ChipSoundNote(static_cast<Wave>(w), hz, dur, vol);
}
static void ChipNote_CopyConstruct(asIScriptGeneric* gen) {
    ChipSoundNote* other = static_cast<ChipSoundNote*>(gen->GetArgObject(0));
    new(gen->GetObject()) ChipSoundNote(*other);
}
static void ChipNote_Destruct(asIScriptGeneric* gen) {
    static_cast<ChipSoundNote*>(gen->GetObject())->~ChipSoundNote();
}
static void ChipNote_Build(asIScriptGeneric* gen) {
    ChipSoundNote* self = static_cast<ChipSoundNote*>(gen->GetObject());
    g_sounds.push_back(make_unique<Sound>(self->build()));
    gen->SetReturnObject(g_sounds.back().get());
}
static void ChipNote_SetWave(asIScriptGeneric* gen) {
    ChipSoundNote* self = static_cast<ChipSoundNote*>(gen->GetObject());
    self->wave = static_cast<Wave>(gen->GetArgDWord(0));
    gen->SetReturnObject(self);
}
static void ChipNote_SetHz(asIScriptGeneric* gen) {
    ChipSoundNote* self = static_cast<ChipSoundNote*>(gen->GetObject());
    self->hz = gen->GetArgFloat(0);
    gen->SetReturnObject(self);
}
static void ChipNote_SetVolume(asIScriptGeneric* gen) {
    ChipSoundNote* self = static_cast<ChipSoundNote*>(gen->GetObject());
    self->volume = gen->GetArgFloat(0);
    gen->SetReturnObject(self);
}
static void ChipNote_SetDuration(asIScriptGeneric* gen) {
    ChipSoundNote* self = static_cast<ChipSoundNote*>(gen->GetObject());
    self->duration = gen->GetArgFloat(0);
    gen->SetReturnObject(self);
}
static void ChipNote_SetAttack(asIScriptGeneric* gen) {
    ChipSoundNote* self = static_cast<ChipSoundNote*>(gen->GetObject());
    self->attack = gen->GetArgFloat(0);
    gen->SetReturnObject(self);
}
static void ChipNote_SetDecay(asIScriptGeneric* gen) {
    ChipSoundNote* self = static_cast<ChipSoundNote*>(gen->GetObject());
    self->decay = gen->GetArgFloat(0);
    gen->SetReturnObject(self);
}
static void ChipNote_SetSustain(asIScriptGeneric* gen) {
    ChipSoundNote* self = static_cast<ChipSoundNote*>(gen->GetObject());
    self->sustain = gen->GetArgFloat(0);
    gen->SetReturnObject(self);
}
static void ChipNote_SetRelease(asIScriptGeneric* gen) {
    ChipSoundNote* self = static_cast<ChipSoundNote*>(gen->GetObject());
    self->release = gen->GetArgFloat(0);
    gen->SetReturnObject(self);
}
static void ChipNote_SetADSR(asIScriptGeneric* gen) {
    ChipSoundNote* self = static_cast<ChipSoundNote*>(gen->GetObject());
    self->attack = gen->GetArgFloat(0);
    self->decay = gen->GetArgFloat(1);
    self->sustain = gen->GetArgFloat(2);
    self->release = gen->GetArgFloat(3);
    gen->SetReturnObject(self);
}

// =============================================================================
// ChipSoundBundle type for AngelScript (reference type)
// =============================================================================
static void ChipBundle_Factory(asIScriptGeneric* gen) {
    g_chipBundles.push_back(make_unique<ChipSoundBundle>());
    gen->SetReturnObject(g_chipBundles.back().get());
}
static void ChipBundle_AddRef(asIScriptGeneric*) { /* no-op */ }
static void ChipBundle_Release(asIScriptGeneric*) { /* no-op */ }

static void ChipBundle_Add(asIScriptGeneric* gen) {
    ChipSoundBundle* self = static_cast<ChipSoundBundle*>(gen->GetObject());
    ChipSoundNote* note = static_cast<ChipSoundNote*>(gen->GetArgObject(0));
    float time = gen->GetArgFloat(1);
    self->add(*note, time);
    gen->SetReturnObject(self);
}
static void ChipBundle_Add_5(asIScriptGeneric* gen) {
    ChipSoundBundle* self = static_cast<ChipSoundBundle*>(gen->GetObject());
    int w = gen->GetArgDWord(0);
    float hz = gen->GetArgFloat(1);
    float dur = gen->GetArgFloat(2);
    float time = gen->GetArgFloat(3);
    float vol = gen->GetArgFloat(4);
    self->add(static_cast<Wave>(w), hz, dur, time, vol);
    gen->SetReturnObject(self);
}
static void ChipBundle_Clear(asIScriptGeneric* gen) {
    ChipSoundBundle* self = static_cast<ChipSoundBundle*>(gen->GetObject());
    self->clear();
}
static void ChipBundle_GetDuration(asIScriptGeneric* gen) {
    ChipSoundBundle* self = static_cast<ChipSoundBundle*>(gen->GetObject());
    gen->SetReturnFloat(self->getDuration());
}
static void ChipBundle_SetVolume(asIScriptGeneric* gen) {
    ChipSoundBundle* self = static_cast<ChipSoundBundle*>(gen->GetObject());
    self->volume = gen->GetArgFloat(0);
    gen->SetReturnObject(self);
}
static void ChipBundle_Build(asIScriptGeneric* gen) {
    ChipSoundBundle* self = static_cast<ChipSoundBundle*>(gen->GetObject());
    g_sounds.push_back(make_unique<Sound>(self->build()));
    gen->SetReturnObject(g_sounds.back().get());
}

// =============================================================================
// Easing functions
// =============================================================================
static void as_ease(asIScriptGeneric* gen) {
    float t = gen->GetArgFloat(0);
    int type = gen->GetArgDWord(1);
    int mode = gen->GetArgDWord(2);
    gen->SetReturnFloat(ease(t, static_cast<EaseType>(type), static_cast<EaseMode>(mode)));
}
static void as_easeIn(asIScriptGeneric* gen) {
    float t = gen->GetArgFloat(0);
    int type = gen->GetArgDWord(1);
    gen->SetReturnFloat(easeIn(t, static_cast<EaseType>(type)));
}
static void as_easeOut(asIScriptGeneric* gen) {
    float t = gen->GetArgFloat(0);
    int type = gen->GetArgDWord(1);
    gen->SetReturnFloat(easeOut(t, static_cast<EaseType>(type)));
}
static void as_easeInOut(asIScriptGeneric* gen) {
    float t = gen->GetArgFloat(0);
    int type = gen->GetArgDWord(1);
    gen->SetReturnFloat(easeInOut(t, static_cast<EaseType>(type)));
}

// =============================================================================
// Tween<float> type for AngelScript (reference type)
// =============================================================================

static void TweenFloat_Factory(asIScriptGeneric* gen) {
    g_tweens.push_back(make_unique<Tween<float>>());
    gen->SetReturnObject(g_tweens.back().get());
}
static void TweenFloat_From(asIScriptGeneric* gen) {
    Tween<float>* self = static_cast<Tween<float>*>(gen->GetObject());
    self->from(gen->GetArgFloat(0));
    gen->SetReturnObject(self);
}
static void TweenFloat_To(asIScriptGeneric* gen) {
    Tween<float>* self = static_cast<Tween<float>*>(gen->GetObject());
    self->to(gen->GetArgFloat(0));
    gen->SetReturnObject(self);
}
static void TweenFloat_Duration(asIScriptGeneric* gen) {
    Tween<float>* self = static_cast<Tween<float>*>(gen->GetObject());
    self->duration(gen->GetArgFloat(0));
    gen->SetReturnObject(self);
}
static void TweenFloat_Ease(asIScriptGeneric* gen) {
    Tween<float>* self = static_cast<Tween<float>*>(gen->GetObject());
    int type = gen->GetArgDWord(0);
    int mode = gen->GetArgDWord(1);
    self->ease(static_cast<EaseType>(type), static_cast<EaseMode>(mode));
    gen->SetReturnObject(self);
}
static void TweenFloat_Ease_1(asIScriptGeneric* gen) {
    Tween<float>* self = static_cast<Tween<float>*>(gen->GetObject());
    int type = gen->GetArgDWord(0);
    self->ease(static_cast<EaseType>(type), EaseMode::InOut);
    gen->SetReturnObject(self);
}
static void TweenFloat_Start(asIScriptGeneric* gen) {
    Tween<float>* self = static_cast<Tween<float>*>(gen->GetObject());
    self->start();
    gen->SetReturnObject(self);
}
static void TweenFloat_Pause(asIScriptGeneric* gen) {
    Tween<float>* self = static_cast<Tween<float>*>(gen->GetObject());
    self->pause();
    gen->SetReturnObject(self);
}
static void TweenFloat_Resume(asIScriptGeneric* gen) {
    Tween<float>* self = static_cast<Tween<float>*>(gen->GetObject());
    self->resume();
    gen->SetReturnObject(self);
}
static void TweenFloat_Reset(asIScriptGeneric* gen) {
    Tween<float>* self = static_cast<Tween<float>*>(gen->GetObject());
    self->reset();
    gen->SetReturnObject(self);
}
static void TweenFloat_Finish(asIScriptGeneric* gen) {
    Tween<float>* self = static_cast<Tween<float>*>(gen->GetObject());
    self->finish();
    gen->SetReturnObject(self);
}
static void TweenFloat_Update(asIScriptGeneric* gen) {
    Tween<float>* self = static_cast<Tween<float>*>(gen->GetObject());
    self->update(gen->GetArgFloat(0));
}
static void TweenFloat_GetValue(asIScriptGeneric* gen) {
    Tween<float>* self = static_cast<Tween<float>*>(gen->GetObject());
    gen->SetReturnFloat(self->getValue());
}
static void TweenFloat_GetProgress(asIScriptGeneric* gen) {
    Tween<float>* self = static_cast<Tween<float>*>(gen->GetObject());
    gen->SetReturnFloat(self->getProgress());
}
static void TweenFloat_GetElapsed(asIScriptGeneric* gen) {
    Tween<float>* self = static_cast<Tween<float>*>(gen->GetObject());
    gen->SetReturnFloat(self->getElapsed());
}
static void TweenFloat_GetDuration(asIScriptGeneric* gen) {
    Tween<float>* self = static_cast<Tween<float>*>(gen->GetObject());
    gen->SetReturnFloat(self->getDuration());
}
static void TweenFloat_IsPlaying(asIScriptGeneric* gen) {
    Tween<float>* self = static_cast<Tween<float>*>(gen->GetObject());
    gen->SetReturnByte(self->isPlaying() ? 1 : 0);
}
static void TweenFloat_IsComplete(asIScriptGeneric* gen) {
    Tween<float>* self = static_cast<Tween<float>*>(gen->GetObject());
    gen->SetReturnByte(self->isComplete() ? 1 : 0);
}
static void TweenFloat_GetStart(asIScriptGeneric* gen) {
    Tween<float>* self = static_cast<Tween<float>*>(gen->GetObject());
    gen->SetReturnFloat(self->getStart());
}
static void TweenFloat_GetEnd(asIScriptGeneric* gen) {
    Tween<float>* self = static_cast<Tween<float>*>(gen->GetObject());
    gen->SetReturnFloat(self->getEnd());
}

// =============================================================================
// Font type for AngelScript (reference type)
// =============================================================================
static void Font_Factory(asIScriptGeneric* gen) {
    g_fonts.push_back(make_unique<Font>());
    gen->SetReturnObject(g_fonts.back().get());
}

static void Font_Load(asIScriptGeneric* gen) {
    Font* self = static_cast<Font*>(gen->GetObject());
    string* path = static_cast<string*>(gen->GetArgObject(0));
    int size = gen->GetArgDWord(1);
    gen->SetReturnByte(self->load(*path, size) ? 1 : 0);
}
static void Font_IsLoaded(asIScriptGeneric* gen) {
    Font* self = static_cast<Font*>(gen->GetObject());
    gen->SetReturnByte(self->isLoaded() ? 1 : 0);
}
static void Font_DrawString_3(asIScriptGeneric* gen) {
    Font* self = static_cast<Font*>(gen->GetObject());
    string* text = static_cast<string*>(gen->GetArgObject(0));
    self->drawString(*text, gen->GetArgFloat(1), gen->GetArgFloat(2));
}
static void Font_GetWidth(asIScriptGeneric* gen) {
    Font* self = static_cast<Font*>(gen->GetObject());
    string* text = static_cast<string*>(gen->GetArgObject(0));
    gen->SetReturnFloat(self->getWidth(*text));
}
static void Font_GetHeight(asIScriptGeneric* gen) {
    Font* self = static_cast<Font*>(gen->GetObject());
    string* text = static_cast<string*>(gen->GetArgObject(0));
    gen->SetReturnFloat(self->getHeight(*text));
}
static void Font_GetLineHeight(asIScriptGeneric* gen) {
    Font* self = static_cast<Font*>(gen->GetObject());
    gen->SetReturnFloat(self->getLineHeight());
}
static void Font_GetSize(asIScriptGeneric* gen) {
    Font* self = static_cast<Font*>(gen->GetObject());
    gen->SetReturnDWord(self->getSize());
}

// =============================================================================
// tcScriptHost implementation
// =============================================================================

tcScriptHost::tcScriptHost() {
    engine_ = asCreateScriptEngine();
    if (!engine_) {
        lastError_ = "Failed to create AngelScript engine";
        return;
    }

    engine_->SetMessageCallback(asFUNCTION(messageCallbackStatic), this, asCALL_CDECL);
    RegisterStdString(engine_);
    RegisterScriptArray(engine_, true);  // true = register 'array<T>' as default array type
    registerTrussCFunctions();
    ctx_ = engine_->CreateContext();
}

tcScriptHost::~tcScriptHost() {
    if (ctx_) ctx_->Release();
    if (engine_) engine_->ShutDownAndRelease();
}

void tcScriptHost::registerTrussCFunctions() {
    tc::logNotice() << "[AngelScript] Starting registration...";
    int r;
    auto checkReg = [](int result, const char* desc) {
        if (result < 0) {
            tc::logError() << "[AngelScript] Registration failed: " << desc << " (code: " << result << ")";
        }
        return result;
    };

    // =========================================================================
    // Value types: Vec2, Vec3, Color, Rect
    // =========================================================================

    // Vec2
    r = engine_->RegisterObjectType("Vec2", sizeof(Vec2), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Vec2>()); assert(r >= 0);
    r = engine_->RegisterObjectProperty("Vec2", "float x", offsetof(Vec2, x)); assert(r >= 0);
    r = engine_->RegisterObjectProperty("Vec2", "float y", offsetof(Vec2, y)); assert(r >= 0);
    r = engine_->RegisterObjectBehaviour("Vec2", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Vec2_Construct), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectBehaviour("Vec2", asBEHAVE_CONSTRUCT, "void f(float, float)", asFUNCTION(Vec2_Construct_2f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectBehaviour("Vec2", asBEHAVE_CONSTRUCT, "void f(float)", asFUNCTION(Vec2_Construct_1f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectBehaviour("Vec2", asBEHAVE_CONSTRUCT, "void f(const Vec2 &in)", asFUNCTION(Vec2_CopyConstruct), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec2", "Vec2& set(float, float)", asFUNCTION(Vec2_Set), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec2", "float length() const", asFUNCTION(Vec2_Length), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec2", "float lengthSquared() const", asFUNCTION(Vec2_LengthSquared), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec2", "Vec2& normalize()", asFUNCTION(Vec2_Normalize), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec2", "Vec2 normalized() const", asFUNCTION(Vec2_Normalized), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec2", "float dot(const Vec2 &in) const", asFUNCTION(Vec2_Dot), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec2", "float distance(const Vec2 &in) const", asFUNCTION(Vec2_Distance), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec2", "float angle() const", asFUNCTION(Vec2_Angle), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec2", "Vec2& rotate(float)", asFUNCTION(Vec2_Rotate), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec2", "Vec2 rotated(float) const", asFUNCTION(Vec2_Rotated), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec2", "Vec2 opAdd(const Vec2 &in) const", asFUNCTION(Vec2_OpAdd), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec2", "Vec2 opSub(const Vec2 &in) const", asFUNCTION(Vec2_OpSub), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec2", "Vec2 opMul(float) const", asFUNCTION(Vec2_OpMulScalar), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec2", "Vec2 opDiv(float) const", asFUNCTION(Vec2_OpDivScalar), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec2", "Vec2 opNeg() const", asFUNCTION(Vec2_OpNeg), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Vec2 Vec2_fromAngle(float)", asFUNCTION(Vec2_fromAngle_1f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Vec2 Vec2_fromAngle(float, float)", asFUNCTION(Vec2_fromAngle_2f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec2", "Vec2& limit(float)", asFUNCTION(Vec2_Limit), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec2", "float cross(const Vec2 &in) const", asFUNCTION(Vec2_Cross), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec2", "float distanceSquared(const Vec2 &in) const", asFUNCTION(Vec2_DistanceSquared), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec2", "Vec2 lerp(const Vec2 &in, float) const", asFUNCTION(Vec2_Lerp), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec2", "Vec2 perpendicular() const", asFUNCTION(Vec2_Perpendicular), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec2", "Vec2 reflected(const Vec2 &in) const", asFUNCTION(Vec2_Reflected), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec2", "float angle(const Vec2 &in) const", asFUNCTION(Vec2_AngleWith), asCALL_GENERIC); assert(r >= 0);

    // Vec3
    r = engine_->RegisterObjectType("Vec3", sizeof(Vec3), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Vec3>()); assert(r >= 0);
    r = engine_->RegisterObjectProperty("Vec3", "float x", offsetof(Vec3, x)); assert(r >= 0);
    r = engine_->RegisterObjectProperty("Vec3", "float y", offsetof(Vec3, y)); assert(r >= 0);
    r = engine_->RegisterObjectProperty("Vec3", "float z", offsetof(Vec3, z)); assert(r >= 0);
    r = engine_->RegisterObjectBehaviour("Vec3", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Vec3_Construct), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectBehaviour("Vec3", asBEHAVE_CONSTRUCT, "void f(float, float, float)", asFUNCTION(Vec3_Construct_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectBehaviour("Vec3", asBEHAVE_CONSTRUCT, "void f(float)", asFUNCTION(Vec3_Construct_1f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectBehaviour("Vec3", asBEHAVE_CONSTRUCT, "void f(const Vec3 &in)", asFUNCTION(Vec3_CopyConstruct), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec3", "Vec3& set(float, float, float)", asFUNCTION(Vec3_Set), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec3", "float length() const", asFUNCTION(Vec3_Length), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec3", "float lengthSquared() const", asFUNCTION(Vec3_LengthSquared), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec3", "Vec3& normalize()", asFUNCTION(Vec3_Normalize), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec3", "Vec3 normalized() const", asFUNCTION(Vec3_Normalized), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec3", "float dot(const Vec3 &in) const", asFUNCTION(Vec3_Dot), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec3", "Vec3 cross(const Vec3 &in) const", asFUNCTION(Vec3_Cross), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec3", "Vec3 opAdd(const Vec3 &in) const", asFUNCTION(Vec3_OpAdd), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec3", "Vec3 opSub(const Vec3 &in) const", asFUNCTION(Vec3_OpSub), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec3", "Vec3 opMul(float) const", asFUNCTION(Vec3_OpMulScalar), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec3", "Vec3 opDiv(float) const", asFUNCTION(Vec3_OpDivScalar), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec3", "Vec3 opNeg() const", asFUNCTION(Vec3_OpNeg), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec3", "Vec3& limit(float)", asFUNCTION(Vec3_Limit), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec3", "float distance(const Vec3 &in) const", asFUNCTION(Vec3_Distance), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec3", "float distanceSquared(const Vec3 &in) const", asFUNCTION(Vec3_DistanceSquared), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec3", "Vec3 lerp(const Vec3 &in, float) const", asFUNCTION(Vec3_Lerp), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec3", "Vec3 reflected(const Vec3 &in) const", asFUNCTION(Vec3_Reflected), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Vec3", "Vec2 xy() const", asFUNCTION(Vec3_XY), asCALL_GENERIC); assert(r >= 0);

    // Color
    r = engine_->RegisterObjectType("Color", sizeof(Color), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Color>()); assert(r >= 0);
    r = engine_->RegisterObjectProperty("Color", "float r", offsetof(Color, r)); assert(r >= 0);
    r = engine_->RegisterObjectProperty("Color", "float g", offsetof(Color, g)); assert(r >= 0);
    r = engine_->RegisterObjectProperty("Color", "float b", offsetof(Color, b)); assert(r >= 0);
    r = engine_->RegisterObjectProperty("Color", "float a", offsetof(Color, a)); assert(r >= 0);
    r = engine_->RegisterObjectBehaviour("Color", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Color_Construct), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectBehaviour("Color", asBEHAVE_CONSTRUCT, "void f(float, float, float)", asFUNCTION(Color_Construct_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectBehaviour("Color", asBEHAVE_CONSTRUCT, "void f(float, float, float, float)", asFUNCTION(Color_Construct_4f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectBehaviour("Color", asBEHAVE_CONSTRUCT, "void f(float)", asFUNCTION(Color_Construct_1f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectBehaviour("Color", asBEHAVE_CONSTRUCT, "void f(const Color &in)", asFUNCTION(Color_CopyConstruct), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Color", "Color& set(float, float, float)", asFUNCTION(Color_Set_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Color", "Color& set(float, float, float, float)", asFUNCTION(Color_Set_4f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Color", "Color lerp(const Color &in, float) const", asFUNCTION(Color_Lerp), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Color", "uint toHex() const", asFUNCTION(Color_ToHex_0), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Color", "uint toHex(bool) const", asFUNCTION(Color_ToHex_1b), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Color", "Color lerpRGB(const Color &in, float) const", asFUNCTION(Color_LerpRGB), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Color", "Color clamped() const", asFUNCTION(Color_Clamped), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Color Color_fromHSB(float, float, float)", asFUNCTION(Color_fromHSB_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Color Color_fromHSB(float, float, float, float)", asFUNCTION(Color_fromHSB_4f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Color colorFromHSB(float, float, float)", asFUNCTION(Color_fromHSB_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Color colorFromHSB(float, float, float, float)", asFUNCTION(Color_fromHSB_4f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Color Color_fromOKLCH(float, float, float)", asFUNCTION(Color_fromOKLCH_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Color Color_fromOKLCH(float, float, float, float)", asFUNCTION(Color_fromOKLCH_4f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Color Color_fromOKLab(float, float, float)", asFUNCTION(Color_fromOKLab_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Color Color_fromOKLab(float, float, float, float)", asFUNCTION(Color_fromOKLab_4f), asCALL_GENERIC); assert(r >= 0);

    // Rect
    r = engine_->RegisterObjectType("Rect", sizeof(Rect), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Rect>()); assert(r >= 0);
    r = engine_->RegisterObjectProperty("Rect", "float x", offsetof(Rect, x)); assert(r >= 0);
    r = engine_->RegisterObjectProperty("Rect", "float y", offsetof(Rect, y)); assert(r >= 0);
    r = engine_->RegisterObjectProperty("Rect", "float width", offsetof(Rect, width)); assert(r >= 0);
    r = engine_->RegisterObjectProperty("Rect", "float height", offsetof(Rect, height)); assert(r >= 0);
    r = engine_->RegisterObjectBehaviour("Rect", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Rect_Construct), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectBehaviour("Rect", asBEHAVE_CONSTRUCT, "void f(float, float, float, float)", asFUNCTION(Rect_Construct_4f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectBehaviour("Rect", asBEHAVE_CONSTRUCT, "void f(const Rect &in)", asFUNCTION(Rect_CopyConstruct), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Rect", "Rect& set(float, float, float, float)", asFUNCTION(Rect_Set), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Rect", "bool contains(float, float) const", asFUNCTION(Rect_Contains), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Rect", "bool intersects(const Rect &in) const", asFUNCTION(Rect_Intersects), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Rect", "float getCenterX() const", asFUNCTION(Rect_GetCenterX), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Rect", "float getCenterY() const", asFUNCTION(Rect_GetCenterY), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Rect", "float getRight() const", asFUNCTION(Rect_GetRight), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Rect", "float getBottom() const", asFUNCTION(Rect_GetBottom), asCALL_GENERIC); assert(r >= 0);

    // Mat4 (4x4 matrix for 3D transformations)
    r = engine_->RegisterObjectType("Mat4", sizeof(Mat4), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Mat4>()); assert(r >= 0);
    r = engine_->RegisterObjectBehaviour("Mat4", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Mat4_Construct), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectBehaviour("Mat4", asBEHAVE_CONSTRUCT, "void f(const Mat4 &in)", asFUNCTION(Mat4_CopyConstruct), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mat4", "Mat4 opMul(const Mat4 &in) const", asFUNCTION(Mat4_OpMul_Mat4), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mat4", "Vec3 opMul(const Vec3 &in) const", asFUNCTION(Mat4_OpMul_Vec3), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mat4", "Mat4 transposed() const", asFUNCTION(Mat4_Transposed), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mat4", "Mat4 inverted() const", asFUNCTION(Mat4_Inverted), asCALL_GENERIC); assert(r >= 0);
    // Mat4 static factory functions
    r = engine_->RegisterGlobalFunction("Mat4 Mat4_identity()", asFUNCTION(Mat4_Identity), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Mat4 Mat4_translate(float, float, float)", asFUNCTION(Mat4_Translate_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Mat4 Mat4_translate(const Vec3 &in)", asFUNCTION(Mat4_Translate_Vec3), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Mat4 Mat4_rotateX(float)", asFUNCTION(Mat4_RotateX), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Mat4 Mat4_rotateY(float)", asFUNCTION(Mat4_RotateY), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Mat4 Mat4_rotateZ(float)", asFUNCTION(Mat4_RotateZ), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Mat4 Mat4_scale(float)", asFUNCTION(Mat4_Scale_1f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Mat4 Mat4_scale(float, float, float)", asFUNCTION(Mat4_Scale_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Mat4 Mat4_lookAt(const Vec3 &in, const Vec3 &in, const Vec3 &in)", asFUNCTION(Mat4_LookAt), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Mat4 Mat4_ortho(float, float, float, float, float, float)", asFUNCTION(Mat4_Ortho), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Mat4 Mat4_perspective(float, float, float, float)", asFUNCTION(Mat4_Perspective), asCALL_GENERIC); assert(r >= 0);

    // Quaternion (unit quaternion for 3D rotations)
    r = engine_->RegisterObjectType("Quaternion", sizeof(Quaternion), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Quaternion>()); assert(r >= 0);
    r = engine_->RegisterObjectProperty("Quaternion", "float w", offsetof(Quaternion, w)); assert(r >= 0);
    r = engine_->RegisterObjectProperty("Quaternion", "float x", offsetof(Quaternion, x)); assert(r >= 0);
    r = engine_->RegisterObjectProperty("Quaternion", "float y", offsetof(Quaternion, y)); assert(r >= 0);
    r = engine_->RegisterObjectProperty("Quaternion", "float z", offsetof(Quaternion, z)); assert(r >= 0);
    r = engine_->RegisterObjectBehaviour("Quaternion", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Quaternion_Construct), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectBehaviour("Quaternion", asBEHAVE_CONSTRUCT, "void f(float, float, float, float)", asFUNCTION(Quaternion_Construct_4f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectBehaviour("Quaternion", asBEHAVE_CONSTRUCT, "void f(const Quaternion &in)", asFUNCTION(Quaternion_CopyConstruct), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Quaternion", "Quaternion opMul(const Quaternion &in) const", asFUNCTION(Quaternion_OpMul), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Quaternion", "Vec3 rotate(const Vec3 &in) const", asFUNCTION(Quaternion_Rotate), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Quaternion", "Vec3 toEuler() const", asFUNCTION(Quaternion_ToEuler), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Quaternion", "Mat4 toMatrix() const", asFUNCTION(Quaternion_ToMatrix), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Quaternion", "Quaternion normalized() const", asFUNCTION(Quaternion_Normalized), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Quaternion", "float length() const", asFUNCTION(Quaternion_Length), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Quaternion", "Quaternion conjugate() const", asFUNCTION(Quaternion_Conjugate), asCALL_GENERIC); assert(r >= 0);
    // Quaternion static factory functions
    r = engine_->RegisterGlobalFunction("Quaternion Quaternion_identity()", asFUNCTION(Quaternion_Identity), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Quaternion Quaternion_fromAxisAngle(const Vec3 &in, float)", asFUNCTION(Quaternion_FromAxisAngle), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Quaternion Quaternion_fromEuler(float, float, float)", asFUNCTION(Quaternion_FromEuler_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Quaternion Quaternion_fromEuler(const Vec3 &in)", asFUNCTION(Quaternion_FromEuler_Vec3), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Quaternion Quaternion_slerp(const Quaternion &in, const Quaternion &in, float)", asFUNCTION(Quaternion_Slerp), asCALL_GENERIC); assert(r >= 0);

    // =========================================================================
    // Reference types: Pixels, Texture, Fbo, Sound
    // (Order matters: types must be declared before being referenced)
    // =========================================================================

    // First, register all object types
    r = engine_->RegisterObjectType("Pixels", 0, asOBJ_REF | asOBJ_NOCOUNT); assert(r >= 0);
    r = engine_->RegisterObjectType("Texture", 0, asOBJ_REF | asOBJ_NOCOUNT); assert(r >= 0);
    r = engine_->RegisterObjectType("Fbo", 0, asOBJ_REF | asOBJ_NOCOUNT); assert(r >= 0);
    r = engine_->RegisterObjectType("Sound", 0, asOBJ_REF | asOBJ_NOCOUNT); assert(r >= 0);
    r = engine_->RegisterObjectType("Font", 0, asOBJ_REF | asOBJ_NOCOUNT); assert(r >= 0);
    r = engine_->RegisterObjectType("Mesh", 0, asOBJ_REF | asOBJ_NOCOUNT); assert(r >= 0);
    r = engine_->RegisterObjectType("Path", 0, asOBJ_REF | asOBJ_NOCOUNT); assert(r >= 0);
    r = engine_->RegisterObjectType("StrokeMesh", 0, asOBJ_REF | asOBJ_NOCOUNT); assert(r >= 0);
    r = engine_->RegisterObjectType("Image", 0, asOBJ_REF | asOBJ_NOCOUNT); assert(r >= 0);
    r = engine_->RegisterObjectType("EasyCam", 0, asOBJ_REF | asOBJ_NOCOUNT); assert(r >= 0);

    // PrimitiveMode enum for Mesh
    r = engine_->RegisterEnum("PrimitiveMode"); assert(r >= 0);
    r = engine_->RegisterEnumValue("PrimitiveMode", "Triangles", static_cast<int>(PrimitiveMode::Triangles)); assert(r >= 0);
    r = engine_->RegisterEnumValue("PrimitiveMode", "TriangleStrip", static_cast<int>(PrimitiveMode::TriangleStrip)); assert(r >= 0);
    r = engine_->RegisterEnumValue("PrimitiveMode", "TriangleFan", static_cast<int>(PrimitiveMode::TriangleFan)); assert(r >= 0);
    r = engine_->RegisterEnumValue("PrimitiveMode", "Lines", static_cast<int>(PrimitiveMode::Lines)); assert(r >= 0);
    r = engine_->RegisterEnumValue("PrimitiveMode", "LineStrip", static_cast<int>(PrimitiveMode::LineStrip)); assert(r >= 0);
    r = engine_->RegisterEnumValue("PrimitiveMode", "LineLoop", static_cast<int>(PrimitiveMode::LineLoop)); assert(r >= 0);
    r = engine_->RegisterEnumValue("PrimitiveMode", "Points", static_cast<int>(PrimitiveMode::Points)); assert(r >= 0);

    // Wave enum for ChipSound
    r = engine_->RegisterEnum("Wave"); assert(r >= 0);

    // ChipSoundNote value type
    r = engine_->RegisterObjectType("ChipSoundNote", sizeof(ChipSoundNote), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<ChipSoundNote>()); assert(r >= 0);
    r = engine_->RegisterObjectBehaviour("ChipSoundNote", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ChipNote_Construct), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectBehaviour("ChipSoundNote", asBEHAVE_CONSTRUCT, "void f(Wave, float, float, float)", asFUNCTION(ChipNote_Construct_4), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectBehaviour("ChipSoundNote", asBEHAVE_CONSTRUCT, "void f(const ChipSoundNote &in)", asFUNCTION(ChipNote_CopyConstruct), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectBehaviour("ChipSoundNote", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(ChipNote_Destruct), asCALL_GENERIC); assert(r >= 0);

    // ChipSoundBundle reference type
    r = engine_->RegisterObjectType("ChipSoundBundle", 0, asOBJ_REF | asOBJ_NOCOUNT); assert(r >= 0);

    // Pixels methods
    r = engine_->RegisterGlobalFunction("Pixels@ createPixels()", asFUNCTION(Pixels_Factory), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Pixels", "void allocate(int, int)", asFUNCTION(Pixels_Allocate_2i), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Pixels", "void allocate(int, int, int)", asFUNCTION(Pixels_Allocate_3i), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Pixels", "Color getColor(int, int) const", asFUNCTION(Pixels_GetColor), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Pixels", "void setColor(int, int, const Color &in)", asFUNCTION(Pixels_SetColor), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Pixels", "bool load(const string &in)", asFUNCTION(Pixels_Load), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Pixels", "bool save(const string &in) const", asFUNCTION(Pixels_Save), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Pixels", "int getWidth() const", asFUNCTION(Pixels_GetWidth), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Pixels", "int getHeight() const", asFUNCTION(Pixels_GetHeight), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Pixels", "bool isAllocated() const", asFUNCTION(Pixels_IsAllocated), asCALL_GENERIC); assert(r >= 0);

    // Texture methods
    r = engine_->RegisterGlobalFunction("Texture@ createTexture()", asFUNCTION(Texture_Factory), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Texture", "void allocate(int, int)", asFUNCTION(Texture_Allocate_2i), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Texture", "void allocate(Pixels@)", asFUNCTION(Texture_Allocate_Pixels), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Texture", "void loadData(Pixels@)", asFUNCTION(Texture_LoadData), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Texture", "void bind()", asFUNCTION(Texture_Bind), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Texture", "void unbind()", asFUNCTION(Texture_Unbind), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Texture", "int getWidth() const", asFUNCTION(Texture_GetWidth), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Texture", "int getHeight() const", asFUNCTION(Texture_GetHeight), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Texture", "bool isAllocated() const", asFUNCTION(Texture_IsAllocated), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Texture", "void draw(float, float)", asFUNCTION(Texture_Draw_2f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Texture", "void draw(float, float, float, float)", asFUNCTION(Texture_Draw_4f), asCALL_GENERIC); assert(r >= 0);

    // Fbo methods
    r = engine_->RegisterGlobalFunction("Fbo@ createFbo()", asFUNCTION(Fbo_Factory), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Fbo", "void allocate(int, int)", asFUNCTION(Fbo_Allocate_2i), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Fbo", "void begin()", asFUNCTION(Fbo_Begin), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Fbo", "void begin(float, float, float, float)", asFUNCTION(Fbo_Begin_4f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Fbo", "void end()", asFUNCTION(Fbo_End), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Fbo", "Texture@ getTexture()", asFUNCTION(Fbo_GetTexture), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Fbo", "int getWidth() const", asFUNCTION(Fbo_GetWidth), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Fbo", "int getHeight() const", asFUNCTION(Fbo_GetHeight), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Fbo", "bool isAllocated() const", asFUNCTION(Fbo_IsAllocated), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Fbo", "void draw(float, float)", asFUNCTION(Fbo_Draw_2f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Fbo", "void draw(float, float, float, float)", asFUNCTION(Fbo_Draw_4f), asCALL_GENERIC); assert(r >= 0);

    // Mesh methods
    r = engine_->RegisterGlobalFunction("Mesh@ createMesh()", asFUNCTION(Mesh_Factory), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "Mesh@ setMode(PrimitiveMode)", asFUNCTION(Mesh_SetMode), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "int getMode() const", asFUNCTION(Mesh_GetMode), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "Mesh@ addVertex(float, float, float)", asFUNCTION(Mesh_AddVertex_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "Mesh@ addVertex(float, float)", asFUNCTION(Mesh_AddVertex_2f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "Mesh@ addVertex(const Vec3 &in)", asFUNCTION(Mesh_AddVertex_Vec3), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "Mesh@ addVertex(const Vec2 &in)", asFUNCTION(Mesh_AddVertex_Vec2), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "Mesh@ addColor(const Color &in)", asFUNCTION(Mesh_AddColor_Color), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "Mesh@ addColor(float, float, float, float)", asFUNCTION(Mesh_AddColor_4f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "Mesh@ addColor(float, float, float)", asFUNCTION(Mesh_AddColor_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "Mesh@ addTexCoord(float, float)", asFUNCTION(Mesh_AddTexCoord_2f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "Mesh@ addTexCoord(const Vec2 &in)", asFUNCTION(Mesh_AddTexCoord_Vec2), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "Mesh@ addNormal(float, float, float)", asFUNCTION(Mesh_AddNormal_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "Mesh@ addNormal(const Vec3 &in)", asFUNCTION(Mesh_AddNormal_Vec3), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "Mesh@ addIndex(uint)", asFUNCTION(Mesh_AddIndex), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "Mesh@ addTriangle(uint, uint, uint)", asFUNCTION(Mesh_AddTriangle), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "Mesh@ clear()", asFUNCTION(Mesh_Clear), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "void draw()", asFUNCTION(Mesh_Draw), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "void drawWireframe()", asFUNCTION(Mesh_DrawWireframe), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "int getNumVertices() const", asFUNCTION(Mesh_GetNumVertices), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "int getNumIndices() const", asFUNCTION(Mesh_GetNumIndices), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "int getNumColors() const", asFUNCTION(Mesh_GetNumColors), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "int getNumNormals() const", asFUNCTION(Mesh_GetNumNormals), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "bool hasColors() const", asFUNCTION(Mesh_HasColors), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "bool hasIndices() const", asFUNCTION(Mesh_HasIndices), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "bool hasNormals() const", asFUNCTION(Mesh_HasNormals), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "bool hasTexCoords() const", asFUNCTION(Mesh_HasTexCoords), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "Mesh@ translate(float, float, float)", asFUNCTION(Mesh_Translate_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "Mesh@ translate(const Vec3 &in)", asFUNCTION(Mesh_Translate_Vec3), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "Mesh@ rotateX(float)", asFUNCTION(Mesh_RotateX), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "Mesh@ rotateY(float)", asFUNCTION(Mesh_RotateY), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "Mesh@ rotateZ(float)", asFUNCTION(Mesh_RotateZ), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "Mesh@ scale(float)", asFUNCTION(Mesh_Scale_1f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "Mesh@ scale(float, float, float)", asFUNCTION(Mesh_Scale_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "Mesh@ addVertices(array<Vec3>@)", asFUNCTION(Mesh_AddVertices_Vec3Array), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "Mesh@ addVertices(array<Vec2>@)", asFUNCTION(Mesh_AddVertices_Vec2Array), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "Mesh@ addColors(array<Color>@)", asFUNCTION(Mesh_AddColors_Array), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "Mesh@ addIndices(array<uint>@)", asFUNCTION(Mesh_AddIndices_Array), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Mesh", "Mesh@ addNormals(array<Vec3>@)", asFUNCTION(Mesh_AddNormals_Array), asCALL_GENERIC); assert(r >= 0);

    // Path (Polyline) methods
    r = engine_->RegisterGlobalFunction("Path@ createPath()", asFUNCTION(Path_Factory), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Path", "Path@ addVertex(float, float)", asFUNCTION(Path_AddVertex_2f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Path", "Path@ addVertex(float, float, float)", asFUNCTION(Path_AddVertex_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Path", "Path@ addVertex(const Vec2 &in)", asFUNCTION(Path_AddVertex_Vec2), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Path", "Path@ addVertex(const Vec3 &in)", asFUNCTION(Path_AddVertex_Vec3), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Path", "Path@ addVertices(array<Vec3>@)", asFUNCTION(Path_AddVertices_Vec3Array), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Path", "Path@ addVertices(array<Vec2>@)", asFUNCTION(Path_AddVertices_Vec2Array), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Path", "Path@ lineTo(float, float)", asFUNCTION(Path_LineTo_2f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Path", "Path@ lineTo(const Vec2 &in)", asFUNCTION(Path_LineTo_Vec2), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Path", "Path@ bezierTo(float, float, float, float, float, float)", asFUNCTION(Path_BezierTo_6f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Path", "Path@ quadBezierTo(float, float, float, float)", asFUNCTION(Path_QuadBezierTo_4f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Path", "Path@ curveTo(float, float)", asFUNCTION(Path_CurveTo_2f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Path", "Path@ curveTo(float, float, float)", asFUNCTION(Path_CurveTo_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Path", "Path@ arc(float, float, float, float, float, float)", asFUNCTION(Path_Arc_6f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Path", "Path@ close()", asFUNCTION(Path_Close), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Path", "Path@ setClosed(bool)", asFUNCTION(Path_SetClosed), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Path", "bool isClosed() const", asFUNCTION(Path_IsClosed), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Path", "Path@ clear()", asFUNCTION(Path_Clear), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Path", "void draw()", asFUNCTION(Path_Draw), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Path", "int size() const", asFUNCTION(Path_Size), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Path", "bool empty() const", asFUNCTION(Path_Empty), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Path", "float getPerimeter() const", asFUNCTION(Path_GetPerimeter), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Path", "Rect getBounds() const", asFUNCTION(Path_GetBounds), asCALL_GENERIC); assert(r >= 0);

    // StrokeMesh methods
    r = engine_->RegisterGlobalFunction("StrokeMesh@ createStrokeMesh()", asFUNCTION(StrokeMesh_Factory), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("StrokeMesh", "StrokeMesh& setWidth(float)", asFUNCTION(StrokeMesh_SetWidth), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("StrokeMesh", "StrokeMesh& setColor(const Color &in)", asFUNCTION(StrokeMesh_SetColor), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("StrokeMesh", "StrokeMesh& setCapType(int)", asFUNCTION(StrokeMesh_SetCapType), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("StrokeMesh", "StrokeMesh& setJoinType(int)", asFUNCTION(StrokeMesh_SetJoinType), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("StrokeMesh", "StrokeMesh& setMiterLimit(float)", asFUNCTION(StrokeMesh_SetMiterLimit), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("StrokeMesh", "StrokeMesh& addVertex(float, float)", asFUNCTION(StrokeMesh_AddVertex_2f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("StrokeMesh", "StrokeMesh& addVertex(float, float, float)", asFUNCTION(StrokeMesh_AddVertex_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("StrokeMesh", "StrokeMesh& addVertex(const Vec2 &in)", asFUNCTION(StrokeMesh_AddVertex_Vec2), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("StrokeMesh", "StrokeMesh& addVertex(const Vec3 &in)", asFUNCTION(StrokeMesh_AddVertex_Vec3), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("StrokeMesh", "StrokeMesh& addVertexWithWidth(float, float, float)", asFUNCTION(StrokeMesh_AddVertexWithWidth_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("StrokeMesh", "StrokeMesh& setShape(Path@)", asFUNCTION(StrokeMesh_SetShape), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("StrokeMesh", "StrokeMesh& setClosed(bool)", asFUNCTION(StrokeMesh_SetClosed), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("StrokeMesh", "StrokeMesh& clear()", asFUNCTION(StrokeMesh_Clear), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("StrokeMesh", "void update()", asFUNCTION(StrokeMesh_Update), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("StrokeMesh", "void draw()", asFUNCTION(StrokeMesh_Draw), asCALL_GENERIC); assert(r >= 0);

    // Image methods
    r = engine_->RegisterGlobalFunction("Image@ createImage()", asFUNCTION(Image_Factory), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Image", "bool load(const string &in)", asFUNCTION(Image_Load), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Image", "bool save(const string &in)", asFUNCTION(Image_Save), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Image", "void allocate(int, int)", asFUNCTION(Image_Allocate_2i), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Image", "void allocate(int, int, int)", asFUNCTION(Image_Allocate_3i), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Image", "void clear()", asFUNCTION(Image_Clear), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Image", "bool isAllocated() const", asFUNCTION(Image_IsAllocated), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Image", "int getWidth() const", asFUNCTION(Image_GetWidth), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Image", "int getHeight() const", asFUNCTION(Image_GetHeight), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Image", "int getChannels() const", asFUNCTION(Image_GetChannels), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Image", "Pixels@ getPixels()", asFUNCTION(Image_GetPixels), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Image", "Color getColor(int, int) const", asFUNCTION(Image_GetColor), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Image", "void setColor(int, int, const Color &in)", asFUNCTION(Image_SetColor), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Image", "void update()", asFUNCTION(Image_Update), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Image", "void setDirty()", asFUNCTION(Image_SetDirty), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Image", "Texture@ getTexture()", asFUNCTION(Image_GetTexture), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Image", "void draw()", asFUNCTION(Image_Draw_0), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Image", "void draw(float, float)", asFUNCTION(Image_Draw_2f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Image", "void draw(float, float, float, float)", asFUNCTION(Image_Draw_4f), asCALL_GENERIC); assert(r >= 0);

    // EasyCam methods
    r = engine_->RegisterGlobalFunction("EasyCam@ createEasyCam()", asFUNCTION(EasyCam_Factory), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("EasyCam", "void begin()", asFUNCTION(EasyCam_Begin), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("EasyCam", "void end()", asFUNCTION(EasyCam_End), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("EasyCam", "void reset()", asFUNCTION(EasyCam_Reset), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("EasyCam", "void setTarget(float, float, float)", asFUNCTION(EasyCam_SetTarget_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("EasyCam", "void setTarget(const Vec3 &in)", asFUNCTION(EasyCam_SetTarget_Vec3), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("EasyCam", "Vec3 getTarget() const", asFUNCTION(EasyCam_GetTarget), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("EasyCam", "void setDistance(float)", asFUNCTION(EasyCam_SetDistance), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("EasyCam", "float getDistance() const", asFUNCTION(EasyCam_GetDistance), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("EasyCam", "void setFov(float)", asFUNCTION(EasyCam_SetFov), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("EasyCam", "float getFov() const", asFUNCTION(EasyCam_GetFov), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("EasyCam", "void setFovDeg(float)", asFUNCTION(EasyCam_SetFovDeg), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("EasyCam", "void setNearClip(float)", asFUNCTION(EasyCam_SetNearClip), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("EasyCam", "void setFarClip(float)", asFUNCTION(EasyCam_SetFarClip), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("EasyCam", "void enableMouseInput()", asFUNCTION(EasyCam_EnableMouseInput), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("EasyCam", "void disableMouseInput()", asFUNCTION(EasyCam_DisableMouseInput), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("EasyCam", "bool isMouseInputEnabled() const", asFUNCTION(EasyCam_IsMouseInputEnabled), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("EasyCam", "void mousePressed(int, int, int)", asFUNCTION(EasyCam_MousePressed), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("EasyCam", "void mouseReleased(int, int, int)", asFUNCTION(EasyCam_MouseReleased), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("EasyCam", "void mouseDragged(int, int, int)", asFUNCTION(EasyCam_MouseDragged), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("EasyCam", "void mouseScrolled(float, float)", asFUNCTION(EasyCam_MouseScrolled), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("EasyCam", "Vec3 getPosition() const", asFUNCTION(EasyCam_GetPosition), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("EasyCam", "void setSensitivity(float)", asFUNCTION(EasyCam_SetSensitivity), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("EasyCam", "void setZoomSensitivity(float)", asFUNCTION(EasyCam_SetZoomSensitivity), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("EasyCam", "void setPanSensitivity(float)", asFUNCTION(EasyCam_SetPanSensitivity), asCALL_GENERIC); assert(r >= 0);

    // Sound methods
    r = engine_->RegisterGlobalFunction("Sound@ createSound()", asFUNCTION(Sound_Factory), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Sound", "bool load(const string &in)", asFUNCTION(Sound_Load), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Sound", "void play()", asFUNCTION(Sound_Play), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Sound", "void stop()", asFUNCTION(Sound_Stop), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Sound", "bool isLoaded() const", asFUNCTION(Sound_IsLoaded), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Sound", "bool isPlaying() const", asFUNCTION(Sound_IsPlaying), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Sound", "void setVolume(float)", asFUNCTION(Sound_SetVolume), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Sound", "void setLoop(bool)", asFUNCTION(Sound_SetLoop), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Sound", "bool isLoop() const", asFUNCTION(Sound_IsLoop), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Sound", "void setPan(float)", asFUNCTION(Sound_SetPan), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Sound", "float getPan() const", asFUNCTION(Sound_GetPan), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Sound", "void setSpeed(float)", asFUNCTION(Sound_SetSpeed), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Sound", "float getSpeed() const", asFUNCTION(Sound_GetSpeed), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Sound", "void pause()", asFUNCTION(Sound_Pause), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Sound", "void resume()", asFUNCTION(Sound_Resume), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Sound", "bool isPaused() const", asFUNCTION(Sound_IsPaused), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Sound", "float getPosition() const", asFUNCTION(Sound_GetPosition), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Sound", "float getDuration() const", asFUNCTION(Sound_GetDuration), asCALL_GENERIC); assert(r >= 0);

    // Wave enum constants
    r = engine_->RegisterEnumValue("Wave", "Sin", kWaveSin); assert(r >= 0);
    r = engine_->RegisterEnumValue("Wave", "Square", kWaveSquare); assert(r >= 0);
    r = engine_->RegisterEnumValue("Wave", "Triangle", kWaveTriangle); assert(r >= 0);
    r = engine_->RegisterEnumValue("Wave", "Sawtooth", kWaveSawtooth); assert(r >= 0);
    r = engine_->RegisterEnumValue("Wave", "Noise", kWaveNoise); assert(r >= 0);
    r = engine_->RegisterEnumValue("Wave", "PinkNoise", kWavePinkNoise); assert(r >= 0);
    r = engine_->RegisterEnumValue("Wave", "Silent", kWaveSilent); assert(r >= 0);

    // ChipSoundNote methods (value type with chaining)
    r = engine_->RegisterObjectMethod("ChipSoundNote", "Sound@ build()", asFUNCTION(ChipNote_Build), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("ChipSoundNote", "ChipSoundNote& wave(Wave)", asFUNCTION(ChipNote_SetWave), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("ChipSoundNote", "ChipSoundNote& hz(float)", asFUNCTION(ChipNote_SetHz), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("ChipSoundNote", "ChipSoundNote& volume(float)", asFUNCTION(ChipNote_SetVolume), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("ChipSoundNote", "ChipSoundNote& duration(float)", asFUNCTION(ChipNote_SetDuration), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("ChipSoundNote", "ChipSoundNote& attack(float)", asFUNCTION(ChipNote_SetAttack), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("ChipSoundNote", "ChipSoundNote& decay(float)", asFUNCTION(ChipNote_SetDecay), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("ChipSoundNote", "ChipSoundNote& sustain(float)", asFUNCTION(ChipNote_SetSustain), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("ChipSoundNote", "ChipSoundNote& release(float)", asFUNCTION(ChipNote_SetRelease), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("ChipSoundNote", "ChipSoundNote& adsr(float, float, float, float)", asFUNCTION(ChipNote_SetADSR), asCALL_GENERIC); assert(r >= 0);

    // ChipSoundBundle methods (reference type)
    r = engine_->RegisterGlobalFunction("ChipSoundBundle@ createChipBundle()", asFUNCTION(ChipBundle_Factory), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("ChipSoundBundle", "ChipSoundBundle& add(const ChipSoundNote &in, float)", asFUNCTION(ChipBundle_Add), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("ChipSoundBundle", "ChipSoundBundle& add(Wave, float, float, float, float)", asFUNCTION(ChipBundle_Add_5), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("ChipSoundBundle", "void clear()", asFUNCTION(ChipBundle_Clear), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("ChipSoundBundle", "float getDuration() const", asFUNCTION(ChipBundle_GetDuration), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("ChipSoundBundle", "ChipSoundBundle& volume(float)", asFUNCTION(ChipBundle_SetVolume), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("ChipSoundBundle", "Sound@ build()", asFUNCTION(ChipBundle_Build), asCALL_GENERIC); assert(r >= 0);

    // Font methods
    r = engine_->RegisterGlobalFunction("Font@ createFont()", asFUNCTION(Font_Factory), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Font", "bool load(const string &in, int)", asFUNCTION(Font_Load), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Font", "bool isLoaded() const", asFUNCTION(Font_IsLoaded), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Font", "void drawString(const string &in, float, float)", asFUNCTION(Font_DrawString_3), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Font", "float getWidth(const string &in) const", asFUNCTION(Font_GetWidth), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Font", "float getHeight(const string &in) const", asFUNCTION(Font_GetHeight), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Font", "float getLineHeight() const", asFUNCTION(Font_GetLineHeight), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Font", "int getSize() const", asFUNCTION(Font_GetSize), asCALL_GENERIC); assert(r >= 0);

    // Font path constants (Web uses CDN URLs)
    r = engine_->RegisterGlobalProperty("const string FONT_SANS", (void*)&fontSans); assert(r >= 0);
    r = engine_->RegisterGlobalProperty("const string FONT_SERIF", (void*)&fontSerif); assert(r >= 0);
    r = engine_->RegisterGlobalProperty("const string FONT_MONO", (void*)&fontMono); assert(r >= 0);

    // =========================================================================
    // Graphics - Clear & Color
    // =========================================================================
    r = engine_->RegisterGlobalFunction("void clear(float)", asFUNCTION(as_clear_1f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void clear(float, float, float)", asFUNCTION(as_clear_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void setColor(float)", asFUNCTION(as_setColor_1f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void setColor(float, float, float)", asFUNCTION(as_setColor_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void setColor(float, float, float, float)", asFUNCTION(as_setColor_4f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void setColorHSB(float, float, float)", asFUNCTION(as_setColorHSB_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void setColorOKLCH(float, float, float)", asFUNCTION(as_setColorOKLCH_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void setColorOKLab(float, float, float)", asFUNCTION(as_setColorOKLab_3f), asCALL_GENERIC); assert(r >= 0);

    // =========================================================================
    // Graphics - Shapes
    // =========================================================================
    r = engine_->RegisterGlobalFunction("void drawRect(float, float, float, float)", asFUNCTION(as_drawRect_4f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void drawCircle(float, float, float)", asFUNCTION(as_drawCircle_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void drawPoint(float, float)", asFUNCTION(as_drawPoint_2f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void drawEllipse(float, float, float, float)", asFUNCTION(as_drawEllipse_4f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void drawLine(float, float, float, float)", asFUNCTION(as_drawLine_4f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void drawTriangle(float, float, float, float, float, float)", asFUNCTION(as_drawTriangle_6f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void drawStroke(float, float, float, float)", asFUNCTION(as_drawStroke_4f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void drawBitmapString(const string &in, float, float)", asFUNCTION(as_drawBitmapString), asCALL_GENERIC); assert(r >= 0);

    // 3D shapes
    r = engine_->RegisterGlobalFunction("void drawBox(float)", asFUNCTION(as_drawBox_1f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void drawBox(float, float, float)", asFUNCTION(as_drawBox_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void drawBox(float, float, float, float)", asFUNCTION(as_drawBox_4f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void drawBox(float, float, float, float, float, float)", asFUNCTION(as_drawBox_6f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void drawSphere(float)", asFUNCTION(as_drawSphere_1f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void drawSphere(float, float, float, float)", asFUNCTION(as_drawSphere_4f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void drawCone(float, float)", asFUNCTION(as_drawCone_2f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void drawCone(float, float, float, float, float)", asFUNCTION(as_drawCone_5f), asCALL_GENERIC); assert(r >= 0);

    // =========================================================================
    // Graphics - Style
    // =========================================================================
    r = engine_->RegisterGlobalFunction("void fill()", asFUNCTION(as_fill), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void noFill()", asFUNCTION(as_noFill), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void setStrokeWeight(float)", asFUNCTION(as_setStrokeWeight_1f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float getStrokeWeight()", asFUNCTION(as_getStrokeWeight), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void setStrokeCap(int)", asFUNCTION(as_setStrokeCap), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("int getStrokeCap()", asFUNCTION(as_getStrokeCap), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void setStrokeJoin(int)", asFUNCTION(as_setStrokeJoin), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("int getStrokeJoin()", asFUNCTION(as_getStrokeJoin), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void setCircleResolution(int)", asFUNCTION(as_setCircleResolution_1i), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("int getCircleResolution()", asFUNCTION(as_getCircleResolution), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("bool isFillEnabled()", asFUNCTION(as_isFillEnabled), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("bool isStrokeEnabled()", asFUNCTION(as_isStrokeEnabled), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void pushStyle()", asFUNCTION(as_pushStyle), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void popStyle()", asFUNCTION(as_popStyle), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Color getColor()", asFUNCTION(as_getColor), asCALL_GENERIC); assert(r >= 0);

    // =========================================================================
    // Shape & Stroke construction
    // =========================================================================
    r = engine_->RegisterGlobalFunction("void beginShape()", asFUNCTION(as_beginShape), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void endShape()", asFUNCTION(as_endShape), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void endShape(bool)", asFUNCTION(as_endShape_bool), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void vertex(float, float)", asFUNCTION(as_vertex_2f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void vertex(float, float, float)", asFUNCTION(as_vertex_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void beginStroke()", asFUNCTION(as_beginStroke), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void endStroke()", asFUNCTION(as_endStroke), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void endStroke(bool)", asFUNCTION(as_endStroke_bool), asCALL_GENERIC); assert(r >= 0);

    // =========================================================================
    // Transform
    // =========================================================================
    r = engine_->RegisterGlobalFunction("void pushMatrix()", asFUNCTION(as_pushMatrix), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void popMatrix()", asFUNCTION(as_popMatrix), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void translate(float, float)", asFUNCTION(as_translate_2f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void translate(float, float, float)", asFUNCTION(as_translate_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void rotate(float)", asFUNCTION(as_rotate_1f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void rotate(float, float, float)", asFUNCTION(as_rotate_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void rotateDeg(float)", asFUNCTION(as_rotateDeg_1f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void rotateDeg(float, float, float)", asFUNCTION(as_rotateDeg_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void rotateX(float)", asFUNCTION(as_rotateX_1f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void rotateY(float)", asFUNCTION(as_rotateY_1f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void rotateZ(float)", asFUNCTION(as_rotateZ_1f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void rotateXDeg(float)", asFUNCTION(as_rotateXDeg_1f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void rotateYDeg(float)", asFUNCTION(as_rotateYDeg_1f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void rotateZDeg(float)", asFUNCTION(as_rotateZDeg_1f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void resetMatrix()", asFUNCTION(as_resetMatrix), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void scale(float)", asFUNCTION(as_scale_1f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void scale(float, float)", asFUNCTION(as_scale_2f), asCALL_GENERIC); assert(r >= 0);

    // =========================================================================
    // Window & Input
    // =========================================================================
    r = engine_->RegisterGlobalFunction("int getWindowWidth()", asFUNCTION(as_getWindowWidth), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("int getWindowHeight()", asFUNCTION(as_getWindowHeight), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float getMouseX()", asFUNCTION(as_getMouseX), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float getMouseY()", asFUNCTION(as_getMouseY), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("bool isMousePressed()", asFUNCTION(as_isMousePressed), asCALL_GENERIC); assert(r >= 0);

    // =========================================================================
    // Time
    // =========================================================================
    r = engine_->RegisterGlobalFunction("float getDeltaTime()", asFUNCTION(as_getDeltaTime), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float getFrameRate()", asFUNCTION(as_getFrameRate), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("int64 getFrameCount()", asFUNCTION(as_getFrameCount), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float getElapsedTimef()", asFUNCTION(as_getElapsedTimef), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float getElapsedTime()", asFUNCTION(as_getElapsedTimef), asCALL_GENERIC); assert(r >= 0);  // alias
    r = engine_->RegisterGlobalFunction("int64 getElapsedTimeMillis()", asFUNCTION(as_getElapsedTimeMillis), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("int64 getElapsedTimeMicros()", asFUNCTION(as_getElapsedTimeMicros), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void resetElapsedTimeCounter()", asFUNCTION(as_resetElapsedTimeCounter), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("int64 getSystemTimeMillis()", asFUNCTION(as_getSystemTimeMillis), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("int64 getSystemTimeMicros()", asFUNCTION(as_getSystemTimeMicros), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("string getTimestampString()", asFUNCTION(as_getTimestampString_0), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("string getTimestampString(const string &in)", asFUNCTION(as_getTimestampString_1), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("int getSeconds()", asFUNCTION(as_getSeconds), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("int getMinutes()", asFUNCTION(as_getMinutes), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("int getHours()", asFUNCTION(as_getHours), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("int getYear()", asFUNCTION(as_getYear), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("int getMonth()", asFUNCTION(as_getMonth), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("int getDay()", asFUNCTION(as_getDay), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("int getWeekday()", asFUNCTION(as_getWeekday), asCALL_GENERIC); assert(r >= 0);

    // =========================================================================
    // Math - Random & Noise
    // =========================================================================
    r = engine_->RegisterGlobalFunction("float random()", asFUNCTION(as_random_0), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float random(float)", asFUNCTION(as_random_1f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float random(float, float)", asFUNCTION(as_random_2f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("int randomInt(int)", asFUNCTION(as_randomInt_1), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("int randomInt(int, int)", asFUNCTION(as_randomInt_2), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void randomSeed(uint)", asFUNCTION(as_randomSeed), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float noise(float)", asFUNCTION(as_noise_1f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float noise(float, float)", asFUNCTION(as_noise_2f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float noise(float, float, float)", asFUNCTION(as_noise_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float signedNoise(float)", asFUNCTION(as_signedNoise_1f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float signedNoise(float, float)", asFUNCTION(as_signedNoise_2f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float signedNoise(float, float, float)", asFUNCTION(as_signedNoise_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float fbm(float, float)", asFUNCTION(as_fbm_2f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float fbm(float, float, int, float, float)", asFUNCTION(as_fbm_5f), asCALL_GENERIC); assert(r >= 0);

    // =========================================================================
    // Math - Interpolation & Trigonometry & General
    // =========================================================================
    r = engine_->RegisterGlobalFunction("float lerp(float, float, float)", asFUNCTION(as_lerp), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float clamp(float, float, float)", asFUNCTION(as_clamp), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float map(float, float, float, float, float)", asFUNCTION(as_map), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float sin(float)", asFUNCTION(as_sin), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float cos(float)", asFUNCTION(as_cos), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float tan(float)", asFUNCTION(as_tan), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float asin(float)", asFUNCTION(as_asin), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float acos(float)", asFUNCTION(as_acos), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float atan(float)", asFUNCTION(as_atan), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float atan2(float, float)", asFUNCTION(as_atan2), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float deg2rad(float)", asFUNCTION(as_deg2rad), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float rad2deg(float)", asFUNCTION(as_rad2deg), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float abs(float)", asFUNCTION(as_abs), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float sqrt(float)", asFUNCTION(as_sqrt), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float sq(float)", asFUNCTION(as_sq), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float pow(float, float)", asFUNCTION(as_pow), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float log(float)", asFUNCTION(as_log), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float exp(float)", asFUNCTION(as_exp), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float min(float, float)", asFUNCTION(as_min), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float max(float, float)", asFUNCTION(as_max), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float floor(float)", asFUNCTION(as_floor), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float ceil(float)", asFUNCTION(as_ceil), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float round(float)", asFUNCTION(as_round), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float fmod(float, float)", asFUNCTION(as_fmod), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float sign(float)", asFUNCTION(as_sign), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float fract(float)", asFUNCTION(as_fract), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float dist(float, float, float, float)", asFUNCTION(as_dist), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float distSquared(float, float, float, float)", asFUNCTION(as_distSquared), asCALL_GENERIC); assert(r >= 0);

    // =========================================================================
    // System & Window
    // =========================================================================
    r = engine_->RegisterGlobalFunction("void toggleFullscreen()", asFUNCTION(as_toggleFullscreen), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void setClipboardString(const string &in)", asFUNCTION(as_setClipboardString), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("string getClipboardString()", asFUNCTION(as_getClipboardString), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void setWindowTitle(const string &in)", asFUNCTION(as_setWindowTitle), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void setWindowSize(int, int)", asFUNCTION(as_setWindowSize), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Vec2 getWindowSize()", asFUNCTION(as_getWindowSize), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Vec2 getMousePos()", asFUNCTION(as_getMousePos), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Vec2 getGlobalMousePos()", asFUNCTION(as_getGlobalMousePos), asCALL_GENERIC); assert(r >= 0);

    // Transform matrix
    r = engine_->RegisterGlobalFunction("Mat4 getCurrentMatrix()", asFUNCTION(as_getCurrentMatrix), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void setMatrix(const Mat4 &in)", asFUNCTION(as_setMatrix), asCALL_GENERIC); assert(r >= 0);

    // Direction enum for text alignment
    r = engine_->RegisterEnum("Direction"); assert(r >= 0);
    r = engine_->RegisterEnumValue("Direction", "Left", 0); assert(r >= 0);
    r = engine_->RegisterEnumValue("Direction", "Center", 1); assert(r >= 0);
    r = engine_->RegisterEnumValue("Direction", "Right", 2); assert(r >= 0);
    r = engine_->RegisterEnumValue("Direction", "Top", 3); assert(r >= 0);
    r = engine_->RegisterEnumValue("Direction", "Bottom", 4); assert(r >= 0);
    r = engine_->RegisterEnumValue("Direction", "Baseline", 5); assert(r >= 0);

    // Text alignment
    r = engine_->RegisterGlobalFunction("void setTextAlign(Direction, Direction)", asFUNCTION(as_setTextAlign), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Direction getTextAlignH()", asFUNCTION(as_getTextAlignH), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Direction getTextAlignV()", asFUNCTION(as_getTextAlignV), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float getBitmapFontHeight()", asFUNCTION(as_getBitmapFontHeight), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float getBitmapStringWidth(const string &in)", asFUNCTION(as_getBitmapStringWidth), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float getBitmapStringHeight(const string &in)", asFUNCTION(as_getBitmapStringHeight), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Rect getBitmapStringBBox(const string &in)", asFUNCTION(as_getBitmapStringBBox), asCALL_GENERIC); assert(r >= 0);

    // Graphics advanced
    r = engine_->RegisterGlobalFunction("void drawMesh(Mesh@)", asFUNCTION(as_drawMesh), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void drawPolyline(Path@)", asFUNCTION(as_drawPolyline), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void drawTexture(Texture@, float, float)", asFUNCTION(as_drawTexture_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void drawTexture(Texture@, float, float, float, float)", asFUNCTION(as_drawTexture_5f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Mesh@ createBox(float)", asFUNCTION(as_createBox_1f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Mesh@ createBox(float, float, float)", asFUNCTION(as_createBox_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Mesh@ createSphere(float)", asFUNCTION(as_createSphere_1f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Mesh@ createSphere(float, int)", asFUNCTION(as_createSphere_2), asCALL_GENERIC); assert(r >= 0);

    // Color static factory functions
    r = engine_->RegisterGlobalFunction("Color Color_fromHex(uint)", asFUNCTION(Color_FromHex_1u), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Color Color_fromHex(uint, bool)", asFUNCTION(Color_FromHex_1u1b), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Color Color_fromBytes(int, int, int)", asFUNCTION(Color_FromBytes_3i), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Color Color_fromBytes(int, int, int, int)", asFUNCTION(Color_FromBytes_4i), asCALL_GENERIC); assert(r >= 0);

    // =========================================================================
    // Utility
    // =========================================================================
    r = engine_->RegisterGlobalFunction("void logNotice(const string &in)", asFUNCTION(as_logNotice), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("string toString(int)", asFUNCTION(as_toString_int), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("string toString(float)", asFUNCTION(as_toString_float), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void beep()", asFUNCTION(as_beep), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void beep(float)", asFUNCTION(as_beep_1f), asCALL_GENERIC); assert(r >= 0);

    // =========================================================================
    // 3D Projection
    // =========================================================================
    r = engine_->RegisterGlobalFunction("void setupScreenPerspective()", asFUNCTION(as_setupScreenPerspective), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void setupScreenPerspective(float)", asFUNCTION(as_setupScreenPerspective_1f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void setupScreenPerspective(float, float, float)", asFUNCTION(as_setupScreenPerspective_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void setupScreenOrtho()", asFUNCTION(as_setupScreenOrtho), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void setupScreenFov(float)", asFUNCTION(as_setupScreenFov_1f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void setupScreenFov(float, float, float)", asFUNCTION(as_setupScreenFov_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("void setDefaultScreenFov(float)", asFUNCTION(as_setDefaultScreenFov), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float getDefaultScreenFov()", asFUNCTION(as_getDefaultScreenFov), asCALL_GENERIC); assert(r >= 0);

    // =========================================================================
    // Constants
    // =========================================================================
    r = engine_->RegisterGlobalProperty("const float TAU", (void*)&TAU); assert(r >= 0);
    r = engine_->RegisterGlobalProperty("const float HALF_TAU", (void*)&kHalfTau); assert(r >= 0);
    r = engine_->RegisterGlobalProperty("const float QUARTER_TAU", (void*)&kQuarterTau); assert(r >= 0);
    r = engine_->RegisterGlobalProperty("const float PI", (void*)&PI); assert(r >= 0);

    // StrokeCap namespace
    engine_->SetDefaultNamespace("StrokeCap");
    r = engine_->RegisterGlobalProperty("const int Butt", (void*)&kStrokeCapButt); assert(r >= 0);
    r = engine_->RegisterGlobalProperty("const int Round", (void*)&kStrokeCapRound); assert(r >= 0);
    r = engine_->RegisterGlobalProperty("const int Square", (void*)&kStrokeCapSquare); assert(r >= 0);
    engine_->SetDefaultNamespace("");

    // StrokeJoin namespace
    engine_->SetDefaultNamespace("StrokeJoin");
    r = engine_->RegisterGlobalProperty("const int Miter", (void*)&kStrokeJoinMiter); assert(r >= 0);
    r = engine_->RegisterGlobalProperty("const int Round", (void*)&kStrokeJoinRound); assert(r >= 0);
    r = engine_->RegisterGlobalProperty("const int Bevel", (void*)&kStrokeJoinBevel); assert(r >= 0);
    engine_->SetDefaultNamespace("");

    // EaseType namespace
    engine_->SetDefaultNamespace("EaseType");
    r = engine_->RegisterGlobalProperty("const int Linear", (void*)&kEaseLinear); assert(r >= 0);
    r = engine_->RegisterGlobalProperty("const int Quad", (void*)&kEaseQuad); assert(r >= 0);
    r = engine_->RegisterGlobalProperty("const int Cubic", (void*)&kEaseCubic); assert(r >= 0);
    r = engine_->RegisterGlobalProperty("const int Quart", (void*)&kEaseQuart); assert(r >= 0);
    r = engine_->RegisterGlobalProperty("const int Quint", (void*)&kEaseQuint); assert(r >= 0);
    r = engine_->RegisterGlobalProperty("const int Sine", (void*)&kEaseSine); assert(r >= 0);
    r = engine_->RegisterGlobalProperty("const int Expo", (void*)&kEaseExpo); assert(r >= 0);
    r = engine_->RegisterGlobalProperty("const int Circ", (void*)&kEaseCirc); assert(r >= 0);
    r = engine_->RegisterGlobalProperty("const int Back", (void*)&kEaseBack); assert(r >= 0);
    r = engine_->RegisterGlobalProperty("const int Elastic", (void*)&kEaseElastic); assert(r >= 0);
    r = engine_->RegisterGlobalProperty("const int Bounce", (void*)&kEaseBounce); assert(r >= 0);
    engine_->SetDefaultNamespace("");

    // EaseMode namespace
    engine_->SetDefaultNamespace("EaseMode");
    r = engine_->RegisterGlobalProperty("const int In", (void*)&kEaseModeIn); assert(r >= 0);
    r = engine_->RegisterGlobalProperty("const int Out", (void*)&kEaseModeOut); assert(r >= 0);
    r = engine_->RegisterGlobalProperty("const int InOut", (void*)&kEaseModeInOut); assert(r >= 0);
    engine_->SetDefaultNamespace("");

    // =========================================================================
    // Easing functions
    // =========================================================================
    r = engine_->RegisterGlobalFunction("float ease(float, int, int)", asFUNCTION(as_ease), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float easeIn(float, int)", asFUNCTION(as_easeIn), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float easeOut(float, int)", asFUNCTION(as_easeOut), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("float easeInOut(float, int)", asFUNCTION(as_easeInOut), asCALL_GENERIC); assert(r >= 0);

    // =========================================================================
    // Tween type (reference type for float animation)
    // =========================================================================
    r = engine_->RegisterObjectType("Tween", 0, asOBJ_REF | asOBJ_NOCOUNT); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Tween@ createTween()", asFUNCTION(TweenFloat_Factory), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Tween", "Tween@ from(float)", asFUNCTION(TweenFloat_From), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Tween", "Tween@ to(float)", asFUNCTION(TweenFloat_To), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Tween", "Tween@ duration(float)", asFUNCTION(TweenFloat_Duration), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Tween", "Tween@ ease(int, int)", asFUNCTION(TweenFloat_Ease), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Tween", "Tween@ ease(int)", asFUNCTION(TweenFloat_Ease_1), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Tween", "Tween@ start()", asFUNCTION(TweenFloat_Start), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Tween", "Tween@ pause()", asFUNCTION(TweenFloat_Pause), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Tween", "Tween@ resume()", asFUNCTION(TweenFloat_Resume), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Tween", "Tween@ reset()", asFUNCTION(TweenFloat_Reset), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Tween", "Tween@ finish()", asFUNCTION(TweenFloat_Finish), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Tween", "void update(float)", asFUNCTION(TweenFloat_Update), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Tween", "float getValue() const", asFUNCTION(TweenFloat_GetValue), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Tween", "float getProgress() const", asFUNCTION(TweenFloat_GetProgress), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Tween", "float getElapsed() const", asFUNCTION(TweenFloat_GetElapsed), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Tween", "float getDuration() const", asFUNCTION(TweenFloat_GetDuration), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Tween", "bool isPlaying() const", asFUNCTION(TweenFloat_IsPlaying), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Tween", "bool isComplete() const", asFUNCTION(TweenFloat_IsComplete), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Tween", "float getStart() const", asFUNCTION(TweenFloat_GetStart), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Tween", "float getEnd() const", asFUNCTION(TweenFloat_GetEnd), asCALL_GENERIC); assert(r >= 0);

    tc::logNotice() << "[AngelScript] Registration complete (" << engine_->GetGlobalFunctionCount() << " global functions, " << engine_->GetObjectTypeCount() << " object types)";
}

bool tcScriptHost::loadScript(const string& code) {
    lastError_.clear();

    // Clean up resources from previous script
    clearScriptResources();

    if (module_) {
        module_->Discard();
        module_ = nullptr;
    }

    setupFunc_ = nullptr;
    updateFunc_ = nullptr;
    drawFunc_ = nullptr;
    mousePressedFunc_ = nullptr;
    mouseReleasedFunc_ = nullptr;
    mouseMovedFunc_ = nullptr;
    mouseDraggedFunc_ = nullptr;
    keyPressedFunc_ = nullptr;
    keyReleasedFunc_ = nullptr;
    windowResizedFunc_ = nullptr;

    module_ = engine_->GetModule("script", asGM_ALWAYS_CREATE);
    if (!module_) {
        lastError_ = "Failed to create script module";
        return false;
    }

    int r = module_->AddScriptSection("main", code.c_str(), code.length());
    if (r < 0) {
        lastError_ = "Failed to add script section";
        return false;
    }

    r = module_->Build();
    if (r < 0) {
        lastError_ = "Script compilation failed";
        return false;
    }

    setupFunc_ = module_->GetFunctionByDecl("void setup()");
    updateFunc_ = module_->GetFunctionByDecl("void update()");
    drawFunc_ = module_->GetFunctionByDecl("void draw()");
    mousePressedFunc_ = module_->GetFunctionByDecl("void mousePressed(float, float, int)");
    mouseReleasedFunc_ = module_->GetFunctionByDecl("void mouseReleased(float, float, int)");
    mouseMovedFunc_ = module_->GetFunctionByDecl("void mouseMoved(float, float)");
    mouseDraggedFunc_ = module_->GetFunctionByDecl("void mouseDragged(float, float, int)");
    keyPressedFunc_ = module_->GetFunctionByDecl("void keyPressed(int)");
    keyReleasedFunc_ = module_->GetFunctionByDecl("void keyReleased(int)");
    windowResizedFunc_ = module_->GetFunctionByDecl("void windowResized(int, int)");

    return true;
}

// =============================================================================
// Multi-file support
// =============================================================================

void tcScriptHost::clearScriptFiles() {
    scriptFiles_.clear();
}

void tcScriptHost::addScriptFile(const string& name, const string& code) {
    scriptFiles_.push_back({name, code});
}

bool tcScriptHost::buildScriptFiles() {
    lastError_.clear();

    // Clean up resources from previous script
    clearScriptResources();

    if (module_) {
        module_->Discard();
        module_ = nullptr;
    }

    setupFunc_ = nullptr;
    updateFunc_ = nullptr;
    drawFunc_ = nullptr;
    mousePressedFunc_ = nullptr;
    mouseReleasedFunc_ = nullptr;
    mouseMovedFunc_ = nullptr;
    mouseDraggedFunc_ = nullptr;
    keyPressedFunc_ = nullptr;
    keyReleasedFunc_ = nullptr;
    windowResizedFunc_ = nullptr;

    module_ = engine_->GetModule("script", asGM_ALWAYS_CREATE);
    if (!module_) {
        lastError_ = "Failed to create script module";
        return false;
    }

    // Add each file as a section
    for (const auto& [name, code] : scriptFiles_) {
        int r = module_->AddScriptSection(name.c_str(), code.c_str(), code.length());
        if (r < 0) {
            lastError_ = "Failed to add script section: " + name;
            return false;
        }
    }

    int r = module_->Build();
    if (r < 0) {
        // Error already captured by message callback
        if (lastError_.empty()) {
            lastError_ = "Script compilation failed";
        }
        return false;
    }

    // Bind lifecycle functions
    setupFunc_ = module_->GetFunctionByDecl("void setup()");
    updateFunc_ = module_->GetFunctionByDecl("void update()");
    drawFunc_ = module_->GetFunctionByDecl("void draw()");
    mousePressedFunc_ = module_->GetFunctionByDecl("void mousePressed(float, float, int)");
    mouseReleasedFunc_ = module_->GetFunctionByDecl("void mouseReleased(float, float, int)");
    mouseMovedFunc_ = module_->GetFunctionByDecl("void mouseMoved(float, float)");
    mouseDraggedFunc_ = module_->GetFunctionByDecl("void mouseDragged(float, float, int)");
    keyPressedFunc_ = module_->GetFunctionByDecl("void keyPressed(int)");
    keyReleasedFunc_ = module_->GetFunctionByDecl("void keyReleased(int)");
    windowResizedFunc_ = module_->GetFunctionByDecl("void windowResized(int, int)");

    return true;
}

void tcScriptHost::callSetup() {
    if (!setupFunc_ || !ctx_) return;
    ctx_->Prepare(setupFunc_);
    int r = ctx_->Execute();
    if (r != asEXECUTION_FINISHED && r == asEXECUTION_EXCEPTION) {
        lastError_ = string("Exception in setup(): ") + ctx_->GetExceptionString();
    }
}

void tcScriptHost::callUpdate() {
    if (!updateFunc_ || !ctx_) return;
    ctx_->Prepare(updateFunc_);
    int r = ctx_->Execute();
    if (r != asEXECUTION_FINISHED && r == asEXECUTION_EXCEPTION) {
        lastError_ = string("Exception in update(): ") + ctx_->GetExceptionString();
    }
}

void tcScriptHost::callDraw() {
    if (!drawFunc_ || !ctx_) return;
    ctx_->Prepare(drawFunc_);
    int r = ctx_->Execute();
    if (r != asEXECUTION_FINISHED && r == asEXECUTION_EXCEPTION) {
        lastError_ = string("Exception in draw(): ") + ctx_->GetExceptionString();
    }
}

void tcScriptHost::callMousePressed(float x, float y, int button) {
    if (!mousePressedFunc_ || !ctx_) return;
    ctx_->Prepare(mousePressedFunc_);
    ctx_->SetArgFloat(0, x);
    ctx_->SetArgFloat(1, y);
    ctx_->SetArgDWord(2, button);
    ctx_->Execute();
}

void tcScriptHost::callMouseReleased(float x, float y, int button) {
    if (!mouseReleasedFunc_ || !ctx_) return;
    ctx_->Prepare(mouseReleasedFunc_);
    ctx_->SetArgFloat(0, x);
    ctx_->SetArgFloat(1, y);
    ctx_->SetArgDWord(2, button);
    ctx_->Execute();
}

void tcScriptHost::callMouseMoved(float x, float y) {
    if (!mouseMovedFunc_ || !ctx_) return;
    ctx_->Prepare(mouseMovedFunc_);
    ctx_->SetArgFloat(0, x);
    ctx_->SetArgFloat(1, y);
    ctx_->Execute();
}

void tcScriptHost::callMouseDragged(float x, float y, int button) {
    if (!mouseDraggedFunc_ || !ctx_) return;
    ctx_->Prepare(mouseDraggedFunc_);
    ctx_->SetArgFloat(0, x);
    ctx_->SetArgFloat(1, y);
    ctx_->SetArgDWord(2, button);
    ctx_->Execute();
}

void tcScriptHost::callKeyPressed(int key) {
    if (!keyPressedFunc_ || !ctx_) return;
    ctx_->Prepare(keyPressedFunc_);
    ctx_->SetArgDWord(0, key);
    ctx_->Execute();
}

void tcScriptHost::callKeyReleased(int key) {
    if (!keyReleasedFunc_ || !ctx_) return;
    ctx_->Prepare(keyReleasedFunc_);
    ctx_->SetArgDWord(0, key);
    ctx_->Execute();
}

void tcScriptHost::callWindowResized(int width, int height) {
    if (!windowResizedFunc_ || !ctx_) return;
    ctx_->Prepare(windowResizedFunc_);
    ctx_->SetArgDWord(0, width);
    ctx_->SetArgDWord(1, height);
    ctx_->Execute();
}

void tcScriptHost::appendError(const string& section, int row, int col, const string& message) {
    // Format: "section (row, col) : message" - parseable by JS
    string errorLine = section + " (" + to_string(row) + ", " + to_string(col) + ") : " + message;

    if (lastError_.empty()) {
        lastError_ = errorLine;
    } else {
        lastError_ += "\n" + errorLine;
    }
}
