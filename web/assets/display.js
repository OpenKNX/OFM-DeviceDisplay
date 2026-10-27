// Live view of the OLED framebuffer, virtual joystick and widget/settings editors. The device sends the
// raw SSD1306 buffer (1bpp, 8 pixels/byte vertically); unpacking happens here so the firmware stays a memcpy.
(function () {
    const $ = function (id) { return document.getElementById(id); };
    const canvas = $('dsp-canvas');
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    const status = $('dsp-status');
    const rate = $('dsp-rate');
    const settings = $('dsp-settings');
    const offNote = $('dsp-off');
    const offTitle = $('dsp-off-title');
    const offDetail = $('dsp-off-detail');
    const widgets = $('dsp-widgets');

    // Lit pixel = the OpenKNX green (#449841, same value as logo.svg/favicon.svg/base.css);
    // unlit = a near-black tint of the same hue, so the panel reads as one unlit surface.
    const ON = [0x44, 0x98, 0x41], OFF = [0x07, 0x14, 0x07];
    let invert = 0; // preview only - does not touch the panel or the BMP export
    let timer = null, busy = false;
    let streaming = rate.value === 'live';
    let fpsAt = 0, fpsFrames = 0;
    let displayOff = false;
    let lastFrame = null;
    let offNoteState = '';

    function setStatus(text, cls) {
        status.textContent = text;
        status.className = 'dsp-status' + (cls ? ' ' + cls : '');
    }

    function clearCanvas(w, h) {
        ctx.fillStyle = '#071407';
        ctx.fillRect(0, 0, w, h);
    }

    function showOffNote() {
        const state = rate.value === '0' ? 'stopped' : displayOff ? 'display-off' : 'on';
        if (state === offNoteState) return;
        offNoteState = state;
        offNote.hidden = state === 'on';
        if (state === 'stopped') {
            offTitle.textContent = 'Live-Übertragung aus';
            offDetail.textContent = 'Aktualisierung auswählen oder „Jetzt“ drücken';
            setStatus('Übertragung aus', 'err');
        } else if (state === 'display-off') {
            offTitle.textContent = 'Display ist aus';
            offDetail.textContent = 'Taste drücken zum Wecken';
        }
    }

    // ONE request from this page at a time. Every response is Connection: close and the RP2040
    // server has three connection slots, so overlapping fetches make it abort accepts.
    let chain = Promise.resolve();
    function serial(fn) {
        const run = chain.then(fn, fn);
        chain = run.catch(function () {});
        return run;
    }

    function post(url) { return serial(function () { return fetch(url, { method: 'POST' }); }); }

    // ── Framebuffer ─────────────────────────────────────────────────────────
    function render(bytes, w, h) {
        lastFrame = bytes;
        if (canvas.width !== w || canvas.height !== h) { canvas.width = w; canvas.height = h; }
        const img = ctx.createImageData(w, h), px = img.data;
        for (let y = 0; y < h; y++) {
            const page = (y >> 3) * w, bit = y & 7;
            for (let x = 0; x < w; x++) {
                const c = (((bytes[page + x] >> bit) & 1) ^ invert) ? ON : OFF, i = (y * w + x) * 4;
                px[i] = c[0]; px[i + 1] = c[1]; px[i + 2] = c[2]; px[i + 3] = 255;
            }
        }
        ctx.putImageData(img, 0, 0);
    }

    function redrawPreview() {
        if (!lastFrame) return;
        render(lastFrame, canvas.width, canvas.height);
        if (displayOff || rate.value === '0') clearCanvas(canvas.width, canvas.height);
    }

    function refresh(manual) {
        if (busy || document.hidden || (!manual && streaming && window.OpenKNX && window.OpenKNX.connected)) return;
        busy = true;
        return serial(async function () {
            try {
                const res = await fetch('/display/fb.bin', { cache: 'no-store' });
                if (!res.ok) throw 0;
                const size = (res.headers.get('X-Display-Size') || '128x64').split('x');
                const w = parseInt(size[0], 10) || 128, h = parseInt(size[1], 10) || 64;
                const bytes = new Uint8Array(await res.arrayBuffer());
                if (bytes.length < (w / 8) * h) throw 0;
                render(bytes, w, h);
                // The buffer keeps stale content while the panel sleeps -> blank the canvas to the unlit
                // colour and put the notice on top as DOM text (canvas text turns to mush when scaled up).
                displayOff = res.headers.get('X-Display-On') === '0';
                if (displayOff || rate.value === '0') clearCanvas(w, h);
                showOffNote();
                if (rate.value !== '0') setStatus(displayOff ? 'Standby' : 'live', displayOff ? 'err' : 'ok');
            } catch (e) {
                setStatus('kein Bild', 'err');
            } finally { busy = false; }
        });
    }

    function reschedule() {
        if (timer) clearInterval(timer);
        timer = null;
        const ms = rate.value === 'live' ? 1000 : +rate.value;
        if (ms > 0) timer = setInterval(function () { refresh(); }, ms);
    }

    function syncStream() {
        if (window.OpenKNX)
            window.OpenKNX.send('dspStream', !document.hidden && streaming ? '1' : '0');
    }

    function receiveFrame(data) {
        if (!streaming) return;
        if (!(data instanceof Uint8Array) || data.length < 4) return;
        const w = data[0] * 8, h = data[1], isOff = data[2] === 0;
        if (!w || w > 128 || !h || h > 64 || h % 8 || data.length !== 3 + w / 8 * h) return;
        render(data.subarray(3), w, h);
        displayOff = isOff;
        const now = Date.now();
        if (!fpsAt) fpsAt = now;
        fpsFrames++;
        if (now - fpsAt >= 1000) {
            const fps = Math.round(fpsFrames * 1000 / (now - fpsAt));
            fpsAt = now;
            fpsFrames = 0;
            setStatus((isOff ? 'Standby' : 'Live') + ' · ' + fps + ' Bilder/s', isOff ? 'err' : 'ok');
        }
        if (isOff) clearCanvas(w, h);
        showOffNote();
    }

    // ── Joystick ────────────────────────────────────────────────────────────
    // A hold sends press+release separately so the device's hold-to-confirm arms; a short click sends one
    // "tap" (one connection instead of two, keeping a click storm inside the server's three slots).
    const HOLD_MS = 250;
    let held = null, holdTimer = null, pressSent = false, kickTimer = null;

    function down(e) {
        if (held !== null) return;
        held = e.currentTarget.dataset.k;
        pressSent = false;
        holdTimer = setTimeout(function () {
            pressSent = true;
            post('/display/key?k=' + held + '&a=0');
        }, HOLD_MS);
        e.preventDefault();
    }

    function up() {
        if (held === null) return;
        const k = held;
        held = null;
        clearTimeout(holdTimer);
        post('/display/key?k=' + k + '&a=' + (pressSent ? 1 : 2));
        // One refresh after the burst, not one per click.
        clearTimeout(kickTimer);
        kickTimer = setTimeout(refresh, 200);
    }
    document.querySelectorAll('.dsp-pad button').forEach(function (b) {
        b.addEventListener('pointerdown', down);
        b.addEventListener('pointerup', up);
        b.addEventListener('pointercancel', up);
        b.addEventListener('pointerleave', up);
    });

    // ── Widgets ─────────────────────────────────────────────────────────────
    function renderWidgets(cfg) {
        const list = cfg.w || [], durations = cfg.d || [];
        widgets.textContent = '';
        if (!list.length) { widgets.textContent = 'Keine Widgets.'; return; }

        const table = document.createElement('table');
        const body = table.createTBody();

        list.forEach(function (w, i) {

            const on = document.createElement('input');
            on.type = 'checkbox';
            on.checked = !!w.e;
            on.title = 'Anzeigen';
            on.addEventListener('change', function () {
                post('/display/widget?i=' + i + '&e=' + (on.checked ? 1 : 0)).then(loadWidgets);
            });

            const dur = document.createElement('select');
            dur.title = 'Anzeigedauer';
            // A widget may carry a duration the menu never offers (its constructor sets it). Show that
            // value as its own entry instead of rendering an empty select that hides the real state.
            const opts = durations.indexOf(w.s) < 0 ? [w.s].concat(durations) : durations;
            opts.forEach(function (s) {
                const o = document.createElement('option');
                o.value = s; o.textContent = s + ' s';
                dur.appendChild(o);
            });
            dur.value = w.s;
            dur.addEventListener('change', function () {
                post('/display/widget?i=' + i + '&s=' + dur.value).then(loadWidgets);
            });

            const row = body.insertRow();
            const nameCell = row.insertCell();
            nameCell.textContent = w.n;
            if (!w.e) nameCell.className = 'gray';
            row.insertCell().appendChild(on);
            row.insertCell().appendChild(dur);

            const moveCell = row.insertCell();
            [-1, 1].forEach(function (d) {
                const b = document.createElement('button');
                b.type = 'button';
                b.className = 'dsp-mv';
                b.textContent = d < 0 ? '▲' : '▼';
                b.disabled = i + d < 0 || i + d >= list.length;
                b.addEventListener('click', function () {
                    post('/display/widget?i=' + i + '&m=' + d).then(loadWidgets);
                });
                moveCell.appendChild(b);
            });
        });

        widgets.appendChild(table);
    }

    function loadWidgets() {
        return serial(async function () {
            try {
                const res = await fetch('/display/widgets', { cache: 'no-store' });
                if (!res.ok) throw 0;
                renderWidgets(await res.json());
            } catch (e) { widgets.textContent = 'Widgets nicht verfügbar.'; }
        });
    }

    // ── Settings ────────────────────────────────────────────────────────────
    // Coalesced writes: a dragging range input fires "change" per step, and each POST is a full apply.
    // Only the last value of a burst is sent, then the list is re-read (the device clamps and wins).
    const pendingSave = {};
    let saveTimer = null;

    function queueSave(id, value) {
        pendingSave[id] = value;
        clearTimeout(saveTimer);
        saveTimer = setTimeout(flushSave, 300);
    }

    async function flushSave() {
        for (const id of Object.keys(pendingSave)) {
            const value = pendingSave[id];
            delete pendingSave[id];
            try {
                const r = await post('/display/config?id=' + encodeURIComponent(id) +
                    '&value=' + encodeURIComponent(value));
                if (!r.ok) throw 0;
            } catch (e) { setStatus('Speichern fehlgeschlagen', 'err'); }
        }
        loadSettings();
    }

    function control(def) {
        let el;
        if (def.kind === 0) {
            el = document.createElement('input');
            el.type = 'checkbox';
            el.checked = def.value !== 0;
        } else if (def.kind === 2) {
            el = document.createElement('select');
            (def.options || []).forEach(function (label, i) {
                const o = document.createElement('option');
                o.value = i; o.textContent = label;
                el.appendChild(o);
            });
            el.value = def.value;
        } else {
            el = document.createElement('input');
            el.type = (def.max - def.min <= 10) ? 'range' : 'number';
            el.min = def.min; el.max = def.max; el.value = def.value;
        }
        return el;
    }

    // Builds one row per definition into the given tbody.
    function settingRow(def, body) {
        const el = control(def);
        el.id = 'set-' + def.id;
        el.addEventListener('change', function () {
            const value = def.kind === 0 ? (el.checked ? 1 : 0) : el.value;
            if (def.id === 'invert') {
                invert = value ? 1 : 0;
                redrawPreview();
            }
            queueSave(def.id, value);
        });

        const row = body.insertRow();
        const label = document.createElement('label');
        label.textContent = def.label;
        label.htmlFor = el.id;
        row.insertCell().appendChild(label);
        row.insertCell().appendChild(el);

        const hint = row.insertCell();
        hint.className = 'gray';
        hint.textContent = def.kind === 1 ? (def.hint || '') + ' (' + def.value + ')' : (def.hint || '');
    }

    function sectionTable(list, group) {
        const t = document.createElement('table');
        t.className = 'attribute-table';
        const body = t.createTBody();
        for (const d of list) if ((d.g || 0) === group) settingRow(d, body);
        return t;
    }

    function renderSettings(list) {
        const invertSetting = list.find(function (d) { return d.id === 'invert'; });
        if (invertSetting) {
            invert = invertSetting.value ? 1 : 0;
            redrawPreview();
        }
        settings.textContent = '';
        settings.appendChild(sectionTable(list, 0));

        const h = document.createElement('h2');
        h.textContent = 'Experten';
        settings.appendChild(h);
        const note = document.createElement('p');
        note.className = 'meta';
        note.textContent = 'Panel-Timing und Screenshot-Details. Nur ändern, wenn du weißt warum - '
            + 'ein schlechter Wert zeigt sich nur am Gerät.';
        settings.appendChild(note);
        settings.appendChild(sectionTable(list, 1));

        // The hold actions live in the Bedienung tab, right next to the keys they belong to.
        const keys = $('dsp-keys');
        if (keys) {
            keys.textContent = '';
            const cap = document.createElement('div');
            cap.className = 'gray';
            cap.textContent = 'Halte-Aktionen';
            keys.appendChild(cap);
            keys.appendChild(sectionTable(list, 2));
        }
    }

    function loadSettings() {
        return serial(async function () {
            try {
                const res = await fetch('/display/config', { cache: 'no-store' });
                if (!res.ok) throw 0;
                renderSettings((await res.json()).settings || []);
            } catch (e) { settings.textContent = 'Einstellungen nicht verfügbar.'; }
        });
    }

    // ── Tabs ────────────────────────────────────────────────────────────────
    const tabs = document.querySelectorAll('.dsp-tab');
    tabs.forEach(function (t) {
        t.addEventListener('click', function () {
            tabs.forEach(function (o) {
                const on = (o === t);
                o.classList.toggle('active', on);
                $('t-' + o.dataset.t).hidden = !on;
            });
        });
    });

    rate.addEventListener('change', function () {
        streaming = this.value === 'live';
        fpsAt = fpsFrames = 0;
        reschedule();
        syncStream();
        showOffNote();
        if (this.value === '0') clearCanvas(canvas.width, canvas.height);
        if (!streaming || !window.OpenKNX || !window.OpenKNX.connected) refresh(true);
    });
    $('dsp-now').addEventListener('click', function () { refresh(true); });
    document.addEventListener('visibilitychange', function () {
        syncStream();
        if (!document.hidden && (!streaming || !window.OpenKNX || !window.OpenKNX.connected)) refresh(true);
    });
    window.addEventListener('pagehide', function () {
        if (window.OpenKNX) window.OpenKNX.send('dspStream', '0');
    });

    function registerSocketHandlers() {
        if (!window.OpenKNX) return;
        window.OpenKNX.on('dspFrame', receiveFrame);
        window.OpenKNX.onState(function (state) {
            if (state === 'open') syncStream();
            else if (streaming && !document.hidden) refresh(true);
        });
        syncStream();
    }
    if (window.OpenKNX) registerSocketHandlers();
    else window.addEventListener('load', registerSocketHandlers, { once: true });

    refresh();
    reschedule();
    loadWidgets();
    loadSettings();
})();
