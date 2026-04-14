"""
================================================
FLASK SERVER  –  Smart Street Light Monitor
- Receives ESP32 POST data
- Emits live updates via Socket.IO
- Sends email alert on fault (once per fault event)
- Stores last 100 readings in memory
================================================
Install:  pip install flask flask-socketio
Run:      python server.py
"""

from flask import Flask, render_template, request, jsonify
from flask_socketio import SocketIO
from datetime import datetime
from collections import deque
import smtplib, threading
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart

# ─────────────────────────────────────────────
#  CONFIG
# ─────────────────────────────────────────────
EMAIL_FROM = "agentakram007@gmail.com"
EMAIL_PASS = "vomnvrrdyeegoehi"   # Gmail App Password (16-char)
EMAIL_TO   = "izonakassistant@gmail.com"

# ─────────────────────────────────────────────
app      = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins="*", async_mode="threading")

# State
latest_data = {}
history     = deque(maxlen=100)
alerts      = deque(maxlen=50)
fault_sent  = False

# ─────────────────────────────────────────────
#  EMAIL
# ─────────────────────────────────────────────
def send_email_async(data: dict):
    def _send():
        try:
            msg = MIMEMultipart("alternative")
            msg["Subject"] = f"⚠️ Street Light FAULT – {data.get('street','?')}"
            msg["From"]    = EMAIL_FROM
            msg["To"]      = EMAIL_TO

            text = (
                f"FAULT DETECTED\n\n"
                f"Street  : {data.get('street')}\n"
                f"Time    : {data.get('timestamp')}\n"
                f"Status  : {data.get('system_status')}\n\n"
                f"Light {data.get('light1_id')}  |  INA219  |  {data.get('led1_current')} mA  |  "
                f"{'FAULT' if data.get('led1_fault') else 'OK'}\n"
                f"Light {data.get('light2_id')}  |  ACS712  |  {data.get('led2_current')} mA  |  "
                f"{'FAULT' if data.get('led2_fault') else 'OK'}\n"
            )

            html = f"""
            <html><body style="font-family:monospace;background:#0b0f1a;color:#e8f0fe;padding:24px;">
            <h2 style="color:#ff1744;">⚠️ Street Light Fault Alert</h2>
            <table style="border-collapse:collapse;width:100%;max-width:480px;">
              <tr><td style="padding:6px 12px;color:#8899bb;">Street</td>
                  <td style="padding:6px 12px;font-weight:bold;">{data.get('street')}</td></tr>
              <tr><td style="padding:6px 12px;color:#8899bb;">Time</td>
                  <td style="padding:6px 12px;">{data.get('timestamp')}</td></tr>
              <tr><td style="padding:6px 12px;color:#8899bb;">Status</td>
                  <td style="padding:6px 12px;color:#ff1744;font-weight:bold;">{data.get('system_status')}</td></tr>
            </table>
            <br>
            <table style="border-collapse:collapse;width:100%;max-width:480px;border:1px solid #1a2a3a;">
              <tr style="background:#111c30;">
                <th style="padding:8px 12px;text-align:left;">Light</th>
                <th style="padding:8px 12px;text-align:left;">Sensor</th>
                <th style="padding:8px 12px;text-align:left;">Current</th>
                <th style="padding:8px 12px;text-align:left;">Status</th>
              </tr>
              <tr>
                <td style="padding:8px 12px;">#{data.get('light1_id')}</td>
                <td style="padding:8px 12px;">INA219</td>
                <td style="padding:8px 12px;">{data.get('led1_current')} mA</td>
                <td style="padding:8px 12px;color:{'#ff1744' if data.get('led1_fault') else '#00e676'};">
                  {'⚠ FAULT' if data.get('led1_fault') else '✓ OK'}</td>
              </tr>
              <tr style="background:#0a1120;">
                <td style="padding:8px 12px;">#{data.get('light2_id')}</td>
                <td style="padding:8px 12px;">ACS712</td>
                <td style="padding:8px 12px;">{data.get('led2_current')} mA</td>
                <td style="padding:8px 12px;color:{'#ff1744' if data.get('led2_fault') else '#00e676'};">
                  {'⚠ FAULT' if data.get('led2_fault') else '✓ OK'}</td>
              </tr>
            </table>
            <p style="margin-top:24px;color:#445577;font-size:12px;">
              Smart Street Light Monitoring System – Auto Alert
            </p>
            </body></html>
            """

            msg.attach(MIMEText(text, "plain"))
            msg.attach(MIMEText(html, "html"))

            server = smtplib.SMTP("smtp.gmail.com", 587)
            server.starttls()
            server.login(EMAIL_FROM, EMAIL_PASS)
            server.send_message(msg)
            server.quit()
            print("[Email] Fault alert sent successfully")

        except Exception as e:
            print(f"[Email] ERROR: {e}")

    threading.Thread(target=_send, daemon=True).start()


# ─────────────────────────────────────────────
#  ROUTES
# ─────────────────────────────────────────────
@app.route("/")
def home():
    return render_template("index.html")


@app.route("/api/data", methods=["POST"])
def receive_data():
    global fault_sent, latest_data

    d = request.get_json(force=True)
    if not d:
        return jsonify({"ok": False, "error": "No JSON"}), 400

    now = datetime.now()

    latest_data = {
        "street"        : d.get("street",        "Unknown"),
        "light1_id"     : d.get("light1_id",     0),
        "light2_id"     : d.get("light2_id",     0),
        "led1_current"  : round(float(d.get("led1_current", 0)), 2),
        "led2_current"  : round(float(d.get("led2_current", 0)), 2),
        "led1_fault"    : bool(d.get("led1_fault",  False)),
        "led2_fault"    : bool(d.get("led2_fault",  False)),
        "system_status" : d.get("system_status", "UNKNOWN"),
        "timestamp"     : now.strftime("%H:%M:%S"),
        "date"          : now.strftime("%d %B %Y"),
    }

    # Append to history for chart
    history.append({
        "t"  : now.strftime("%H:%M:%S"),
        "l1" : latest_data["led1_current"],
        "l2" : latest_data["led2_current"],
    })

    # Fault handling
    is_fault = latest_data["system_status"] != "ALL_WORKING"
    if is_fault:
        if not fault_sent:
            send_email_async(latest_data)
            fault_sent = True
        alerts.appendleft(dict(latest_data))
    else:
        fault_sent = False

    # Push to browser via Socket.IO
    socketio.emit("sensor_update", latest_data)
    socketio.emit("history_update", list(history)[-30:])

    print(f"[Data] {latest_data['street']} | "
          f"L1={latest_data['led1_current']}mA {'FAULT' if latest_data['led1_fault'] else 'OK'} | "
          f"L2={latest_data['led2_current']}mA {'FAULT' if latest_data['led2_fault'] else 'OK'} | "
          f"{latest_data['system_status']}")

    return jsonify({"ok": True})


@app.route("/api/history")
def get_history():
    return jsonify(list(history))


@app.route("/api/alerts")
def get_alerts():
    return jsonify(list(alerts))


# ─────────────────────────────────────────────
if __name__ == "__main__":
    print("=== Smart Street Light Server ===")
    print("→  Dashboard: http://0.0.0.0:5001")
    socketio.run(app, host="0.0.0.0", port=5001, debug=True)
