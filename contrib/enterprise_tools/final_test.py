import asyncio
import websockets
import json
import subprocess
import time
import os

async def test():
    print("Starting final end-to-end test...")
    # Ensure spacenavd is running
    # I assume it's already started by the bash script or I start it here.

    uri = "ws://localhost:8000"
    try:
        async with websockets.connect(uri) as ws:
            print("Connected to WebSocket.")

            # Send commands
            print("Sending LED OFF command...")
            await ws.send(json.dumps({"cmd": "set_led", "state": False}))

            print("Sending LCD Update command...")
            await ws.send(json.dumps({"cmd": "set_lcd", "text": "Final Test"}))

            # Start simulator to send a motion and a button event
            print("Running simulator...")
            subprocess.run(["python3", "/home/jules/self_created_tools/enterprise_sim.py"])

            # Wait for events
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
