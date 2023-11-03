// main.js
const SERVICE_UUID = "199a9fa8-94f8-46bc-8228-ce67c9e807e6";
const CHARACTERISTIC_UUID = "208c149e-8266-4686-8918-981e90546c2a";

let bleConnection = new BleConnection(SERVICE_UUID, CHARACTERISTIC_UUID);

document.getElementById('sendTimeBtn').addEventListener('click', () => {
    bleConnection.sendCurrentTime();
});

document.getElementById('sendABtn').addEventListener('click', () => {
    const asciiA = 'A'.charCodeAt(0);
    bleConnection.sendData(asciiA);
});

document.getElementById('sendBBtn').addEventListener('click', () => {
    const asciiB = 'B'.charCodeAt(0);
    bleConnection.sendData(asciiB);
});
