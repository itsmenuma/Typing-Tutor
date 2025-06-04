import tkinter as tk
from tkinter import messagebox
import random
import time
import threading
import os

# Optional: For sound effects (cross-platform)
try:
    import pygame
    pygame.init()
    SOUND_ENABLED = True
except ImportError:
    SOUND_ENABLED = False

# Motivational messages
MOTIVATIONAL_MESSAGES = [
    "Great job! Keep going!",
    "You're improving!",
    "Fantastic speed!",
    "Accuracy is key!",
    "Keep up the momentum!",
    "You're a typing star!"
]

# Load paragraphs from file (reuse C version's file if possible)
PARAGRAPH_FILE = "build/paragraphs.txt"

def load_paragraphs():
    if not os.path.exists(PARAGRAPH_FILE):
        return ["The quick brown fox jumps over the lazy dog."]
    with open(PARAGRAPH_FILE, 'r') as f:
        return [line.strip() for line in f if line.strip()]

class TypingTutorGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Typing Tutor - Modern GUI")
        self.root.geometry("700x400")
        self.root.configure(bg="#f0f4f8")
        self.paragraphs = load_paragraphs()
        self.current_paragraph = ""
        self.start_time = None
        self.timer_running = False
        self.create_widgets()
        self.next_test()

    def create_widgets(self):
        self.title_label = tk.Label(self.root, text="Typing Tutor", font=("Helvetica", 24, "bold"), bg="#f0f4f8", fg="#2d3748")
        self.title_label.pack(pady=10)

        self.paragraph_label = tk.Label(self.root, text="", wraplength=650, font=("Helvetica", 14), bg="#e2e8f0", fg="#2d3748", padx=10, pady=10)
        self.paragraph_label.pack(pady=10)

        self.entry = tk.Text(self.root, height=5, width=80, font=("Consolas", 13), wrap=tk.WORD, bd=2, relief=tk.GROOVE)
        self.entry.pack(pady=10)
        self.entry.bind("<KeyPress>", self.on_keypress)

        self.feedback_label = tk.Label(self.root, text="", font=("Helvetica", 12), bg="#f0f4f8", fg="#38a169")
        self.feedback_label.pack(pady=5)

        self.stats_label = tk.Label(self.root, text="", font=("Helvetica", 12), bg="#f0f4f8", fg="#2b6cb0")
        self.stats_label.pack(pady=5)

        self.next_button = tk.Button(self.root, text="Next Paragraph", command=self.next_test, font=("Helvetica", 12), bg="#4299e1", fg="white", activebackground="#2b6cb0")
        self.next_button.pack(pady=10)

    def play_sound(self, sound_type):
        if not SOUND_ENABLED:
            return
        # Use simple beep for success/error if pygame is not set up with sound files
        if sound_type == "success":
            try:
                pygame.mixer.Sound('success.wav').play()
            except Exception:
                try:
                    import winsound
                    winsound.MessageBeep(winsound.MB_ICONASTERISK)
                except Exception:
                    pass
        elif sound_type == "error":
            try:
                pygame.mixer.Sound('error.wav').play()
            except Exception:
                try:
                    import winsound
                    winsound.MessageBeep(winsound.MB_ICONHAND)
                except Exception:
                    pass

    def animate_feedback(self, color1="#38a169", color2="#f0f4f8", steps=10, delay=50):
        # Animate feedback label color fade in/out
        def fade(step):
            if step > steps:
                self.feedback_label.config(fg=color1)
                return
            r1, g1, b1 = self.root.winfo_rgb(color1)
            r2, g2, b2 = self.root.winfo_rgb(color2)
            r = int(r1 + (r2 - r1) * step / steps) // 256
            g = int(g1 + (g2 - g1) * step / steps) // 256
            b = int(b1 + (b2 - b1) * step / steps) // 256
            color = f"#{r:02x}{g:02x}{b:02x}"
            self.feedback_label.config(fg=color)
            self.root.after(delay, lambda: fade(step + 1))
        fade(0)

    def show_motivational_popup(self, message):
        popup = tk.Toplevel(self.root)
        popup.title("Motivation!")
        popup.geometry("300x120")
        popup.configure(bg="#e6fffa")
        label = tk.Label(popup, text=message, font=("Helvetica", 14, "bold"), bg="#e6fffa", fg="#2c7a7b")
        label.pack(expand=True, fill=tk.BOTH, pady=20)
        popup.after(1500, popup.destroy)

    def update_live_stats(self, event=None):
        if not self.timer_running:
            return
        user_input = self.entry.get('1.0', tk.END).strip()
        elapsed = time.time() - self.start_time if self.start_time else 0
        wpm = len(user_input.split()) / (elapsed / 60) if elapsed > 0 else 0
        accuracy = sum(1 for a, b in zip(user_input, self.current_paragraph) if a == b) / max(len(self.current_paragraph), 1) * 100
        self.stats_label.config(text=f"WPM: {wpm:.1f}   Accuracy: {accuracy:.1f}%   Time: {elapsed:.1f}s")
        self.root.after(500, self.update_live_stats)

    def on_keypress(self, event):
        if not self.timer_running:
            self.start_time = time.time()
            self.timer_running = True
            self.update_live_stats()
        # Optionally animate or highlight as user types

    def next_test(self):
        self.current_paragraph = random.choice(self.paragraphs)
        self.paragraph_label.config(text=self.current_paragraph)
        self.entry.delete('1.0', tk.END)
        self.feedback_label.config(text="")
        self.stats_label.config(text="")
        self.timer_running = False
        self.start_time = None

    def check_result(self):
        user_input = self.entry.get('1.0', tk.END).strip()
        elapsed = time.time() - self.start_time if self.start_time else 0
        correct = user_input == self.current_paragraph
        wpm = len(user_input.split()) / (elapsed / 60) if elapsed > 0 else 0
        accuracy = sum(1 for a, b in zip(user_input, self.current_paragraph) if a == b) / max(len(self.current_paragraph), 1) * 100
        if correct:
            msg = random.choice(MOTIVATIONAL_MESSAGES)
            self.feedback_label.config(text=msg, fg="#38a169")
            self.animate_feedback()
            self.show_motivational_popup(msg)
            self.play_sound("success")
        else:
            self.feedback_label.config(text="Keep trying!", fg="#e53e3e")
            self.animate_feedback(color1="#e53e3e")
            self.play_sound("error")
        self.stats_label.config(text=f"WPM: {wpm:.1f}   Accuracy: {accuracy:.1f}%   Time: {elapsed:.1f}s")

    def run(self):
        # Bind check_result to Return key
        self.root.bind('<Return>', lambda e: self.check_result())
        self.root.mainloop()

if __name__ == "__main__":
    root = tk.Tk()
    app = TypingTutorGUI(root)
    app.run()
