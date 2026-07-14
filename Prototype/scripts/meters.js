document.addEventListener("DOMContentLoaded", () => {
    const meterL = document.querySelectorAll('.meter-bar.l-channel');
    const meterR = document.querySelectorAll('.meter-bar.r-channel');
    
    // Simple spring/momentum simulation for audio levels
    let levelL = 0;
    let levelR = 0;
    
    function animateMeters() {
        // Random target level mimicking a beat/transient
        const targetL = Math.random() > 0.85 ? 0.3 + Math.random() * 0.7 : Math.random() * 0.2;
        const targetR = Math.random() > 0.85 ? 0.3 + Math.random() * 0.7 : Math.random() * 0.2;
        
        // Instant attack, slow release (falloff)
        if (targetL > levelL) levelL = targetL;
        else levelL = Math.max(0, levelL - 0.03); // Falloff speed
        
        if (targetR > levelR) levelR = targetR;
        else levelR = Math.max(0, levelR - 0.03);
        
        // Update DOM scale
        meterL.forEach(bar => bar.style.transform = `scaleY(${levelL})`);
        meterR.forEach(bar => bar.style.transform = `scaleY(${levelR})`);
        
        requestAnimationFrame(animateMeters);
    }
    
    animateMeters();
});
