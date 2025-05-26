const express = require("express");
const fs = require("fs");
const cors = require("cors");
const path = require("path");

const app = express();
const PORT = 3000;
const LOG_FILE_PATH = path.join(__dirname, "logs", "mpi.log");

app.use(cors());

const parseLogFile = () => {
  const logs = fs.readFileSync(LOG_FILE_PATH, "utf-8").split("\n");
  const processorStatus = {};
  const jobList = [];

  logs.forEach((line) => {
    const hbMatch = line.match(/heartbeat from processor (\d+)/i);
    const jobAssignMatch = line.match(
      /Job (\d+) assigned to processor (\d+) using ([\w-]+)/i
    );

    if (hbMatch) {
      const id = parseInt(hbMatch[1]);
      if (!processorStatus[id]) processorStatus[id] = { alive: true, jobs: [] };
      else processorStatus[id].alive = true;
    }

    if (jobAssignMatch) {
      const jobId = parseInt(jobAssignMatch[1]);
      const procId = parseInt(jobAssignMatch[2]);
      const strategy = jobAssignMatch[3];

      if (!processorStatus[procId])
        processorStatus[procId] = { alive: true, jobs: [] };

      processorStatus[procId].jobs.push(jobId);

      jobList.push({
        id: jobId,
        processorId: procId,
        strategy: strategy,
        status: "assigned",
      });
    }
  });

  const processors = Object.entries(processorStatus).map(([id, data]) => ({
    id: parseInt(id),
    alive: data.alive,
    jobs: data.jobs,
  }));

  return { processors, jobList, rawLogs: logs.join("\n") };
};

app.get("/api/processors", (req, res) => {
  try {
    const { processors } = parseLogFile();
    res.json(processors);
  } catch (err) {
    console.error("Error reading processors:", err);
    res.status(500).json({ error: "Failed to read processors" });
  }
});

app.get("/api/jobs", (req, res) => {
  try {
    const { jobList } = parseLogFile();
    res.json(jobList);
  } catch (err) {
    console.error("Error reading jobs:", err);
    res.status(500).json({ error: "Failed to read jobs" });
  }
});

app.get("/api/logs", (req, res) => {
  try {
    const { rawLogs } = parseLogFile();
    res.send(rawLogs);
  } catch (err) {
    console.error("Error reading logs:", err);
    res.status(500).send("Failed to read logs");
  }
});

app.listen(PORT, () => {
  console.log(`Backend running at http://localhost:${PORT}`);
});
