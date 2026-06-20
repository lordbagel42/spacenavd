import asyncio
import websockets
import json

async def test():
    async with websockets.connect("ws://localhost:8000") as ws:
        await ws.send(json.dumps({"cmd": "set_led", "state": False}))
        await ws.send(json.dumps({"cmd": "set_lcd", "text": "Hello Enterprise"}))

asyncio.get_event_loop().run_until_complete(test())
