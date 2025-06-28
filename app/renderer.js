const { ipcRenderer } = require('electron');

let currentParagraph = '';
let selectedDifficulty = 'Easy';
let currentUser = '';
let startTime = 0;
let testMode = 'paragraph';
let timeDuration = 1;
let timerInterval = null;
let timedTestActive = false;
let typingSpeedFactor = 1.0;
let lastKeyPressTime = 0;
const realismFactor = 5;
let keyPressAudios = Array.from({ length: realismFactor }, () => new Audio('public/key-press.wav'));
let currentAudioIndex = 0;
let soundEnabled = true;
let realtimeTyping = null;

const ignoredKeys = [
  'Shift', 'CapsLock', 'Tab', 'ArrowLeft', 'ArrowRight',
  'ArrowUp', 'ArrowDown', 'Escape', 'PageUp', 'PageDown',
  'Insert', 'Delete', 'Home', 'End'
];

keyPressAudios.forEach(audio => audio.load());

// Validate and set username
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
    usernameInput.disabled = true;
    return true;
};

// Set test mode (paragraph or timed)
window.setTestMode = function(mode) {
    testMode = mode;
    document.getElementById('paragraphModeBtn').classList.toggle('active', mode === 'paragraph');
    document.getElementById('timedModeBtn').classList.toggle('active', mode === 'timed');
    document.getElementById('timeDurationContainer').style.display = mode === 'timed' ? 'block' : 'none';
    checkStartConditions();
};

// Set time duration for timed mode
window.setTimeDuration = function(minutes) {
    timeDuration = minutes;
    document.getElementById('time1MinBtn').classList.toggle('active', minutes === 1);
    document.getElementById('time3MinBtn').classList.toggle('active', minutes === 3);
    document.getElementById('time5MinBtn').classList.toggle('active', minutes === 5);
    checkStartConditions();
};

// Check conditions to enable start button
function checkStartConditions() {
    const username = document.getElementById('username').value.trim();
    const useCustom = document.getElementById('useCustomText').checked;
    const customText = document.getElementById('customParagraph').value.trim();

    const hasValidCustomText = useCustom && customText.length > 0;
    const hasValidDifficulty = !useCustom && selectedDifficulty;

    const startBtn = document.getElementById('startBtn');
    startBtn.disabled = !(username && (hasValidCustomText || hasValidDifficulty));
}

// Set difficulty level
window.setDifficulty = function(level) {
    selectedDifficulty = level.charAt(0).toUpperCase() + level.slice(1).toLowerCase();
    ['easyBtn', 'mediumBtn', 'hardBtn'].forEach(btn => {
        document.getElementById(btn).style.fontWeight = level.toLowerCase() === btn.slice(0, -3) ? 'bold' : 'normal';
    });
    checkStartConditions();
};

// Start timer for timed mode
function startTimer(minutes) {
    clearInterval(timerInterval);
    const timerDisplay = document.getElementById('timerDisplay');
    const timeLeftSpan = document.getElementById('timeLeft');
    timerDisplay.style.display = 'block';

    let totalSeconds = minutes * 60;
    timedTestActive = true;

    function updateTimer() {
        const mins = Math.floor(totalSeconds / 60);
        const secs = totalSeconds % 60;
        timeLeftSpan.textContent = `${mins}:${secs.toString().padStart(2, '0')}`;

        if (totalSeconds <= 0) {
            clearInterval(timerInterval);
            timedTestActive = false;
            submitTyping(true);
            return;
        }
        totalSeconds--;
    }

    updateTimer();
    timerInterval = setInterval(updateTimer, 1000);
}

// Realtime typing class for error highlighting
class RealtimeTypingMode {
    constructor(targetText, isTimed) {
        this.targetText = targetText;
        this.userInput = '';
        this.currentPos = 0;
        this.errors = 0;
        this.startTime = Date.now();
        this.isActive = true;
        this.isTimed = isTimed;
        this.stats = { cpm: 0, wpm: 0, accuracy: 100, progress: 0 };
        this.updateInterval = null;
    }

    initialize() {
        this.setupUI();
        this.bindKeyEvents();
        this.updateDisplay();
        if (this.isTimed) {
            startTimer(timeDuration);
        }
        this.startStatsUpdate();
    }

    setupUI() {
        const container = document.getElementById('tab-content-test');
        container.innerHTML = `
            <div class="input-group">
                <label for="username">Name:</label>
                <input type="text" id="username" value="${currentUser}" disabled>
            </div>
            <div class="difficulty-section">
                <h2>${this.isTimed ? 'Timed' : 'Paragraph'} Test</h2>
                <div class="test-mode-section">
                    <h3>Test Mode</h3>
                    <div class="mode-buttons">
                        <button id="paragraphModeBtn" class="mode-btn ${testMode === 'paragraph' ? 'active' : ''}" onclick="setTestMode('paragraph')">Paragraph</button>
                        <button id="timedModeBtn" class="mode-btn ${testMode === 'timed' ? 'active' : ''}" onclick="setTestMode('timed')">Timed</button>
                    </div>
                    <div id="timeDurationContainer" class="time-duration" style="display:${testMode === 'timed' ? 'block' : 'none'};">
                        <label>Duration:</label>
                        <div class="time-buttons">
                            <button id="time1MinBtn" class="time-btn ${timeDuration === 1 ? 'active' : ''}" onclick="setTimeDuration(1)">1 min</button>
                            <button id="time3MinBtn" class="time-btn ${timeDuration === 3 ? 'active' : ''}" onclick="setTimeDuration(3)">3 min</button>
                            <button id="time5MinBtn" class="time-btn ${timeDuration === 5 ? 'active' : ''}" onclick="setTimeDuration(5)">5 min</button>
                        </div>
                    </div>
                    <div class="difficulty-buttons">
                        <button id="easyBtn" onclick="setDifficulty('Easy')" ${selectedDifficulty === 'Easy' ? 'style="font-weight: bold"' : ''}>Easy</button>
                        <button id="mediumBtn" onclick="setDifficulty('Medium')" ${selectedDifficulty === 'Medium' ? 'style="font-weight: bold"' : ''}>Medium</button>
                        <button id="hardBtn" onclick="setDifficulty('Hard')" ${selectedDifficulty === 'Hard' ? 'style="font-weight: bold"' : ''}>Hard</button>
                    </div>
                    <label class="case-label">
                        <input type="checkbox" id="caseSensitive" checked>
                        Case Sensitive
                    </label>
                    <label style="display: block; margin-top: 10px;">
                        <input type="checkbox" id="useCustomText">
                        Use custom paragraph input
                    </label>
                    <textarea id="customParagraph" rows="4" cols="60" ${this.isTimed ? 'disabled' : ''} placeholder="Enter custom paragraph..." disabled style="margin-top: 10px;"></textarea>
                </div>
            </div>
            <button id="startBtn" onclick="runTypingTutor()" disabled>Start ${this.isTimed ? 'Timed' : 'Paragraph'} Test</button>
            <div id="timerDisplay" class="timer" style="display:${this.isTimed ? 'block' : 'none'};">Time Left: <span id="timeLeft">0:00</span></div>
            <div class="text-display">
                <h3>Target Text:</h3>
                <div id="target-text-display" class="target-text"></div>
            </div>
            <div class="input-display">
                <h3>Your Input:</h3>
                <textarea id="userInput" rows="6" cols="60" placeholder="Type here..."></textarea>
                <div id="user-input-display" class="user-input"></div>
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
            <button id="submitBtn" onclick="submitTyping()">Submit</button>
            <button id="exportBtn" onclick="exportStats()" disabled>Export Stats</button>
            <div id="result"></div>
        `;
        toggleCustomInput();
        document.getElementById('useCustomText').addEventListener('change', toggleCustomInput);
        document.getElementById('customParagraph').addEventListener('input', checkStartConditions);
    }

    bindKeyEvents() {
        this.keyHandler = (e) => {
            if (!this.isActive) return;

            if (e.key === 'Escape') {
                this.endRealtimeMode(true);
                return;
            }

            if (e.key === 'Enter' && !e.shiftKey) {
                e.preventDefault();
                submitTyping(this.isTimed && timedTestActive);
                return;
            }

            if (e.key === 'Backspace') {
                this.handleBackspace();
                return;
            }

            if (e.key.length === 1 && this.currentPos < this.targetText.length) {
                e.preventDefault(); // Prevent duplicate characters
                playKeySound(e);     // Play sound
                this.handleCharacterInput(e.key);
                return;
            }
        };

        document.getElementById('userInput').addEventListener('keydown', this.keyHandler);
    }

    handleCharacterInput(char) {
        const caseSensitive = document.getElementById('caseSensitive')?.checked;
        const targetChar = caseSensitive ? this.targetText[this.currentPos] : this.targetText[this.currentPos].toLowerCase();
        const inputChar = caseSensitive ? char : char.toLowerCase();

        this.userInput += char;
        if (inputChar !== targetChar) {
            this.errors++;
        }
        this.currentPos++;
        document.getElementById('userInput').value = this.userInput;
        this.updateDisplay();

        if (this.currentPos >= this.targetText.length && !this.isTimed) {
            setTimeout(() => this.endRealtimeMode(false), 500);
        }
    }

    handleBackspace() {
        if (this.currentPos > 0) {
            this.currentPos--;
            this.userInput = this.userInput.slice(0, -1);
            document.getElementById('userInput').value = this.userInput;
            this.recalculateErrors();
            this.updateDisplay();
        }
    }

    recalculateErrors() {
        const caseSensitive = document.getElementById('caseSensitive')?.checked;
        this.errors = 0;
        for (let i = 0; i < this.currentPos; i++) {
            const targetChar = caseSensitive ? this.targetText[i] : this.targetText[i].toLowerCase();
            const inputChar = caseSensitive ? this.userInput[i] : this.userInput[i].toLowerCase();
            if (inputChar !== targetChar) {
                this.errors++;
            }
        }
    }

    updateDisplay() {
        this.updateTargetTextDisplay();
        this.updateUserInputDisplay();
        this.updateStats();
    }

    updateTargetTextDisplay() {
        const targetDisplay = document.getElementById('target-text-display');
        if (!targetDisplay) return;

        const caseSensitive = document.getElementById('caseSensitive')?.checked;
        let html = '';
        for (let i = 0; i < this.targetText.length; i++) {
            const char = this.targetText[i];
            let className = '';
            if (i < this.currentPos) {
                const userChar = caseSensitive ? this.userInput[i] : this.userInput[i]?.toLowerCase();
                const targetChar = caseSensitive ? char : char.toLowerCase();
                className = userChar === targetChar ? 'char-correct' : 'char-incorrect';
            } else if (i === this.currentPos) {
                className = 'char-current';
            } else {
                className = 'char-pending';
            }
            const displayChar = char === ' ' ? ' ' : char;
            html += `<span class="${className}">${displayChar}</span>`;
        }
        targetDisplay.innerHTML = html;
    }

    updateUserInputDisplay() {
        const inputDisplay = document.getElementById('user-input-display');
        if (!inputDisplay) return;

        const caseSensitive = document.getElementById('caseSensitive')?.checked;
        let html = '';
        for (let i = 0; i < this.userInput.length; i++) {
            const char = this.userInput[i];
            const targetChar = caseSensitive ? this.targetText[i] : this.targetText[i]?.toLowerCase();
            const inputChar = caseSensitive ? char : char.toLowerCase();
            const isCorrect = inputChar === targetChar;
            const className = isCorrect ? 'input-correct' : 'input-incorrect';
            const displayChar = char === ' ' ? ' ' : char;
            html += `<span class="${className}">${displayChar}</span>`;
        }
        inputDisplay.innerHTML = html;
    }

    startStatsUpdate() {
        this.updateInterval = setInterval(() => {
            this.updateStats();
        }, 100);
    }

    updateStats() {
        const now = Date.now();
        const elapsedSeconds = (now - this.startTime) / 1000;

        if (elapsedSeconds > 0) {
            this.stats.cpm = Math.round((this.currentPos / elapsedSeconds) * 60);
            this.stats.wpm = Math.round(this.stats.cpm / 5);
        }

        if (this.currentPos > 0) {
            this.stats.accuracy = Math.round(((this.currentPos - this.errors) / this.currentPos) * 100);
        }

        this.stats.progress = this.currentPos;
        this.updateStatElements(elapsedSeconds);
    }

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

    endRealtimeMode(cancelled = false) {
        this.isActive = false;

        if (this.updateInterval) {
            clearInterval(this.updateInterval);
            this.updateInterval = null;
        }

        if (this.keyHandler) {
            const userInput = document.getElementById('userInput');
            if (userInput) {
                userInput.removeEventListener('keydown', this.keyHandler);
            }
            this.keyHandler = null;
        }

        document.getElementById('userInput').disabled = true;
        document.getElementById('userInput').classList.add('dimmed');
        document.getElementById('submitBtn').disabled = true;

        if (cancelled) {
            document.getElementById('result').innerText = 'Test cancelled by user';
        } else {
            this.saveResults();
            submitTyping(this.isTimed && timedTestActive);
            document.getElementById('exportBtn').disabled = false;
        }
    }

    calculateFinalStats() {
        const elapsedSeconds = (Date.now() - this.startTime) / 1000;
        return {
            cpm: this.stats.cpm || 0,
            wpm: this.stats.wpm || 0,
            accuracy: this.stats.accuracy || 100,
            errors: this.errors || 0,
            timeElapsed: elapsedSeconds.toFixed(1),
            characters: this.currentPos || 0
        };
    }

    async saveResults() {
        const finalStats = this.calculateFinalStats();
        const caseInsensitive = document.getElementById('caseSensitive')?.checked ? 0 : 1;

        const args = [
            currentUser,
            selectedDifficulty,
            caseInsensitive.toString(),
            finalStats.timeElapsed,
            this.userInput,
            this.targetText,
            testMode,
            timeDuration.toString()
        ];

        try {
            await ipcRenderer.invoke('run-typing-tutor', args);
        } catch (error) {
            console.error('Error saving results:', error);
            showToast('Failed to save results: ' + error.message);
        }
    }
}

// Run typing tutor
window.runTypingTutor = async function(isRestart = false) {
    if (!setUsername()) return;

    const useCustom = document.getElementById('useCustomText')?.checked;
    const customText = document.getElementById('customParagraph')?.value.trim();

    // ✅ Only show loading UI if not restarting AND file is not custom
    if (!isRestart && !useCustom) {
        document.getElementById('tab-content-test').innerHTML = `
            <div class="loading">Loading...</div>
        `;
    }

    try {
        if (useCustom && customText.length > 0) {
            currentParagraph = customText;
        } else {
            const result = await ipcRenderer.invoke('run-typing-tutor', ['--get-paragraph', selectedDifficulty]);

            // ✅ Handle file read failure from backend
            if (result.startsWith("ERROR:")) {
                showToast(result.replace("ERROR:", "").trim(), true); // toast and quit
                return;
            }

            const match = result.match(/Random Paragraph:\s*([\s\S]*)/);
            currentParagraph = match ? match[1].trim() : '';

            if (!currentParagraph) {
                showToast("Paragraph could not be loaded", true); // fallback toast and quit
                return;
            }
        }

        // ✅ Restore full test UI (only now after paragraph loads)
        //document.getElementById('tab-content-test').innerHTML = originalTestHTML; // You should store this at startup
        realtimeTyping = new RealtimeTypingMode(currentParagraph, testMode === 'timed');
        realtimeTyping.initialize();
    } catch (error) {
        showToast("Unexpected error: " + error.message, true);
    }
};

// Submit typing results
window.submitTyping = async function(isTimedEnd = false) {
    if (!currentUser || !realtimeTyping) return;

    // ✅ Stop timer if running (both timed end and manual)
    if (isTimedEnd || timedTestActive) {
        clearInterval(timerInterval);
        document.getElementById('timerDisplay').style.display = 'none';
        timedTestActive = false;
    }

    const userInput = document.getElementById('userInput').value;
    if (!userInput.trim()) {
        showToast('Please type something before submitting.');
        return;
    }

    // ✅ Disable submit button right after user clicks
    document.getElementById('submitBtn').disabled = true;

    // ✅ Show warning if user hasn't finished the paragraph (manual only)
    if (!isTimedEnd && realtimeTyping.currentPos < currentParagraph.length) {
        showToast('Please complete the paragraph before submitting.');
        return;
    }

    // Update internal state before sending
    realtimeTyping.userInput = userInput;
    realtimeTyping.currentPos = userInput.length;
    realtimeTyping.recalculateErrors();

    const finalStats = realtimeTyping.calculateFinalStats();
    const caseInsensitive = document.getElementById('caseSensitive')?.checked ? 0 : 1;

    const args = [
        currentUser,
        selectedDifficulty,
        caseInsensitive.toString(),
        finalStats.timeElapsed,
        userInput,
        currentParagraph,
        testMode,
        timeDuration.toString()
    ];

    try {
        const result = await ipcRenderer.invoke('run-typing-tutor', args);
        const statsMatch = result.match(/Typing Stats:\n([\s\S]*)/);

        // Optionally display via stats dashboard
        // displayFinalStats(statsMatch ? statsMatch[1].trim() : result);

        document.getElementById('exportBtn').disabled = false;
        showLeaderboard(); // ✅ Grid layout will now update
    } catch (error) {
        console.error('Error submitting results:', error);
        showToast('Failed to submit results: ' + error.message);
    }
};



// Export stats
window.exportStats = async function () {
  const format = 'txt'; // or dynamically choose if you want a dropdown later

  // Extract values from dashboard
  const stats = [];

  document.querySelectorAll('.stat-card').forEach((card) => {
    const label = card.querySelector('.stat-label')?.textContent?.trim();
    const value = card.querySelector('.stat-value')?.textContent?.trim();
    if (label && value) stats.push(`${label}: ${value}`);
  });

  if (stats.length === 0) {
    showToast("No stats found to export.");
    return;
  }

  const statText = stats.join('\n');

  const success = await ipcRenderer.invoke('export-stats', statText, format);
  if (success) {
    showToast("Stats exported successfully!");
  } else {
    showToast("Export cancelled.");
  }
};


// Toggle custom input
function toggleCustomInput() {
    const isCustom = document.getElementById('useCustomText').checked;
    document.getElementById('customParagraph').disabled = !isCustom;
    ['easyBtn', 'mediumBtn', 'hardBtn'].forEach(id => {
        document.getElementById(id).disabled = isCustom;
    });
    checkStartConditions();
}

// Return to main menu
window.returnToMainMenu = function() {
    if (realtimeTyping) {
        realtimeTyping.endRealtimeMode(true);
        realtimeTyping = null;
    }
    timedTestActive = false;
    clearInterval(timerInterval);
    const container = document.getElementById('tab-content-test');
    container.innerHTML = `
        <div class="input-group">
            <label for="username">Name:</label>
            <input type="text" id="username" placeholder="Enter your name" maxlength="20">
        </div>
        <div class="difficulty-section">
            <h2>Customize Test</h2>
            <div class="test-mode-section">
                <h3>Test Mode</h3>
                <div class="mode-buttons">
                    <button id="paragraphModeBtn" class="mode-btn active" onclick="setTestMode('paragraph')">Paragraph</button>
                    <button id="timedModeBtn" class="mode-btn" onclick="setTestMode('timed')">Timed</button>
                </div>
                <div id="timeDurationContainer" class="time-duration" style="display:none;">
                    <label>Duration:</label>
                    <div class="time-buttons">
                        <button id="time1MinBtn" class="time-btn active" onclick="setTimeDuration(1)">1 min</button>
                        <button id="time3MinBtn" class="time-btn" onclick="setTimeDuration(3)">3 min</button>
                        <button id="time5MinBtn" class="time-btn" onclick="setTimeDuration(5)">5 min</button>
                    </div>
                </div>
                <div class="difficulty-buttons">
                    <button id="easyBtn" onclick="setDifficulty('Easy')">Easy</button>
                    <button id="mediumBtn" onclick="setDifficulty('Medium')">Medium</button>
                    <button id="hardBtn" onclick="setDifficulty('Hard')">Hard</button>
                </div>
                <label class="case-label">
                    <input type="checkbox" id="caseSensitive" checked>
                    Case Sensitive
                </label>
                <label style="display: block; margin-top: 10px;">
                    <input type="checkbox" id="useCustomText">
                    Use custom paragraph input
                </label>
                <textarea id="customParagraph" rows="4" cols="60" placeholder="Enter custom paragraph..." disabled style="margin-top: 10px;"></textarea>
            </div>
        </div>
        <button id="startBtn" onclick="runTypingTutor()" disabled>Start Typing Test</button>
        <div id="timerDisplay" class="timer" style="display:none;">Time Left: <span id="timeLeft">0:00</span></div>
        <pre id="output"></pre>
        <textarea id="userInput" rows="6" cols="60" placeholder="Type here..." disabled></textarea><br>
        <button id="submitBtn" onclick="submitTyping()">Submit</button>
        <button id="exportBtn" onclick="exportStats()" disabled>Export Stats</button>
        <div id="result"></div>
    `;

    currentUser = '';
    selectedDifficulty = 'Easy';
    testMode = 'paragraph';
    timeDuration = 1;
    document.getElementById('easyBtn').style.fontWeight = 'bold';
    document.getElementById('paragraphModeBtn').classList.add('active');
    document.getElementById('time1MinBtn').classList.add('active');

    document.getElementById('username').addEventListener('input', checkStartConditions);
    document.getElementById('useCustomText').addEventListener('change', toggleCustomInput);
    document.getElementById('customParagraph').addEventListener('input', checkStartConditions);
    checkStartConditions();
};

// Show leaderboard
async function showLeaderboard(difficulty = 'Easy') {
  const entries = document.getElementById('leaderboard-entries');
  entries.innerHTML = '';

  try {
    const result = await ipcRenderer.invoke('run-typing-tutor', ['--get-leaderboard', difficulty]);
    console.log('Raw output from C program:', JSON.stringify(result));

    // Parse the table-formatted output
    const lines = result.trim().split('\n')
      .filter(line => line.trim() !== '' && line.includes('|')) // Keep lines with table data
      .filter(line => !line.includes('Rank') && !line.includes('---')) // Skip header and separator
      .map(line => {
        // Extract fields using regex to handle multiple spaces and pipes
        const match = line.match(/\|\s*(\d+)\s*\|\s*(\w+)\s*\|\s*(\d+\.\d+)\s*\|\s*(\d+\.\d+)\s*\|\s*(\d+\.\d+)\s*\|/);
        if (!match) {
          console.warn(`Invalid line format: ${line}`);
          return null;
        }
        const [, rank, name, cpm, wpm, accuracy] = match;
        const parsedCpm = parseFloat(cpm);
        const parsedWpm = parseFloat(wpm);
        const parsedAccuracy = parseFloat(accuracy);
        if (isNaN(parsedCpm) || isNaN(parsedWpm) || isNaN(parsedAccuracy)) {
          console.warn(`Invalid numeric values in line: ${line}`);
          return null;
        }
        return { name, cpm: parsedCpm, wpm: parsedWpm, accuracy: parsedAccuracy, difficulty };
      })
      .filter(entry => entry !== null);

    const uniqueScores = lines.reduce((acc, current) => {
      const existing = acc.find(item => item.name === current.name);
      if (!existing || existing.cpm < current.cpm) {
        const index = acc.findIndex(item => item.name === current.name);
        if (index !== -1) acc.splice(index, 1);
        acc.push(current);
      }
      return acc;
    }, []);

    uniqueScores.sort((a, b) => b.cpm - a.cpm);

    if (uniqueScores.length === 0) {
      entries.innerHTML = `<div class="error-message">No valid leaderboard entries for ${difficulty}. Raw output: <pre>${result || 'Empty output'}</pre></div>`;
      return;
    }

    // Display up to 5 entries
    uniqueScores.slice(0, 5).forEach((score, index) => {
      const entry = document.createElement('div');
      entry.className = `leaderboard-entry rank-${index + 1}`;
      if (score.name === currentUser) entry.classList.add('highlight');
      entry.innerHTML = `
        <span class="rank-col">${index + 1}</span>
        <span class="name-col">${score.name}</span>
        <span class="speed-col">${score.cpm.toFixed(1)}</span>
        <span class="wpm-col">${score.wpm.toFixed(1)}</span>
        <span class="accuracy-col">${score.accuracy.toFixed(1)}%</span>
      `;
      entries.appendChild(entry);
    });
  } catch (error) {
    console.error('Error loading leaderboard:', error);
    entries.innerHTML = `<div class="error-message">Error loading leaderboard data: ${error.message}. Raw output: <pre>${result || 'No output received'}</pre></div>`;
  }
}

// Show full leaderboard
async function showFullLeaderboard(difficulty = 'Easy') {
    let modal = document.getElementById('fullLeaderboardModal');
    if (!modal) {
        modal = document.createElement('div');
        modal.id = 'fullLeaderboardModal';
        modal.className = 'full-leaderboard-modal';
        modal.innerHTML = `
            <div class="modal-content">
                <span class="close-modal">×</span>
                <div class="difficulty-tabs">
                    <button class="diff-tab active" data-difficulty="Easy">Easy</button>
                    <button class="diff-tab" data-difficulty="Medium">Medium</button>
                    <button class="diff-tab" data-difficulty="Hard">Hard</button>
                </div>
                <div class="leaderboard-header">
                    <span class="rank-col">Rank</span>
                    <span class="name-col">Name</span>
                    <span class="speed-col">CPM</span>
                    <span class="wpm-col">WPM</span>
                    <span class="accuracy-col">Accuracy</span>
                </div>
                <div id="full-leaderboard-entries"></div>
            </div>
        `;
        document.body.appendChild(modal);

        modal.querySelector('.close-modal').onclick = () => {
            modal.style.display = 'none';
        };

        modal.querySelector('.difficulty-tabs').addEventListener('click', (e) => {
            if (e.target.classList.contains('diff-tab')) {
                modal.querySelectorAll('.diff-tab').forEach(tab => tab.classList.remove('active'));
                e.target.classList.add('active');
                showFullLeaderboard(e.target.dataset.difficulty);
            }
        });
    }

    try {
        const result = await ipcRenderer.invoke('run-typing-tutor', ['--get-leaderboard', difficulty]);
        console.log('Raw output from C program:', JSON.stringify(result));
        if (!result) throw new Error('No leaderboard data received');

        const entries = document.getElementById('full-leaderboard-entries');
        entries.innerHTML = '';

        const lines = result.trim().split('\n')
            .filter(line => line.trim() !== '' && line.includes('|')) // Keep lines with table data
            .filter(line => !line.includes('Rank') && !line.includes('---')) // Skip header and separator
            .map(line => {
                // Extract fields using regex to handle table format
                const match = line.match(/\|\s*(\d+)\s*\|\s*(\w+)\s*\|\s*(\d+\.\d+)\s*\|\s*(\d+\.\d+)\s*\|\s*(\d+\.\d+)\s*\|/);
                if (!match) {
                    console.warn(`Invalid line format: ${line}`);
                    return null;
                }
                const [, rank, name, cpm, wpm, accuracy] = match;
                const parsedCpm = parseFloat(cpm);
                const parsedWpm = parseFloat(wpm);
                const parsedAccuracy = parseFloat(accuracy);
                if (isNaN(parsedCpm) || isNaN(parsedWpm) || isNaN(parsedAccuracy)) {
                    console.warn(`Invalid numeric values in line: ${line}`);
                    return null;
                }
                return { name, cpm: parsedCpm, wpm: parsedWpm, accuracy: parsedAccuracy, difficulty };
            })
            .filter(entry => entry !== null);

        const uniqueScores = lines.reduce((acc, current) => {
            const existing = acc.find(item => item.name === current.name);
            if (!existing || existing.cpm < current.cpm) {
                const index = acc.findIndex(item => item.name === current.name);
                if (index !== -1) acc.splice(index, 1);
                acc.push(current);
            }
            return acc;
        }, []);

        uniqueScores.sort((a, b) => b.cpm - a.cpm);

        if (uniqueScores.length === 0) {
            entries.innerHTML = `<div class="error-message">No valid leaderboard entries for ${difficulty}. Raw output: <pre>${result || 'Empty output'}</pre></div>`;
            return;
        }

        // Display up to 10 entries
        uniqueScores.slice(0, 10).forEach((score, index) => {
            const entry = document.createElement('div');
            entry.className = `leaderboard-entry rank-${index + 1}`;
            if (score.name === currentUser) entry.classList.add('highlight');
            entry.innerHTML = `
                <span class="rank-col">${index + 1}</span>
                <span class="name-col">${score.name}</span>
                <span class="speed-col">${score.cpm.toFixed(1)}</span>
                <span class="wpm-col">${score.wpm.toFixed(1)}</span>
                <span class="accuracy-col">${score.accuracy.toFixed(1)}%</span>
            `;
            entries.appendChild(entry);
        });

        modal.style.display = 'block';
    } catch (error) {
        console.error('Error loading full leaderboard:', error);
        const entries = document.getElementById('full-leaderboard-entries');
        entries.innerHTML = `<div class="error-message">Error loading leaderboard data: ${error.message}. Raw output: <pre>${result || 'No output received'}</pre></div>`;
        modal.style.display = 'block';
    }
}

// Toggle sound
function toggleSound() {
    const muteBtn = document.getElementById('muteBtn');
    if (muteBtn) {
        soundEnabled = !soundEnabled;
        muteBtn.innerHTML = soundEnabled ? 'Mute' : 'Unmute';
        showToast(`Sound ${soundEnabled ? 'enabled' : 'disabled'}`);
    }
}

// Show toast notification
function showToast(message, exitApp = false) {
  const toast = document.getElementById('toast');
  toast.textContent = message + (exitApp ? " Exiting the app..." : "");
  toast.style.display = 'block';

  toast.style.animation = 'none';
  void toast.offsetHeight;
  toast.style.animation = '';

  setTimeout(() => {
    toast.style.display = 'none';
    if (exitApp) ipcRenderer.send('quit-app');
  }, 2500);
}

function playKeySound(e) {
  if (
    !e.ctrlKey &&
    !e.altKey &&
    !e.metaKey &&
    soundEnabled &&
    !ignoredKeys.includes(e.key)
  ) {
    currentAudioIndex = (currentAudioIndex + 1) % realismFactor;
    const audio = keyPressAudios[currentAudioIndex];
    audio.volume = 0.8 + Math.random() * 0.2;

    const now = Date.now();
    if (lastKeyPressTime > 0) {
      const delta = now - lastKeyPressTime;
      if (delta < 100) {
        typingSpeedFactor = Math.min(1.5, typingSpeedFactor + 0.01);
      } else if (delta > 300) {
        typingSpeedFactor = Math.max(0.7, typingSpeedFactor - 0.01);
      }
    }
    lastKeyPressTime = now;

    audio.playbackRate = typingSpeedFactor * (0.97 + Math.random() * 0.06);
    audio.currentTime = 0;
    audio.play().catch((err) => console.error('Audio error:', err));
  }
}

// Keypress sound effects
document.addEventListener('DOMContentLoaded', () => {
    document.getElementById('easyBtn').style.fontWeight = 'bold';
    document.getElementById('paragraphModeBtn').classList.add('active');
    document.getElementById('time1MinBtn').classList.add('active');
    document.getElementById('username').addEventListener('input', checkStartConditions);
    document.getElementById('useCustomText').addEventListener('change', toggleCustomInput);
    document.getElementById('customParagraph').addEventListener('input', checkStartConditions);
    document.getElementById('viewAllBtn').addEventListener('click', () => {
        const activeDifficulty = document.querySelector('.diff-tab.active').dataset.difficulty;
        showFullLeaderboard(activeDifficulty);
    });
    document.querySelectorAll('.diff-tab').forEach(tab => {
        tab.addEventListener('click', (e) => {
            document.querySelectorAll('.diff-tab').forEach(t => t.classList.remove('active'));
            e.target.classList.add('active');
            showLeaderboard(e.target.dataset.difficulty);
        });
    });
    checkStartConditions();

    // ✅ Attach listener safely to "View Full Leaderboard"
  const viewAllBtn = document.getElementById('viewAllBtn');
  if (viewAllBtn) {
    viewAllBtn.addEventListener('click', () => {
      const activeTab = document.querySelector('.diff-tab.active');
      const diff = activeTab?.dataset.difficulty || 'Easy';
      showFullLeaderboard(diff);
    });
  }

  // ✅ Tab switcher for difficulty inside leaderboard
  document.querySelectorAll('.diff-tab').forEach(tab => {
    tab.addEventListener('click', () => {
      document.querySelectorAll('.diff-tab').forEach(t => t.classList.remove('active'));
      tab.classList.add('active');
      showLeaderboard(tab.dataset.difficulty);
    });
  });
});