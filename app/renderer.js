const { ipcRenderer } = require('electron');

let currentParagraph = '';
let startTime = 0;
let selectedDifficulty = 'Easy'; // Default

window.setDifficulty = function(level) {
  selectedDifficulty = level;
  // Optional: highlight selected button
  document.getElementById('easyBtn').style.fontWeight = (level === 'Easy') ? 'bold' : 'normal';
  document.getElementById('mediumBtn').style.fontWeight = (level === 'Medium') ? 'bold' : 'normal';
  document.getElementById('hardBtn').style.fontWeight = (level === 'Hard') ? 'bold' : 'normal';
};

window.runTypingTutor = async function() {
  document.getElementById('output').innerText = 'Loading...';
  document.getElementById('result').innerText = '';
  document.getElementById('userInput').value = '';
  const result = await ipcRenderer.invoke('run-typing-tutor', ['--get-paragraph']);
  console.log("Raw result from C:", result); // Debug line
  document.getElementById('output').innerText = result; // Show raw output for now
  // const match = result.match(/Random Paragraph:\n([\s\S]*)/);
  // currentParagraph = match ? match[1].trim() : '';
  // document.getElementById('output').innerText = currentParagraph;
  startTime = Date.now();
};

window.submitTyping = async function() {
  const userInput = document.getElementById('userInput').value;
  const timeTaken = (Date.now() - startTime) / 1000; // seconds
  const username = "guest";
  const caseInsensitive = 1;

  const args = [
    username,
    selectedDifficulty, // Use selected difficulty
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