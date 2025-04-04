const aedes = require('aedes')();
const http = require('http');
const websocketStream = require('websocket-stream');
const mqtt = require('mqtt');

// Start WebSocket server for the browser
const server = http.createServer();
websocketStream.createServer({ server }, aedes.handle);
server.listen(5500, () => {
  console.log('WebSocket MQTT server listening on http://localhost:5500');
});

// Connect to your SSL-only broker as a client
const upstreamClient = mqtt.connect('tcp://broker.mqtt.cool:1883', {
  clientId: 'bridge-client',
  // username: 'your-username',
  // password: 'your-password'
});

// When this bridge client gets a message from upstream broker...
upstreamClient.on('connect', () => {
  console.log('Connected to upstream MQTT broker');
  upstreamClient.subscribe('#');
});

upstreamClient.on('message', (topic, payload) => {
  aedes.publish({ topic, payload });
});

// Forward messages from browser to upstream
aedes.on('publish', (packet, client) => {
  if (client) {
    upstreamClient.publish(packet.topic, packet.payload);
  }
});
