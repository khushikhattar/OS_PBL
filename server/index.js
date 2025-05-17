const express = require("express");
const fs = require("fs");
const cors = require("cors");
const path = require("path");

const app = express();
const PORT = 4000;

app.use(cors());

// Root route handler
app.get("/", (req, res) => {
  res.send("Welcome to the MPI Backend Server");
});

app.get("/logs", (req, res) => {
  const logPath = path.join(__dirname, "logs", "mpi.log");
  fs.readFile(logPath, "utf8", (err, data) => {
    if (err) return res.status(500).send("Error reading log");
    res.send(data);
  });
});

app.listen(PORT, () => {
  console.log(`🟢 Backend running at http://localhost:${PORT}`);
});
