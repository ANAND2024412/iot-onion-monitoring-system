const express = require("express");
const mysql = require("mysql");
const cors = require("cors");
require("dotenv").config();

const app = express();

// ✅ CORS Configuration
app.use(cors({
  origin: "http://localhost:3000",  // Allow frontend access
  methods: ["GET", "POST", "PUT"],  // Allow PUT requests
  allowedHeaders: ["Content-Type"]
}));

app.use(express.json()); // Middleware to parse JSON requests

// ✅ Secure Database Connection
const db = mysql.createConnection({
  host: "127.0.0.1",
  user: "root",
  password: "Adarsh123@#$",
  database: "onion_warehouse",
  multipleStatements: true
});

// ✅ Handle MySQL Connection
db.connect((err) => {
  if (err) {
    console.error("❌ Database connection failed:", err);
    setTimeout(() => db.connect(), 2000); // Try reconnecting after 2 sec
  } else {
    console.log("✅ Connected to MySQL Database");
  }
});

// ✅ Handle MySQL Errors
db.on("error", (err) => {
  console.error("❌ MySQL Connection Error:", err);
  if (err.code === "PROTOCOL_CONNECTION_LOST") {
    setTimeout(() => db.connect(), 2000);
  } else {
    throw err;
  }
});

// ✅ Fetch Latest Warehouse Data (Latest entry for each warehouse)
app.get("/api/warehouse-data", (req, res) => {
  const query = `
    SELECT wd.*
    FROM warehouse_data wd
    INNER JOIN (
      SELECT warehouse_id, MAX(timestamp) AS latest_time
      FROM warehouse_data
      GROUP BY warehouse_id
    ) latest_data
    ON wd.warehouse_id = latest_data.warehouse_id AND wd.timestamp = latest_data.latest_time
    ORDER BY wd.warehouse_id;
  `;

  db.query(query, (err, results) => {
    if (err) {
      console.error("❌ Error fetching warehouse data:", err);
      return res.status(500).json({ error: "Database query failed" });
    }
    res.json(results);
  });
});

// ✅ Update Fan Status for the Latest Warehouse Entry
app.put("/api/update-fan", (req, res) => {
  const { warehouse_id, fan_status } = req.body;

  if (!warehouse_id || !fan_status) {
    return res.status(400).json({ error: "Missing warehouse_id or fan_status" });
  }

  console.log(`🔄 Received fan update request: Warehouse ${warehouse_id}, Status: ${fan_status}`);

  const sql = `
    UPDATE warehouse_data
    SET fan_status = ?
    WHERE warehouse_id = ? 
    ORDER BY timestamp DESC 
    LIMIT 1;
  `;

  db.query(sql, [fan_status, warehouse_id], (err, result) => {
    if (err) {
      console.error("❌ Error updating fan status:", err);
      return res.status(500).send("Database update failed");
    }

    console.log(`✅ Fan status updated for Warehouse ${warehouse_id}: ${fan_status}`);
    res.status(200).json({ message: "Fan status updated successfully" });
  });
});

// ✅ Start the Server
const PORT = process.env.PORT || 5000;
app.listen(PORT, () => {
  console.log(`🚀 Server running on port ${PORT}`);
});
