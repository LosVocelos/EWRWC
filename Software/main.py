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
spi.max_speed_hz = 4000000

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
        image = picam2.capture_array()
        if line or colors:
            output = cv2.zeros(image.shape, np.uint8)
        else:
            output = image

        if line:
            binary = analyzer.preprocessing(image)
            deviation, out, con = analyzer.find_centroid(binary)
            np.biwise(output, out)
        if colors:
            verdict, out = analyzer.find_colors(image)
            np.biwise(output, out)
        ret, jpeg = cv2.imencode('.jpg', output)
        yield b'--frame\r\nContent-Type: image/jpeg\r\n\r\n' + jpeg.tobytes() + b'\r\n'


def motor_speed(left: int, right: int):

    motors.speed = (int(left) * 256, int(right) * 256)

    print("left motor ", motors.speed[0])
    print("right motor ", motors.speed[1])
    print("lm ", left)
    print("rm ", right, "\n")


async def spi_read(websocket: WebSocket):
    i = 0
    msg = {"id": "", "value": 0}
    while i < 4:
        for j in range(4):
            if spi.readbytes(1)[0] == 0xFF:
                print("yay")
                break
        else:
            print("nooo")
            return
        data_bytes = spi.xfer([0, 0, 0], 4000000, 5000)
        print(data_bytes)
        if data_bytes[0] == 0x6B:
            msg["id"] = "voltage"
        elif data_bytes[0] == 0x6C:
            msg["id"] = "current"
        elif data_bytes[0] == 0x6D:
            msg["id"] = "ch_stat"
        elif data_bytes[0] == 0x29:
            msg["id"] = "distance"

        msg["value"] = int.from_bytes(bytes(data_bytes[1:]), "big")

        print(msg)
        await websocket.send_text(json.dumps(msg))
        i += 1


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
    print("Camera unloading...")
