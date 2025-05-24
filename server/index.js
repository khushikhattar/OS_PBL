const express = require("express");
const fs = require("fs");
const cors = require("cors");
const app = express();
const PORT = 3000;

app.use(cors());

const LOG_FILE_PATH = "./logs/mpi.log";
app.get("/api/processors", (req, res) => {
  try {
    const logs = fs.readFileSync(LOG_FILE_PATH, "utf-8").split("\n");
    const status = {};
    logs.forEach((line) => {
      const hbMatch = line.match(/Heartbeat from processor (\d+)/i);
      const jobAssignMatch = line.match(
        /Job (\d+) assigned to processor (\d+)/i
      );

      if (hbMatch) {
        const id = parseInt(hbMatch[1]);
        if (!status[id]) status[id] = { alive: true, jobs: [] };
        else status[id].alive = true;
      }

      if (jobAssignMatch) {
        const jobId = parseInt(jobAssignMatch[1]);
        const pId = parseInt(jobAssignMatch[2]);
        if (!status[pId]) status[pId] = { alive: true, jobs: [] };
        status[pId].jobs.push(jobId);
      }
    });

    const result = Object.keys(status).map((id) => ({
      id: parseInt(id),
      alive: status[id].alive,
      jobs: status[id].jobs,
    }));

    res.json(result);
  } catch (err) {
    console.error("Error reading processors:", err);
    res.status(500).send("Processors error");
  }
});

app.get("/api/jobs", (req, res) => {
  try {
    const logs = fs.readFileSync(LOG_FILE_PATH, "utf-8").split("\n");
    const jobs = [];

    logs.forEach((line) => {
      const match = line.match(/Job (\d+) assigned to processor (\d+)/i);
      if (match) {
        const jobId = parseInt(match[1]);
        const processorId = parseInt(match[2]);
        jobs.push({
          id: jobId,
          processorId,
          strategy: null,
          status: "assigned",
        });
      }
    });

    res.json(jobs);
  } catch (err) {
    console.error("Error reading jobs:", err);
    res.status(500).send("Jobs error");
  }
});

app.get("/api/logs", (req, res) => {
  try {
    const data = fs.readFileSync(LOG_FILE_PATH, "utf-8");
    res.send(data);
  } catch (err) {
    console.error("Error reading logs:", err);
    res.status(500).send("Logs error");
  }
});

app.listen(PORT, () => {
  console.log(`Backend server running at http://localhost:${PORT}`);
});
