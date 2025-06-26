// index.js
const express = require('express');
const http = require('http');
const { Server } = require('socket.io');
const mqtt = require('mqtt');

const app = express();
const server = http.createServer(app);
const io = new Server(server);

// MQTT connection options
const mqttUrl = 'mqtts://38f07a1ee3754972a26af0f040402fde.s1.eu.hivemq.cloud:8883';
const mqttOptions = {
  username: 'Paradox', // <-- Set your HiveMQ Cloud username
  password: 'Paradox1', // <-- Set your HiveMQ Cloud password
};
const mqttTopic = 'mrhasan/heart';

// Serve static files (for frontend)
app.use(express.static('public'));

// MQTT client setup
const mqttClient = mqtt.connect(mqttUrl, mqttOptions);

mqttClient.on('connect', () => {
  console.log('Connected to HiveMQ Cloud MQTT broker');
  mqttClient.subscribe(mqttTopic, (err) => {
    if (err) {
      console.error('MQTT subscribe error:', err);
    } else {
      console.log('Subscribed to topic:', mqttTopic);
    }
  });
});

mqttClient.on('message', (topic, message) => {
  // Forward MQTT message to all connected web clients via Socket.IO
  try {
    const data = JSON.parse(message.toString());
    io.emit('mqtt-data', data);
  } catch (e) {
    console.error('Invalid MQTT message:', message.toString());
  }
});

mqttClient.on('error', (err) => {
  console.error('MQTT error:', err);
});

// Web page route
app.get('/', (req, res) => {
  res.sendFile(__dirname + '/public/index.html');
});

// Socket.IO connection
io.on('connection', (socket) => {
  console.log('Web client connected');
  socket.on('disconnect', () => {
    console.log('Web client disconnected');
  });
});

const PORT = process.env.PORT || 3000;
server.listen(PORT, () => {
  console.log(`Web app listening at http://localhost:${PORT}`);
});
