"""
CinematicLaserHarp — OSC Remote Controller + MIDI Bridge
A Python GUI to control the laser harp ESP8266 via OSC,
and bridge incoming OSC note events to a MIDI output with note remapping.

Requirements:
    pip install python-osc mido python-rtmidi

Usage:
    python laser_harp_controller.py
"""

import tkinter as tk
from tkinter import ttk
import time
import datetime
import threading
import re
import mido
from pythonosc.udp_client import SimpleUDPClient
from pythonosc.osc_server import BlockingOSCUDPServer
from pythonosc.dispatcher import Dispatcher

# ─── OSC Protocol Constants (matching CinematicLaserHarp.ino) ─────────────
OSC_SEND_PORT = 8001
OSC_LISTEN_PORT = 8010

OSC_PATH_START_LH = "/laserharp/startLH"
OSC_PATH_START_DMX = "/laserharp/startDMX"
OSC_PATH_START_IDLE = "/laserharp/startIDLE"
OSC_PATH_VIRTUAL_BUTTON = "/laserharp/virtualButton"
OSC_NOTE_PREFIX = "/vkb_midi/0/note/"

BTN_UP     = 1
BTN_DOWN   = 2
BTN_LEFT   = 3
BTN_RIGHT  = 4
BTN_SELECT = 5
LONG_PRESS_OFFSET = BTN_SELECT

NB_BEAM = 7
DEFAULT_NOTES = [36, 48, 60, 62, 64, 65, 67]

# ─── MIDI Helpers ─────────────────────────────────────────────────────────
NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]

def midi_to_name(midi_num):
    if 0 <= midi_num <= 127:
        return f"{NOTE_NAMES[midi_num % 12]}{(midi_num // 12) - 1}"
    return "?"

def name_to_midi(name):
    name = name.strip()
    m = re.match(r'^([A-Ga-g]#?)(-?\d)$', name)
    if not m:
        return -1
    note_str = m.group(1).upper()
    octave = int(m.group(2))
    if note_str in NOTE_NAMES:
        return NOTE_NAMES.index(note_str) + (octave + 1) * 12
    return -1

# ─── Color Theme ──────────────────────────────────────────────────────────
BG_DARK       = "#0d0d1a"
BG_PANEL      = "#161629"
FG_TEXT       = "#e0e0f0"
ACCENT_CYAN   = "#00e5ff"
ACCENT_MAGENTA = "#ff00e5"
ACCENT_GREEN  = "#00ff88"
ACCENT_ORANGE = "#ff9100"
ACCENT_RED    = "#ff3355"
ACCENT_DIM    = "#444466"
ACCENT_YELLOW = "#ffe600"
GLOW_CYAN     = "#003344"
GLOW_MAGENTA  = "#330033"

DPAD_BTN_W = 4
DPAD_BTN_H = 2
DPAD_PAD   = 4

# ─── Help Text ────────────────────────────────────────────────────────────
HELP_TEXT = """\
╔══════════════════════════════════════════════════╗
║          CINEMATIC LASER HARP — HELP             ║
╠══════════════════════════════════════════════════╣
║                                                  ║
║  DIRECT MODE BUTTONS                             ║
║  🎵 LASER HARP : Start with selected preset      ║
║  💡 DMX MODE   : Switch to Art-Net DMX control   ║
║  ⏹ IDLE       : Return to home / stop all       ║
║                                                  ║
╠══════════════════════════════════════════════════╣
║                                                  ║
║  NAVIGATION D-Pad — by mode:                     ║
║                                                  ║
║  HOME:  ▲Settings ▼LaserHarp ●Info ●̲WiFiReset   ║
║  LH IDLE: ▲Home ▼Sequence ◄►Preset ●Start ●̲Edit ║
║  LH PLAY: ►Stop                                  ║
║  EDITING: ▲▼Note ◄►Beam ●Save ●̲SaveNew          ║
║  DMX: ▲▼Stop ◄►Address ●StoreAddr               ║
║  CALIB: ▲▼Offset ◄►Beam ●̲Save ●Back             ║
║                                                  ║
║  ●̲ = long press SELECT (hold >0.6s)              ║
║                                                  ║
╠══════════════════════════════════════════════════╣
║                                                  ║
║  MIDI BRIDGE                                     ║
║  Listens on UDP port 8010 for OSC note events.   ║
║  Remaps the 7 default beam notes to your custom  ║
║  targets and forwards to the selected MIDI port. ║
║  Edit target notes using names (C4, F#5) or      ║
║  MIDI numbers. Press Enter/Tab to apply.         ║
║                                                  ║
╚══════════════════════════════════════════════════╝
"""


class MidiBridge:
    """OSC listener + MIDI note remapping + MIDI output."""

    def __init__(self, on_note_callback=None, on_any_osc_callback=None):
        self.note_map = {}
        for n in DEFAULT_NOTES:
            self.note_map[n] = n

        self.midi_port = None
        self.midi_port_name = None
        self.osc_server = None
        self.osc_thread = None
        self.running = False
        self.on_note_callback = on_note_callback
        self.on_any_osc_callback = on_any_osc_callback

    def set_target_note(self, beam_index, target_midi):
        self.note_map[DEFAULT_NOTES[beam_index]] = target_midi

    def get_target_note(self, beam_index):
        return self.note_map[DEFAULT_NOTES[beam_index]]

    def open_midi_port(self, port_name):
        self.close_midi_port()
        try:
            self.midi_port = mido.open_output(port_name)
            self.midi_port_name = port_name
            return True
        except Exception:
            self.midi_port = None
            self.midi_port_name = None
            return False

    def close_midi_port(self):
        if self.midi_port:
            try:
                self.midi_port.close()
            except Exception:
                pass
            self.midi_port = None
            self.midi_port_name = None

    def start_osc_listener(self):
        if self.running:
            return True
        self.running = True
        dispatcher = Dispatcher()
        dispatcher.map("/vkb_midi/0/note/*", self._handle_osc_note)
        dispatcher.set_default_handler(self._handle_any_osc)
        try:
            self.osc_server = BlockingOSCUDPServer(("0.0.0.0", OSC_LISTEN_PORT), dispatcher)
        except OSError:
            self.running = False
            return False
        self.osc_thread = threading.Thread(target=self.osc_server.serve_forever, daemon=True)
        self.osc_thread.start()
        return True

    def stop_osc_listener(self):
        self.running = False
        if self.osc_server:
            self.osc_server.shutdown()
            self.osc_server = None

    def _handle_osc_note(self, address, *args):
        try:
            note_num = int(address.split("/")[-1])
        except (ValueError, IndexError):
            return

        velocity = int(args[0]) if args else 0

        beam_index = None
        if note_num in DEFAULT_NOTES:
            beam_index = DEFAULT_NOTES.index(note_num)

        target_note = self.note_map.get(note_num, note_num)

        if self.midi_port:
            try:
                if velocity > 0:
                    msg = mido.Message('note_on', note=target_note, velocity=velocity, channel=0)
                else:
                    msg = mido.Message('note_off', note=target_note, velocity=0, channel=0)
                self.midi_port.send(msg)
            except Exception:
                pass

        if self.on_note_callback and beam_index is not None:
            self.on_note_callback(beam_index, velocity)

    def _handle_any_osc(self, address, *args):
        """Catch-all: forward every OSC message to the monitor callback."""
        if self.on_any_osc_callback:
            self.on_any_osc_callback(address, args)
        # Also route note messages to the note handler
        if address.startswith("/vkb_midi/0/note/"):
            self._handle_osc_note(address, *args)

    @staticmethod
    def get_midi_ports():
        try:
            return mido.get_output_names()
        except Exception:
            return []


class LaserHarpController:
    def __init__(self, root):
        self.root = root
        self.root.title("🎵 Cinematic Laser Harp Controller")
        self.root.configure(bg=BG_DARK)
        self.root.resizable(False, False)

        self.osc_client = None
        self.status_var = tk.StringVar(value="Not connected")
        self._select_press_time = 0

        self.bridge = MidiBridge(
            on_note_callback=self._on_bridge_note,
            on_any_osc_callback=self._on_any_osc
        )
        self.beam_indicators = []
        self.target_entries = []
        self.monitor_text = None  # will be set if monitor window is open
        self._osc_msg_count = 0

        self._build_ui()
        self._center_window()

        if self.bridge.start_osc_listener():
            self._set_status("✓ OSC listener started on port 8010", ACCENT_GREEN)
        else:
            self._set_status("⚠ Could not bind port 8010 (already in use?)", ACCENT_RED)

        self.root.protocol("WM_DELETE_WINDOW", self._on_close)

    def _on_close(self):
        self.bridge.stop_osc_listener()
        self.bridge.close_midi_port()
        self.root.destroy()

    # ─── OSC Sending ──────────────────────────────────────────────────

    def _get_client(self):
        ip = self.ip_entry.get().strip()
        if not ip:
            self._set_status("⚠ Enter ESP8266 IP address", ACCENT_RED)
            return None
        try:
            self.osc_client = SimpleUDPClient(ip, OSC_SEND_PORT)
            return self.osc_client
        except Exception as e:
            self._set_status(f"⚠ Invalid IP: {e}", ACCENT_RED)
            return None

    def _send_osc(self, path, *args):
        client = self._get_client()
        if not client:
            return
        try:
            client.send_message(path, list(args))
            arg_str = ", ".join(str(a) for a in args) if args else ""
            self._set_status(f"✓ {path} {arg_str}", ACCENT_GREEN)
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

    # ─── Bridge Callbacks ─────────────────────────────────────────────

    def _on_bridge_note(self, beam_index, velocity):
        self.root.after(0, self._update_beam_indicator, beam_index, velocity)

    def _update_beam_indicator(self, beam_index, velocity):
        indicator = self.beam_indicators[beam_index]
        if velocity > 0:
            indicator.configure(bg=ACCENT_GREEN, fg="black", text="▶")
            self.root.after(300, lambda: indicator.configure(
                bg=BG_PANEL, fg=ACCENT_DIM, text="○"))
        else:
            indicator.configure(bg=BG_PANEL, fg=ACCENT_DIM, text="○")
        target = self.bridge.get_target_note(beam_index)
        self._set_status(
            f"♪ Beam {beam_index}: {midi_to_name(DEFAULT_NOTES[beam_index])} → "
            f"{midi_to_name(target)} vel={velocity}", ACCENT_GREEN)

    def _on_midi_port_selected(self, event=None):
        port_name = self.midi_combo.get()
        if not port_name:
            return
        if self.bridge.open_midi_port(port_name):
            self._set_status(f"✓ MIDI out: {port_name}", ACCENT_GREEN)
        else:
            self._set_status(f"⚠ Failed: {port_name}", ACCENT_RED)

    def _refresh_midi_ports(self):
        ports = MidiBridge.get_midi_ports()
        self.midi_combo["values"] = ports
        if ports:
            self.midi_combo.set(ports[0])

    def _apply_note_mapping(self, beam_index):
        entry = self.target_entries[beam_index]
        text = entry.get().strip()
        midi_val = name_to_midi(text)
        if midi_val < 0:
            try:
                midi_val = int(text)
            except ValueError:
                midi_val = -1
        if 0 <= midi_val <= 127:
            self.bridge.set_target_note(beam_index, midi_val)
            entry.configure(fg=ACCENT_GREEN)
            self._set_status(
                f"✓ Beam {beam_index}: → {midi_to_name(midi_val)}", ACCENT_GREEN)
        else:
            entry.configure(fg=ACCENT_RED)
            self._set_status(f"⚠ Invalid note: {text}", ACCENT_RED)

    # ─── OSC Monitor ──────────────────────────────────────────────────

    def _on_any_osc(self, address, args):
        """Called from OSC thread for every incoming message."""
        self._osc_msg_count += 1
        ts = datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]
        args_str = "  ".join(str(a) for a in args) if args else ""
        line = f"[{ts}]  {address}  {args_str}\n"
        self.root.after(0, self._append_monitor_line, line)

    def _append_monitor_line(self, line):
        if self.monitor_text and self.monitor_text.winfo_exists():
            self.monitor_text.configure(state="normal")
            self.monitor_text.insert("end", line)
            # Keep max 500 lines
            line_count = int(self.monitor_text.index("end-1c").split(".")[0])
            if line_count > 500:
                self.monitor_text.delete("1.0", "2.0")
            self.monitor_text.see("end")
            self.monitor_text.configure(state="disabled")

    def _show_monitor(self):
        mw = tk.Toplevel(self.root)
        mw.title("📡 OSC Monitor")
        mw.configure(bg=BG_DARK)
        mw.geometry("560x400")

        # Header
        hdr = tk.Frame(mw, bg=BG_DARK, padx=10, pady=6)
        hdr.pack(fill="x")
        tk.Label(hdr, text="📡 OSC INCOMING (port 8010)",
                 font=("Consolas", 11, "bold"), fg=ACCENT_CYAN,
                 bg=BG_DARK).pack(side="left")

        clear_btn = tk.Button(hdr, text="Clear", font=("Consolas", 9, "bold"),
                              fg=ACCENT_RED, bg=BG_PANEL, borderwidth=0,
                              cursor="hand2",
                              command=lambda: self._clear_monitor())
        clear_btn.pack(side="right")
        clear_btn.bind("<Enter>", lambda e: clear_btn.configure(bg=ACCENT_RED, fg="black"))
        clear_btn.bind("<Leave>", lambda e: clear_btn.configure(bg=BG_PANEL, fg=ACCENT_RED))

        # Scrollable text area
        text_frame = tk.Frame(mw, bg=BG_DARK, padx=10, pady=10)
        text_frame.pack(fill="both", expand=True)

        scrollbar = tk.Scrollbar(text_frame)
        scrollbar.pack(side="right", fill="y")

        self.monitor_text = tk.Text(text_frame, font=("Consolas", 8),
                                    fg=ACCENT_GREEN, bg="#0a0a14",
                                    borderwidth=1, relief="solid",
                                    wrap="none", cursor="arrow",
                                    yscrollcommand=scrollbar.set)
        self.monitor_text.pack(fill="both", expand=True)
        self.monitor_text.configure(state="disabled")
        scrollbar.configure(command=self.monitor_text.yview)

        mw.transient(self.root)
        x = self.root.winfo_x() + self.root.winfo_width() + 10
        y = self.root.winfo_y()
        mw.geometry(f"+{x}+{y}")

        def on_close():
            self.monitor_text = None
            mw.destroy()
        mw.protocol("WM_DELETE_WINDOW", on_close)

    def _clear_monitor(self):
        if self.monitor_text and self.monitor_text.winfo_exists():
            self.monitor_text.configure(state="normal")
            self.monitor_text.delete("1.0", "end")
            self.monitor_text.configure(state="disabled")

    # ─── Help Window ──────────────────────────────────────────────────

    def _show_help(self):
        hw = tk.Toplevel(self.root)
        hw.title("Help — Button Reference")
        hw.configure(bg=BG_DARK)
        hw.resizable(False, False)
        text = tk.Text(hw, font=("Consolas", 9), fg=ACCENT_CYAN, bg=BG_DARK,
                       borderwidth=0, padx=16, pady=12, wrap="none",
                       width=54, height=36, cursor="arrow")
        text.insert("1.0", HELP_TEXT)
        text.configure(state="disabled")
        text.pack()
        tk.Button(hw, text="Close", font=("Consolas", 10, "bold"),
                  fg=BG_DARK, bg=ACCENT_CYAN, activebackground=ACCENT_GREEN,
                  borderwidth=0, padx=20, pady=4, cursor="hand2",
                  command=hw.destroy).pack(pady=(0, 12))
        hw.transient(self.root)
        hw.update_idletasks()
        x = self.root.winfo_x() + (self.root.winfo_width() // 2) - (hw.winfo_width() // 2)
        y = self.root.winfo_y() + 20
        hw.geometry(f"+{x}+{y}")
        hw.grab_set()

    # ─── UI Helpers ───────────────────────────────────────────────────

    def _hover_btn(self, parent, text, fg, bg, cmd, font_size=11, width=14, height=2):
        btn = tk.Button(parent, text=text, font=("Consolas", font_size, "bold"),
                        fg=fg, bg=bg, activeforeground="white", activebackground=fg,
                        width=width, height=height, borderwidth=0, cursor="hand2", command=cmd)
        btn.bind("<Enter>", lambda e, b=btn, c=fg: b.configure(bg=c, fg="black"))
        btn.bind("<Leave>", lambda e, b=btn, c=bg, fc=fg: b.configure(bg=c, fg=fc))
        return btn

    # ─── Main UI Build ────────────────────────────────────────────────

    def _build_ui(self):
        outer = tk.Frame(self.root, bg=BG_DARK, padx=16, pady=12)
        outer.pack(fill="both", expand=True)

        # ── Title Row ──
        title_row = tk.Frame(outer, bg=BG_DARK)
        title_row.pack(fill="x", pady=(0, 10))

        tk.Label(title_row, text="⚡ CINEMATIC LASER HARP ⚡",
                 font=("Consolas", 16, "bold"), fg=ACCENT_CYAN,
                 bg=BG_DARK).pack(side="left", expand=True)

        help_btn = tk.Button(title_row, text=" ? ", font=("Consolas", 12, "bold"),
                             fg=ACCENT_ORANGE, bg=BG_PANEL, borderwidth=0,
                             cursor="hand2", command=self._show_help)
        help_btn.pack(side="right")
        help_btn.bind("<Enter>", lambda e: help_btn.configure(bg=ACCENT_ORANGE, fg="black"))
        help_btn.bind("<Leave>", lambda e: help_btn.configure(bg=BG_PANEL, fg=ACCENT_ORANGE))

        mon_btn = tk.Button(title_row, text=" 📡 ", font=("Consolas", 12, "bold"),
                            fg=ACCENT_GREEN, bg=BG_PANEL, borderwidth=0,
                            cursor="hand2", command=self._show_monitor)
        mon_btn.pack(side="right", padx=(0, 4))
        mon_btn.bind("<Enter>", lambda e: mon_btn.configure(bg=ACCENT_GREEN, fg="black"))
        mon_btn.bind("<Leave>", lambda e: mon_btn.configure(bg=BG_PANEL, fg=ACCENT_GREEN))

        # ── IP Config ──
        ip_frame = tk.Frame(outer, bg=BG_PANEL, padx=12, pady=6,
                            highlightbackground=ACCENT_DIM, highlightthickness=1)
        ip_frame.pack(fill="x", pady=(0, 10))

        tk.Label(ip_frame, text="ESP8266 IP:", font=("Consolas", 10),
                 fg=FG_TEXT, bg=BG_PANEL).pack(side="left")
        self.ip_entry = tk.Entry(ip_frame, font=("Consolas", 12), width=16,
                                 bg=BG_DARK, fg=ACCENT_CYAN, insertbackground=ACCENT_CYAN,
                                 borderwidth=1, relief="solid")
        self.ip_entry.insert(0, "192.168.1.100")
        self.ip_entry.pack(side="left", padx=(8, 0))

        # ══════════════════════════════════════════════════════════════
        #  HORIZONTAL LAYOUT: left_col | right_col
        # ══════════════════════════════════════════════════════════════
        body = tk.Frame(outer, bg=BG_DARK)
        body.pack(fill="both", expand=True, pady=(0, 8))

        left_col = tk.Frame(body, bg=BG_DARK)
        left_col.pack(side="left", fill="both", padx=(0, 8))

        sep = tk.Frame(body, bg=ACCENT_DIM, width=1)
        sep.pack(side="left", fill="y", padx=4)

        right_col = tk.Frame(body, bg=BG_DARK)
        right_col.pack(side="left", fill="both", expand=True)

        # ─────────────────── LEFT: Modes + D-Pad ─────────────────────

        # Mode buttons
        mode_frame = tk.LabelFrame(left_col, text=" MODES ", font=("Consolas", 9, "bold"),
                                   fg=ACCENT_DIM, bg=BG_DARK, padx=8, pady=8,
                                   borderwidth=1, relief="groove")
        mode_frame.pack(fill="x", pady=(0, 8))

        preset_row = tk.Frame(mode_frame, bg=BG_DARK)
        preset_row.pack(pady=(0, 8))
        tk.Label(preset_row, text="Preset:", font=("Consolas", 9),
                 fg=FG_TEXT, bg=BG_DARK).pack(side="left")
        self.preset_spin = tk.Spinbox(preset_row, from_=0, to=30, width=4,
                                       font=("Consolas", 11), bg=BG_DARK, fg=ACCENT_CYAN,
                                       buttonbackground=BG_PANEL, borderwidth=1, relief="solid")
        self.preset_spin.pack(side="left", padx=(6, 0))

        btn_row = tk.Frame(mode_frame, bg=BG_DARK)
        btn_row.pack(fill="x")
        for c in range(3):
            btn_row.columnconfigure(c, weight=1)

        mode_defs = [
            ("🎵 HARP", ACCENT_CYAN, GLOW_CYAN,
             lambda: self._send_osc(OSC_PATH_START_LH, int(self.preset_spin.get()))),
            ("💡 DMX", ACCENT_MAGENTA, GLOW_MAGENTA,
             lambda: self._send_osc(OSC_PATH_START_DMX, 0)),
            ("⏹ IDLE", ACCENT_ORANGE, BG_PANEL,
             lambda: self._send_osc(OSC_PATH_START_IDLE, 0)),
        ]
        for i, (text, fg, bg, cmd) in enumerate(mode_defs):
            btn = self._hover_btn(btn_row, text, fg, bg, cmd, width=8, height=1)
            btn.grid(row=0, column=i, padx=3, pady=2, sticky="ew")

        # D-Pad
        dpad_frame = tk.LabelFrame(left_col, text=" NAVIGATION ", font=("Consolas", 9, "bold"),
                                   fg=ACCENT_DIM, bg=BG_DARK, padx=8, pady=8,
                                   borderwidth=1, relief="groove")
        dpad_frame.pack(fill="x")

        dpad_grid = tk.Frame(dpad_frame, bg=BG_DARK)
        dpad_grid.pack()

        for c in range(3):
            dpad_grid.columnconfigure(c, weight=1, uniform="dp", minsize=68)
        for r in range(3):
            dpad_grid.rowconfigure(r, weight=1, uniform="dpr", minsize=48)

        dpad_map = {
            (0, 1): ("▲", BTN_UP),
            (1, 0): ("◄", BTN_LEFT),
            (1, 2): ("►", BTN_RIGHT),
            (2, 1): ("▼", BTN_DOWN),
        }

        for (row, col), (symbol, code) in dpad_map.items():
            btn = tk.Button(dpad_grid, text=symbol, font=("Consolas", 14, "bold"),
                            fg=ACCENT_CYAN, bg=GLOW_CYAN,
                            activeforeground="white", activebackground=ACCENT_CYAN,
                            width=DPAD_BTN_W, height=DPAD_BTN_H,
                            borderwidth=0, cursor="hand2",
                            command=lambda c=code: self._send_button(c))
            btn.grid(row=row, column=col, padx=DPAD_PAD, pady=DPAD_PAD, sticky="nsew")
            btn.bind("<Enter>", lambda e, b=btn: b.configure(bg=ACCENT_CYAN, fg="black"))
            btn.bind("<Leave>", lambda e, b=btn: b.configure(bg=GLOW_CYAN, fg=ACCENT_CYAN))

        sel_btn = tk.Button(dpad_grid, text="●", font=("Consolas", 14, "bold"),
                            fg=ACCENT_MAGENTA, bg=GLOW_MAGENTA,
                            activeforeground="white", activebackground=ACCENT_MAGENTA,
                            width=DPAD_BTN_W, height=DPAD_BTN_H,
                            borderwidth=0, cursor="hand2")
        sel_btn.grid(row=1, column=1, padx=DPAD_PAD, pady=DPAD_PAD, sticky="nsew")
        sel_btn.bind("<ButtonPress-1>", self._on_select_press)
        sel_btn.bind("<ButtonRelease-1>", self._on_select_release)
        sel_btn.bind("<Enter>", lambda e: sel_btn.configure(bg=ACCENT_MAGENTA, fg="black"))
        sel_btn.bind("<Leave>", lambda e: sel_btn.configure(bg=GLOW_MAGENTA, fg=ACCENT_MAGENTA))

        tk.Label(dpad_frame, text="hold ● = long press",
                 font=("Consolas", 7), fg=ACCENT_DIM, bg=BG_DARK).pack(pady=(2, 0))

        # ─────────────────── RIGHT: MIDI Bridge ──────────────────────

        midi_frame = tk.LabelFrame(right_col, text=" MIDI BRIDGE ", font=("Consolas", 9, "bold"),
                                   fg=ACCENT_DIM, bg=BG_DARK, padx=10, pady=8,
                                   borderwidth=1, relief="groove")
        midi_frame.pack(fill="both", expand=True)

        # MIDI port row
        port_row = tk.Frame(midi_frame, bg=BG_DARK)
        port_row.pack(fill="x", pady=(0, 8))

        tk.Label(port_row, text="MIDI Out:", font=("Consolas", 9),
                 fg=FG_TEXT, bg=BG_DARK).pack(side="left")

        self.midi_combo = ttk.Combobox(port_row, font=("Consolas", 9), width=26,
                                        state="readonly")
        self.midi_combo.pack(side="left", padx=(6, 4))
        self.midi_combo.bind("<<ComboboxSelected>>", self._on_midi_port_selected)

        refresh_btn = tk.Button(port_row, text="⟳", font=("Consolas", 11, "bold"),
                                fg=ACCENT_CYAN, bg=BG_PANEL, borderwidth=0,
                                cursor="hand2", command=self._refresh_midi_ports)
        refresh_btn.pack(side="left")
        refresh_btn.bind("<Enter>", lambda e: refresh_btn.configure(bg=ACCENT_CYAN, fg="black"))
        refresh_btn.bind("<Leave>", lambda e: refresh_btn.configure(bg=BG_PANEL, fg=ACCENT_CYAN))

        # Column headers
        header = tk.Frame(midi_frame, bg=BG_DARK)
        header.pack(fill="x", pady=(4, 2))
        header.columnconfigure(0, minsize=28)
        header.columnconfigure(1, weight=1)
        header.columnconfigure(2, minsize=50)
        header.columnconfigure(3, minsize=24)
        header.columnconfigure(4, minsize=50)

        tk.Label(header, text="", width=3, bg=BG_DARK).grid(row=0, column=0)
        tk.Label(header, text="Beam", font=("Consolas", 8, "bold"),
                 fg=ACCENT_DIM, bg=BG_DARK, anchor="w").grid(row=0, column=1, sticky="w")
        tk.Label(header, text="From", font=("Consolas", 8, "bold"),
                 fg=ACCENT_DIM, bg=BG_DARK).grid(row=0, column=2)
        tk.Label(header, text="→", font=("Consolas", 8),
                 fg=ACCENT_DIM, bg=BG_DARK).grid(row=0, column=3)
        tk.Label(header, text="To", font=("Consolas", 8, "bold"),
                 fg=ACCENT_DIM, bg=BG_DARK).grid(row=0, column=4)

        # Beam mapping rows
        self.beam_indicators = []
        self.target_entries = []

        for i in range(NB_BEAM):
            row_f = tk.Frame(midi_frame, bg=BG_DARK)
            row_f.pack(fill="x", pady=2)
            row_f.columnconfigure(0, minsize=28)
            row_f.columnconfigure(1, weight=1)
            row_f.columnconfigure(2, minsize=50)
            row_f.columnconfigure(3, minsize=24)
            row_f.columnconfigure(4, minsize=50)

            # Activity indicator
            ind = tk.Label(row_f, text="○", font=("Consolas", 10),
                           fg=ACCENT_DIM, bg=BG_PANEL, width=3)
            ind.grid(row=0, column=0, padx=(0, 4))
            self.beam_indicators.append(ind)

            # Beam label
            tk.Label(row_f, text=f"Beam {i}", font=("Consolas", 9),
                     fg=FG_TEXT, bg=BG_DARK, anchor="w").grid(row=0, column=1, sticky="w")

            # Source note
            tk.Label(row_f, text=midi_to_name(DEFAULT_NOTES[i]),
                     font=("Consolas", 10, "bold"),
                     fg=ACCENT_CYAN, bg=BG_PANEL, width=5).grid(row=0, column=2, padx=2)

            # Arrow
            tk.Label(row_f, text="→", font=("Consolas", 10),
                     fg=ACCENT_DIM, bg=BG_DARK).grid(row=0, column=3)

            # Target note (editable)
            entry = tk.Entry(row_f, font=("Consolas", 10, "bold"), width=5,
                             bg=BG_PANEL, fg=ACCENT_YELLOW, insertbackground=ACCENT_YELLOW,
                             borderwidth=1, relief="solid", justify="center")
            entry.insert(0, midi_to_name(DEFAULT_NOTES[i]))
            entry.grid(row=0, column=4, padx=2)
            entry.bind("<Return>", lambda e, idx=i: self._apply_note_mapping(idx))
            entry.bind("<FocusOut>", lambda e, idx=i: self._apply_note_mapping(idx))
            self.target_entries.append(entry)

        tk.Label(midi_frame, text="Enter / Tab to apply",
                 font=("Consolas", 7), fg=ACCENT_DIM, bg=BG_DARK).pack(pady=(6, 0))

        # ── Status Bar (full width at bottom) ──
        status_frame = tk.Frame(outer, bg=BG_PANEL, padx=8, pady=5,
                                highlightbackground=ACCENT_DIM, highlightthickness=1)
        status_frame.pack(fill="x", pady=(4, 0))

        self.status_label = tk.Label(status_frame, textvariable=self.status_var,
                                     font=("Consolas", 9), fg=ACCENT_DIM, bg=BG_PANEL,
                                     anchor="w")
        self.status_label.pack(fill="x")

        # Populate MIDI ports
        self._refresh_midi_ports()

    def _center_window(self):
        self.root.update_idletasks()
        w = self.root.winfo_width()
        h = self.root.winfo_height()
        x = (self.root.winfo_screenwidth() // 2) - (w // 2)
        y = (self.root.winfo_screenheight() // 2) - (h // 2)
        self.root.geometry(f"+{x}+{y}")

    # ─── SELECT Long Press ────────────────────────────────────────────

    def _on_select_press(self, event):
        self._select_press_time = time.time()

    def _on_select_release(self, event):
        elapsed = time.time() - self._select_press_time
        self._send_button(BTN_SELECT, long_press=(elapsed >= 0.6))


if __name__ == "__main__":
    root = tk.Tk()
    app = LaserHarpController(root)
    root.mainloop()
