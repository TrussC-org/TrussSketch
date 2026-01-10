#include "tcScriptHost.h"
#include <scriptstdstring/scriptstdstring.h>
#include <cmath>
#include <vector>
#include <memory>

// Global containers for reference types (cleaned up on script reload)
static vector<unique_ptr<Texture>> g_textures;
static vector<unique_ptr<Fbo>> g_fbos;
static vector<unique_ptr<Pixels>> g_pixels;
static vector<unique_ptr<Sound>> g_sounds;
static vector<unique_ptr<Font>> g_fonts;

static void clearScriptResources() {
    g_textures.clear();
    g_fbos.clear();
    g_pixels.clear();
    g_sounds.clear();
    g_fonts.clear();
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
    registerTrussCFunctions();
    ctx_ = engine_->CreateContext();
}

tcScriptHost::~tcScriptHost() {
    if (ctx_) ctx_->Release();
    if (engine_) engine_->ShutDownAndRelease();
}

void tcScriptHost::registerTrussCFunctions() {
    int r;

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
    r = engine_->RegisterGlobalFunction("Color Color_fromHSB(float, float, float)", asFUNCTION(Color_fromHSB_3f), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterGlobalFunction("Color Color_fromHSB(float, float, float, float)", asFUNCTION(Color_fromHSB_4f), asCALL_GENERIC); assert(r >= 0);
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

    // Sound methods
    r = engine_->RegisterGlobalFunction("Sound@ createSound()", asFUNCTION(Sound_Factory), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Sound", "bool load(const string &in)", asFUNCTION(Sound_Load), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Sound", "void play()", asFUNCTION(Sound_Play), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Sound", "void stop()", asFUNCTION(Sound_Stop), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Sound", "bool isLoaded() const", asFUNCTION(Sound_IsLoaded), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Sound", "bool isPlaying() const", asFUNCTION(Sound_IsPlaying), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Sound", "void setVolume(float)", asFUNCTION(Sound_SetVolume), asCALL_GENERIC); assert(r >= 0);
    r = engine_->RegisterObjectMethod("Sound", "void setLoop(bool)", asFUNCTION(Sound_SetLoop), asCALL_GENERIC); assert(r >= 0);

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
