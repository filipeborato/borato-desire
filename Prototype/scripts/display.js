document.addEventListener("DOMContentLoaded", () => {
    const displaySvg = document.getElementById('dynamic-display');
    const spectrumLayer = document.getElementById('spectrum-layer');
    const curveLayer = document.getElementById('curve-layer');
    const dancerLayer = document.getElementById('dancer-layer');
    
    // --- Setup Spectrum Path ---
    const spectrumPath = document.createElementNS("http://www.w3.org/2000/svg", "path");
    spectrumPath.setAttribute("fill", "url(#spectrumGrad)");
    spectrumPath.setAttribute("stroke", "var(--pink)");
    spectrumPath.setAttribute("stroke-width", "2");
    spectrumPath.setAttribute("opacity", "0.8");
    
    // Insert Gradients
    let defs = displaySvg.querySelector('defs');
    if (!defs) {
        defs = document.createElementNS("http://www.w3.org/2000/svg", "defs");
        displaySvg.insertBefore(defs, displaySvg.firstChild);
    }
    defs.innerHTML += `
        <linearGradient id="spectrumGrad" x1="0%" y1="0%" x2="0%" y2="100%">
            <stop offset="0%" stop-color="#FF318F" stop-opacity="0.4" />
            <stop offset="100%" stop-color="#FF318F" stop-opacity="0.0" />
        </linearGradient>
    `;
    spectrumLayer.appendChild(spectrumPath);
    
    // --- Setup Curve Path ---
    const curvePath = document.createElementNS("http://www.w3.org/2000/svg", "path");
    curvePath.setAttribute("fill", "none");
    curvePath.setAttribute("stroke", "var(--magenta)");
    curvePath.setAttribute("stroke-width", "4");
    curvePath.style.filter = "drop-shadow(0px 0px 8px var(--magenta))"; // Neon glow
    curveLayer.appendChild(curvePath);
    
    // --- Animation Loop ---
    let time = 0;
    const width = 1156;
    const height = 540;
    
    function animate() {
        time += 0.05;
        
        // 1. Update Spectrum (Fake FFT)
        const numBins = 64;
        const binWidth = width / (numBins - 1);
        let pathD = `M 0 ${height} `; // start at bottom left
        
        for (let i = 0; i < numBins; i++) {
            const x = i * binWidth;
            // Generate some fluid math noise
            const noise = Math.sin(i * 0.4 + time * 3) * Math.cos(i * 0.1 - time);
            const bassBoost = (1 - (i / numBins)) * 180; // More amplitude on the left
            const val = Math.abs(noise) * 150 + bassBoost * (Math.sin(time*2)*0.3 + 0.7);
            
            const y = height - Math.max(10, Math.min(height, val));
            pathD += `L ${x} ${y} `;
        }
        pathD += `L ${width} ${height} Z`; // close path at bottom right
        spectrumPath.setAttribute("d", pathD);
        
        // 2. Update Processing Curve
        // A perna esquerda da dançarina termina em torno de X=411. 
        // Vamos fazer a curva iniciar exatamente em X=411 e ir para a direita,
        // ou criar uma curva contínua que passa pelo pé dela.
        let curveD = ``;
        for(let x = 0; x <= width; x += 20) {
            const y = 270 - Math.sin(x * 0.003 - time) * 80 - Math.cos(x * 0.007 + time * 0.5) * 40;
            if (x === 0) curveD = `M ${x} ${y} `;
            else curveD += `L ${x} ${y} `;
        }
        curvePath.setAttribute("d", curveD);
        
        // 3. Particles (Dancer / Silk effect)
        if (Math.random() > 0.6) {
            spawnParticle();
        }
        
        requestAnimationFrame(animate);
    }
    
    function spawnParticle() {
        if(dancerLayer.children.length > 80) return; // limit
        
        const p = document.createElementNS("http://www.w3.org/2000/svg", "circle");
        const x = Math.random() * width;
        const size = Math.random() * 5 + 1;
        p.setAttribute("cx", x);
        p.setAttribute("cy", height);
        p.setAttribute("r", size);
        p.setAttribute("fill", "var(--cyan)");
        p.style.filter = "blur(2px)";
        dancerLayer.appendChild(p);
        
        let y = height;
        let life = 1.0;
        let speed = Math.random() * 2 + 1;
        
        function move() {
            y -= speed;
            life -= 0.005; // fade out
            p.setAttribute("cy", y);
            p.setAttribute("opacity", Math.max(0, life * 0.7).toFixed(2));
            
            if (life > 0 && y > 0) {
                requestAnimationFrame(move);
            } else {
                p.remove();
            }
        }
        move();
    }
    
    // Start engine
    animate();
});
