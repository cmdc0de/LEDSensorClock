from flask import Flask, json
from flask_cors import CORS
from flask import request

scans = [ { "id": 1, "ssid": "sid1", "rssi": -9, "channel": 11, "authMode": "WPA2PSK" },
    { "id": 2, "ssid": "sid2", "rssi": -94, "channel": 9, "authMode": "WPA2PSK" }]

calibration = {"top_left_x": 1,  "top_left_y": 1, "top_right_x": 1, "top_right_y": 1, 
    "bottom_left_x": 1, "bottom_left_y": 1,  "bottom_right_x": 1, "bottom_right_y": 1,
  "mid_x": 1, "mid_y": 1 }

sinfo = { "Free HeapSize": 1, "Free Min HeapSize": 1, "Free 32 Bit HeapSize": 1,
    "Free 32 Bit Min HeapSize": 1, "Free DMA HeapSize": 1, "Free DMA Min HeapSize": 1,
    "Free Internal HeapSize": 1, "Free Internal Min HeapSize": 1,
    "Free Default HeapSize": 1, "Free Default Min HeapSize" :1, "Free Exec": 1,
    "Free Exec Min": 1, "Model":1,  "Features":1, "EMB_FLASH":1, "WIFI_BGN":1,
    "BLE":1, "BT":1, "Cores":2, "Revision":1, "IDF Version": 'versionifo' }

settings = [{"name": "SecondsOnMainScreen", "value": 300}, {"name": "SecondsInGameOfLife", "value": 300}, 
        { "name" : "SecondsIn3D", "value": 300}, {"name": "SecondsShowingDrawings", "value": 120} ]

tz = { "tz": 'Etc/GMT'}

api = Flask(__name__)
CORS(api)

@api.route('/wifiscan', methods=['GET'])
def get_wifiscans():
  return json.dumps(scans)

@api.route('/calibration', methods=['GET'])
def get_calibration():
  return json.dumps(calibration)

@api.route('/systeminfo', methods=['GET'])
def get_systeminfo():
  return json.dumps(sinfo)

@api.route('/tz', methods=['GET'])
def get_tz():
  return json.dumps(tz)

@api.route('/settz', methods=['POST'])
def post_tz():
  request_data = request.get_json(True)
  print(type(request_data))
  print("post data  is ", request_data)
  return json.dumps(tz)

@api.route('/setting', methods=['GET'])
def get_settings():
  return json.dumps(settings)

if __name__ == '__main__':
    api.run()


