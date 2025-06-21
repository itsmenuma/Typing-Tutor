const { ipcRenderer } = require('electron');

let currentParagraph = '';
let selectedDifficulty = 'Easy';
let currentUser = '';
let startTime = 0;

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

function checkStartConditions() {
    const username = document.getElementById('username').value.trim();
    const useCustom = document.getElementById('useCustomText').checked;
    const customText = document.getElementById('customParagraph').value.trim();

    const hasValidCustomText = useCustom && customText.length > 0;
    const hasValidDifficulty = !useCustom && selectedDifficulty;

    const startBtn = document.getElementById('startBtn');
    startBtn.disabled = !(username && (hasValidCustomText || hasValidDifficulty));
}

window.setDifficulty = function(level) {
    selectedDifficulty = level.charAt(0).toUpperCase() + level.slice(1).toLowerCase();
    ['easyBtn', 'mediumBtn', 'hardBtn'].forEach(btn => {
        document.getElementById(btn).style.fontWeight = 'normal';
    });
    document.getElementById(level.toLowerCase() + 'Btn').style.fontWeight = 'bold';
    checkStartConditions();
};

window.runTypingTutor = async function () {
    if (!setUsername()) return;

    const useCustom = document.getElementById('useCustomText').checked;
    const customText = document.getElementById('customParagraph').value.trim();

    document.getElementById('output').innerText = 'Loading...';
    document.getElementById('result').innerText = '';
    document.getElementById('userInput').value = '';
    document.getElementById('submitBtn').disabled = false;
    document.getElementById('userInput').disabled = false; // ✅ Enable input field

    if (useCustom && customText.length > 0) {
        currentParagraph = customText;
    } else {
        const result = await ipcRenderer.invoke('run-typing-tutor', ['--get-paragraph', selectedDifficulty]);
        const match = result.match(/Random Paragraph:\s*([\s\S]*)/);
        currentParagraph = match ? match[1].trim() : '';
    }

    document.getElementById('output').innerText = currentParagraph || "Could not load paragraph!";
    startTime = Date.now();
    typingSpeedFactor = 1.0;
    lastKeyPressTime = 0;
};

window.submitTyping = async function() {
    if (!currentUser) return;

    const userInput = document.getElementById('userInput').value;
    const timeTaken = (Date.now() - startTime) / 1000;
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
    const statsMatch = result.match(/Typing Stats:\n([\s\S]*)/);
    document.getElementById('result').innerText = statsMatch ? statsMatch[1].trim() : result;
    document.getElementById('submitBtn').disabled = true;

    showLeaderboard();
};

window.showLeaderboard = async function() {
    const user = currentUser || '';
    const result = await ipcRenderer.invoke('run-typing-tutor', ['--get-leaderboard', selectedDifficulty || "Easy", user]);

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

document.getElementById('username').addEventListener('input', checkStartConditions);
document.getElementById('useCustomText').addEventListener('change', checkStartConditions);
document.getElementById('customParagraph').addEventListener('input', checkStartConditions);

document.addEventListener('DOMContentLoaded', function () {
    document.getElementById('easyBtn').style.fontWeight = 'bold';
    checkStartConditions();
});

let typingSpeedFactor = 1.0;
let lastKeyPressTime = 0;
const realismFactor = 5;
let keyPressAudios = Array.from({ length: realismFactor }, () => new Audio('public/key-press.wav'));
let currentAudioIndex = 0;
let soundEnabled = true;

keyPressAudios.forEach(audio => audio.load());

const ignoredKeys = [
    'Shift', 'CapsLock', 'Tab', 'ArrowLeft', 'ArrowRight',
    'ArrowUp', 'ArrowDown', 'Escape', 'PageUp', 'PageDown',
    'Insert', 'Delete', 'Home', 'End'
];

function toggleSound() {
    const muteBtn = document.getElementById('muteBtn');
    if (muteBtn) {
        soundEnabled = !soundEnabled;
        muteBtn.innerHTML = soundEnabled ? 'Mute' : 'Unmute';
    }
}

document.getElementById('userInput').addEventListener('keydown', function (e) {
    if (!e.ctrlKey && !e.altKey && !e.metaKey && soundEnabled && !ignoredKeys.includes(e.key)) {
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
        audio.play().catch(err => console.error('Audio error:', err));
    }

    if (e.key === 'Enter' && !e.shiftKey) {
        e.preventDefault();
        submitTyping();
    }
});
