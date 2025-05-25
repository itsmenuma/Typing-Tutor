const { ipcRenderer } = require('electron');

let currentParagraph = '';
let startTime = 0;
let selectedDifficulty = null; // No default

window.setDifficulty = function(level) {
  selectedDifficulty = level.charAt(0).toUpperCase() + level.slice(1).toLowerCase();
  document.getElementById('easyBtn').style.fontWeight = (level === 'Easy') ? 'bold' : 'normal';
  document.getElementById('mediumBtn').style.fontWeight = (level === 'Medium') ? 'bold' : 'normal';
  document.getElementById('hardBtn').style.fontWeight = (level === 'Hard') ? 'bold' : 'normal';
  document.getElementById('startBtn').disabled = false; // Enable start button
};

window.runTypingTutor = async function() {
  if (!selectedDifficulty) return;
  document.getElementById('output').innerText = 'Loading...';
  document.getElementById('result').innerText = '';
  document.getElementById('userInput').value = '';
  // Pass the selected difficulty to the backend
  const result = await ipcRenderer.invoke('run-typing-tutor', ['--get-paragraph', selectedDifficulty]);
  console.log("Backend result:", result); // Debug: See what you get
  const match = result.match(/Random Paragraph:\s*([\s\S]*)/);
  currentParagraph = match ? match[1].trim() : '';
  document.getElementById('output').innerText = currentParagraph || "Could not load paragraph!";
  startTime = Date.now();
};

window.submitTyping = async function() {
  const userInput = document.getElementById('userInput').value;
  const timeTaken = (Date.now() - startTime) / 1000; // seconds
  const username = "guest";
  const caseInsensitive = document.getElementById('caseSensitive').checked ? 0 : 1; // 0 = case sensitive, 1 = insensitive

  const args = [
    username,
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
};

document.getElementById('userInput').addEventListener('keydown', function(e) {
  if (e.key === 'Enter' && !e.shiftKey) {
    e.preventDefault();
    submitTyping();
  }
});