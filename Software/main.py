from fastapi import FastAPI, Request, WebSocket, WebSocketDisconnect
from fastapi.responses import HTMLResponse, StreamingResponse
from fastapi.staticfiles import StaticFiles
from fastapi.templating import Jinja2Templates
from random import randint
import cv2
from time import sleep
from spidev import SpiDev
from motors import Motors
from analyzer import Analyzer
from picamera2 import Picamera2

picam2 = Picamera2()
config = picam2.create_preview_configuration(raw=picam2.sensor_modes[5])
picam2.configure(config)
picam2.start()

# Init the APP
app = FastAPI()
templates = Jinja2Templates(directory="templates")
app.mount("/static", StaticFiles(directory="static"), name="static")

# Init spi
spi = SpiDev()
spi.open(0,0)
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
    global analyzer, line, colors, deviation, verdict
    image = picam2.capture_array()
    if line or colors:
        output = cv2.zeros(image.shape, np.uint8)
    else:
        output = image

    if line:
        binary = analyzer.preprocessing(image)
        deviation, out, con = find_centroid(binary)
        np.biwise(output, out)
    if colors:
        verdict, out = find_colors(image)
        np.biwise(output, out)
    ret, jpeg = cv2.imencode('.jpg', output)
    return b'--frame\r\nContent-Type: image/jpeg\r\n\r\n' + jpeg.tobytes() + b'\r\n'


def motor_speed(direction: int, velocity: int):
    lm = velocity
    rm = velocity
    lm += direction
    rm -= direction

    motors.speed = (int(lm) * 256, int(rm) * 256)

    print("left motor ", motors.speed[0])
    print("right motor ", motors.speed[1])
    print("lm ", lm)
    print("rm ", rm, "\n")


@app.get('/video_feed', response_class=StreamingResponse)
async def video_feed():
    """Video streaming route. Put this in the src attribute of an img tag."""
    # sleep(0.05)
    return StreamingResponse(gen(), media_type='multipart/x-mixed-replace; boundary=frame')


@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()
    # ser.open()
    while True:
        data = await websocket.receive_text()
        com, *vals = data.split(":")

        if com == "get_data":
            await websocket.send_text(f"{deviation}:{verdict}")
            print(f"{deviation}:{verdict}")
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
    while True:
        print(spi.readbytes(1))
        sleep(1)

    uvicorn.run(app=app, host="0.0.0.0", port=8010)

    del picam2
    print("Camera unloading...")

