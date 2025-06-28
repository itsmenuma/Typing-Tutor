// Simple progress tracking
let progressData = [];

// Load progress from file (replace with actual file reading)
function loadProgress() {
    // TODO: Read from build/progress.txt
    // Format: wpm,accuracy,date (one line per test)
    // Example implementation:
    /*
    const fs = require('fs');
    try {
        const data = fs.readFileSync('../build/progress.txt', 'utf8');
        const lines = data.trim().split('\n');
        progressData = lines.map(line => {
            const [wpm, accuracy, date] = line.split(',');
            return {
                wpm: parseInt(wpm),
                accuracy: parseInt(accuracy),
                date: date,
                timestamp: new Date(date).getTime()
            };
        });
    } catch (error) {
        console.log('No progress file found or error reading file');
        progressData = [];
    }
    */
    
    updateStats();
    drawGraph();
}

// Save progress to file (replace with actual file writing)
function saveProgress() {
    // TODO: Write to build/progress.txt
    // This function is not needed here since C backend handles saving
}

// Add a new score (call this function when user completes a test)
function addScore(wpm, accuracy) {
    const date = new Date().toLocaleDateString();
    progressData.push({
        date: date,
        wpm: Math.round(wpm),
        accuracy: Math.round(accuracy),
        timestamp: Date.now()
    });
    
    // Keep only last 20 scores
    if (progressData.length > 20) {
        progressData = progressData.slice(-20);
    }
    
    updateStats();
    drawGraph();
}

// Update statistics display
function updateStats() {
    if (progressData.length === 0) {
        document.getElementById('avgSpeed').textContent = '0';
        document.getElementById('avgAccuracy').textContent = '0';
        document.getElementById('totalTests').textContent = '0';
        return;
    }
    
    const avgSpeed = Math.round(progressData.reduce((sum, item) => sum + item.wpm, 0) / progressData.length);
    const avgAccuracy = Math.round(progressData.reduce((sum, item) => sum + item.accuracy, 0) / progressData.length);
    
    document.getElementById('avgSpeed').textContent = avgSpeed;
    document.getElementById('avgAccuracy').textContent = avgAccuracy;
    document.getElementById('totalTests').textContent = progressData.length;
}

// Draw the progress graph
function drawGraph() {
    const svg = document.getElementById('progressGraph');
    
    // Clear existing lines and points
    const existingElements = svg.querySelectorAll('.line, .point');
    existingElements.forEach(el => el.remove());
    
    if (progressData.length < 2) {
        return; // Need at least 2 points to draw a line
    }
    
    const width = 700;
    const height = 260;
    const padding = 40;
    const graphWidth = width - 2 * padding;
    const graphHeight = height - 2 * padding;
    
    // Find min/max values
    const maxWpm = Math.max(...progressData.map(d => d.wpm));
    const minWpm = Math.min(...progressData.map(d => d.wpm));
    const maxAccuracy = Math.max(...progressData.map(d => d.accuracy));
    const minAccuracy = Math.min(...progressData.map(d => d.accuracy));
    
    // Scale values to fit graph
    const scaleX = (index) => padding + (index / (progressData.length - 1)) * graphWidth;
    const scaleWpm = (wpm) => height - padding - ((wpm - minWpm) / (maxWpm - minWpm)) * graphHeight;
    const scaleAccuracy = (accuracy) => height - padding - ((accuracy - minAccuracy) / (maxAccuracy - minAccuracy)) * graphHeight;
    
    // Create speed line
    let speedPath = `M ${scaleX(0)} ${scaleWpm(progressData[0].wpm)}`;
    for (let i = 1; i < progressData.length; i++) {
        speedPath += ` L ${scaleX(i)} ${scaleWpm(progressData[i].wpm)}`;
    }
    
    const speedLine = document.createElementNS('http://www.w3.org/2000/svg', 'path');
    speedLine.setAttribute('d', speedPath);
    speedLine.setAttribute('class', 'line speed-line');
    svg.appendChild(speedLine);
    
    // Create accuracy line
    let accuracyPath = `M ${scaleX(0)} ${scaleAccuracy(progressData[0].accuracy)}`;
    for (let i = 1; i < progressData.length; i++) {
        accuracyPath += ` L ${scaleX(i)} ${scaleAccuracy(progressData[i].accuracy)}`;
    }
    
    const accuracyLine = document.createElementNS('http://www.w3.org/2000/svg', 'path');
    accuracyLine.setAttribute('d', accuracyPath);
    accuracyLine.setAttribute('class', 'line accuracy-line');
    svg.appendChild(accuracyLine);
    
    // Add points
    progressData.forEach((item, index) => {
        // Speed point
        const speedPoint = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
        speedPoint.setAttribute('cx', scaleX(index));
        speedPoint.setAttribute('cy', scaleWpm(item.wpm));
        speedPoint.setAttribute('class', 'point speed-point');
        speedPoint.addEventListener('mouseenter', (e) => showTooltip(e, item, 'speed'));
        speedPoint.addEventListener('mouseleave', hideTooltip);
        svg.appendChild(speedPoint);
        
        // Accuracy point
        const accuracyPoint = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
        accuracyPoint.setAttribute('cx', scaleX(index));
        accuracyPoint.setAttribute('cy', scaleAccuracy(item.accuracy));
        accuracyPoint.setAttribute('class', 'point accuracy-point');
        accuracyPoint.addEventListener('mouseenter', (e) => showTooltip(e, item, 'accuracy'));
        accuracyPoint.addEventListener('mouseleave', hideTooltip);
        svg.appendChild(accuracyPoint);
    });
}

// Show tooltip on hover
function showTooltip(event, data, type) {
    const tooltip = document.getElementById('tooltip');
    const value = type === 'speed' ? `${data.wpm} WPM` : `${data.accuracy}%`;
    tooltip.innerHTML = `${data.date}<br>${value}`;
    tooltip.style.left = event.pageX + 10 + 'px';
    tooltip.style.top = event.pageY - 10 + 'px';
    tooltip.style.opacity = '1';
}

// Hide tooltip
function hideTooltip() {
    document.getElementById('tooltip').style.opacity = '0';
}

// Go back to main page
function goBack() {
    // In Electron, you might use:
    // window.history.back();
    // or navigate to main page
    window.location.href = 'index.html';
}

// Initialize - load progress when page opens
loadProgress();
