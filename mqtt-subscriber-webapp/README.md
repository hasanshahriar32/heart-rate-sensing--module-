# MQTT Subscriber Web App

This Node.js web app subscribes to your HiveMQ Cloud MQTT broker and displays real-time heart rate and signal data from your ESP8266 project.

## Features
- Connects securely to HiveMQ Cloud using MQTT over TLS
- Subscribes to the topic `mrhasan/heart`
- Displays live BPM and signal data in the browser
- Uses Express for the web server and Socket.IO for real-time updates

## Setup
1. Install dependencies:
   ```bash
   npm install
   ```
2. Update your HiveMQ Cloud username and password in `index.js` if needed.
3. Start the server:
   ```bash
   npm start
   ```
4. Open your browser at [http://localhost:3000](http://localhost:3000)

## Deployment
You can deploy this app to any Node.js hosting platform (Heroku, Render, Railway, etc.).

---

**Security Note:**
- Never commit your real HiveMQ Cloud credentials to a public repository.
- For production, consider using environment variables for secrets.
