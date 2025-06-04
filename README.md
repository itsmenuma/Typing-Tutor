# 🧠 Typing Tutor Desktop ⌨️

> _Boost your typing speed and accuracy — one paragraph at a time!_

Typing Tutor Desktop is a **modern desktop application** (Electron + C backend) designed to help users improve their **typing speed**, **accuracy**, and **confidence**. It features a beautiful UI, customizable difficulty, real-time performance feedback, and a dynamic leaderboard.

---

## ✨ Features

- 📄 **Random Paragraph Selection**
  - Each typing session presents a new, randomly chosen paragraph from a categorized text file.

- ⚡ **Typing Speed & Accuracy Analysis**
  - Measures your typing speed in **Characters Per Minute (CPM)** and calculates **accuracy** as a percentage.

- 🎯 **Difficulty Levels**
  - Choose from **Easy**, **Medium**, or **Hard**. Each level has its own set of paragraphs and stricter speed thresholds.

- 🖥️ **Modern Desktop UI**
  - Built with Electron and styled for a clean, responsive, and attractive look.

- 📝 **Case Sensitivity Option**
  - Toggle case sensitivity for a more challenging typing test.

- 📊 **Dynamic Leaderboard**
  - Displays the top 5 scores for each difficulty level.
  - Highlights the current user if they are on the leaderboard.
  - Ensures each user has only one entry per difficulty, updating their score if they achieve a better result.

- 🧑‍💻 **Custom Username**
  - Users can enter their name before starting the test, which will be displayed on the leaderboard.

---

## 🚀 Getting Started

### ✅ Prerequisites

- **Node.js** and **npm** (for Electron)
- A **C compiler** (e.g., `gcc`) to build the backend
- A text file named **`paragraphs.txt`** in the `build/` folder, containing paragraphs for typing practice, formatted as:

#Easy ...easy paragraphs... #Medium ...medium paragraphs... #Hard ...hard paragraphs...


### 🛠️ How to Use

1. **Build the C Backend**
   - Compile `typingtutor.c` to `typingtutor.exe` and place it in the `build/` folder.
   - cmd for windows = gcc typingtutor.c -o build/typingtutor.exe
  - cmd for Mac/ windows = gcc typingtutor.c -o build/typingtutor.exe

2. **Prepare Paragraphs**
   - Edit `build/paragraphs.txt` with your own categorized paragraphs.

3. **Start the App**
   - In the `app/` folder, run:
     ```bash
     cd app
     npm install
     npm start
     ```

4. **Enter Your Name**
   - Enter your name in the input field before starting the test.

5. **Choose Difficulty & Options**
   - Select Easy, Medium, or Hard and toggle case sensitivity.

6. **Start Typing**
   - Click "Start Typing Test" to get a random paragraph. Type it as fast and accurately as possible.

7. **Get Your Stats**
   - See your CPM, WPM, accuracy, and performance feedback instantly.

8. **View the Leaderboard**
   - Click "Leaderboard" to see the top 5 scores for the selected difficulty.
   - Your name will be highlighted if you are on the leaderboard.

---

## 📁 Files

- `app/` – Electron frontend (HTML, CSS, JS)
- `build/typingtutor.exe` – Compiled C backend
- `build/paragraphs.txt` – Paragraphs for practice (categorized)
- `build/leaderboard.txt` – Dynamic leaderboard file
- `typingtutor.c` – Main C source code

---

## 🤝 Contributing

We ❤️ contributions!

- Fork the repository
- Create a new branch: `git checkout -b feature-name`
- Commit your changes
- Push to your branch: `git push origin feature-name`
- Open a **pull request**

If you find a bug or have a feature suggestion, please [open an issue](https://github.com/itsmenuma/Typing-Tutor/issues) with details!

---

## 📜 License

Licensed under the [MIT License](LICENSE)

---

## 🙌 Acknowledgments

Inspired by traditional typing tutor tools and modern desktop app design.

---

## 📬 Contact

Got a question or feedback? Reach out at: **[numarahamath@gmail.com](mailto:numarahamath@gmail.com)**