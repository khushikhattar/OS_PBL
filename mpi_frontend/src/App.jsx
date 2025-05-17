import { useEffect, useState } from "react";
import "./App.css";

function App() {
  const [logs, setLogs] = useState("");

  useEffect(() => {
    const fetchLogs = () => {
      fetch("http://localhost:4000/logs")
        .then((res) => res.text())
        .then(setLogs)
        .catch(console.error);
    };

    fetchLogs();
    const interval = setInterval(fetchLogs, 2000); // Refresh every 2s
    return () => clearInterval(interval);
  }, []);

  return (
    <div className="container">
      <h1>MPI Project Live Logs</h1>
      <pre className="log-box">{logs}</pre>
    </div>
  );
}

export default App;
