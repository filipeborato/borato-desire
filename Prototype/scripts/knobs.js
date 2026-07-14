document.addEventListener("DOMContentLoaded", () => {
    const knobs = document.querySelectorAll('.desire-knob');
    
    let activeKnob = null;
    let startY = 0;
    let startVal = 0;
    
    // Initialize all knobs
    knobs.forEach(knob => {
        const initialVal = parseFloat(knob.dataset.value || "0");
        updateKnobVisual(knob, initialVal);
        
        knob.addEventListener('mousedown', (e) => {
            activeKnob = knob;
            startY = e.clientY;
            startVal = parseFloat(knob.dataset.value || "0");
            document.body.style.cursor = 'ns-resize';
            knob.classList.add('is-dragging');
        });
    });
    
    document.addEventListener('mousemove', (e) => {
        if (!activeKnob) return;
        
        const deltaY = startY - e.clientY;
        const dragSensitivity = 250; // pixels to go from 0 to 1
        
        let newVal = startVal + (deltaY / dragSensitivity);
        newVal = Math.max(0, Math.min(1, newVal)); // Clamp
        
        activeKnob.dataset.value = newVal;
        updateKnobVisual(activeKnob, newVal);
    });
    
    document.addEventListener('mouseup', () => {
        if (activeKnob) {
            activeKnob.classList.remove('is-dragging');
            activeKnob = null;
            document.body.style.cursor = 'default';
        }
    });
});

function updateKnobVisual(knob, value) {
    const minAngle = -135;
    const maxAngle = 135;
    const angle = minAngle + value * (maxAngle - minAngle);
    
    const pointer = knob.querySelector('.knob-pointer');
    if (pointer) {
        pointer.style.transform = `rotate(${angle}deg)`;
    }
    
    const arcPaths = knob.querySelectorAll('.knob-arc-path');
    if (arcPaths.length > 0) {
        arcPaths.forEach(arc => {
            const radius = parseFloat(arc.getAttribute('r')) || 48;
            const circumference = 2 * Math.PI * radius;
            const totalArcLength = circumference * (270 / 360);
            const currentLength = totalArcLength * value;
            arc.style.strokeDasharray = `${currentLength} ${circumference}`;
        });
    }
    
    const readout = knob.querySelector('.knob-readout');
    if (readout) {
        const param = knob.dataset.param;
        let displayValue = "";
        
        if (param === "input" || param === "output") {
            const db = -24 + (value * 36); // Maps 0-1 to -24dB to +12dB
            displayValue = (db > 0 ? "+" : "") + db.toFixed(1) + " dB";
        } else if (param === "width") {
            displayValue = Math.round(value * 200) + " %";
        } else {
            displayValue = Math.round(value * 100) + " %";
        }
        
        readout.textContent = displayValue;
    }
}
