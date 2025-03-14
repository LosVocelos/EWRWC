from fastapi import FastAPI, Request, WebSocket, WebSocketDisconnect
from fastapi.responses import HTMLResponse, StreamingResponse
from fastapi.staticfiles import StaticFiles
from fastapi.templating import Jinja2Templates
from random import randint
import cv2
from time import sleep, time
from spidev import SpiDev
from picamera2 import Picamera2
import numpy as np
import json
from ctypes import c_short
import asyncio

from motors import Motors
from analyzer import Analyzer

t_start = time()
picam2 = Picamera2()
print(time() - t_start)
t_start = time()
config = picam2.create_preview_configuration(raw=picam2.sensor_modes[5])
print(time() - t_start)
t_start = time()
picam2.configure(config)
print(time() - t_start)
t_start = time()
picam2.start()
print(time() - t_start)

# Init the APP
app = FastAPI()
templates = Jinja2Templates(directory="templates")
app.mount("/static", StaticFiles(directory="static"), name="static")

# Init spi
spi = SpiDev()
spi.open(0, 0)
spi.max_speed_hz = 4_000_000

# Init motors
motors = Motors(spi)
motors.enable()

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
        image = picam2.capture_array()[:, :, :3]
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

    motors.speed = (int(left) * 256, int(right) * 256)
    message = motors.msg.copy()
    spi.xfer(message)

    print("left motor ", motors.speed[0])
    print("right motor ", motors.speed[1])
    print("lm ", left)
    print("rm ", right, "\n")


async def spi_read(websocket: WebSocket):
    msg = {"id": "", "value": 0}
    for j in range(6):
        ret = spi.readbytes(1)[0]
        if ret == 0xFF:
            print("yay")
            break
        print("read:", ret)
    else:
        print("nooo")
        return

    data_bytes = spi.readbytes(3)
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

    del picam2
    del motors
    spi.close()
    print("Camera unloading...")
