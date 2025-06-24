const { ipcRenderer } = require('electron');

let currentParagraph = '';
let selectedDifficulty = 'Easy';
let currentUser = '';
let startTime = 0;
let testMode = "paragraph";
let timeDuration = 1;
let timerInterval = null;
let timedTestActive = false;

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

// Add function to set test mode
window.setTestMode = function (mode) {
  testMode = mode;
  document
    .getElementById("paragraphModeBtn")
    .classList.toggle("active", mode === "paragraph");
  document
    .getElementById("timedModeBtn")
    .classList.toggle("active", mode === "timed");
  document.getElementById("timeDurationContainer").style.display =
    mode === "timed" ? "block" : "none";
  checkStartConditions();
};

// Add function to set time duration
window.setTimeDuration = function (minutes) {
  timeDuration = minutes;
  document
    .getElementById("time1MinBtn")
    .classList.toggle("active", minutes === 1);
  document
    .getElementById("time3MinBtn")
    .classList.toggle("active", minutes === 3);
  document
    .getElementById("time5MinBtn")
    .classList.toggle("active", minutes === 5);
};

// Add this function to check if we can enable the start button
function checkStartConditions() {
    const username = document.getElementById('username').value.trim();
    const useCustom = document.getElementById('useCustomText').checked;
    const customText = document.getElementById('customParagraph').value.trim();

    const hasValidCustomText = useCustom && customText.length > 0;
    const hasValidDifficulty = !useCustom && selectedDifficulty;

    const startBtn = document.getElementById('startBtn');
    startBtn.disabled = !(username && (hasValidCustomText || hasValidDifficulty));
}

// Update setDifficulty function
window.setDifficulty = function(level) {
    selectedDifficulty = level.charAt(0).toUpperCase() + level.slice(1).toLowerCase();
    document.getElementById('easyBtn').style.fontWeight = (level === 'Easy') ? 'bold' : 'normal';
    document.getElementById('mediumBtn').style.fontWeight = (level === 'Medium') ? 'bold' : 'normal';
    document.getElementById('hardBtn').style.fontWeight = (level === 'Hard') ? 'bold' : 'normal';
    checkStartConditions(); // Check if we can enable start button
};

// Function to start the timer for timed tests
function startTimer(minutes) {
  clearInterval(timerInterval); // Clear any existing timers

  const timerDisplay = document.getElementById("timerDisplay");
  const timeLeftSpan = document.getElementById("timeLeft");
  timerDisplay.style.display = "block";

  let totalSeconds = minutes * 60;
  timedTestActive = true;

  function updateTimer() {
    const mins = Math.floor(totalSeconds / 60);
    const secs = totalSeconds % 60;
    timeLeftSpan.textContent = `${mins}:${secs.toString().padStart(2, "0")}`;

    if (totalSeconds <= 0) {
      clearInterval(timerInterval);
      timedTestActive = false;
      submitTyping(true); // Auto-submit when time is up
      return;
    }
    totalSeconds--;
  }

  updateTimer(); // Call immediately to show initial time
  timerInterval = setInterval(updateTimer, 1000);
}

// Update runTypingTutor to handle both modes
window.runTypingTutor = async function () {
    document.getElementById("userInput").classList.remove("dimmed");

    if (!setUsername()) return; // Validate username first
    if (!selectedDifficulty) return;

    document.getElementById("output").innerText = "Loading...";
    document.getElementById("result").innerText = "";
    document.getElementById("userInput").value = "";
    document.getElementById("submitBtn").disabled = false; // Enable submit on new test
    document.getElementById("timerDisplay").style.display = "none"; // Hide timer initially

    const useCustom = document.getElementById('useCustomText').checked;
    const customText = document.getElementById('customParagraph').value.trim();

    document.getElementById('userInput').disabled = false;

    if (useCustom && customText.length > 0) {
        currentParagraph = customText;
        document.getElementById('output').innerText = currentParagraph;
    } else {
        const arg = ["--get-paragraph", selectedDifficulty];
        const result = await ipcRenderer.invoke("run-typing-tutor", arg);

        // Handle paragraph file error
        if (result.startsWith("ERROR:")) {
            showToast(result.replace("ERROR:", "").trim(), true); // Show and exit
            document.getElementById("output").innerText = "Could not load paragraph!";
            return;
        }

        const match = result.match(/Random Paragraph:\s*([\s\S]*)/);
        currentParagraph = match ? match[1].trim() : '';
        document.getElementById('output').innerText = currentParagraph || "Could not load paragraph!";
    }

    if (testMode === "timed") {
        startTimer(timeDuration);
    }

    startTime = Date.now();
    typingSpeedFactor = 1.0;
    lastKeyPressTime = 0;
};

// Update submitTyping to handle both modes
window.submitTyping = async function (isTimedEnd = false) {
  if (!currentUser) return; // Ensure we have a username

  // For timed mode with auto-submit, make sure timer is cleared
  if (isTimedEnd) {
    clearInterval(timerInterval);
    document.getElementById("timerDisplay").style.display = "none";
  } else if (timedTestActive) {
    // If manually submitting during a timed test
    clearInterval(timerInterval);
    document.getElementById("timerDisplay").style.display = "none";
    timedTestActive = false;
  }

  const userInput = document.getElementById("userInput").value;
  if (!userInput.trim()) {
    alert("Please type something before submitting.");
    return;
  }
  const timeTaken = (Date.now() - startTime) / 1000; // seconds
  const caseInsensitive = document.getElementById('caseSensitive').checked ? 0 : 1;

  // Add test mode to arguments
  const args = [
    currentUser,
    selectedDifficulty,
    caseInsensitive.toString(),
    timeTaken.toString(),
    userInput,
    currentParagraph,
    testMode,
    timeDuration.toString(),
  ];

  const result = await ipcRenderer.invoke("run-typing-tutor", args);

  // Only show the stats part (everything after "Typing Stats:")
  const statsMatch = result.match(/Typing Stats:\n([\s\S]*)/);
  document.getElementById("result").innerText = statsMatch
    ? statsMatch[1].trim()
    : result;
    
  document.getElementById("submitBtn").disabled = true; // Disable after submit

  // Disable user input area after submitting the test
  document.getElementById("userInput").disabled = true;
  document.getElementById("userInput").classList.add("dimmed");

  // After updating the leaderboard data, refresh the leaderboard UI:
  showLeaderboard();
};

// Add this function to show toast and optionally exit the app
function showToast(message, exitApp = false) {
  const toast = document.getElementById('toast');
  toast.textContent = message + (exitApp ? " Exiting the app..." : "");
  toast.style.display = 'block';

  // Trigger reflow to restart animation
  toast.style.animation = 'none';
  void toast.offsetHeight;
  toast.style.animation = '';

  setTimeout(() => {
    toast.style.display = 'none';
    if (exitApp) {
      ipcRenderer.send('quit-app'); // Ask main process to exit
    }
  }, 2500);
}


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

// Add event listeners for custom paragraph input
document.getElementById('useCustomText').addEventListener('change', checkStartConditions);
document.getElementById('customParagraph').addEventListener('input', checkStartConditions);

// When page loads, highlight Easy button
document.addEventListener('DOMContentLoaded', function() {
    document.getElementById('easyBtn').style.fontWeight = 'bold';
    checkStartConditions(); // Check if we can enable start button
});


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
