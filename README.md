# SepsisGuard AI System

Real-Time Early Sepsis Detection System leveraging IoT Edge computing (ESP8266) with a Next.js 14 Dashboard and an Express + Socket.io Backend.

## Folder Structure
- `/esp8266`: Contains the NodeMCU Arduino firmware `sepsis_guard.ino`.
- `/backend`: Contains the Node.js Express server, MongoDB models, and Socket.io.
- `/frontend`: Contains the Next.js 14 application with TailwindCSS and shadcn-ui.

---

## 🚀 Setup Guide

### 1. Backend Setup
1. Open terminal and navigate to: `cd backend`
2. Run `npm install`
3. Make sure MongoDB is running locally on port 27017, or update the `.env` file with your Mongo URI:
   ```
   MONGO_URI=mongodb://127.0.0.1:27017/sepsis_guard
   PORT=5000
   ```
4. Start the server:
   ```bash
   node server.js
   ```
   *The backend will run on `http://localhost:5000`.*

### 2. Frontend Setup
1. Open a new terminal and navigate to: `cd frontend`
2. Run `npm install` (if not already installed)
3. Start the Next.js development server:
   ```bash
   npm run dev
   ```
4. Open your browser and go to `http://localhost:3000`.

### 3. ESP8266 Setup
1. Open `esp8266/sepsis_guard/sepsis_guard.ino` in Arduino IDE.
2. Update the configurations inside the `.ino` file:
   - `WIFI_SSID` & `WIFI_PASSWORD`
   - `SERVER_URL` (Change `192.168.1.xxx` to the Local IPv4 address of the computer running your backend).
3. Connect the MAX30102, DS18B20, AD8232, and OLED as per diagram in `esp8266/README.md`.
4. Upload to the NodeMCU.

---

## 🧪 Testing without Hardware
If you don't have the ESP8266 ready, you can still test the Dashboard and WebSocket real-time changes!
1. Start the Backend and Frontend as described above.
2. Open a third terminal in the `backend` folder.
3. Install axios: `npm install axios`
4. Run the test script: `node test_post.js`
5. Go to `http://localhost:3000/dashboard`, you will see live data streaming dynamically.

---

## ☁️ Deployment Guide

### Backend & Database (Render / Heroku / DigitalOcean)
1. Provision a MongoDB database (e.g., MongoDB Atlas).
2. Deploy the `backend` folder to a Node.js host (like Render Web Service).
3. Add Environment Variables:
   - `MONGO_URI` = your Atlas connection string.
   - `PORT` = `5000` or host provided.

### Frontend (Vercel)
1. Push your code to GitHub.
2. Link the repository to Vercel and select the `frontend` folder as the Root Directory.
3. Framework Preset: **Next.js**.
4. Important: Update the `socket.io-client` connection URL and fetch URLs in the React pages from `http://localhost:5000` to your new Deployed Backend URL.
5. Click **Deploy**.

### Edge Device (ESP8266)
1. Once deployed, update the `SERVER_URL` in `sepsis_guard.ino` to the exact endpoint, e.g., `https://my-backend.onrender.com/api/vitals`.
2. Recompile and upload to the ESP8266.
