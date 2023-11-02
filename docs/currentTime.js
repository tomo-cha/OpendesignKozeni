let bleDevice = null;
let bleServer = null;
let timeService = null;
let timeCharacteristic = null;

const serviceUUID = "199a9fa8-94f8-46bc-8228-ce67c9e807e6"; // あなたのサービスのUUID
const characteristicUUID = "208c149e-8266-4686-8918-981e90546c2a"; // あなたのキャラクタリスティックのUUID

async function connectBLE() {
    try {
        bleDevice = await navigator.bluetooth.requestDevice({
            filters: [{services: [serviceUUID]}]
        });
        bleServer = await bleDevice.gatt.connect();
        timeService = await bleServer.getPrimaryService(serviceUUID);
        timeCharacteristic = await timeService.getCharacteristic(characteristicUUID);
    } catch (error) {
        console.error("Error connecting to BLE device:", error);
    }
}

async function sendCurrentTime() {
    if (!timeCharacteristic) {
        await connectBLE();
    }
    const now = new Date();
    const currentTime = `${now.getHours()}:${now.getMinutes()}:${now.getSeconds()}`;
    const encoder = new TextEncoder('utf-8');
    const data = encoder.encode(currentTime);
    try {
        await timeCharacteristic.writeValue(data);
        console.log(`Sent time: ${currentTime}`);
    } catch (error) {
        console.error("Error sending time:", error);
    }
}
