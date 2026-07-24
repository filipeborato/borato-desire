document.addEventListener("DOMContentLoaded", () => {
    const knobs = document.querySelectorAll('.desire-knob');
    const knobByParameter = new Map();
    
    let activeKnob = null;
    let startY = 0;
    let startVal = 0;
    let dragSensitivity = 250;
    
    // Initialize all knobs
    knobs.forEach(knob => {
        knobByParameter.set(knob.dataset.param, knob);
        const initialVal = parseFloat(knob.dataset.value || "0");
        updateKnobVisual(knob, initialVal);
        
        knob.addEventListener('mousedown', (e) => {
            activeKnob = knob;
            startY = e.clientY;
            startVal = parseFloat(knob.dataset.value || "0");
            dragSensitivity = Math.max(100, knob.getBoundingClientRect().height * 2.272727);
            document.body.style.cursor = 'ns-resize';
            knob.classList.add('is-dragging');
            knob.dispatchEvent(new CustomEvent("desire:knobgesturestart", {
                bubbles: true,
                detail: { param: knob.dataset.param }
            }));
        });

        knob.addEventListener('dblclick', () => {
            const param = knob.dataset.param;
            const defaultValue = DesireMapping.DEFAULT_NORMALISED[param];
            if (defaultValue === undefined) return;

            knob.dataset.value = defaultValue;
            updateKnobVisual(knob, defaultValue);
            knob.dispatchEvent(new CustomEvent("desire:knobgesturestart", { bubbles: true, detail: { param } }));
            knob.dispatchEvent(new CustomEvent("desire:knobchange", { bubbles: true, detail: { param, value: defaultValue } }));
            knob.dispatchEvent(new CustomEvent("desire:knobgestureend", { bubbles: true, detail: { param } }));
        });
    });
    
    document.addEventListener('mousemove', (e) => {
        if (!activeKnob) return;
        
        const deltaY = startY - e.clientY;
        let newVal = startVal + (deltaY / dragSensitivity);
        newVal = Math.max(0, Math.min(1, newVal)); // Clamp
        
        activeKnob.dataset.value = newVal;
        updateKnobVisual(activeKnob, newVal);
        activeKnob.dispatchEvent(new CustomEvent("desire:knobchange", {
            bubbles: true,
            detail: { param: activeKnob.dataset.param, value: newVal }
        }));
    });
    
    document.addEventListener('mouseup', () => {
        if (activeKnob) {
            activeKnob.dispatchEvent(new CustomEvent("desire:knobgestureend", {
                bubbles: true,
                detail: { param: activeKnob.dataset.param }
            }));
            activeKnob.classList.remove('is-dragging');
            activeKnob = null;
            document.body.style.cursor = 'default';
        }
    });

    window.DesireKnobs = {
        setValue(param, value) {
            const knob = knobByParameter.get(param);
            if (!knob) return;
            const normalized = Math.max(0, Math.min(1, Number(value)));
            knob.dataset.value = normalized;
            updateKnobVisual(knob, normalized);
        }
    };
});

function updateKnobVisual(knob, value) {
    const angle = DesireMapping.knobValueToAngleDeg(value);

    const pointer = knob.querySelector('.knob-pointer');
    if (pointer) {
        pointer.style.transform = `rotate(${angle}deg)`;
    }

    const arcPaths = knob.querySelectorAll('.knob-arc-path');
    if (arcPaths.length > 0) {
        arcPaths.forEach(arc => {
            const radius = parseFloat(arc.getAttribute('r')) || 48;
            const { current, circumference } = DesireMapping.arcStrokeDashArray(value, radius);
            arc.style.strokeDasharray = `${current} ${circumference}`;
        });
    }

    const readout = knob.querySelector('.knob-readout');
    if (readout) {
        const param = knob.dataset.param;
        let displayValue = "";

        if (param === "input" || param === "output") {
            const db = DesireMapping.knobValueToDb(value);
            displayValue = (db > 0 ? "+" : "") + db.toFixed(1) + " dB";
        } else if (param === "width") {
            displayValue = Math.round(DesireMapping.knobValueToWidthPercent(value)) + " %";
        } else {
            displayValue = Math.round(DesireMapping.knobValueToPercent(value)) + " %";
        }

        readout.textContent = displayValue;
    }

    // Single choke point for "this knob's displayed value changed", regardless
    // of whether it came from a user drag, double-click reset, or a
    // host/bridge-driven programmatic sync (window.DesireKnobs.setValue calls
    // this same function). Anything that needs to react to live parameter
    // values -- like the response curve -- listens here instead of duplicating
    // drag-tracking logic.
    knob.dispatchEvent(new CustomEvent("desire:knobvisualupdate", {
        bubbles: true,
        detail: { param: knob.dataset.param, value }
    }));
}
