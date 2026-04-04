from flask import Flask, render_template, request, jsonify
from flask_socketio import SocketIO
from datetime import datetime
import smtplib
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart

app = Flask(__name__)
app.config["SECRET_KEY"] = "street-light-secret"
socketio = SocketIO(app, cors_allowed_origins="*")

# 🔹 Email Config
EMAIL = "akrammohammad0705@gmail.com"
PASSWORD = "utvsjnsxgtyjgqjn"   # Gmail App Password
TO_EMAIL = "agentakram007@gmail.com"

# 🔹 Prevent email spam
fault_sent = False

# 🔹 Store latest data
latest_data = {
    "current_mA": 0,
    "current_A": 0,
    "adc": 0,
    "offset": 0,
    "status": "No Load",
    "timestamp": None,
}

# ================================================
# EMAIL FUNCTION (UTF-8 SAFE)
# ================================================
def send_email(message):
    try:
        msg = MIMEMultipart()
        msg["From"] = EMAIL
        msg["To"] = TO_EMAIL
        msg["Subject"] = "Street Light Fault Alert"

        # UTF-8 FIX (important)
        msg.attach(MIMEText(message, "plain", "utf-8"))

        server = smtplib.SMTP("smtp.gmail.com", 587)
        server.starttls()
        server.login(EMAIL, PASSWORD)

        server.send_message(msg)
        server.quit()

        print("✅ Email Sent")

    except Exception as e:
        print("❌ Email Error:", e)

# ================================================
# HOME
# ================================================
@app.route("/")
def index():
    return render_template("index.html")

# ================================================
# RECEIVE DATA FROM ESP32
# ================================================
@app.route("/api/data", methods=["POST"])
def receive_data():
    global fault_sent

    data = request.get_json(silent=True)
    if not data:
        return jsonify({"error": "invalid json"}), 400

    # Store values
    latest_data["current_mA"] = data.get("current_mA", 0)
    latest_data["current_A"] = data.get("current_A", 0)
    latest_data["adc"] = data.get("adc", 0)
    latest_data["offset"] = data.get("offset", 0)
    latest_data["status"] = data.get("status", "Unknown")
    latest_data["timestamp"] = datetime.now().strftime("%H:%M:%S")

    current = latest_data["current_mA"]

    print(f"Current: {current} mA")

    # ============================================
    # 🔥 FAULT CONDITION (NEGATIVE CURRENT)
    # ============================================
    if current < 0:
        if not fault_sent:
            send_email(
                f"Fault Detected!\n\n"
                f"Current: {current} mA\n"
                f"Status: {latest_data['status']}\n"
                f"Time: {latest_data['timestamp']}"
            )
            fault_sent = True
    else:
        fault_sent = False  # reset when normal

    # Send to dashboard
    socketio.emit("sensor_update", latest_data)

    return jsonify({"ok": True}), 200

# ================================================
# GET LATEST DATA
# ================================================
@app.route("/api/latest")
def get_latest():
    return jsonify(latest_data)

# ================================================
# SOCKET EVENTS
# ================================================
@socketio.on("connect")
def handle_connect():
    print(f"Client connected: {request.sid}")
    socketio.emit("sensor_update", latest_data, to=request.sid)

@socketio.on("disconnect")
def handle_disconnect():
    print(f"Client disconnected: {request.sid}")

# ================================================
# RUN SERVER
# ================================================
if __name__ == "__main__":
    socketio.run(app, host="0.0.0.0", port=5000, debug=True)
