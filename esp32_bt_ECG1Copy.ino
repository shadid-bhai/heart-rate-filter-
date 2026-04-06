#include <WiFi.h>
#include <WebServer.h>

const int ECG_PIN = 34;
const int LO_PLUS = 26;
const int LO_MINUS = 27;

const char* ssid = "ESP32-ECG";
const char* password = "12345678";

WebServer server(80);

int latestECG = 0;
int latestLOPlus = 0;
int latestLOMinus = 0;

const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 ECG Recorder</title>
  <style>
    body {
      margin: 0;
      padding: 0;
      background: #111;
      color: #ddd;
      font-family: Arial, sans-serif;
      text-align: center;
    }

    h2 {
      margin: 16px 0 10px 0;
      font-weight: normal;
    }

    .topbar {
      margin-bottom: 10px;
      font-size: 15px;
    }

    .status {
      margin: 10px 0;
      font-size: 16px;
    }

    .good { color: #00e676; }
    .bad { color: #ff5252; }
    .warn { color: #ffd54f; }

    .panel {
      width: 95%;
      max-width: 900px;
      margin: 10px auto;
      padding: 14px;
      background: #1a1a1a;
      border: 1px solid #333;
      border-radius: 10px;
      box-sizing: border-box;
    }

    .row {
      display: flex;
      flex-wrap: wrap;
      justify-content: center;
      gap: 10px;
      margin: 10px 0;
    }

    input {
      padding: 10px;
      font-size: 15px;
      border: 1px solid #444;
      border-radius: 6px;
      background: #0f1720;
      color: #fff;
      width: 220px;
      max-width: 90%;
      box-sizing: border-box;
    }

    button {
      padding: 10px 16px;
      font-size: 15px;
      border: none;
      border-radius: 6px;
      cursor: pointer;
      background: #2b7cff;
      color: white;
    }

    button:disabled {
      background: #555;
      cursor: not-allowed;
    }

    .danger { background: #d32f2f; }
    .success { background: #2e7d32; }

    canvas {
      display: block;
      margin: 15px auto 20px auto;
      background: #0f1720;
      border: 1px solid #333;
      width: 98vw;
      max-width: 1400px;
      height: 500px;
    }

    .info {
      margin-top: 10px;
      font-size: 14px;
      color: #bbb;
    }
  </style>
</head>
<body>
  <h2>ESP32 ECG Recorder</h2>
  <div class="topbar">Wi-Fi: <b>ESP32-ECG</b> | Open: <b>192.168.4.1</b></div>

  <div class="panel">
    <div class="row">
      <input type="text" id="patientName" placeholder="Patient Name">
      <input type="number" id="patientAge" placeholder="Age">
    </div>

    <div class="row">
      <button id="startBtn" class="success" onclick="startRecording()">Start Recording</button>
      <button id="stopBtn" class="danger" onclick="stopRecording()" disabled>Stop Recording</button>
      <button id="downloadBtn" onclick="downloadExcel()" disabled>Download Excel</button>
    </div>

    <div class="status" id="recordStatus"><span class="warn">Enter patient info, then start.</span></div>
    <div class="status" id="leadStatus">Lead status: checking.</div>
    <div class="info" id="sessionInfo">No recording started.</div>
  </div>

  <canvas id="graph" width="1400" height="500"></canvas>

  <script>
    const canvas = document.getElementById("graph");
    const ctx = canvas.getContext("2d");

    const width = canvas.width;
    const height = canvas.height;

    // smaller visible sample window = zoom in on X axis
    const visibleSamples = 180;

    let data = new Array(visibleSamples).fill(1800);

    let isRecording = false;
    let sessionStartTime = 0;
    let patientName = "";
    let patientAge = "";
    let records = [];

    function drawGrid() {
      ctx.strokeStyle = "rgba(255,255,255,0.08)";
      ctx.lineWidth = 1;

      for (let x = 0; x <= width; x += 100) {
        ctx.beginPath();
        ctx.moveTo(x, 0);
        ctx.lineTo(x, height);
        ctx.stroke();
      }

      for (let y = 0; y <= height; y += 100) {
        ctx.beginPath();
        ctx.moveTo(0, y);
        ctx.lineTo(width, y);
        ctx.stroke();
      }
    }

    function mapValueToY(v) {
      if (v < 0) v = 0;
      if (v > 4095) v = 4095;

      // display only compression to reduce visible peak height
      const center = 1900;
      const gain = 0.22;

      let scaled = center + (v - center) * gain;

      const minV = 1400;
      const maxV = 2400;

      if (scaled < minV) scaled = minV;
      if (scaled > maxV) scaled = maxV;

      return height - ((scaled - minV) / (maxV - minV)) * height;
    }

    function drawWave() {
      ctx.clearRect(0, 0, width, height);
      drawGrid();

      ctx.strokeStyle = "#00e5ff";
      ctx.lineWidth = 2;
      ctx.lineJoin = "round";
      ctx.lineCap = "round";
      ctx.beginPath();

      for (let i = 0; i < data.length; i++) {
        const x = (i / (data.length - 1)) * width;
        const y = mapValueToY(data[i]);

        if (i === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      }

      ctx.stroke();
    }

    function escapeHtml(text) {
      return String(text)
        .replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/>/g, "&gt;")
        .replace(/"/g, "&quot;")
        .replace(/'/g, "&#039;");
    }

    function startRecording() {
      const nameInput = document.getElementById("patientName").value.trim();
      const ageInput = document.getElementById("patientAge").value.trim();

      if (nameInput === "" || ageInput === "") {
        document.getElementById("recordStatus").innerHTML =
          '<span class="bad">Please enter patient name and age.</span>';
        return;
      }

      patientName = nameInput;
      patientAge = ageInput;
      records = [];
      data = new Array(visibleSamples).fill(1800);
      sessionStartTime = Date.now();
      isRecording = true;

      document.getElementById("startBtn").disabled = true;
      document.getElementById("stopBtn").disabled = false;
      document.getElementById("downloadBtn").disabled = true;

      document.getElementById("recordStatus").innerHTML =
        '<span class="good">Recording started.</span>';
      document.getElementById("sessionInfo").innerText =
        "Patient: " + patientName + " | Age: " + patientAge;
    }

    function stopRecording() {
      isRecording = false;

      document.getElementById("startBtn").disabled = false;
      document.getElementById("stopBtn").disabled = true;
      document.getElementById("downloadBtn").disabled = records.length === 0;

      document.getElementById("recordStatus").innerHTML =
        '<span class="warn">Recording stopped. You can now download the Excel file.</span>';
    }

    function downloadExcel() {
      if (records.length === 0) {
        document.getElementById("recordStatus").innerHTML =
          '<span class="bad">No data available.</span>';
        return;
      }

      let html = `
        <html>
        <head>
          <meta charset="UTF-8">
        </head>
        <body>
          <table border="1">
            <tr><td><b>Patient Name</b></td><td>${escapeHtml(patientName)}</td></tr>
            <tr><td><b>Age</b></td><td>${escapeHtml(patientAge)}</td></tr>
            <tr><td><b>Recording Start</b></td><td>${escapeHtml(new Date(sessionStartTime).toLocaleString())}</td></tr>
            <tr></tr>
            <tr>
              <th>Time (ms)</th>
              <th>ECG</th>
              <th>LO_PLUS</th>
              <th>LO_MINUS</th>
            </tr>
      `;

      for (let i = 0; i < records.length; i++) {
        html += `
          <tr>
            <td>${records[i].time}</td>
            <td>${records[i].ecg}</td>
            <td>${records[i].loPlus}</td>
            <td>${records[i].loMinus}</td>
          </tr>
        `;
      }

      html += `
          </table>
        </body>
        </html>
      `;

      const blob = new Blob([html], { type: "application/vnd.ms-excel" });
      const url = URL.createObjectURL(blob);

      const safeName = patientName.replace(/[^a-zA-Z0-9_-]/g, "_");
      const fileName = safeName + "_age_" + patientAge + "_ecg.xls";

      const a = document.createElement("a");
      a.href = url;
      a.download = fileName;
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);

      URL.revokeObjectURL(url);
    }

    async function fetchECG() {
      try {
        const response = await fetch("/ecg");
        const text = await response.text();
        const parts = text.split(",");

        const ecg = parseInt(parts[0]);
        const loPlus = parseInt(parts[1]);
        const loMinus = parseInt(parts[2]);

        if (!isNaN(ecg)) {
          data.push(ecg);
          data.shift();
        }

        const leadStatus = document.getElementById("leadStatus");
        if (loPlus === 1 || loMinus === 1) {
          leadStatus.innerHTML = '<span class="bad">Electrodes disconnected</span>';
        } else {
          leadStatus.innerHTML = '<span class="good">Electrodes connected</span>';
        }

        if (isRecording && !isNaN(ecg)) {
          const elapsed = Date.now() - sessionStartTime;
          records.push({
            time: elapsed,
            ecg: ecg,
            loPlus: loPlus,
            loMinus: loMinus
          });
        }

        drawWave();
      } catch (e) {
        document.getElementById("leadStatus").innerHTML =
          '<span class="bad">Connection error</span>';
      }
    }

    drawWave();
    setInterval(fetchECG, 20);
  </script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", MAIN_page);
}

void handleECG() {
  String data = String(latestECG) + "," + String(latestLOPlus) + "," + String(latestLOMinus);
  server.send(200, "text/plain", data);
}

void setup() {
  Serial.begin(115200);

  pinMode(ECG_PIN, INPUT);
  pinMode(LO_PLUS, INPUT);
  pinMode(LO_MINUS, INPUT);

  analogReadResolution(12);

  WiFi.softAP(ssid, password);

  Serial.println();
  Serial.println("WiFi started");
  Serial.print("Connect to: ");
  Serial.println(ssid);
  Serial.print("Open browser at: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/ecg", handleECG);
  server.begin();
}

void loop() {
  server.handleClient();

  latestLOPlus = digitalRead(LO_PLUS);
  latestLOMinus = digitalRead(LO_MINUS);

  if (latestLOPlus == 1 || latestLOMinus == 1) {
    latestECG = 0;
  } else {
    latestECG = analogRead(ECG_PIN);
  }

  Serial.println(latestECG);
  delay(15);
}