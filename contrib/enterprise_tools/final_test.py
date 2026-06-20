import asyncio
import websockets
import json
import subprocess
import time

async def test():
    print("Starting final end-to-end test...")
    uri = "ws://localhost:8000"
    try:
        async with websockets.connect(uri) as ws:
            print("Connected to WebSocket.")
            await ws.send(json.dumps({"cmd": "set_led", "state": False}))
            await ws.send(json.dumps({"cmd": "set_lcd", "text": "Final Test"}))
            print("Running simulator...")
            subprocess.run(["python3", "contrib/enterprise_tools/enterprise_sim.py"])
            print("Waiting for events from WebSocket...")
            for _ in range(3):
                try:
                    msg = await asyncio.wait_for(ws.recv(), timeout=2.0)
                    print(f"WS Event Received: {msg}")
                except asyncio.TimeoutError:
                    break
    except Exception as e:
        print(f"Test failed: {e}")

if __name__ == "__main__":
    asyncio.run(test())
