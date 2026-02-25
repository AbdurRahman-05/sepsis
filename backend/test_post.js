const axios = require('axios');

const testPost = async () => {
    const payload = {
        patientId: "patient_12345",
        heartRate: Math.floor(Math.random() * (120 - 60) + 60),
        spo2: Math.floor(Math.random() * (100 - 85) + 85),
        temperatureC: (Math.random() * (39 - 36) + 36),
        temperatureF: 0,
        ecgValue: Math.floor(Math.random() * 800)
    };
    payload.temperatureF = payload.temperatureC * 9 / 5 + 32;

    try {
        const res = await axios.post('http://localhost:5000/api/vitals', payload);
        console.log("Mock Vitals Sent:", res.data);
    } catch (e) {
        console.error("Error sending mock data", e.response?.data || e.message);
    }
};

console.log("Starting mock data pump every 2 seconds...");
setInterval(testPost, 2000);
