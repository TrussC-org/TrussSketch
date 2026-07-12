/*
 * TrussSketch embed loader
 * =========================
 * A tiny vanilla loader that boots the TrussSketch (Lua) WASM engine into a
 * <canvas> on any page and runs a sketch. No build step, no ES modules.
 *
 * Usage (production, version-pinned):
 *
 *     <canvas id="canvas" width="600" height="600"></canvas>
 *     <script src="https://cdn.trussc.org/sketch@0.6.5.js"></script>
 *     <script>
 *       TrussSketch.mount('#canvas', `
 *         function draw()
 *           clear(0.1)
 *           setColor(0.3, 0.6, 0.95)
 *           drawCircle(getMouseX(), getMouseY(), 40)
 *         end
 *       `).then(function (sketch) {
 *         // sketch.run(code) / sketch.pause() / sketch.resume() / sketch.getError()
 *       });
 *     </script>
 *
 * mount(target, code, opts) -> Promise<controller>
 *   target : a <canvas> element or a CSS selector that resolves to one.
 *   code   : Lua source string (optional if you pass opts.files).
 *   opts   : {
 *     cdnBase : base URL the engine (.js/.wasm/.data) is served from.
 *               Default: this loader's own version, i.e.
 *               'https://cdn.trussc.org/<VERSION>/'.
 *     files   : { "name.lua": "code", ... } extra Lua files loaded before `code`.
 *     autorun : run the sketch immediately once ready (default true).
 *     onError : fn(message) — called once per distinct error (build + runtime).
 *     onReady : fn(controller) — called when the engine is ready.
 *   }
 *
 * controller:
 *   run(codeOrFiles) -> errorString ('' on success). Pass a Lua string or a
 *                       { name: code } map. Rebuilds and hot-swaps the sketch.
 *   pause()          -> pause the engine (stops update/draw, saves power).
 *   resume()         -> resume the engine.
 *   getError()       -> current build/runtime error string ('' if none).
 *
 * SINGLE-INSTANCE CAVEAT
 * ----------------------
 * The engine is a plain (non-MODULARIZE) Emscripten build: there is exactly ONE
 * global `Module` and ONE engine instance per page. Therefore mount() may be
 * called only ONCE per page load — a second call rejects. To run a different
 * sketch, either use controller.run(...) to swap the code in place, or reload
 * the page. You cannot host two independent TrussSketch canvases on one page.
 *
 * VERSION PINNING
 * ---------------
 * Always embed a pinned version (sketch@0.6.5.js). The `sketch@latest.js` alias
 * exists for quick testing only and can change/break at any time — never point
 * production at @latest.
 */
(function (global) {
    'use strict';

    // Substituted by tools/deploy_cdn.sh at upload time (per version).
    var VERSION = '__TRUSSKETCH_VERSION__';

    // Non-MODULARIZE engine => one instance per page. Guards a second mount().
    var mounted = false;

    // Resolve `target` to a <canvas>, assigning id="canvas" when needed so the
    // sokol shell's baked-in "#canvas" selector can also find it. (The engine
    // additionally routes through Module.canvas, which we set directly, but the
    // id keeps the selector fallback honest.)
    function resolveCanvas(target) {
        var el = target;
        if (typeof target === 'string') {
            el = document.querySelector(target);
            if (!el) {
                throw new Error('TrussSketch.mount: no element matches selector "' + target + '"');
            }
        }
        if (!el || el.tagName !== 'CANVAS') {
            throw new Error('TrussSketch.mount: target must be a <canvas> element or a CSS selector for one');
        }
        if (!el.id) {
            var existing = document.getElementById('canvas');
            if (existing && existing !== el) {
                throw new Error(
                    'TrussSketch.mount: cannot assign id="canvas" to the target because a ' +
                    'different element already owns id="canvas". Give your <canvas> id="canvas" ' +
                    'explicitly (and remove it from the other element), then mount again.');
            }
            el.id = 'canvas';
        }
        return el;
    }

    // Normalize a code string OR a {name: code} map into an ordered pair list.
    function normalizeFiles(codeOrFiles) {
        if (typeof codeOrFiles === 'string') {
            return [['main.lua', codeOrFiles]];
        }
        var pairs = [];
        if (codeOrFiles && typeof codeOrFiles === 'object') {
            for (var k in codeOrFiles) {
                if (Object.prototype.hasOwnProperty.call(codeOrFiles, k)) {
                    pairs.push([k, String(codeOrFiles[k])]);
                }
            }
        }
        return pairs;
    }

    // Inject the engine <script>. Module must already be on window.
    function loadEngineScript(cdnBase) {
        return new Promise(function (resolve, reject) {
            var s = document.createElement('script');
            s.async = true;
            s.src = cdnBase + 'TrussSketch.js';
            s.onload = function () { resolve(); };
            s.onerror = function () {
                reject(new Error('TrussSketch.mount: failed to load engine script ' + s.src));
            };
            (document.body || document.head || document.documentElement).appendChild(s);
        });
    }

    // Poll isAppReady() until the C++ app is fully constructed (g_app != null).
    // With WebGPU + ASYNCIFY main() finishes asynchronously after runtime init,
    // so a simple onRuntimeInitialized is not enough.
    function waitForReady(timeoutMs) {
        return new Promise(function (resolve, reject) {
            var start = Date.now();
            (function poll() {
                var M = global.Module;
                try {
                    if (M && M.ccall && M.ccall('isAppReady', 'number', [], [])) {
                        resolve();
                        return;
                    }
                } catch (e) {
                    // Runtime not ready yet; ccall throws. Keep polling.
                }
                if (Date.now() - start > timeoutMs) {
                    reject(new Error('TrussSketch.mount: engine did not become ready within ' +
                        timeoutMs + 'ms (check cdnBase / network / that the .wasm and .data URLs resolve)'));
                    return;
                }
                setTimeout(poll, 50);
            })();
        });
    }

    var READY_TIMEOUT_MS = 30000;
    var RUNTIME_POLL_MS = 500;

    function mount(target, code, opts) {
        opts = opts || {};

        if (mounted) {
            return Promise.reject(new Error(
                'TrussSketch.mount: already mounted. The engine is a single global instance ' +
                '(non-MODULARIZE Emscripten build), so mount() can run only once per page. ' +
                'Use controller.run(code) to swap the sketch, or reload the page.'));
        }

        // Validate the canvas BEFORE committing `mounted`, so a bad target can
        // be corrected and retried.
        var canvas;
        try {
            canvas = resolveCanvas(target);
        } catch (e) {
            return Promise.reject(e);
        }
        mounted = true;

        // Resolve the CDN base (with trailing slash).
        var cdnBase = opts.cdnBase || ('https://cdn.trussc.org/' + VERSION + '/');
        if (cdnBase.charAt(cdnBase.length - 1) !== '/') {
            cdnBase += '/';
        }

        // Set up the global Emscripten Module BEFORE the engine script runs.
        var Module = {
            canvas: canvas,
            // Required so direct keyboard input reaches the sketch.
            keyboardListeningElement: canvas,
            // Map engine-relative assets (TrussSketch.wasm / .data) onto cdnBase.
            locateFile: function (path) {
                return cdnBase + path;
            },
            print: function (text) {
                try { console.log('[TrussSketch] ' + text); } catch (e) {}
            },
            printErr: function (text) {
                try { console.warn('[TrussSketch] ' + text); } catch (e) {}
            }
        };
        global.Module = Module;

        // Single dedupe channel for build + runtime errors: same message fires
        // onError at most once in a row.
        var lastReportedError = '';
        function reportError(msg) {
            if (msg && msg !== lastReportedError) {
                lastReportedError = msg;
                if (typeof opts.onError === 'function') {
                    try { opts.onError(msg); } catch (e) {}
                }
            }
        }

        function run(codeOrFiles) {
            if (!Module.ccall) {
                return 'engine not ready';
            }
            var pairs = normalizeFiles(codeOrFiles);
            var err = '';
            try {
                Module.ccall('resumeEngine', null, [], []);
                Module.ccall('clearScriptFiles', null, [], []);
                for (var i = 0; i < pairs.length; i++) {
                    Module.ccall('addScriptFile', null, ['string', 'string'], [pairs[i][0], pairs[i][1]]);
                }
                // buildScriptFiles returns the error string directly ('' on ok);
                // fall back to getScriptError for safety.
                err = Module.ccall('buildScriptFiles', 'string', [], []) || '';
                if (!err) {
                    err = Module.ccall('getScriptError', 'string', [], []) || '';
                }
            } catch (e) {
                err = (e && e.message) ? e.message : String(e);
            }
            if (err) {
                reportError(err);
            }
            return err;
        }

        function pause() {
            try { Module.ccall('pauseEngine', null, [], []); } catch (e) {}
        }

        function resume() {
            try { Module.ccall('resumeEngine', null, [], []); } catch (e) {}
        }

        function getError() {
            try {
                return Module.ccall('getRuntimeError', 'string', [], []) ||
                       Module.ccall('getScriptError', 'string', [], []) || '';
            } catch (e) {
                return '';
            }
        }

        var controller = {
            run: run,
            pause: pause,
            resume: resume,
            getError: getError
        };

        return loadEngineScript(cdnBase)
            .then(function () {
                return waitForReady(READY_TIMEOUT_MS);
            })
            .then(function () {
                // Poll for errors raised inside per-frame callbacks (draw/update/
                // events); these only exist at runtime, so without polling a
                // nil-crash in draw() would be invisible.
                setInterval(function () {
                    if (!Module.ccall) return;
                    try {
                        var err = Module.ccall('getRuntimeError', 'string', [], []);
                        if (err) reportError(err);
                    } catch (e) {
                        // engine mid-reload; try again next tick
                    }
                }, RUNTIME_POLL_MS);

                // Initial run: extra files first, main `code` last (so helpers
                // are defined before the main chunk uses them).
                if (opts.autorun !== false) {
                    var initial;
                    if (opts.files && typeof opts.files === 'object') {
                        initial = {};
                        for (var k in opts.files) {
                            if (Object.prototype.hasOwnProperty.call(opts.files, k)) {
                                initial[k] = opts.files[k];
                            }
                        }
                        if (typeof code === 'string') {
                            initial['main.lua'] = code;
                        }
                        run(initial);
                    } else if (typeof code === 'string') {
                        run(code);
                    }
                }

                if (typeof opts.onReady === 'function') {
                    try { opts.onReady(controller); } catch (e) {}
                }
                return controller;
            });
    }

    global.TrussSketch = {
        version: VERSION,
        mount: mount
    };

})(typeof window !== 'undefined' ? window : this);
