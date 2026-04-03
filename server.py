from flask import Flask, render_template, request, jsonify
from flask_socketio import SocketIO
from datetime import datetime

app = Flask(__name__)
app.config["SECRET_KEY"] = "street-light-secret"
socketio = SocketIO(app, cors_allowed_origins="*")

latest_data = {
    "current_mA": 0,
    "current_A": 0,
    "adc": 0,
    "offset": 0,
    "status": "No Load",
    "timestamp": None,
}


@app.route("/")
def index():
    return render_template("index.html")


@app.route("/api/data", methods=["POST"])
def receive_data():
    data = request.get_json(silent=True)
    if not data:
        return jsonify({"error": "invalid json"}), 400

    latest_data["current_mA"] = data.get("current_mA", 0)
    latest_data["current_A"] = data.get("current_A", 0)
    latest_data["adc"] = data.get("adc", 0)
    latest_data["offset"] = data.get("offset", 0)
    latest_data["status"] = data.get("status", "Unknown")
    latest_data["timestamp"] = datetime.now().strftime("%H:%M:%S")

    socketio.emit("sensor_update", latest_data)
    return jsonify({"ok": True}), 200


@app.route("/api/latest")
def get_latest():
    return jsonify(latest_data)


@socketio.on("connect")
def handle_connect():
    print(f"Client connected: {request.sid}")
    socketio.emit("sensor_update", latest_data, to=request.sid)


@socketio.on("disconnect")
def handle_disconnect():
    print(f"Client disconnected: {request.sid}")


if __name__ == "__main__":
    socketio.run(app, host="0.0.0.0", port=5000, debug=True)
