require('dotenv').config();
const express = require('express');
const http = require('http');
const mongoose = require('mongoose');
const cors = require('cors');
const { Server } = require('socket.io');

const app = express();
const server = http.createServer(app);

// Enable CORS for frontend
const io = new Server(server, {
    cors: {
        origin: "*", // allow all in dev
        methods: ["GET", "POST"]
    }
});

app.use(cors());
app.use(express.json());

// MongoDB Connection
const MONGO_URI = process.env.MONGO_URI || "mongodb://127.0.0.1:27017/sepsis_guard";
mongoose.connect(MONGO_URI)
    .then(() => console.log('MongoDB Connected'))
    .catch(err => console.error('MongoDB error:', err));

// Socket.io Connection
io.on('connection', (socket) => {
    console.log(`Client connected: ${socket.id}`);

    // Optional: Frontend can join specific patient rooms
    socket.on('join-room', (patientId) => {
        socket.join(patientId);
        console.log(`Socket ${socket.id} joined room ${patientId}`);
    });

    socket.on('disconnect', () => {
        console.log(`Client disconnected: ${socket.id}`);
    });
});

// Import API routes passing the io instance
const apiRoutes = require('./routes/api')(io);
app.use('/api', apiRoutes);

const PORT = process.env.PORT || 5000;
server.listen(PORT, () => console.log(`Backend server running on port ${PORT}`));
