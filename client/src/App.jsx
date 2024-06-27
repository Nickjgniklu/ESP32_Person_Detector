import React, { useState, useEffect } from 'react';
import './App.css';

function App() {
  const [message, setMessage] = useState('');
  const host = '192.168.50.21'
  const mjpegUrl = `http://${host}/mjpeg`;
  const wsUrl = `ws://${host}/ws`;

  useEffect(() => {
    const ws = new WebSocket(wsUrl);

    ws.onmessage = (event) => {
      //const data = JSON.parse(event.data);
      setMessage(event.data); // Assuming the message is in a property called 'message'
    };

    return () => {
      ws.close();
    };
  }, []);

  return (
    <div className="App">
      <header className="App-header">
        <img src={mjpegUrl} alt="MJPEG Stream" />
        <p>Message from WebSocket: {message}</p>
      </header>
    </div>
  );
}

export default App;