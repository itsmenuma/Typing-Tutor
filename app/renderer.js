const { ipcRenderer } = require('electron');

let currentParagraph = '';
let startTime = 0;
let selectedDifficulty = 'Easy'; // Set default difficulty
let currentUser = ''; // Add this to store username globally
let realtimeMode = false;
let realtimeTyping = null;

// Add this function to validate and set username
window.setUsername = function() {
    const usernameInput = document.getElementById('username');
    const username = usernameInput.value.trim();
    
    if (!username) {
        usernameInput.style.borderColor = '#ff4444';
        usernameInput.placeholder = 'Please enter your name first';
        setTimeout(() => {
            usernameInput.style.borderColor = '#3e497a';
            usernameInput.placeholder = 'Enter your name';
        }, 2000);
        return false;
    }
    
    currentUser = username;
    usernameInput.disabled = true; // Lock the username field
    return true;
};

// Add this function to check if we can enable the start button
function checkStartConditions() {
    const username = document.getElementById('username').value.trim();
    const startBtn = document.getElementById('startBtn');
    startBtn.disabled = !username || !selectedDifficulty;
}

// Update setDifficulty function
window.setDifficulty = function(level) {
    selectedDifficulty = level.charAt(0).toUpperCase() + level.slice(1).toLowerCase();
    document.getElementById('easyBtn').style.fontWeight = (level === 'Easy') ? 'bold' : 'normal';
    document.getElementById('mediumBtn').style.fontWeight = (level === 'Medium') ? 'bold' : 'normal';
    document.getElementById('hardBtn').style.fontWeight = (level === 'Hard') ? 'bold' : 'normal';
    checkStartConditions(); // Check if we can enable start button
};

// Update runTypingTutor to use stored username
window.runTypingTutor = async function() {
    if (!setUsername()) return; // Validate username first
    if (!selectedDifficulty) return;

    document.getElementById('output').innerText = 'Loading...';
    document.getElementById('result').innerText = '';
    document.getElementById('userInput').value = '';
    document.getElementById('submitBtn').disabled = false; // Enable submit on new test

    const result = await ipcRenderer.invoke('run-typing-tutor', ['--get-paragraph', selectedDifficulty]);
    const match = result.match(/Random Paragraph:\s*([\s\S]*)/);
    currentParagraph = match ? match[1].trim() : '';
    document.getElementById('output').innerText = currentParagraph || "Could not load paragraph!";
    startTime = Date.now();
    typingSpeedFactor = 1.0;
    lastKeyPressTime = 0;
};

// Update submitTyping to use stored username
window.submitTyping = async function() {
    if (!currentUser) return; // Ensure we have a username

    const userInput = document.getElementById('userInput').value;
    const timeTaken = (Date.now() - startTime) / 1000; // seconds
    const caseInsensitive = document.getElementById('caseSensitive').checked ? 0 : 1;

    const args = [
        currentUser,
        selectedDifficulty,
        caseInsensitive.toString(),
        timeTaken.toString(),
        userInput,
        currentParagraph
    ];
    const result = await ipcRenderer.invoke('run-typing-tutor', args);

    // Only show the stats part (everything after "Typing Stats:")
    const statsMatch = result.match(/Typing Stats:\n([\s\S]*)/);
    document.getElementById('result').innerText = statsMatch ? statsMatch[1].trim() : result;
    document.getElementById('submitBtn').disabled = true; // Disable after submit

    // After updating the leaderboard data, refresh the leaderboard UI:
    showLeaderboard();
};

// Update showLeaderboard to use stored username
window.showLeaderboard = async function() {
    // Use empty string if currentUser is not set
    const user = currentUser || "";
    const result = await ipcRenderer.invoke('run-typing-tutor', [
        '--get-leaderboard',
        selectedDifficulty || "Easy",
        user
    ]);
    const lines = result.trim().split('\n');
    let leaderboardHTML = '';
    let userHighlighted = false;
    let count = 0;
    for (let i = 0; i < lines.length && count < 10; i++) {
        const line = lines[i];
        if (line.includes(currentUser)) {
            leaderboardHTML += `<span class="your-result">${line}</span>\n`;
            userHighlighted = true;
        } else {
            leaderboardHTML += line + '\n';
        }
        count++;
    }
    // If user not in top 10, show their rank below
    if (!userHighlighted) {
        for (let i = 10; i < lines.length; i++) {
            if (lines[i].includes(currentUser)) {
                leaderboardHTML += `\n<span class="your-result">Your Rank: ${i+1}: ${lines[i]}</span>\n`;
                break;
            }
        }
    }
    document.getElementById('leaderboard').innerHTML = leaderboardHTML;
};

document.getElementById('userInput').addEventListener('keydown', function(e) {
  if (e.key === 'Enter' && !e.shiftKey) {
    e.preventDefault();
    submitTyping();
  }
});

// Add event listener for username input
document.getElementById('username').addEventListener('input', function() {
    checkStartConditions();
});

// When page loads, highlight Easy button
document.addEventListener('DOMContentLoaded', function() {
    document.getElementById('easyBtn').style.fontWeight = 'bold';
    checkStartConditions(); // Check if we can enable start button
});

// ===== REAL-TIME TYPING MODE =====
// This class handles the real-time typing mode with instant feedback and statistics
class RealtimeTypingMode {
    constructor() {
        this.targetText = '';
        this.userInput = '';
        this.currentPos = 0;
        this.errors = 0;
        this.startTime = null;
        this.isActive = false;
        this.stats = {
            cpm: 0,
            wpm: 0,
            accuracy: 100,
            progress: 0
        };
        this.updateInterval = null;
    }

    // Initialize real-time mode
    async initializeRealtimeMode(targetText) {
        this.targetText = targetText;
        this.userInput = '';
        this.currentPos = 0;
        this.errors = 0;
        this.startTime = Date.now();
        this.isActive = true;
        
        this.setupUI();
        this.bindKeyEvents();
        this.updateDisplay();
        this.startStatsUpdate();
    }

    // Setup the UI for real-time mode
    setupUI() {
        const container = document.getElementById('tab-content-test');
        container.innerHTML = `
            <div class="realtime-mode">
                <div class="mode-header">
                    <h2>🔴 Real-Time Typing Mode</h2>
                    <div class="user-info">
                        <span><strong>User:</strong> ${currentUser}</span> | 
                        <span><strong>Difficulty:</strong> ${selectedDifficulty}</span>
                    </div>
                    <div class="controls-info">
                        <span class="control-item">ESC: Quit</span> | 
                        <span class="control-item">Backspace: Correct</span> | 
                        <span class="control-item">Type to continue</span>
                    </div>
                </div>
                
                <div class="text-display">
                    <h3>Target Text:</h3>
                    <div id="target-text-display" class="target-text"></div>
                </div>
                
                <div class="input-display">
                    <h3>Your Input:</h3>
                    <div id="user-input-display" class="user-input"></div>
                    <div class="cursor blink">_</div>
                </div>
                
                <div class="stats-dashboard">
                    <div class="stat-card">
                        <div class="stat-label">Progress</div>
                        <div id="progress-stat" class="stat-value">0/${this.targetText.length}</div>
                    </div>
                    <div class="stat-card">
                        <div class="stat-label">Errors</div>
                        <div id="errors-stat" class="stat-value">0</div>
                    </div>
                    <div class="stat-card">
                        <div class="stat-label">Time</div>
                        <div id="time-stat" class="stat-value">0.0s</div>
                    </div>
                    <div class="stat-card">
                        <div class="stat-label">Speed</div>
                        <div id="speed-stat" class="stat-value">0 CPM</div>
                    </div>
                    <div class="stat-card">
                        <div class="stat-label">Accuracy</div>
                        <div id="accuracy-stat" class="stat-value">100%</div>
                    </div>
                </div>
                
                <div class="action-buttons">
                    <button onclick="exitRealtimeMode()" class="btn-secondary">Exit Real-Time Mode</button>
                </div>
            </div>
        `;
    }

    // Bind keyboard events for character input
    bindKeyEvents() {
        this.keyHandler = (e) => {
            if (!this.isActive) return;
            
            e.preventDefault();
            e.stopPropagation();
            
            if (e.key === 'Escape') {
                this.endRealtimeMode(true); // Cancelled
                return;
            }
            
            if (e.key === 'Backspace') {
                this.handleBackspace();
                return;
            }
            
            // Handle printable characters
            if (e.key.length === 1 && this.currentPos < this.targetText.length) {
                this.handleCharacterInput(e.key);
            }
        };
        
        document.addEventListener('keydown', this.keyHandler);
        
        // Focus on the container to capture key events
        const container = document.querySelector('.realtime-mode');
        container.setAttribute('tabindex', '0');
        container.focus();
    }

    // Handle character input
    handleCharacterInput(char) {
        this.userInput += char;
        
        // Check if character is correct
        if (char !== this.targetText[this.currentPos]) {
            this.errors++;
        }
        
        this.currentPos++;
        this.updateDisplay();
        
        // Check if completed
        if (this.currentPos >= this.targetText.length) {
            setTimeout(() => this.endRealtimeMode(false), 500);
        }
    }

    // Handle backspace
    handleBackspace() {
        if (this.currentPos > 0) {
            this.currentPos--;
            this.userInput = this.userInput.slice(0, -1);
            this.recalculateErrors();
            this.updateDisplay();
        }
    }

    // Recalculate error count
    recalculateErrors() {
        this.errors = 0;
        for (let i = 0; i < this.currentPos; i++) {
            if (this.userInput[i] !== this.targetText[i]) {
                this.errors++;
            }
        }
    }

    // Update the visual display
    updateDisplay() {
        this.updateTargetTextDisplay();
        this.updateUserInputDisplay();
        this.updateStats();
    }

    // Update target text with highlighting
    updateTargetTextDisplay() {
        const targetDisplay = document.getElementById('target-text-display');
        if (!targetDisplay) return;
        
        let html = '';
        
        for (let i = 0; i < this.targetText.length; i++) {
            const char = this.targetText[i];
            let className = '';
            
            if (i < this.currentPos) {
                // Already typed
                if (this.userInput[i] === char) {
                    className = 'char-correct';
                } else {
                    className = 'char-incorrect';
                }
            } else if (i === this.currentPos) {
                // Current position
                className = 'char-current';
            } else {
                // Not yet typed
                className = 'char-pending';
            }
            
            const displayChar = char === ' ' ? '&nbsp;' : char;
            html += `<span class="${className}">${displayChar}</span>`;
        }
        
        targetDisplay.innerHTML = html;
    }

    // Update user input display
    updateUserInputDisplay() {
        const inputDisplay = document.getElementById('user-input-display');
        if (!inputDisplay) return;
        
        let html = '';
        
        for (let i = 0; i < this.userInput.length; i++) {
            const char = this.userInput[i];
            const isCorrect = char === this.targetText[i];
            const className = isCorrect ? 'input-correct' : 'input-incorrect';
            const displayChar = char === ' ' ? '&nbsp;' : char;
            
            html += `<span class="${className}">${displayChar}</span>`;
        }
        
        inputDisplay.innerHTML = html;
    }

    // Start updating stats regularly
    startStatsUpdate() {
        this.updateInterval = setInterval(() => {
            this.updateStats();
        }, 100); // Update every 100ms for smooth stats
    }

    // Update statistics
    updateStats() {
        const now = Date.now();
        const elapsedSeconds = (now - this.startTime) / 1000;
        
        // Calculate speed
        if (elapsedSeconds > 0) {
            this.stats.cpm = Math.round((this.currentPos / elapsedSeconds) * 60);
            this.stats.wpm = Math.round(this.stats.cpm / 5);
        }
        
        // Calculate accuracy
        if (this.currentPos > 0) {
            this.stats.accuracy = Math.round(((this.currentPos - this.errors) / this.currentPos) * 100);
        }
        
        this.stats.progress = this.currentPos;
        
        // Update display elements
        this.updateStatElements(elapsedSeconds);
    }

    // Update stat display elements
    updateStatElements(elapsedSeconds) {
        const elements = {
            'progress-stat': `${this.currentPos}/${this.targetText.length}`,
            'errors-stat': this.errors,
            'time-stat': `${elapsedSeconds.toFixed(1)}s`,
            'speed-stat': `${this.stats.cpm} CPM (${this.stats.wpm} WPM)`,
            'accuracy-stat': `${this.stats.accuracy}%`
        };
        
        Object.entries(elements).forEach(([id, value]) => {
            const element = document.getElementById(id);
            if (element) {
                element.textContent = value;
            }
        });
    }

    // End real-time mode
    endRealtimeMode(cancelled = false) {
        this.isActive = false;
        
        if (this.updateInterval) {
            clearInterval(this.updateInterval);
            this.updateInterval = null;
        }
        
        if (this.keyHandler) {
            document.removeEventListener('keydown', this.keyHandler);
            this.keyHandler = null;
        }
        
        if (cancelled) {
            this.showResults('Test cancelled by user', 'warning');
        } else {
            this.showResults('Test completed!', 'success');
            this.saveResults();
        }
    }

    // Show results
    showResults(message, type) {
        const container = document.getElementById('tab-content-test');
        const finalStats = this.calculateFinalStats();
        
        container.innerHTML = `
            <div class="results-display ${type}">
                <h2>${message}</h2>
                <div class="final-stats-grid">
                    <div class="stat-group">
                        <h3>Speed</h3>
                        <div class="big-stat">${finalStats.cpm}</div>
                        <div class="stat-unit">CPM</div>
                        <div class="small-stat">${finalStats.wpm} WPM</div>
                    </div>
                    <div class="stat-group">
                        <h3>Accuracy</h3>
                        <div class="big-stat">${finalStats.accuracy}</div>
                        <div class="stat-unit">%</div>
                        <div class="small-stat">${finalStats.errors} errors</div>
                    </div>
                    <div class="stat-group">
                        <h3>Time</h3>
                        <div class="big-stat">${finalStats.timeElapsed}</div>
                        <div class="stat-unit">seconds</div>
                        <div class="small-stat">Characters: ${finalStats.characters}</div>
                    </div>
                </div>
                <div class="action-buttons">
                    <button onclick="startNewRealtimeTest()" class="btn-primary">Try Again (Real-Time)</button>
                    <button onclick="returnToMainMenu()" class="btn-secondary">Return to Main Menu</button>
                    <button onclick="showLeaderboard()" class="btn-secondary">View Leaderboard</button>
                </div>
            </div>
        `;
    }

    // Calculate final statistics
    calculateFinalStats() {
        const elapsedSeconds = (Date.now() - this.startTime) / 1000;
        return {
            cpm: this.stats.cpm,
            wpm: this.stats.wpm,
            accuracy: this.stats.accuracy,
            errors: this.errors,
            timeElapsed: elapsedSeconds.toFixed(1),
            characters: this.currentPos
        };
    }

    // Save results using existing backend
    async saveResults() {
        const finalStats = this.calculateFinalStats();
        const caseInsensitive = document.getElementById('caseSensitive')?.checked ? 0 : 1;
        
        const args = [
            currentUser,
            selectedDifficulty,
            caseInsensitive.toString(),
            finalStats.timeElapsed,
            this.userInput,
            this.targetText
        ];
        
        try {
            await ipcRenderer.invoke('run-typing-tutor', args);
            // Refresh leaderboard after saving
            showLeaderboard();
        } catch (error) {
            console.error('Error saving results:', error);
        }
    }
}

// ===== INTEGRATION FUNCTIONS =====

const originalRunTypingTutor = window.runTypingTutor;
window.runTypingTutor = async function() {
    if (!setUsername()) return;
    if (!selectedDifficulty) return;

    // Show mode selection
    showTypingModeSelection();
};

// Show typing mode selection
function showTypingModeSelection() {
    const container = document.getElementById('tab-content-test');
    container.innerHTML = `
        <div class="mode-selection">
            <h2>Choose Typing Mode</h2>
            <div class="mode-options">
                <div class="mode-card" onclick="startClassicMode()">
                    <div class="mode-icon">📝</div>
                    <h3>Classic Mode</h3>
                    <p>Type the entire paragraph, then see your results. Good for longer texts and traditional typing practice.</p>
                    <div class="mode-features">
                        ✓ Full paragraph typing<br>
                        ✓ Complete when finished<br>
                        ✓ Traditional feedback
                    </div>
                </div>
                
                <div class="mode-card highlight" onclick="startRealtimeMode()">
                    <div class="mode-icon">🔴</div>
                    <h3>Real-Time Mode</h3>
                    <p>See errors highlighted instantly as you type! Perfect for learning and immediate feedback.</p>
                    <div class="mode-features">
                        ✓ Instant error highlighting<br>
                        ✓ Live statistics<br>
                        ✓ Character-by-character feedback
                    </div>
                </div>
            </div>
            
            <div class="back-button">
                <button onclick="returnToMainMenu()" class="btn-secondary">← Back to Setup</button>
            </div>
        </div>
    `;
}

// Start classic mode
window.startClassicMode = async function() {
    realtimeMode = false;
    
    // Use original functionality
    document.getElementById('tab-content-test').innerHTML = `
        <div class="input-group">
            <label for="username">Name:</label>
            <input type="text" id="username" value="${currentUser}" disabled>
        </div>
        <div class="difficulty-section">
            <h2>Customize Test</h2>
            <div class="difficulty-buttons">
                <button id="easyBtn" onclick="setDifficulty('Easy')" ${selectedDifficulty === 'Easy' ? 'style="font-weight: bold"' : ''}>Easy</button>
                <button id="mediumBtn" onclick="setDifficulty('Medium')" ${selectedDifficulty === 'Medium' ? 'style="font-weight: bold"' : ''}>Medium</button>
                <button id="hardBtn" onclick="setDifficulty('Hard')" ${selectedDifficulty === 'Hard' ? 'style="font-weight: bold"' : ''}>Hard</button>
            </div>
            <label class="case-label">
                <input type="checkbox" id="caseSensitive" checked>
                Case Sensitive
            </label>
        </div>
        <button onclick="runClassicTest()" class="btn-primary">Start Classic Test</button>
        <pre id="output"></pre>
        <textarea id="userInput" rows="6" cols="60" placeholder="Type here..."></textarea><br>
        <button id="submitBtn" onclick="submitTyping()">Submit</button>
        <div id="result"></div>
        <div class="back-button">
            <button onclick="showTypingModeSelection()" class="btn-secondary">← Change Mode</button>
        </div>
    `;
};

// Run classic test
window.runClassicTest = async function() {
    document.getElementById('output').innerText = 'Loading...';
    document.getElementById('result').innerText = '';
    document.getElementById('userInput').value = '';
    document.getElementById('submitBtn').disabled = false;

    const result = await ipcRenderer.invoke('run-typing-tutor', ['--get-paragraph', selectedDifficulty]);
    const match = result.match(/Random Paragraph:\s*([\s\S]*)/);
    currentParagraph = match ? match[1].trim() : '';
    document.getElementById('output').innerText = currentParagraph || "Could not load paragraph!";
    startTime = Date.now();
};

// Start real-time mode
window.startRealtimeMode = async function() {
    realtimeMode = true;
    
    // Show loading
    const container = document.getElementById('tab-content-test');
    container.innerHTML = '<div class="loading">Loading paragraph...</div>';
    
    try {
        // Get paragraph
        const result = await ipcRenderer.invoke('run-typing-tutor', ['--get-paragraph', selectedDifficulty]);
        const match = result.match(/Random Paragraph:\s*([\s\S]*)/);
        const paragraph = match ? match[1].trim() : '';
        
        if (!paragraph) {
            throw new Error('Could not load paragraph');
        }
        
        // Initialize real-time typing
        realtimeTyping = new RealtimeTypingMode();
        await realtimeTyping.initializeRealtimeMode(paragraph);
        
    } catch (error) {
        container.innerHTML = `
            <div class="error-message">
                <h3>Error loading paragraph</h3>
                <p>${error.message}</p>
                <button onclick="showTypingModeSelection()" class="btn-primary">Try Again</button>
            </div>
        `;
    }
};

// Start new real-time test
window.startNewRealtimeTest = function() {
    startRealtimeMode();
};

// Exit real-time mode
window.exitRealtimeMode = function() {
    if (realtimeTyping) {
        realtimeTyping.endRealtimeMode(true);
    }
};

// Return to main menu
window.returnToMainMenu = function() {
    realtimeMode = false;
    
    // Reset to original state
    const container = document.getElementById('tab-content-test');
    container.innerHTML = `
        <div class="input-group">
            <label for="username">Name:</label>
            <input type="text" id="username" placeholder="Enter your name" maxlength="20">
        </div>
        <div class="difficulty-section">
            <h2>Customize Test</h2>
            <div class="difficulty-buttons">
                <button id="easyBtn" onclick="setDifficulty('Easy')">Easy</button>
                <button id="mediumBtn" onclick="setDifficulty('Medium')">Medium</button>
                <button id="hardBtn" onclick="setDifficulty('Hard')">Hard</button>
            </div>
            <label class="case-label">
                <input type="checkbox" id="caseSensitive" checked>
                Case Sensitive
            </label>
        </div>
        <button id="startBtn" onclick="runTypingTutor()" disabled>Start Typing Test</button>
        <pre id="output"></pre>
        <textarea id="userInput" rows="6" cols="60" placeholder="Type here..."></textarea><br>
        <button id="submitBtn" onclick="submitTyping()">Submit</button>
        <div id="result"></div>
    `;
    
    // Reset state
    currentUser = '';
    selectedDifficulty = 'Easy';
    document.getElementById('easyBtn').style.fontWeight = 'bold';
    
    // Re-add event listeners
    document.getElementById('username').addEventListener('input', checkStartConditions);
    checkStartConditions();
};


let typingSpeedFactor = 1.0;
let lastKeyPressTime = 0;
const realismFactor = 5; // Controls how many audio objects to create for overlapping sounds
let keyPressAudios = Array.from(
  { length: realismFactor },
  () => new Audio('public/key-press.wav')
);
let currentAudioIndex = 0;
let soundEnabled = true;
keyPressAudios.forEach((audio) => audio.load());

const ignoredKeys = [
  'Shift',
  'CapsLock',
  'Tab',
  'ArrowLeft',
  'ArrowRight',
  'ArrowUp',
  'ArrowDown',
  'Escape',
  'PageUp',
  'PageDown',
  'Insert',
  'Delete',
  'Home',
  'End',
];

keyPressAudios.forEach((audio) => audio.load());

function toggleSound() {
  const muteBtn = document.getElementById('muteBtn');
  if (muteBtn) {
    soundEnabled = !soundEnabled;
    muteBtn.innerHTML = soundEnabled ? 'Mute' : 'Unmute';
  }
}

document.getElementById('userInput').addEventListener('keydown', function (e) {
  if (
    !e.ctrlKey &&
    !e.altKey &&
    !e.metaKey &&
    soundEnabled &&
    !ignoredKeys.includes(e.key)
  ) {
    // Use alternating audio objects to allow overlapping sounds
    currentAudioIndex = (currentAudioIndex + 1) % realismFactor;
    const audio = keyPressAudios[currentAudioIndex];
    audio.volume = 0.8 + Math.random() * 0.2; // Slight volume variation

    // Adjust playback rate based on typing speed with slight randomization
    const now = Date.now();
    if (lastKeyPressTime > 0) {
      const timeBetweenKeyPresses = now - lastKeyPressTime;
      if (timeBetweenKeyPresses < 100) {
        // Fast typing
        typingSpeedFactor = Math.min(1.5, typingSpeedFactor + 0.01); // Gradually increase speed
      } else if (timeBetweenKeyPresses > 300) {
        // Slow typing
        typingSpeedFactor = Math.max(0.7, typingSpeedFactor - 0.01); // Gradually decrease speed
      }
    }
    lastKeyPressTime = now;

    // Apply the speed factor with slight randomization for natural feel
    audio.playbackRate = typingSpeedFactor * (0.97 + Math.random() * 0.06);
    audio.currentTime = 0;
    audio.play().catch((err) => console.error('Error playing sound:', err));
  }

  if (e.key === 'Enter' && !e.shiftKey) {
    e.preventDefault();
    submitTyping();
  }
});

