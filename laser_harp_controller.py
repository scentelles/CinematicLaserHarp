"""
CinematicLaserHarp — OSC Remote Controller
A Python GUI to control the laser harp ESP8266 via OSC over WiFi.

Requirements:
    pip install python-osc

Usage:
    python laser_harp_controller.py
"""

import tkinter as tk
import time
from pythonosc.udp_client import SimpleUDPClient

# ─── OSC Protocol Constants (matching CinematicLaserHarp.ino) ─────────────
OSC_PORT = 8001

OSC_PATH_START_LH = "/laserharp/startLH"
OSC_PATH_START_DMX = "/laserharp/startDMX"
OSC_PATH_START_IDLE = "/laserharp/startIDLE"
OSC_PATH_VIRTUAL_BUTTON = "/laserharp/virtualButton"

# Virtual button codes (from setVirtualButton handler)
BTN_UP     = 1
BTN_DOWN   = 2
BTN_LEFT   = 3
BTN_RIGHT  = 4
BTN_SELECT = 5
LONG_PRESS_OFFSET = BTN_SELECT

# ─── Color Theme ──────────────────────────────────────────────────────────
BG_DARK       = "#0d0d1a"
BG_PANEL      = "#161629"
BG_BUTTON     = "#1e1e3a"
FG_TEXT       = "#e0e0f0"
ACCENT_CYAN   = "#00e5ff"
ACCENT_MAGENTA = "#ff00e5"
ACCENT_GREEN  = "#00ff88"
ACCENT_ORANGE = "#ff9100"
ACCENT_RED    = "#ff3355"
ACCENT_DIM    = "#444466"
GLOW_CYAN     = "#003344"
GLOW_MAGENTA  = "#330033"

DPAD_BTN_W = 5
DPAD_BTN_H = 2
DPAD_PAD   = 6


# ─── Help Text (extracted from the .ino state machine) ────────────────────
HELP_TEXT = """\
╔══════════════════════════════════════════════════╗
║          CINEMATIC LASER HARP — HELP             ║
╠══════════════════════════════════════════════════╣
║                                                  ║
║  DIRECT MODE BUTTONS (top panel)                 ║
║  ─────────────────────────────────               ║
║  🎵 LASER HARP : Start playing with selected     ║
║                   preset number                  ║
║  💡 DMX MODE   : Switch to Art-Net DMX control   ║
║  ⏹ IDLE       : Return to home / stop all       ║
║                                                  ║
╠══════════════════════════════════════════════════╣
║                                                  ║
║  NAVIGATION (D-Pad) — depends on current mode:   ║
║                                                  ║
║  ── HOME / IDLE ──────────────────────────────── ║
║  ▲ UP      → Settings menu                      ║
║  ▼ DOWN    → LaserHarp menu                     ║
║  ● SELECT  → Show device info (v1.0)            ║
║  ● SELECT (long) → WiFi reset prompt            ║
║                                                  ║
║  ── LASERHARP IDLE ───────────────────────────── ║
║  ▲ UP      → Back to Home                       ║
║  ▼ DOWN    → Sequence mode                      ║
║  ◄ LEFT    → Previous preset                    ║
║  ► RIGHT   → Next preset                        ║
║  ● SELECT  → Start playing                      ║
║  ● SELECT (long) → Edit current preset          ║
║                                                  ║
║  ── LASERHARP PLAYING ────────────────────────── ║
║  ► RIGHT   → Stop & return to LaserHarp menu    ║
║                                                  ║
║  ── PRESET EDITING ───────────────────────────── ║
║  ▲ UP / ▼ DOWN  → Change note (+1 / -1)         ║
║  ◄ LEFT / ► RIGHT → Navigate between beams      ║
║  ● SELECT  → Save preset                        ║
║  ● SELECT (long) → Save as new preset           ║
║                                                  ║
║  ── DMX MODE ─────────────────────────────────── ║
║  ▲ UP / ▼ DOWN → Stop DMX                       ║
║  ◄ LEFT   → DMX address -1                      ║
║  ► RIGHT  → DMX address +1                      ║
║  ● SELECT → Store DMX address to EEPROM         ║
║                                                  ║
║  ── CALIBRATION ──────────────────────────────── ║
║  ▲ UP / ▼ DOWN  → Adjust offset (+1 / -1)       ║
║  ◄ LEFT / ► RIGHT → Navigate between beams      ║
║  ● SELECT (long) → Save calibration             ║
║  ● SELECT → Back to calibration menu             ║
║                                                  ║
╚══════════════════════════════════════════════════╝
"""


class LaserHarpController:
    def __init__(self, root):
        self.root = root
        self.root.title("🎵 Cinematic Laser Harp Controller")
        self.root.configure(bg=BG_DARK)
        self.root.resizable(False, False)

        self.osc_client = None
        self.status_var = tk.StringVar(value="Not connected")
        self._select_press_time = 0

        self._build_ui()
        self._center_window()

    # ─── OSC Communication ────────────────────────────────────────────

    def _get_client(self):
        ip = self.ip_entry.get().strip()
        if not ip:
            self._set_status("⚠ Enter ESP8266 IP address", ACCENT_RED)
            return None
        try:
            self.osc_client = SimpleUDPClient(ip, OSC_PORT)
            return self.osc_client
        except Exception as e:
            self._set_status(f"⚠ Invalid IP: {e}", ACCENT_RED)
            return None

    def _send_osc(self, path, *args):
        client = self._get_client()
        if client is None:
            return
        try:
            client.send_message(path, list(args))
            arg_str = ", ".join(str(a) for a in args) if args else ""
            self._set_status(f"✓ Sent {path} {arg_str}", ACCENT_GREEN)
        except Exception as e:
            self._set_status(f"✗ Send failed: {e}", ACCENT_RED)

    def _send_button(self, button_code, long_press=False):
        code = button_code + LONG_PRESS_OFFSET if long_press else button_code
        label = {BTN_UP: "UP", BTN_DOWN: "DOWN", BTN_LEFT: "LEFT",
                 BTN_RIGHT: "RIGHT", BTN_SELECT: "SELECT"}.get(button_code, "?")
        lp = " (long)" if long_press else ""
        self._send_osc(OSC_PATH_VIRTUAL_BUTTON, code)
        self._set_status(f"✓ Button {label}{lp}", ACCENT_CYAN)

    def _set_status(self, text, color=FG_TEXT):
        self.status_var.set(text)
        self.status_label.configure(foreground=color)

    # ─── Help Window ──────────────────────────────────────────────────

    def _show_help(self):
        help_win = tk.Toplevel(self.root)
        help_win.title("Help — Button Reference")
        help_win.configure(bg=BG_DARK)
        help_win.resizable(False, False)

        text = tk.Text(help_win, font=("Consolas", 9), fg=ACCENT_CYAN, bg=BG_DARK,
                       borderwidth=0, padx=16, pady=12, wrap="none",
                       width=54, height=52, cursor="arrow")
        text.insert("1.0", HELP_TEXT)
        text.configure(state="disabled")
        text.pack()

        close_btn = tk.Button(help_win, text="Close", font=("Consolas", 10, "bold"),
                              fg=BG_DARK, bg=ACCENT_CYAN, activebackground=ACCENT_GREEN,
                              borderwidth=0, padx=20, pady=4, cursor="hand2",
                              command=help_win.destroy)
        close_btn.pack(pady=(0, 12))

        # Center on parent
        help_win.transient(self.root)
        help_win.update_idletasks()
        x = self.root.winfo_x() + (self.root.winfo_width() // 2) - (help_win.winfo_width() // 2)
        y = self.root.winfo_y() + 20
        help_win.geometry(f"+{x}+{y}")
        help_win.grab_set()

    # ─── UI Construction ──────────────────────────────────────────────

    def _make_hover_button(self, parent, text, fg_color, bg_color, command,
                           font_size=11, width=14, height=2):
        """Create a button with hover glow effect."""
        btn = tk.Button(parent, text=text, font=("Consolas", font_size, "bold"),
                        fg=fg_color, bg=bg_color, activeforeground="white",
                        activebackground=fg_color, width=width, height=height,
                        borderwidth=0, cursor="hand2", command=command)
        btn.bind("<Enter>", lambda e, b=btn, c=fg_color: b.configure(bg=c, fg="black"))
        btn.bind("<Leave>", lambda e, b=btn, c=bg_color, fc=fg_color: b.configure(bg=c, fg=fc))
        return btn

    def _build_ui(self):
        main = tk.Frame(self.root, bg=BG_DARK, padx=20, pady=16)
        main.pack(fill="both", expand=True)

        # ── Title Row (with help button) ──
        title_row = tk.Frame(main, bg=BG_DARK)
        title_row.pack(fill="x", pady=(0, 12))

        tk.Label(title_row, text="⚡ CINEMATIC LASER HARP ⚡",
                 font=("Consolas", 16, "bold"), fg=ACCENT_CYAN,
                 bg=BG_DARK).pack(side="left", expand=True)

        help_btn = tk.Button(title_row, text=" ? ", font=("Consolas", 12, "bold"),
                             fg=ACCENT_ORANGE, bg=BG_PANEL,
                             activeforeground="black", activebackground=ACCENT_ORANGE,
                             borderwidth=0, cursor="hand2", command=self._show_help)
        help_btn.pack(side="right")
        help_btn.bind("<Enter>", lambda e: help_btn.configure(bg=ACCENT_ORANGE, fg="black"))
        help_btn.bind("<Leave>", lambda e: help_btn.configure(bg=BG_PANEL, fg=ACCENT_ORANGE))

        # ── IP Config ──
        ip_frame = tk.Frame(main, bg=BG_PANEL, padx=12, pady=8,
                            highlightbackground=ACCENT_DIM, highlightthickness=1)
        ip_frame.pack(fill="x", pady=(0, 16))

        tk.Label(ip_frame, text="ESP8266 IP:", font=("Consolas", 10),
                 fg=FG_TEXT, bg=BG_PANEL).pack(side="left")
        self.ip_entry = tk.Entry(ip_frame, font=("Consolas", 12), width=16,
                                 bg=BG_DARK, fg=ACCENT_CYAN, insertbackground=ACCENT_CYAN,
                                 borderwidth=1, relief="solid")
        self.ip_entry.insert(0, "192.168.1.100")
        self.ip_entry.pack(side="left", padx=(8, 0))

        # ── Mode Buttons ──
        mode_frame = tk.LabelFrame(main, text=" MODES ", font=("Consolas", 10, "bold"),
                                   fg=ACCENT_DIM, bg=BG_DARK, padx=10, pady=10,
                                   borderwidth=1, relief="groove")
        mode_frame.pack(fill="x", pady=(0, 16))

        # Preset selector
        preset_row = tk.Frame(mode_frame, bg=BG_DARK)
        preset_row.pack(pady=(0, 10))
        tk.Label(preset_row, text="Preset:", font=("Consolas", 9),
                 fg=FG_TEXT, bg=BG_DARK).pack(side="left")
        self.preset_spin = tk.Spinbox(preset_row, from_=0, to=30, width=4,
                                       font=("Consolas", 12), bg=BG_DARK, fg=ACCENT_CYAN,
                                       buttonbackground=BG_PANEL, borderwidth=1, relief="solid")
        self.preset_spin.pack(side="left", padx=(6, 0))

        # Mode button row — evenly spaced
        btn_row = tk.Frame(mode_frame, bg=BG_DARK)
        btn_row.pack(fill="x")
        btn_row.columnconfigure(0, weight=1)
        btn_row.columnconfigure(1, weight=1)
        btn_row.columnconfigure(2, weight=1)

        mode_defs = [
            ("🎵 LASER HARP", ACCENT_CYAN, GLOW_CYAN,
             lambda: self._send_osc(OSC_PATH_START_LH, int(self.preset_spin.get()))),
            ("💡 DMX MODE", ACCENT_MAGENTA, GLOW_MAGENTA,
             lambda: self._send_osc(OSC_PATH_START_DMX, 0)),
            ("⏹ IDLE", ACCENT_ORANGE, BG_PANEL,
             lambda: self._send_osc(OSC_PATH_START_IDLE, 0)),
        ]
        for i, (text, fg, bg, cmd) in enumerate(mode_defs):
            btn = self._make_hover_button(btn_row, text, fg, bg, cmd)
            btn.grid(row=0, column=i, padx=6, pady=2, sticky="ew")

        # ── D-Pad ──
        dpad_frame = tk.LabelFrame(main, text=" NAVIGATION ", font=("Consolas", 10, "bold"),
                                   fg=ACCENT_DIM, bg=BG_DARK, padx=10, pady=10,
                                   borderwidth=1, relief="groove")
        dpad_frame.pack(fill="x", pady=(0, 12))

        dpad_grid = tk.Frame(dpad_frame, bg=BG_DARK)
        dpad_grid.pack()

        # Configure uniform column/row sizes for even spacing
        for c in range(3):
            dpad_grid.columnconfigure(c, weight=1, uniform="dpad_col", minsize=80)
        for r in range(3):
            dpad_grid.rowconfigure(r, weight=1, uniform="dpad_row", minsize=56)

        dpad_map = {
            (0, 1): ("▲", BTN_UP),
            (1, 0): ("◄", BTN_LEFT),
            (1, 2): ("►", BTN_RIGHT),
            (2, 1): ("▼", BTN_DOWN),
        }

        # Regular D-pad buttons
        for (row, col), (symbol, code) in dpad_map.items():
            btn = tk.Button(dpad_grid, text=symbol, font=("Consolas", 16, "bold"),
                            fg=ACCENT_CYAN, bg=GLOW_CYAN,
                            activeforeground="white", activebackground=ACCENT_CYAN,
                            width=DPAD_BTN_W, height=DPAD_BTN_H,
                            borderwidth=0, cursor="hand2",
                            command=lambda c=code: self._send_button(c))
            btn.grid(row=row, column=col, padx=DPAD_PAD, pady=DPAD_PAD, sticky="nsew")
            btn.bind("<Enter>", lambda e, b=btn: b.configure(bg=ACCENT_CYAN, fg="black"))
            btn.bind("<Leave>", lambda e, b=btn: b.configure(bg=GLOW_CYAN, fg=ACCENT_CYAN))

        # SELECT button (center) — with long-press detection
        sel_btn = tk.Button(dpad_grid, text="●", font=("Consolas", 16, "bold"),
                            fg=ACCENT_MAGENTA, bg=GLOW_MAGENTA,
                            activeforeground="white", activebackground=ACCENT_MAGENTA,
                            width=DPAD_BTN_W, height=DPAD_BTN_H,
                            borderwidth=0, cursor="hand2")
        sel_btn.grid(row=1, column=1, padx=DPAD_PAD, pady=DPAD_PAD, sticky="nsew")
        sel_btn.bind("<ButtonPress-1>", self._on_select_press)
        sel_btn.bind("<ButtonRelease-1>", self._on_select_release)
        sel_btn.bind("<Enter>", lambda e: sel_btn.configure(bg=ACCENT_MAGENTA, fg="black"))
        sel_btn.bind("<Leave>", lambda e: sel_btn.configure(bg=GLOW_MAGENTA, fg=ACCENT_MAGENTA))

        # Hint label under dpad
        tk.Label(dpad_frame, text="hold SELECT for long press",
                 font=("Consolas", 8), fg=ACCENT_DIM, bg=BG_DARK).pack(pady=(4, 0))

        # ── Status Bar ──
        status_frame = tk.Frame(main, bg=BG_PANEL, padx=8, pady=6,
                                highlightbackground=ACCENT_DIM, highlightthickness=1)
        status_frame.pack(fill="x", pady=(4, 0))

        self.status_label = tk.Label(status_frame, textvariable=self.status_var,
                                     font=("Consolas", 9), fg=ACCENT_DIM, bg=BG_PANEL,
                                     anchor="w")
        self.status_label.pack(fill="x")

    def _center_window(self):
        self.root.update_idletasks()
        w = self.root.winfo_width()
        h = self.root.winfo_height()
        x = (self.root.winfo_screenwidth() // 2) - (w // 2)
        y = (self.root.winfo_screenheight() // 2) - (h // 2)
        self.root.geometry(f"+{x}+{y}")

    # ─── SELECT Long Press Logic ──────────────────────────────────────

    def _on_select_press(self, event):
        self._select_press_time = time.time()

    def _on_select_release(self, event):
        elapsed = time.time() - self._select_press_time
        self._send_button(BTN_SELECT, long_press=(elapsed >= 0.6))


# ─── Main ─────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    root = tk.Tk()
    app = LaserHarpController(root)
    root.mainloop()
