document.addEventListener("DOMContentLoaded", () => {
    const selector = document.querySelector(".mode-selector");
    if (!selector) {
        return;
    }

    const options = Array.from(selector.querySelectorAll(".mode-option"));

    function activate(option, notify) {
        options.forEach((candidate) => {
            const selected = candidate === option;
            candidate.classList.toggle("is-active", selected);
            candidate.setAttribute("aria-checked", selected ? "true" : "false");
            candidate.tabIndex = selected ? 0 : -1;
        });

        if (notify) {
            selector.dispatchEvent(new CustomEvent("desire:modechange", {
                bubbles: true,
                detail: { mode: option.dataset.mode }
            }));
        }
    }

    options.forEach((option, index) => {
        option.addEventListener("click", () => activate(option, true));
        option.addEventListener("keydown", (event) => {
            if (!["ArrowLeft", "ArrowRight", "Home", "End"].includes(event.key)) {
                return;
            }

            event.preventDefault();
            let nextIndex = index;
            if (event.key === "ArrowLeft") nextIndex = (index - 1 + options.length) % options.length;
            if (event.key === "ArrowRight") nextIndex = (index + 1) % options.length;
            if (event.key === "Home") nextIndex = 0;
            if (event.key === "End") nextIndex = options.length - 1;

            options[nextIndex].focus();
            activate(options[nextIndex], true);
        });
    });

    activate(options.find((option) => option.classList.contains("is-active")) || options[0], false);

    window.DesireModes = {
        setMode(mode) {
            const option = options.find((candidate) => candidate.dataset.mode === mode);
            if (option) activate(option, false);
        }
    };
});
