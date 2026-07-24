document.addEventListener("DOMContentLoaded", () => {
    const meters = Array.from(document.querySelectorAll(".stereo-meter"));
    const bars = meters.map((meter) => ({
        left: meter.querySelector(".meter-bar.l-channel"),
        right: meter.querySelector(".meter-bar.r-channel")
    }));
    const targets = [0, 0, 0, 0];
    const displayed = [0, 0, 0, 0];

    document.addEventListener("desire:levels", (event) => {
        const level = event.detail || {};
        targets[0] = DesireMapping.meterLevelToHeight(Number(level.inputL) || 0);
        targets[1] = DesireMapping.meterLevelToHeight(Number(level.inputR) || 0);
        targets[2] = DesireMapping.meterLevelToHeight(Number(level.outputL) || 0);
        targets[3] = DesireMapping.meterLevelToHeight(Number(level.outputR) || 0);
    });

    function animate() {
        for (let index = 0; index < displayed.length; index += 1) {
            displayed[index] = DesireMapping.meterBallisticStep(targets[index], displayed[index], 0.88);
        }
        targets.fill(0);

        if (bars[0]) {
            bars[0].left.style.transform = `scaleY(${displayed[0]})`;
            bars[0].right.style.transform = `scaleY(${displayed[1]})`;
        }
        if (bars[1]) {
            bars[1].left.style.transform = `scaleY(${displayed[2]})`;
            bars[1].right.style.transform = `scaleY(${displayed[3]})`;
        }
        requestAnimationFrame(animate);
    }

    requestAnimationFrame(animate);
});
