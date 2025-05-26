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

      const [procData, jobsData, logsData] = await Promise.all([
        procRes.json(),
        jobsRes.json(),
        logsRes.text(),
      ]);

      setProcessors(procData);
      setJobs(jobsData);
      setLogs(logsData);
      setLoading(false);
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
        <h1>Distributed System Monitor</h1>
        <p>
          Real-time monitoring of <strong>MPI-based processors</strong> using{" "}
          <strong>OS concepts</strong> like fault tolerance and load balancing.
        </p>
      </header>

      {loading && <p className="info-message">Loading...</p>}
      {error && <p className="error-message">Error: {error}</p>}

      {!loading && !error && (
        <>
          <section className="processors-section">
            <h2>Processor Status</h2>
            {processors.length === 0 ? (
              <p>No processor data found.</p>
            ) : (
              <div className="processors-list">
                {processors.map(({ id, alive, jobs }) => (
                  <div
                    key={id}
                    className={`processor-card ${alive ? "alive" : "dead"}`}
                  >
                    <h3>Processor #{id}</h3>
                    <p>
                      Status: <strong>{alive ? "Alive" : "Dead"}</strong>
                    </p>
                    <p>Jobs: {jobs.length ? jobs.join(", ") : "None"}</p>
                    {jobs.length > 0 && (
                      <ul>
                        {getJobsForProcessor(id).map((job) => (
                          <li key={job.id}>
                            Job #{job.id} — Strategy: {job.strategy || "N/A"} —
                            Status: {job.status}
                          </li>
                        ))}
                      </ul>
                    )}
                  </div>
                ))}
              </div>
            )}
          </section>

          <section className="jobs-section">
            <h2>Job Assignments</h2>
            {jobs.length === 0 ? (
              <p>No jobs found.</p>
            ) : (
              <table className="jobs-table">
                <thead>
                  <tr>
                    <th>Job ID</th>
                    <th>Processor</th>
                    <th>Strategy</th>
                    <th>Status</th>
                  </tr>
                </thead>
                <tbody>
                  {jobs.map(({ id, processorId, strategy, status }) => (
                    <tr key={id}>
                      <td>{id}</td>
                      <td>{processorId}</td>
                      <td>{strategy || "N/A"}</td>
                      <td>{status}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            )}
          </section>

          <section className="logs-section">
            <h2>Raw Logs</h2>
            <pre>{logs || "No logs available"}</pre>
          </section>

          <footer>
            <p>
              Developed by <strong>Your Name</strong> — MPI, Fault Tolerance,
              Load Balancing, Distributed OS Concepts.
            </p>
          </footer>
        </>
      )}
    </div>
  );
}

export default App;
