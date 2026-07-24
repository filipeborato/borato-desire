document.addEventListener("DOMContentLoaded", () => {
    const bypassButton = document.getElementById("toolbar-bypass");
    const presetName = document.getElementById("toolbar-preset-name");
    const presetPrev = document.getElementById("toolbar-preset-prev");
    const presetNext = document.getElementById("toolbar-preset-next");
    const presetSave = document.getElementById("toolbar-preset-save");
    const abToggle = document.getElementById("toolbar-ab-toggle");
    const abA = document.getElementById("toolbar-ab-a");
    const abB = document.getElementById("toolbar-ab-b");
    const abCopy = document.getElementById("toolbar-ab-copy");
    const undoButton = document.getElementById("toolbar-undo");
    const redoButton = document.getElementById("toolbar-redo");

    if (!bypassButton) return;

    function dispatchCommand(command, arg) {
        document.dispatchEvent(new CustomEvent("desire:presetcommand", {
            detail: { command, arg }
        }));
    }

    let bypassed = false;
    bypassButton.addEventListener("click", () => {
        bypassed = !bypassed;
        bypassButton.classList.toggle("is-active", bypassed);
        bypassButton.setAttribute("aria-pressed", bypassed ? "true" : "false");
        document.dispatchEvent(new CustomEvent("desire:bypasschange", { detail: { bypassed } }));
    });

    presetPrev?.addEventListener("click", () => dispatchCommand("previousPreset"));
    presetNext?.addEventListener("click", () => dispatchCommand("nextPreset"));
    presetSave?.addEventListener("click", () => {
        const name = window.prompt("Preset name?");
        if (name) dispatchCommand("saveUserPreset", name);
    });

    abToggle?.addEventListener("click", () => {
        dispatchCommand("selectAB", abA?.classList.contains("is-active") ? "B" : "A");
    });
    abCopy?.addEventListener("click", () => {
        dispatchCommand(abA?.classList.contains("is-active") ? "copyAToB" : "copyBToA");
    });

    undoButton?.addEventListener("click", () => dispatchCommand("undo"));
    redoButton?.addEventListener("click", () => dispatchCommand("redo"));

    document.addEventListener("desire:status", (event) => {
        const status = event.detail || {};
        if (presetName) presetName.textContent = status.presetName || "Late Night Confessions";
        if (abA) abA.classList.toggle("is-active", status.abSlot === "A");
        if (abB) abB.classList.toggle("is-active", status.abSlot === "B");
        if (undoButton) undoButton.disabled = !status.canUndo;
        if (redoButton) redoButton.disabled = !status.canRedo;
    });
});
