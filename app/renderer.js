const { ipcRenderer } = require('electron');

let currentParagraph = '';
let selectedDifficulty = 'Easy'; // Set default difficulty
let currentUser = ''; // Add this to store username globally
let testMode = "paragraph";
let timeDuration = 1;
let timerInterval = null;
let timedTestActive = false;
let startTime = 0;


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
  if (!setUsername()) return; // Validate username first
  if (!selectedDifficulty) return;

  document.getElementById("output").innerText = "Loading...";
  document.getElementById("result").innerText = "";
  document.getElementById("userInput").value = "";
  document.getElementById("submitBtn").disabled = false; // Enable submit on new test
  document.getElementById("timerDisplay").style.display = "none"; // Hide timer initially

  // Fetch multiple paragraphs for timed mode
  const arg = ["--get-paragraph", selectedDifficulty];

  if (testMode === "timed") {
    let result = await ipcRenderer.invoke("run-typing-tutor", ["--get-paragraph", "Hard"]);
    let match = result.match(/Random Paragraph:\s*([\s\S]*)/);
    currentParagraph = match ? match[1].trim() : "";
    document.getElementById("output").innerText =
      currentParagraph || "Could not load paragraph!";

    startTime = Date.now();
    startTimer(timeDuration);
  } else {
    // For paragraph mode, just display the paragraph
    const result = await ipcRenderer.invoke("run-typing-tutor", arg);
    const match = result.match(/Random Paragraph:\s*([\s\S]*)/);
    currentParagraph = match ? match[1].trim() : "";
    document.getElementById("output").innerText =
      currentParagraph || "Could not load paragraph!";
    console.log(currentParagraph);
    startTime = Date.now();
  }
  
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
  const timeTaken = (Date.now() - startTime) / 1000; // seconds
  const caseInsensitive = document.getElementById("caseSensitive").checked
    ? 0
    : 1;

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