import customtkinter as ctk
import json
import threading
import asyncio
import websockets
import sys

ctk.set_appearance_mode("Dark")
ctk.set_default_color_theme("blue")

class SpnavCfgApp(ctk.CTk):
    def __init__(self):
        super().__init__()
        self.title("SpaceMouse Enterprise Configuration")
        self.geometry("900x600")

        # Configure layout
        self.grid_columnconfigure(1, weight=1)
        self.grid_rowconfigure(0, weight=1)

        self.ws = None
        self.loop = asyncio.new_event_loop()
        self.connected = False

        self.setup_ui()
        threading.Thread(target=self.start_async_loop, daemon=True).start()

    def start_async_loop(self):
        asyncio.set_event_loop(self.loop)
        while True:
            try:
                self.loop.run_until_complete(self.ws_handler())
            except Exception as e:
                self.connected = False
                self.update_status("Disconnected")
                print(f"WS error: {e}")
            import time
            time.sleep(5)

    async def ws_handler(self):
        async with websockets.connect("ws://localhost:8000") as ws:
            self.ws = ws
            self.connected = True
            self.update_status("Connected")
            print("Connected to spacenavd")
            while True:
                msg = await ws.recv()
                # Process events from daemon if needed

    def setup_ui(self):
        # Sidebar
        self.sidebar_frame = ctk.CTkFrame(self, width=200, corner_radius=0)
        self.sidebar_frame.grid(row=0, column=0, rowspan=4, sticky="nsew")
        self.sidebar_frame.grid_rowconfigure(4, weight=1)

        self.logo_label = ctk.CTkLabel(self.sidebar_frame, text="3Dconnexion", font=ctk.CTkFont(size=24, weight="bold"))
        self.logo_label.grid(row=0, column=0, padx=20, pady=(20, 10))
        self.sublogo_label = ctk.CTkLabel(self.sidebar_frame, text="Open Driver", font=ctk.CTkFont(size=12))
        self.sublogo_label.grid(row=1, column=0, padx=20, pady=(0, 20))

        self.btn_gen = ctk.CTkButton(self.sidebar_frame, text="General Settings", corner_radius=0, height=40, border_spacing=10,
                                     fg_color="transparent", text_color=("gray10", "gray90"), hover_color=("gray70", "gray30"),
                                     anchor="w", command=lambda: self.show_tab("general"))
        self.btn_gen.grid(row=2, column=0, sticky="ew")

        self.btn_btn = ctk.CTkButton(self.sidebar_frame, text="Button Mapping", corner_radius=0, height=40, border_spacing=10,
                                     fg_color="transparent", text_color=("gray10", "gray90"), hover_color=("gray70", "gray30"),
                                     anchor="w", command=lambda: self.show_tab("buttons"))
        self.btn_btn.grid(row=3, column=0, sticky="ew")

        self.btn_lcd = ctk.CTkButton(self.sidebar_frame, text="LCD Display", corner_radius=0, height=40, border_spacing=10,
                                     fg_color="transparent", text_color=("gray10", "gray90"), hover_color=("gray70", "gray30"),
                                     anchor="w", command=lambda: self.show_tab("lcd"))
        self.btn_lcd.grid(row=4, column=0, sticky="ew")

        self.status_label = ctk.CTkLabel(self.sidebar_frame, text="Status: Disconnected", text_color="red")
        self.status_label.grid(row=5, column=0, padx=20, pady=20)

        # Content area
        self.content_frame = ctk.CTkFrame(self, corner_radius=10, fg_color=("gray95", "gray15"))
        self.content_frame.grid(row=0, column=1, sticky="nsew", padx=20, pady=20)
        self.content_frame.grid_columnconfigure(0, weight=1)

        self.tabs = {}
        self.setup_general_tab()
        self.setup_buttons_tab()
        self.setup_lcd_tab()
        self.show_tab("general")

    def update_status(self, status):
        color = "green" if status == "Connected" else "red"
        self.status_label.configure(text=f"Status: {status}", text_color=color)

    def setup_general_tab(self):
        tab = ctk.CTkFrame(self.content_frame, fg_color="transparent")
        self.tabs["general"] = tab

        header = ctk.CTkLabel(tab, text="General Configuration", font=ctk.CTkFont(size=20, weight="bold"))
        header.pack(pady=20, padx=20, anchor="w")

        card = ctk.CTkFrame(tab, fg_color=("white", "gray20"), corner_radius=10)
        card.pack(fill="x", padx=20, pady=10)

        ctk.CTkLabel(card, text="Global Sensitivity", font=ctk.CTkFont(weight="bold")).pack(pady=(10,0), padx=20, anchor="w")
        self.sens_slider = ctk.CTkSlider(card, from_=0.1, to=4.0, number_of_steps=39, command=self.update_sens)
        self.sens_slider.set(1.0)
        self.sens_slider.pack(padx=20, pady=20, fill="x")

        self.led_switch = ctk.CTkSwitch(card, text="LED Backlight Ring", font=ctk.CTkFont(weight="bold"), command=self.apply_led)
        self.led_switch.select()
        self.led_switch.pack(pady=20, padx=20, anchor="w")

    def setup_buttons_tab(self):
        tab = ctk.CTkFrame(self.content_frame, fg_color="transparent")
        self.tabs["buttons"] = tab

        header = ctk.CTkLabel(tab, text="Button Mapping", font=ctk.CTkFont(size=20, weight="bold"))
        header.pack(pady=20, padx=20, anchor="w")

        self.scroll = ctk.CTkScrollableFrame(tab, fg_color="transparent")
        self.scroll.pack(padx=20, pady=0, fill="both", expand=True)

        buttons = [
            ("V1", "Macro 1"), ("V2", "Macro 2"), ("V3", "Macro 3"),
            ("Menu", "Application Menu"), ("Fit", "Fit to Screen"),
            ("Top", "Top View"), ("Right", "Right View"), ("Front", "Front View"),
            ("ISO", "ISO View"), ("Left", "Left View"), ("Back", "Back View"),
            ("Roll CW", "Roll Clockwise"), ("Roll CCW", "Roll Counter-Clockwise")
        ]

        for name, desc in buttons:
            f = ctk.CTkFrame(self.scroll, fg_color=("white", "gray20"), corner_radius=8)
            f.pack(fill="x", pady=5)
            ctk.CTkLabel(f, text=name, font=ctk.CTkFont(weight="bold"), width=80).pack(side="left", padx=10, pady=10)
            ctk.CTkLabel(f, text=desc, text_color="gray").pack(side="left", padx=10)
            ctk.CTkComboBox(f, values=["Default", "Esc", "Shift", "Ctrl", "Alt", "Enter", "Space", "Delete", "Tab"], width=150).pack(side="right", padx=10)

    def setup_lcd_tab(self):
        tab = ctk.CTkFrame(self.content_frame, fg_color="transparent")
        self.tabs["lcd"] = tab

        header = ctk.CTkLabel(tab, text="Enterprise LCD Control", font=ctk.CTkFont(size=20, weight="bold"))
        header.pack(pady=20, padx=20, anchor="w")

        card = ctk.CTkFrame(tab, fg_color=("white", "gray20"), corner_radius=10)
        card.pack(fill="x", padx=20, pady=10)

        ctk.CTkLabel(card, text="Status Display Text", font=ctk.CTkFont(weight="bold")).pack(pady=(10,0), padx=20, anchor="w")
        self.lcd_entry = ctk.CTkEntry(card, placeholder_text="Type something to show on the Enterprise screen...", height=40)
        self.lcd_entry.pack(padx=20, pady=20, fill="x")

        update_btn = ctk.CTkButton(card, text="Update Device LCD", command=self.update_lcd, font=ctk.CTkFont(weight="bold"), height=40)
        update_btn.pack(padx=20, pady=(0, 20), anchor="e")

    def show_tab(self, name):
        for t in self.tabs.values(): t.pack_forget()
        self.tabs[name].pack(fill="both", expand=True)

    def send_cmd(self, cmd):
        if self.ws and self.connected:
            self.loop.call_soon_threadsafe(asyncio.create_task, self.ws.send(json.dumps(cmd)))

    def update_sens(self, val): self.send_cmd({"cmd": "set_sens", "val": float(val)})
    def apply_led(self): self.send_cmd({"cmd": "set_led", "state": self.led_switch.get() == 1})
    def update_lcd(self): self.send_cmd({"cmd": "set_lcd", "text": self.lcd_entry.get()})

if __name__ == "__main__":
    app = SpnavCfgApp()
    app.mainloop()
