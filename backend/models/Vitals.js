const mongoose = require('mongoose');

const VitalsSchema = new mongoose.Schema({
    patientId: {
        type: String,
        required: true,
        index: true
    },
    heartRate: {
        type: Number,
        required: true,
    },
    spo2: {
        type: Number,
        required: true,
    },
    temperatureC: {
        type: Number,
        required: true,
    },
    temperatureF: {
        type: Number,
        required: true,
    },
    ecgValue: {
        type: Number,
        required: true,
    },
    riskScore: {
        type: Number,
        required: true,
    },
    riskCategory: {
        type: String,
        enum: ['Low', 'Med', 'High'],
        required: true,
    },
    timestamp: {
        type: Date,
        default: Date.now,
    }
});

module.exports = mongoose.model('Vitals', VitalsSchema);
