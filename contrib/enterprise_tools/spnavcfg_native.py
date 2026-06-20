import tkinter as tk
from tkinter import ttk
import json
import threading
import asyncio
import websockets

class SpnavCfgApp:
    def __init__(self, root):
        self.root = root
        self.root.title("SpaceMouse Enterprise Configuration")
        self.root.geometry("600x400")

        self.ws = None
        self.loop = asyncio.new_event_loop()

        self.setup_ui()
        threading.Thread(target=self.start_async_loop, daemon=True).start()

    def start_async_loop(self):
        asyncio.set_event_loop(self.loop)
        self.loop.run_until_complete(self.ws_handler())

    async def ws_handler(self):
        async with websockets.connect("ws://localhost:8000") as ws:
            self.ws = ws
            print("Connected to spacenavd via WebSocket")
            while True:
                msg = await ws.recv()
                # Handle incoming events if needed

    def setup_ui(self):
        tabControl = ttk.Notebook(self.root)
        self.tab1 = ttk.Frame(tabControl); tabControl.add(self.tab1, text='General')
        self.tab2 = ttk.Frame(tabControl); tabControl.add(self.tab2, text='Buttons')
        self.tab3 = ttk.Frame(tabControl); tabControl.add(self.tab3, text='LCD')
        tabControl.pack(expand=1, fill="both")

        ttk.Label(self.tab1, text="Sensitivity:").grid(column=0, row=0, padx=10, pady=10)
        self.sens_scale = ttk.Scale(self.tab1, from_=0.1, to=2.0, orient=tk.HORIZONTAL)
        self.sens_scale.set(1.0); self.sens_scale.grid(column=1, row=0, padx=10, pady=10, sticky="ew")

        ttk.Label(self.tab1, text="LED:").grid(column=0, row=1, padx=10, pady=10)
        self.led_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(self.tab1, variable=self.led_var, command=self.apply_led).grid(column=1, row=1, padx=10, pady=10, sticky="w")

        ttk.Label(self.tab3, text="LCD Text:").pack(pady=5)
        self.lcd_text = tk.Entry(self.tab3)
        self.lcd_text.pack(padx=10, pady=5, fill="x")
        ttk.Button(self.tab3, text="Update LCD", command=self.update_lcd).pack(pady=5)

    def send_cmd(self, cmd):
        if self.ws:
            self.loop.call_soon_threadsafe(asyncio.create_task, self.ws.send(json.dumps(cmd)))

    def apply_led(self):
        self.send_cmd({"cmd": "set_led", "state": self.led_var.get()})

    def update_lcd(self):
        self.send_cmd({"cmd": "set_lcd", "text": self.lcd_text.get()})

if __name__ == "__main__":
    root = tk.Tk()
    app = SpnavCfgApp(root)
    root.mainloop()
