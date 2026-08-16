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
    const widgets = $('dsp-widgets');

    // Lit pixel = the OpenKNX green (#449841, same value as logo.svg/favicon.svg/base.css);
    // unlit = a near-black tint of the same hue, so the panel reads as one unlit surface.
    const ON = [0x44, 0x98, 0x41], OFF = [0x07, 0x14, 0x07];
    let invert = 0; // preview only - does not touch the panel or the BMP export
    let timer = null, busy = false;

    function setStatus(text, cls) {
        status.textContent = text;
        status.className = 'dsp-status' + (cls ? ' ' + cls : '');
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

    function refresh() {
        if (busy || document.hidden) return;
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
                setStatus('live', 'ok');
            } catch (e) {
                setStatus('kein Bild', 'err');
            } finally { busy = false; }
        });
    }

    function reschedule() {
        if (timer) clearInterval(timer);
        timer = null;
        const ms = parseInt(rate.value, 10);
        if (ms > 0) timer = setInterval(refresh, ms);
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
            [['▲', -1, i > 0], ['▼', 1, i < list.length - 1]].forEach(function (m) {
                const b = document.createElement('button');
                b.type = 'button';
                b.className = 'dsp-mv';
                b.textContent = m[0];
                b.disabled = !m[2];
                b.addEventListener('click', function () {
                    post('/display/widget?i=' + i + '&m=' + m[1]).then(loadWidgets);
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

    function renderSettings(list) {
        settings.textContent = '';
        const table = document.createElement('table');
        table.className = 'attribute-table';
        const body = table.createTBody();

        list.forEach(function (def) {
            const el = control(def);
            el.id = 'set-' + def.id;
            el.addEventListener('change', function () {
                queueSave(def.id, def.kind === 0 ? (el.checked ? 1 : 0) : el.value);
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
        });

        settings.appendChild(table);
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

    rate.addEventListener('change', reschedule);
    $('dsp-now').addEventListener('click', refresh);
    $('dsp-inv').addEventListener('change', function () {
        invert = this.checked ? 1 : 0;
        refresh();
    });
    document.addEventListener('visibilitychange', function () { if (!document.hidden) refresh(); });

    refresh();
    reschedule();
    loadWidgets();
    loadSettings();
})();
