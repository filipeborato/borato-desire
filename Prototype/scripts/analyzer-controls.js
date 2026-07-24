document.addEventListener("DOMContentLoaded", () => {
    const controls = document.querySelector(".analyzer-controls");
    const view = document.getElementById("analyzer-view");
    const tap = document.getElementById("analyzer-tap");
    const low = document.getElementById("analyzer-lf");
    const high = document.getElementById("analyzer-hf");
    const lowValue = document.getElementById("analyzer-lf-value");
    const highValue = document.getElementById("analyzer-hf-value");

    if (!controls || !view || !tap || !low || !high || !lowValue || !highValue) {
        return;
    }

    function publish(changedControl) {
        if (Number(low.value) > Number(high.value)) {
            if (changedControl === low) high.value = low.value;
            else low.value = high.value;
        }

        const lowHz = DesireMapping.analyzerNormalizedToHz(Number(low.value));
        const highHz = DesireMapping.analyzerNormalizedToHz(Number(high.value));
        lowValue.value = DesireMapping.analyzerFormatFrequency(lowHz);
        highValue.value = DesireMapping.analyzerFormatFrequency(highHz);
        controls.classList.toggle("is-disabled", view.value === "off");

        controls.dispatchEvent(new CustomEvent("desire:analyzerchange", {
            bubbles: true,
            detail: {
                view: view.value,
                tap: tap.value,
                lowHz,
                highHz
            }
        }));
    }

    view.addEventListener("change", () => publish(view));
    tap.addEventListener("change", () => publish(tap));
    low.addEventListener("input", () => publish(low));
    high.addEventListener("input", () => publish(high));

    publish(null);
});
