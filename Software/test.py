from fastapi import FastAPI, Request, WebSocket, WebSocketDisconnect
from fastapi.responses import HTMLResponse, StreamingResponse
from fastapi.staticfiles import StaticFiles
from fastapi.templating import Jinja2Templates
from random import randint
import cv2
from time import sleep, time
import numpy as np
import json
from ctypes import c_short
import asyncio

from motors import Motors
from analyzer import Analyzer

t_start = time()
print(time() - t_start)
cam = cv2.VideoCapture(0)

# Init the APP
app = FastAPI()
templates = Jinja2Templates(directory="templates")
app.mount("/static", StaticFiles(directory="static"), name="static")

# Init spi

# Init motors

# Init image analyzation
analyzer = Analyzer()
line = False
colors = False
deviation = 0
verdict = [0, 0]


@app.get("/", response_class=HTMLResponse)
async def index(request: Request):
    return templates.TemplateResponse('index.html', {"request": request, "video": "video_feed"})


def gen():
    """Video streaming generator function."""
    while True:
        global analyzer, line, colors, deviation, verdict
        ret, image = cam.read()
        if line or colors:
            output = np.zeros(image.shape, np.uint8)
            print("yepieee")
        else:
            output = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)

        if line:
            binary, ret = analyzer.preprocessing(image)
            deviation, out, con = analyzer.find_centroid(binary)
            output += out
        if colors:
            verdict, out = analyzer.find_colors(image)
            output += out

        ret, jpeg = cv2.imencode('.jpg', output)
        yield b'--frame\r\nContent-Type: image/jpeg\r\n\r\n' + jpeg.tobytes() + b'\r\n'


def motor_speed(left: int, right: int):

    print("left motor ", int(left) * 256)
    print("right motor ", int(right) * 256)
    print("lm ", left)
    print("rm ", right, "\n")


async def spi_read(websocket: WebSocket):
    msg = {"id": "", "value": 0}

    data_bytes = [0xFF, 0xFF, 0xFF]
    print(data_bytes)
    if data_bytes[0] == 0x6B:
        msg["id"] = "voltage"
    elif data_bytes[0] == 0x6C:
        msg["id"] = "current"
    elif data_bytes[0] == 0x6D:
        msg["id"] = "ch_stat"
    elif data_bytes[0] == 0x29:
        msg["id"] = "distance"

    msg["value"] = c_short(int.from_bytes(bytes(data_bytes[1:3]), "big")).value
    print(msg)
    await websocket.send_text(json.dumps(msg))


@app.get('/video_feed', response_class=StreamingResponse)
async def video_feed():
    """Video streaming route. Put this in the src attribute of an img tag."""
    return StreamingResponse(gen(), media_type='multipart/x-mixed-replace; boundary=frame')


@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()
    # ser.open()
    while True:
        global line, colors
        data = await websocket.receive_text()
        com, *vals = data.split(":")

        if com == "get_data":
            await spi_read(websocket)
        elif com == "line":
            line = vals[0] != "0"
            print(line)
        elif com == "colors":
            colors = vals[0] != "0"
            print(colors)
        elif com == "motors":
            direction, velocity = vals[0].split(",")
            motor_speed(int(direction), int(velocity))
        else:
            print(data)


if __name__ == "__main__":
    import uvicorn
    # while True:
    #     print(spi.readbytes(1))
    #     sleep(1)

    uvicorn.run(app=app, host="0.0.0.0", port=8010)

    del cam
    print("Camera unloading...")
