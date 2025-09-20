import asyncio
from websockets.asyncio.server import serve
from io import BytesIO

from PIL import Image, UnidentifiedImageError

def is_valid_image(image_bytes):
    try:
        Image.open(BytesIO(image_bytes))
        return True
    except UnidentifiedImageError:
        print("image invalid")
        return False

async def echo(websocket):
    global counter
    async for message in websocket:
        if len(message) > 2800:
            if is_valid_image(message):
                with open("image.jpg", "wb") as f:
                    f.write(message)
            else:
                await websocket.send("Invalid image data received.")
        else:
            await websocket.send(message)

async def main():
    async with serve(echo, "0.0.0.0", 3001, ping_interval=None) as server:
        print("Server started at ws://0.0.0.0:3001")
        try:
            await server.serve_forever()
        except asyncio.CancelledError:
            server.close()
            await server.wait_closed()
            print("Server was cancelled")

if __name__ == "__main__":
    asyncio.run(main())