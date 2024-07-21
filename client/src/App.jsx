import React, { useState, useEffect } from 'react';
import './App.css';

function App() {
  const [message, setMessage] = useState('');
  const [prediction, setPrediction] = useState('');
  const [usedBytes, setUsedBytes] = useState(0);
  const [uptime, setUptime] = useState(0);
  const host = '192.168.50.23'
  const mjpegUrl = `http://${host}/mjpeg`;
  const wsUrl = `ws://${host}/ws`;

  useEffect(() => {
    const ws = new WebSocket(wsUrl);

    ws.onmessage = (event) => {
      const data = JSON.parse(event.data);
      if (data.responseType === 'prediction') {
        setPrediction(data.topPredictionClassName);
      }
      if (data.responseType === 'sdInfo') {
        setUsedBytes(data.usedBytes);
      }
      if (data.responseType === 'systemInfo') {
        setUptime(data.uptimeMs);

      }
      setMessage(event.data); // Assuming the message is in a property called 'message'
    };
    const intervalId = setInterval(() => {
      if (ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({ requestType: 'sdInfo' }));
        ws.send(JSON.stringify({ requestType: 'systemInfo' }));
      }
    }, 10000); // Send message every 10 seconds

    return () => {
      clearInterval(intervalId);
      ws.close();
    };
  }, []);

  return (
    <div className="App">
      <header className="App-header">
        <img src={mjpegUrl} alt="MJPEG Stream" />
        <p>prediction: {prediction}</p>
        <p>usedBytes: {usedBytes}</p>
        <p>Uptime: {uptime}</p>
      </header>
    </div>
  );
}

export default App;