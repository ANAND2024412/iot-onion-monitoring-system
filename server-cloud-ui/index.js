const express = require("express");
const cors = require("cors");
const bodyParser = require("body-parser");
const admin = require("firebase-admin");
const serviceAccount = require("./serviceAccountKey.json");

const app = express();
const PORT = 5000;

// Middleware
app.use(cors());
app.use(bodyParser.json());

// Firebase Admin Initialization
admin.initializeApp({
  credential: admin.credential.cert(serviceAccount),
  databaseURL: "https://onion-warehouse-default-rtdb.firebaseio.com"
});

const db = admin.database();

// Variable to hold the latest ESP32 sensor data
let latestSensorData = {
  warehouse1: {},
  warehouse2: {},
  avg_humidity: "--"
};

// 🟢 GET warehouse data from ESP32
app.get("/api/warehouse-data", (req, res) => {
  try {
    // Return the latest sensor data received from the ESP32
    res.json({
      data: [
        {
          warehouse_id: 1,
          temperature: latestSensorData.warehouse1.temperature || "--",
          humidity: latestSensorData.warehouse1.humidity || "--",
          methane_level: latestSensorData.warehouse1.methane || "--",
          onion_weight: latestSensorData.warehouse1.weight || 0,
          fan_status: latestSensorData.warehouse1.fan || "OFF"
        },
        {
          warehouse_id: 2,
          temperature: latestSensorData.warehouse2.temperature || "--",
          humidity: latestSensorData.warehouse2.humidity || "--",
          methane_level: latestSensorData.warehouse2.methane || "--",
          onion_weight: latestSensorData.warehouse2.weight || 0,
          fan_status: latestSensorData.warehouse2.fan || "OFF",
          flame: latestSensorData.warehouse2.flame || 0
        }//default to zero 
      ],
      avg_humidity: latestSensorData.avg_humidity || "--"
    });
  } catch (error) {
    console.error("Error fetching warehouse data:", error);
    res.status(500).json({ error: "Failed to fetch data" });
  }
});

//  POST - Update sensor data including flame (if any)
app.post("/api/update-sensors", async (req, res) => {
  const { warehouse_id, temperature, humidity, methane, weight, flame, avg_humidity, fan } = req.body;

  try {
    // Update in-memory data
    if (warehouse_id === 1) {
      latestSensorData.warehouse1 = { temperature, humidity, methane, weight, fan: fan || "OFF" };
    } else if (warehouse_id === 2) {
      latestSensorData.warehouse2 = { temperature, humidity, methane, weight, flame, fan: fan || "OFF" };
    }

    if (avg_humidity !== undefined) {
      latestSensorData.avg_humidity = avg_humidity;
    }

    // Update Firebase
    const updates = {};
    if (warehouse_id === 1) {
      updates["/warehouse1"] = { temperature, humidity, methane, weight, fan_status: fan || "OFF" };
    } else if (warehouse_id === 2) {
      updates["/warehouse2"] = { temperature, humidity, methane, flame, fan_status: fan || "OFF" };
    }

    if (avg_humidity !== undefined) {
      updates["/avg_humidity"] = avg_humidity;
    }

    await db.ref().update(updates);

    res.json({ message: "Sensor data updated and pushed to Firebase!" });
  } catch (error) {
    console.error("Failed to update Firebase:", error);
    res.status(500).json({ error: "Failed to update Firebase" });
  }
});

// 🏠 Home route - Show live data received from ESP32
app.get('/', (req, res) => {
  try {
    const warehouse1 = latestSensorData.warehouse1 || {};
    const warehouse2 = latestSensorData.warehouse2 || {};
    const avgHumidity = latestSensorData.avg_humidity || "--";

    res.send(`
      <html>
        <head>
          <title>Onion Warehouse Dashboard</title>
          <style>
            body {
              font-family: Arial, sans-serif;
              background: #111;
              color: #eee;
              padding: 30px;
            }
            h1 {
              color: #0f0;
            }
            h2 {
              margin-top: 30px;
              color: #ff0;
            }
            pre {
              background-color: #222;
              padding: 20px;
              border-radius: 8px;
              overflow-x: auto;
              font-size: 14px;
              color: #0ff;
            }
            .label {
              color: #ccc;
            }
          </style>
        </head>
        <body>
          <h1>📦 Onion Warehouse Server- Live Sensor Data</h1>
          <div class="label">Average Humidity: <strong>${avgHumidity}</strong></div>

          <h2>🏬 Warehouse 1</h2>
          <pre>${JSON.stringify(warehouse1, null, 2)}</pre>

          <h2>🏬 Warehouse 2</h2>
          <pre>${JSON.stringify(warehouse2, null, 2)}</pre>

          <p class="label">Data refreshed from ESP32 at: ${new Date().toLocaleString()}</p>
        </body>
      </html>
    `);
  } catch (error) {
    res.status(500).send(`<h1>Error fetching data from ESP32</h1><pre>${error.message}</pre>`);
  }
});

// 🚀 Start server
app.listen(PORT, () => {
  console.log(`🚀 Server running at http://localhost:${PORT}`);
});
