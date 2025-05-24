import React, { useEffect, useState } from "react";
import "./App.css";

function App() {
  const [processors, setProcessors] = useState([]);
  const [jobs, setJobs] = useState([]);
  const [logs, setLogs] = useState("");
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);

  const fetchData = async () => {
    try {
      const [procRes, jobsRes, logsRes] = await Promise.all([
        fetch("http://localhost:3000/api/processors"),
        fetch("http://localhost:3000/api/jobs"),
        fetch("http://localhost:3000/api/logs"),
      ]);
      if (!procRes.ok || !jobsRes.ok || !logsRes.ok) {
        throw new Error("Failed to fetch data from server");
      }
      const procData = await procRes.json();
      const jobsData = await jobsRes.json();
      const logsData = await logsRes.text();

      setProcessors(procData);
      setJobs(jobsData);
      setLogs(logsData);
      setLoading(false);
      setError(null);
    } catch (err) {
      setError(err.message);
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchData();
    const interval = setInterval(fetchData, 5000);
    return () => clearInterval(interval);
  }, []);

  const getJobsForProcessor = (procId) =>
    jobs.filter((job) => job.processorId === procId);

  return (
    <div className="app-container">
      <header>
        <h1>Distributed System Monitor - MPI & OS Concepts</h1>
        <p>
          This project demonstrates <strong>MPI-based load balancing</strong>,{" "}
          <strong>fault tolerance</strong>, and <strong>job scheduling</strong>{" "}
          in a distributed system, mimicking Operating System concepts.
        </p>
      </header>

      {loading && <div className="info-message">Loading data...</div>}
      {error && <div className="error-message">Error: {error}</div>}

      {!loading && !error && (
        <>
          <section className="processors-section">
            <h2>Processors Status</h2>
            {processors.length === 0 && (
              <p>
                No processor data available. Check if your MPI system is running
                and logs are generated.
              </p>
            )}
            <div className="processors-list">
              {processors.map(({ id, alive, jobs }) => (
                <div
                  key={id}
                  className={`processor-card ${alive ? "alive" : "dead"}`}
                  title={`Processor ${id} is ${alive ? "alive" : "dead"}`}
                >
                  <h3>Processor #{id}</h3>
                  <p>
                    Status: <strong>{alive ? "Alive" : "Failed"}</strong>
                  </p>
                  <p>
                    Assigned Jobs:{" "}
                    {jobs.length > 0
                      ? jobs.join(", ")
                      : "No jobs assigned currently"}
                  </p>
                  {jobs.length > 0 && (
                    <ul>
                      {getJobsForProcessor(id).map((job) => (
                        <li key={job.id}>
                          Job #{job.id} (Strategy: {job.strategy}, Status:{" "}
                          {job.status})
                        </li>
                      ))}
                    </ul>
                  )}
                </div>
              ))}
            </div>
          </section>

          <section className="jobs-section">
            <h2>All Jobs Overview</h2>
            {jobs.length === 0 && <p>No jobs assigned yet.</p>}
            <table className="jobs-table">
              <thead>
                <tr>
                  <th>Job ID</th>
                  <th>Assigned Processor</th>
                  <th>Strategy Used</th>
                  <th>Status</th>
                </tr>
              </thead>
              <tbody>
                {jobs.map(({ id, processorId, strategy, status }) => (
                  <tr key={id}>
                    <td>{id}</td>
                    <td>{processorId}</td>
                    <td>{strategy}</td>
                    <td>{status}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </section>

          <section className="logs-section">
            <h2>Raw Logs (Last Lines)</h2>
            <pre>{logs || "No logs available"}</pre>
          </section>

          <footer>
            <p>
              Project by <strong>Your Name</strong> | Concepts: MPI, Load
              Balancing, Fault Tolerance, Distributed Systems, Operating Systems
            </p>
          </footer>
        </>
      )}
    </div>
  );
}

export default App;
