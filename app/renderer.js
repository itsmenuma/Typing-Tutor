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

    // Wait a short moment to ensure file is written
    setTimeout(() => {
        showLeaderboard(selectedDifficulty);
    }, 100);
};

async function showLeaderboard(difficulty = 'Easy') {
  const entries = document.getElementById('leaderboard-entries');
  entries.innerHTML = '';

  try {
    // Use IPC renderer to get leaderboard data
    const result = await ipcRenderer.invoke('run-typing-tutor', ['--get-leaderboard']);
    if (!result) {
      throw new Error('No leaderboard data received');
    }

    // Parse the leaderboard data
    const lines = result.trim().split('\n')
      .filter(line => line.includes(difficulty))
      .map(line => {
        const [name, cpm, wpm, accuracy, diff] = line.split(' ');
        return {
          name,
          cpm: parseFloat(cpm),
          wpm: parseFloat(wpm),
          accuracy: parseFloat(accuracy),
          difficulty: diff
        };
      });

    // Remove duplicates keeping highest score
    const uniqueScores = lines.reduce((acc, current) => {
      const existing = acc.find(item => item.name === current.name);
      if (!existing || existing.cpm < current.cpm) {
        const index = acc.findIndex(item => item.name === current.name);
        if (index !== -1) {
          acc.splice(index, 1);
        }
        acc.push(current);
      }
      return acc;
    }, []);

    // Sort by CPM descending
    uniqueScores.sort((a, b) => b.cpm - a.cpm);

    // Display top 10
    uniqueScores.slice(0, 10).forEach((score, index) => {
      const entry = document.createElement('div');
      entry.className = `leaderboard-entry rank-${index + 1}`;
      if (score.name === currentUser) {
        entry.classList.add('highlight');
      }

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
    entries.innerHTML = '<div class="error-message">Error loading leaderboard data</div>';
  }
}

// Add difficulty tab handling
document.querySelectorAll('.diff-tab').forEach(tab => {
  tab.addEventListener('click', (e) => {
    document.querySelectorAll('.diff-tab').forEach(t => t.classList.remove('active'));
    e.target.classList.add('active');
    showLeaderboard(e.target.dataset.difficulty);
  });
});

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

async function showFullLeaderboard(difficulty = 'Easy') {
  // Create modal if it doesn't exist
  let modal = document.getElementById('fullLeaderboardModal');
  if (!modal) {
    modal = document.createElement('div');
    modal.id = 'fullLeaderboardModal';
    modal.className = 'full-leaderboard-modal';
    modal.innerHTML = `
      <div class="modal-content">
        <span class="close-modal">&times;</span>
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

    // Add event listeners
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
    const result = await ipcRenderer.invoke('run-typing-tutor', ['--get-leaderboard']);
    if (!result) throw new Error('No leaderboard data received');

    const entries = document.getElementById('full-leaderboard-entries');
    entries.innerHTML = '';

    const lines = result.trim().split('\n')
      .filter(line => line.includes(difficulty))
      .map(line => {
        const [name, cpm, wpm, accuracy, diff] = line.split(' ');
        return {
          name,
          cpm: parseFloat(cpm),
          wpm: parseFloat(wpm),
          accuracy: parseFloat(accuracy),
          difficulty: diff
        };
      });

    // Remove duplicates keeping highest score
    const uniqueScores = lines.reduce((acc, current) => {
      const existing = acc.find(item => item.name === current.name);
      if (!existing || existing.cpm < current.cpm) {
        const index = acc.findIndex(item => item.name === current.name);
        if (index !== -1) {
          acc.splice(index, 1);
        }
        acc.push(current);
      }
      return acc;
    }, []);

    // Sort by CPM descending
    uniqueScores.sort((a, b) => b.cpm - a.cpm);

    // Display all entries
    uniqueScores.forEach((score, index) => {
      const entry = document.createElement('div');
      entry.className = `leaderboard-entry rank-${index + 1}`;
      if (score.name === currentUser) {
        entry.classList.add('highlight');
      }

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
  }
}

// Add click handler for the view all button
document.addEventListener('DOMContentLoaded', () => {
  // ...existing DOMContentLoaded code...
  
  document.getElementById('viewAllBtn').addEventListener('click', () => {
    const activeDifficulty = document.querySelector('.diff-tab.active').dataset.difficulty;
    showFullLeaderboard(activeDifficulty);
  });
});
