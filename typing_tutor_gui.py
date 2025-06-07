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
        self.root.geometry("900x400")
        self.root.configure(bg="#f0f4f8")
        self.paragraphs = load_paragraphs()
        self.current_paragraph = ""
        self.start_time = None
        self.timer_running = False
        # Theme state
        self.theme = "light"
        self.theme_colors = {
            "light": {
                "bg_main": "#f5f5f2",
                "bg_header": "#eae7dc",
                "fg_header": "#22223b",
                "card_bg": "#fff",
                "card_border": "#bcbcbc",
                "card_fg": "#22223b",
                "entry_bg": "#fff",
                "entry_fg": "#22223b",
                "entry_border": "#3b82f6",
                "feedback_bg": "#f5f5f2",
                "xp_bg": "#eae7dc",
                "xp_fg": "#232946",
                "xp_fill": "#68d391",
                "footer_bg": "#f5f5f2",
                "footer_fg": "#8d99ae",
                "streak_fg": "#b5838d",
                "milestone_fg": "#6d6875",
                "music_btn_bg": "#eae7dc",
                "music_btn_fg": "#22223b",
                "music_btn_active": "#d8c3a5",
            },
            "dark": {
                "bg_main": "#181c24",
                "bg_header": "#232946",
                "fg_header": "#f7f7fa",
                "card_bg": "#232946",
                "card_border": "#393e4c",
                "card_fg": "#f7f7fa",
                "entry_bg": "#232946",
                "entry_fg": "#f7f7fa",
                "entry_border": "#3b82f6",
                "feedback_bg": "#181c24",
                "xp_bg": "#232946",
                "xp_fg": "#f7f7fa",
                "xp_fill": "#68d391",
                "footer_bg": "#181c24",
                "footer_fg": "#a0aec0",
                "streak_fg": "#f59e42",
                "milestone_fg": "#7c3aed",
                "music_btn_bg": "#232946",
                "music_btn_fg": "#f7f7fa",
                "music_btn_active": "#393e4c",
            }
        }
        self.create_widgets()
        self.next_test()

    def create_widgets(self):
        # Background: off-white, typewriter feel
        self.bg_canvas = tk.Canvas(self.root, width=900, height=400, highlightthickness=0)
        self.bg_canvas.place(x=0, y=0, relwidth=1, relheight=1)
        self.bg_canvas.create_rectangle(0, 0, 900, 400, fill="#f5f5f2", outline="")
        self.bg_canvas.create_rectangle(0, 0, 900, 120, fill="#eae7dc", outline="")

        # Header: classic typewriter font, subtle
        self.title_label = tk.Label(self.root, text="Typing Tutor", font=("Courier New", 28, "bold"), bg="#eae7dc", fg="#22223b")
        self.title_label.place(x=0, y=10, relwidth=1)

        # Add theme toggle button
        self.theme_btn = tk.Button(self.root, text="🌙", command=self.toggle_theme, font=("Segoe UI", 12), bg=self.theme_colors[self.theme]["bg_header"], fg=self.theme_colors[self.theme]["fg_header"], bd=0, relief=tk.FLAT, activebackground=self.theme_colors[self.theme]["xp_bg"], cursor="hand2")
        self.theme_btn.place(x=650, y=18, width=32, height=32)

        # Music controls (minimal, typewriter style)
        self.music_frame = tk.Frame(self.root, bg=self.theme_colors[self.theme]["bg_header"])
        self.music_frame.place(x=20, y=18)
        self.music_btn = tk.Button(self.music_frame, text="🔊", command=self.toggle_music, font=("Courier New", 14), bg=self.theme_colors[self.theme]["music_btn_bg"], fg=self.theme_colors[self.theme]["music_btn_fg"], bd=0, relief=tk.FLAT, activebackground=self.theme_colors[self.theme]["music_btn_active"], cursor="hand2")
        self.music_btn.pack(side=tk.LEFT)
        self.music_btn_tip = tk.Label(self.music_frame, text="Toggle background music", font=("Segoe UI", 8), bg=self.theme_colors[self.theme]["bg_header"], fg="#8d99ae")
        self.music_btn.bind("<Enter>", lambda e: self.music_btn_tip.place(x=40, y=0))
        self.music_btn.bind("<Leave>", lambda e: self.music_btn_tip.place_forget())
        self.music_playing = False
        self.music_loaded = False
        if SOUND_ENABLED:
            try:
                pygame.mixer.music.load("app/bg-ambience.wav")
                pygame.mixer.music.set_volume(50)
                self.music_loaded = True
            except Exception:
                self.music_loaded = False

        # Gamification variables
        self.streak = 0
        self.xp = 0
        self.level = 1
        # XP bar (subtle, typewriter style)
        self.xp_bar = tk.Canvas(self.root, width=320, height=18, bg="#eae7dc", highlightthickness=0)
        self.xp_bar.place(x=190, y=60)
        self.xp_bar_tip = tk.Label(self.root, text="XP progress to next level", font=("Segoe UI", 8), bg="#eae7dc", fg="#8d99ae")
        self.xp_bar.bind("<Enter>", lambda e: self.xp_bar_tip.place(x=510, y=60))
        self.xp_bar.bind("<Leave>", lambda e: self.xp_bar_tip.place_forget())
        self.streak_label = tk.Label(self.root, text="", font=("Courier New", 13, "bold"), bg="#f5f5f2", fg="#b5838d")
        self.streak_label.place(x=30, y=100)
        self.milestone_label = tk.Label(self.root, text="", font=("Courier New", 13, "bold"), bg="#f5f5f2", fg="#6d6875")
        self.milestone_label.place(x=520, y=100)

        # Paragraph display as a card (typewriter: white, bold, shadow, very visible)
        # Move the paragraph card higher and make the font/cursor more visible
        self.paragraph_card = tk.Frame(self.root, bg="#fff", bd=0, highlightbackground="#bcbcbc", highlightthickness=2)
        self.paragraph_card.place(x=30, y=90, width=840, height=90)  # y changed from 130 to 90
        self.paragraph_label = tk.Label(
            self.paragraph_card,
            text="",
            wraplength=820,
            font=("Courier New", 22, "bold"),  # Larger font for visibility
            bg="#fff",
            fg="#22223b",
            padx=16,
            pady=16,
            justify=tk.LEFT
        )
        self.paragraph_label.pack(expand=True, fill=tk.BOTH)

        # Feedback canvas: always visible, fixed height, placed just below paragraph card
        self.feedback_canvas = tk.Canvas(self.root, width=820, height=70, bg="#f5f5f2", highlightthickness=0)
        self.feedback_canvas.place(x=40, y=185)

        # Entry for user input: fixed position below feedback
        self.entry_frame = tk.Frame(self.root, bg="#f5f5f2", highlightbackground="#3b82f6", highlightthickness=2, bd=0)
        self.entry_frame.place(x=40, y=245, width=820, height=60)  # y changed from 235 to 245
        self.entry = tk.Text(self.entry_frame, height=2, width=110, font=("Courier New", 15), wrap=tk.WORD, bd=0, fg="#22223b", bg="#fff", insertbackground="#3b82f6")  # Blue insert cursor
        self.entry.pack(expand=True, fill=tk.BOTH, padx=4, pady=4)
        self.entry.config(insertwidth=3, insertofftime=200, insertontime=600)  # Thicker, blinking cursor
        self.entry.bind("<KeyPress>", self.on_keypress)
        self.entry.bind("<KeyRelease>", self.update_canvas_feedback)
        self.entry.bind("<ButtonRelease>", self.update_canvas_feedback)
        self.entry.bind("<FocusIn>", self.update_canvas_feedback)
        self.entry.bind("<<Paste>>", self.update_canvas_feedback)
        self.entry.bind("<<Cut>>", self.update_canvas_feedback)
        # Ensure live stats update starts on any keypress
        self.entry.bind("<KeyPress>", self.start_live_stats_update, add='+')

        # Stats panel (typewriter: white bg, blue/gray text)
        self.stats_panel = tk.Frame(self.root, bg="#f5f5f2", highlightbackground="#bcbcbc", highlightthickness=1, bd=0)
        self.stats_panel.place(x=40, y=335, width=820, height=38)
        self.feedback_label = tk.Label(self.stats_panel, text="", font=("Segoe UI", 12), bg="#f5f5f2", fg="#38a169")
        self.feedback_label.pack(side=tk.LEFT, padx=8)
        # Use a frame to keep stats grouped and inside the box
        self.stats_values_frame = tk.Frame(self.stats_panel, bg="#f5f5f2")
        self.stats_values_frame.pack(side=tk.RIGHT, padx=8)
        self.wpm_label = tk.Label(self.stats_values_frame, text="WPM: 0.0", font=("Segoe UI", 12), bg="#f5f5f2", fg="#3b82f6")
        self.wpm_label.pack(side=tk.LEFT, padx=(0, 10))
        self.acc_label = tk.Label(self.stats_values_frame, text="Accuracy: 0.0%", font=("Segoe UI", 12), bg="#f5f5f2", fg="#3b82f6")
        self.acc_label.pack(side=tk.LEFT, padx=(0, 10))
        self.time_label = tk.Label(self.stats_values_frame, text="Time: 0.0s", font=("Segoe UI", 12), bg="#f5f5f2", fg="#3b82f6")
        self.time_label.pack(side=tk.LEFT)

        # Next button (classic, blue, bold)
        self.next_button = tk.Button(self.root, text="Next Paragraph", command=self.next_test, font=("Segoe UI", 12, "bold"), bg="#3b82f6", fg="#fff", activebackground="#2563eb", activeforeground="#fff", bd=0, relief=tk.FLAT, cursor="hand2", highlightthickness=0)
        # Place the button just below the entry box, right-aligned
        self.next_button.place(x=740, y=375, width=120, height=32)  # y changed from 305 to 375

        # Footer (subtle)
        self.footer = tk.Label(self.root, text="© 2025 Typing Tutor | Enhanced by GitHub Copilot", font=("Segoe UI", 9), bg="#f5f5f2", fg="#8d99ae")
        self.footer.place(x=0, y=410, relwidth=1)

        # Ensure XP bar is visible at startup
        self.update_xp_bar()

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

    def play_trumpet(self):
        if SOUND_ENABLED:
            try:
                pygame.mixer.Sound('success.wav').play()
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
        popup.geometry("300x120+{}+{}".format(self.root.winfo_x() + 200, self.root.winfo_y() - 150))
        popup.overrideredirect(True)
        popup.configure(bg="#e6fffa")
        # Rounded corners and drop shadow (simulate with border and padding)
        popup.attributes("-topmost", True)
        frame = tk.Frame(popup, bg="#e6fffa", bd=0, highlightthickness=0)
        frame.pack(expand=True, fill=tk.BOTH, padx=8, pady=8)
        label = tk.Label(frame, text=message, font=("Helvetica", 14, "bold"), bg="#e6fffa", fg="#2c7a7b", wraplength=260)
        label.pack(expand=True, fill=tk.BOTH, pady=20)
        # Simulate drop shadow
        shadow = tk.Label(popup, bg="#b2f5ea")
        shadow.place(x=6, y=6, relwidth=1, relheight=1)
        frame.lift()
        # Slide in from top
        def slide_in(y):
            if y < 60:
                popup.geometry(f"300x120+{self.root.winfo_x() + 200}+{self.root.winfo_y() + y}")
                self.root.after(10, lambda: slide_in(y + 12))
            else:
                self.root.after(1200, lambda: slide_out(y))
        def slide_out(y):
            if y > -150:
                popup.geometry(f"300x120+{self.root.winfo_x() + 200}+{self.root.winfo_y() + y}")
                self.root.after(10, lambda: slide_out(y - 16))
            else:
                popup.destroy()
        slide_in(-150)

    def show_streak_popup(self):
        popup = tk.Toplevel(self.root)
        popup.title("Streak!")
        pygame.mixer.Sound('app/levelup.wav').play()
        popup.geometry(f"220x80+{self.root.winfo_x() + 250}+{self.root.winfo_y() + 60}")
        popup.overrideredirect(True)
        popup.configure(bg="#fffbe6")
        popup.attributes("-topmost", True)
        streak_label = tk.Label(popup, text=f"🔥 Streak! {self.streak}", font=("Segoe UI", 18, "bold"), bg="#fffbe6", fg="#d97706")
        streak_label.pack(expand=True, fill=tk.BOTH, pady=10)
        def fade_out(step=0):
            if step < 10:
                popup.attributes("-alpha", 1 - step * 0.1)
                self.root.after(60, lambda: fade_out(step + 1))
            else:
                popup.destroy()
        self.root.after(1200, fade_out)

    def show_milestone_popup(self):
        popup = tk.Toplevel(self.root)
        popup.title("Milestone!")
        popup.geometry(f"260x90+{self.root.winfo_x() + 220}+{self.root.winfo_y() + 100}")
        popup.overrideredirect(True)
        popup.configure(bg="#e0e7ff")
        popup.attributes("-topmost", True)
        label = tk.Label(popup, text="🏅 Milestone Unlocked!", font=("Segoe UI", 16, "bold"), bg="#e0e7ff", fg="#3730a3")
        label.pack(expand=True, fill=tk.BOTH, pady=18)
        pygame.mixer.Sound('app/levelup.wav').play()
        def fade_out(step=0):
            if step < 10:
                popup.attributes("-alpha", 1 - step * 0.1)
                self.root.after(60, lambda: fade_out(step + 1))
            else:
                popup.destroy()
        self.root.after(1400, fade_out)

    def update_live_stats(self, event=None):
        if not self.timer_running:
            return
        user_input = self.entry.get('1.0', tk.END).strip()
        elapsed = time.time() - self.start_time if self.start_time else 0
        wpm = len(user_input.split()) / (elapsed / 60) if elapsed > 0 else 0
        accuracy = sum(1 for a, b in zip(user_input, self.current_paragraph) if a == b) / max(len(self.current_paragraph), 1) * 100
        self.wpm_label.config(text=f"WPM: {wpm:.1f}")
        self.acc_label.config(text=f"Accuracy: {accuracy:.1f}%")
        self.time_label.config(text=f"Time: {elapsed:.1f}s")
        self.root.after(500, self.update_live_stats)

    def update_canvas_feedback(self, event=None):
        # Real-time colored feedback: green for correct, red for incorrect, gray for next chars
        self.feedback_canvas.delete("all")
        user_input = self.entry.get('1.0', tk.END).rstrip('\n')
        para = self.current_paragraph
        x, y = 5, 10
        font = ("Consolas", 13)
        max_width = 610
        line_height = 22
        for i, c in enumerate(user_input):
            if i < len(para):
                color = "#38a169" if c == para[i] else "#e53e3e"
            else:
                color = "#a0aec0"
            self.feedback_canvas.create_text(x, y, text=c, anchor="nw", font=font, fill=color)
            x += 12
            if x > max_width:
                x = 5
                y += line_height
        for j in range(len(user_input), min(len(para), len(user_input)+10)):
            self.feedback_canvas.create_text(x, y, text=para[j], anchor="nw", font=font, fill="#a0aec0")
            x += 12
            if x > max_width:
                x = 5
                y += line_height
        # Do NOT resize or move widgets dynamically here; keep feedback always visible in its area.

    def shake_widget(self, widget, shakes=8, dist=4, speed=20):
        # Shake animation for mistakes
        orig_x = widget.winfo_x()
        orig_y = widget.winfo_y()
        def shake(count):
            if count > 0:
                offset = dist if count % 2 == 0 else -dist
                widget.place(x=orig_x + offset, y=orig_y)
                self.root.after(speed, lambda: shake(count - 1))
            else:
                widget.place(x=orig_x, y=orig_y)
        widget.place(x=widget.winfo_x(), y=widget.winfo_y())
        shake(shakes)

    def on_keypress(self, event):
        if not self.timer_running:
            self.start_time = time.time()
            self.timer_running = True
            self.update_live_stats()
        # Realtime feedback: shake on wrong key
        user_input = self.entry.get('1.0', tk.END).rstrip('\n')
        para = self.current_paragraph
        if event.char and len(user_input) < len(para):
            idx = len(user_input)
            if event.char != para[idx] and event.char != "\r" and event.char != "\n":
                # Shake the entire typing area for more visible feedback
                self.shake_widget(self.entry_frame)
                pygame.mixer.Sound('error.wav').play()
        # Update colored feedback
        self.root.after(1, self.update_canvas_feedback)

    def fade_paragraph(self, text, step=0, fade_out=True):
        # Fade out/in effect for paragraph transitions
        if fade_out:
            color = f"#{255-step*20:02x}{255-step*20:02x}{255-step*20:02x}"
            self.paragraph_label.config(fg=color)
            if step < 10:
                self.root.after(20, lambda: self.fade_paragraph(text, step+1, True))
            else:
                self.paragraph_label.config(text=text)
                self.fade_paragraph(text, 0, False)
        else:
            color = f"#{55+step*20:02x}{55+step*20:02x}{55+step*20:02x}"
            self.paragraph_label.config(fg=color)
            if step < 10:
                self.root.after(20, lambda: self.fade_paragraph(text, step+1, False))
            else:
                self.paragraph_label.config(fg="#2d3748")

    def next_test(self):
        # Pick a new paragraph and limit to 80 chars, split at space if possible
        new_para = random.choice(self.paragraphs)
        if len(new_para) > 80:
            # Try to split at the last space before 80
            split_idx = new_para.rfind(' ', 0, 80)
            if split_idx == -1:
                split_idx = 80
            new_para = new_para[:split_idx].strip()
        self.fade_paragraph(new_para)
        self.current_paragraph = new_para
        self.entry.delete('1.0', tk.END)
        self.feedback_label.config(text="")
        self.wpm_label.config(text="WPM: 0.0")
        self.acc_label.config(text="Accuracy: 0.0%")
        self.time_label.config(text="Time: 0.0s")
        self.timer_running = False
        self.start_time = None
        self.update_canvas_feedback()

    def toggle_theme(self):
        self.theme = "dark" if self.theme == "light" else "light"
        c = self.theme_colors[self.theme]
        self.root.configure(bg=c["bg_main"])
        self.bg_canvas.itemconfig(1, fill=c["bg_main"])
        self.bg_canvas.itemconfig(2, fill=c["bg_header"])
        self.title_label.config(bg=c["bg_header"], fg=c["fg_header"])
        self.theme_btn.config(bg=c["bg_header"], fg=c["fg_header"], text="☀️" if self.theme=="dark" else "🌙")
        self.music_frame.config(bg=c["bg_header"])
        self.music_btn.config(bg=c["music_btn_bg"], fg=c["music_btn_fg"], activebackground=c["music_btn_active"])
        self.music_btn_tip.config(bg=c["bg_header"])
        self.xp_bar.config(bg=c["xp_bg"])
        self.xp_bar_tip.config(bg=c["xp_bg"])
        self.streak_label.config(bg=c["bg_main"], fg=c["streak_fg"])
        self.milestone_label.config(bg=c["bg_main"], fg=c["milestone_fg"])
        self.paragraph_card.config(bg=c["card_bg"], highlightbackground=c["card_border"])
        self.paragraph_label.config(bg=c["card_bg"], fg=c["card_fg"])
        self.feedback_canvas.config(bg=c["feedback_bg"])
        self.entry_frame.config(bg=c["feedback_bg"], highlightbackground=c["entry_border"])
        self.entry.config(bg=c["entry_bg"], fg=c["entry_fg"], insertbackground=c["entry_fg"])
        self.stats_panel.config(bg=c["feedback_bg"])
        self.feedback_label.config(bg=c["feedback_bg"])
        self.stats_label.config(bg=c["feedback_bg"])
        self.next_button.config(bg="#3b82f6" if self.theme=="light" else "#2563eb", fg="#fff", activebackground="#2563eb")
        self.footer.config(bg=c["footer_bg"], fg=c["footer_fg"])
        self.update_xp_bar()

    def toggle_music(self):
        if not SOUND_ENABLED or not self.music_loaded:
            return
        if self.music_playing:
            pygame.mixer.music.pause()
            self.music_btn.config(text="🔇")  # Muted mic
            self.music_playing = False
        else:
            if pygame.mixer.music.get_busy():
                pygame.mixer.music.unpause()
            else:
                pygame.mixer.music.play(-1)
            self.music_btn.config(text="🔊")  # Speaker
            self.music_playing = True

    def update_xp_bar(self):
        self.xp_bar.delete("all")
        fill = min(self.xp % 100, 100) * 3.2
        self.xp_bar.create_rectangle(0, 0, 320, 18, fill="#232946", outline="")
        self.xp_bar.create_rectangle(0, 0, fill, 18, fill="#68d391", outline="")
        self.xp_bar.create_text(160, 9, text=f"Level {self.level}  XP: {self.xp % 100}/100", fill="#22543d", font=("Segoe UI", 10, "bold"), anchor="c")

    def show_streak_badge(self):
        self.streak_label.config(text=f"🔥 Streak: {self.streak}!")
        self.streak_label.after(2000, lambda: self.streak_label.config(text=""))

    def show_milestone_badge(self):
        self.milestone_label.config(text=f"🏅 Milestone Unlocked!")
        self.milestone_label.after(2200, lambda: self.milestone_label.config(text=""))

    def animate_stats(self, wpm, accuracy, elapsed):
        # Animate WPM/accuracy counting up inside the stats panel
        def animate(val, target, label, fmt, step=0):
            if val < target:
                val = min(val + max(target/20, 1), target)
                label.config(text=fmt.format(val))
                self.root.after(30, lambda: animate(val, target, label, fmt))
            else:
                label.config(text=fmt.format(target))
        self.wpm_label.config(text="WPM: 0.0")
        self.acc_label.config(text="Accuracy: 0.0%")
        self.time_label.config(text=f"Time: {elapsed:.1f}s")
        animate(0, wpm, self.wpm_label, "WPM: {:.1f}")
        animate(0, accuracy, self.acc_label, "Accuracy: {:.1f}%")

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
            # Gamification: streak, XP, badges
            self.streak += 1
            self.xp += 20
            streak_popup = False
            milestone_popup = False
            show_streak = self.streak > 0 and self.streak % 5 == 0
            show_levelup = self.xp // 100 + 1 > self.level
            show_milestone = self.streak > 0 and self.streak % 5 == 0
            if show_levelup:
                self.level += 1
            self.update_xp_bar()
            self.animate_stats(wpm, accuracy, elapsed)
            self.root.after(1, self.update_canvas_feedback)
            # Chain popups: motivational (1.2s), then streak (if any, 1.2s), then milestone (if any, 1.4s)
            delay = 1200
            if show_streak or show_levelup:
                self.root.after(delay, self.show_streak_popup)
                delay += 1200
            if show_milestone:
                self.root.after(delay, self.show_milestone_popup)
                delay += 1400
            # Automatically go to next paragraph after all popups
            self.root.after(delay, self.next_test)
        else:
            self.feedback_label.config(text="Keep trying!", fg="#e53e3e")
            self.animate_feedback(color1="#e53e3e")
            self.play_sound("error")
            self.streak = 0
            self.update_xp_bar()
            self.animate_stats(wpm, accuracy, elapsed)
            self.root.after(1, self.update_canvas_feedback)

    def run(self):
        # Bind check_result to Return key
        self.root.bind('<Return>', lambda e: self.check_result())
        self.root.mainloop()

    def start_live_stats_update(self, event=None):
        if not self.timer_running:
            self.start_time = time.time()
            self.timer_running = True
            self.update_live_stats()

if __name__ == "__main__":
    root = tk.Tk()
    app = TypingTutorGUI(root)
    app.run()
