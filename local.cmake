# Project-specific CMake — loaded by trussc_app(); SURVIVES `trusscli update`
# (unlike CMakeLists.txt, which trusscli regenerates).
#
# TrussSketch needs JS<->C interop the default web build doesn't enable:
#   - ccall/cwrap  : the editor drives the engine via Module.ccall('buildScriptFiles', ...)
#   - FS           : sketch-ui.js mounts user assets into the Emscripten filesystem
# The C entry points (updateScriptCode / buildScriptFiles / ...) are already kept
# by EMSCRIPTEN_KEEPALIVE, so no EXPORTED_FUNCTIONS list is needed.
if(EMSCRIPTEN)
    target_link_options(${PROJECT_NAME} PRIVATE
        -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','FS']
        -sFORCE_FILESYSTEM=1
        # emscripten 6.x defaults to resizable ArrayBuffers for memory growth,
        # and Chrome's TextDecoder rejects views over a resizable buffer (throws
        # in UTF8ArrayToString via the wgpu string path, every frame). Disable.
        -sGROWABLE_ARRAYBUFFERS=0
    )
endif()
