const express = require('express');
const router = express.Router();
const Patient = require('../models/Patient');
const Vitals = require('../models/Vitals');

// Inject io instance from server.js
module.exports = function (io) {

    // Risk Calculation Logic
    // Risk: 0-2 Low, 3-4 Medium, 5-8 High
    const calculateRisk = (hr, spo2, tempC, ecg) => {
        let score = 0;

        if (hr > 100 || hr < 50) score += 2;
        if (spo2 < 94) score += 2;
        if (tempC > 38.0 || tempC < 36.0) score += 2;
        if (ecg == 0 || ecg > 1000) score += 2; // Basic generic abnormal range

        let category = "Low";
        if (score >= 3 && score <= 4) category = "Med";
        if (score >= 5) category = "High";

        return { score, category };
    };

    // POST /api/vitals - Receive data from ESP8266
    router.post('/vitals', async (req, res) => {
        try {
            const { patientId, heartRate, spo2, temperatureC, temperatureF, ecgValue } = req.body;

            if (!patientId) return res.status(400).json({ error: "Missing patientId" });

            const { score, category } = calculateRisk(heartRate, spo2, temperatureC, ecgValue);

            const newVitals = new Vitals({
                patientId,
                heartRate,
                spo2,
                temperatureC,
                temperatureF,
                ecgValue,
                riskScore: score,
                riskCategory: category
            });

            await newVitals.save();

            // Broadcast new data to frontend clients matching this patientId
            // Ensure frontend joins the patient 'room' or catches everything
            io.emit(`vitals-${patientId}`, newVitals);

            // Also broadcast a global dashboard event
            io.emit('dashboard-update', newVitals);

            res.status(201).json(newVitals);
        } catch (err) {
            console.error("Error saving vitals:", err);
            res.status(500).json({ error: "Internal Server Error" });
        }
    });

    // GET /api/vitals/:patientId - Get history for chart
    router.get('/vitals/:patientId', async (req, res) => {
        try {
            const { patientId } = req.params;
            const limit = parseInt(req.query.limit) || 50;

            const vitals = await Vitals.find({ patientId })
                .sort({ timestamp: -1 })
                .limit(limit);

            res.json(vitals.reverse()); // return chronological
        } catch (err) {
            res.status(500).json({ error: "Internal Server Error" });
        }
    });

    // POST /api/patients - Register a new patient
    router.post('/patients', async (req, res) => {
        try {
            const { patientId, name, age, gender, roomNumber } = req.body;

            const existing = await Patient.findOne({ patientId });
            if (existing) return res.status(400).json({ error: "Patient ID already exists" });

            const newPatient = new Patient({ patientId, name, age, gender, roomNumber });
            await newPatient.save();

            res.status(201).json(newPatient);
        } catch (err) {
            res.status(500).json({ error: "Internal Server Error" });
        }
    });

    // GET /api/patients - Get all patients
    router.get('/patients', async (req, res) => {
        try {
            const patients = await Patient.find();
            res.json(patients);
        } catch (err) {
            res.status(500).json({ error: "Internal Server Error" });
        }
    });

    // GET /api/patients/:patientId
    router.get('/patients/:patientId', async (req, res) => {
        try {
            const patient = await Patient.findOne({ patientId: req.params.patientId });
            if (!patient) return res.status(404).json({ error: "Not found" });
            res.json(patient);
        } catch (err) {
            res.status(500).json({ error: "Internal Server Error" });
        }
    });

    return router;
};
