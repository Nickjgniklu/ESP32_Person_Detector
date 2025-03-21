import React, { useState, useEffect } from 'react';
import './App.css';

function App() {
  const [message, setMessage] = useState('');
  const [prediction, setPrediction] = useState('');
  const [probability, setProbability] = useState('');
  const [usedBytes, setUsedBytes] = useState(0);
  const [uptime, setUptime] = useState(0);
  const esp32Ip = '192.168.50.21'
  const host = import.meta.env.MODE === 'development' ? esp32Ip : '';

  let mjpegUrl = `${host}/mjpeg`;
  let wsUrl = `${host}/ws`;
  if (import.meta.env.MODE === 'development') {
    mjpegUrl = `http://${mjpegUrl}`
    wsUrl = `ws://${wsUrl}`;

  }

  useEffect(() => {
    let ws;
    let reconnectInterval;

    const connectWebSocket = () => {
      ws = new WebSocket(wsUrl);

      ws.onopen = () => {
        console.log('WebSocket connected');
        if (reconnectInterval) {
          clearInterval(reconnectInterval);
          reconnectInterval = null;
        }
      };

      ws.onmessage = (event) => {
        let data;
        try {
          data = JSON.parse(event.data);
        } catch (e) {
          console.log(event.data);
          return;
        }

        if (data.responseType === 'prediction') {
          setPrediction(data.prediction);
          setProbability(data.probability);
        }
        if (data.responseType === 'sdInfo') {
          setUsedBytes(data.usedBytes);
        }
        if (data.responseType === 'systemInfo') {
          setUptime(data.uptimeMs);
        }
        setMessage(event.data); // Assuming the message is in a property called 'message'
      };

      ws.onerror = (error) => {
        console.error('WebSocket error:', error);
        ws.close();
      };

      ws.onclose = () => {
        console.log('WebSocket closed, attempting to reconnect...');
        if (!reconnectInterval) {
          reconnectInterval = setInterval(connectWebSocket, 5000); // Attempt to reconnect every 5 seconds
        }
      };
    };

    connectWebSocket();

    const intervalId = setInterval(() => {
      if (ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({ requestType: 'sdInfo' }));
        ws.send(JSON.stringify({ requestType: 'systemInfo' }));
      }
    }, 10000); // Send message every 10 seconds

    return () => {
      clearInterval(intervalId);
      if (reconnectInterval) {
        clearInterval(reconnectInterval);
      }
      ws.close();
    };
  }, []);

  return (
    <div className="App">
      <header className="App-header">
        <img src={mjpegUrl} alt="MJPEG Stream" />
        <p>prediction: {prediction}</p>
        <p>probability: {probability}</p>
        <p>usedBytes: {usedBytes}</p>
        <p>Uptime: {uptime}</p>
      </header>
    </div>
  );
}

export default App;