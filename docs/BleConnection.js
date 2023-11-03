// BleConnection.js
class BleConnection {
    constructor(serviceUUID, characteristicUUID) {
        this.bleDevice = null;
        this.bleServer = null;
        this.timeService = null;
        this.timeCharacteristic = null;
        this.serviceUUID = serviceUUID;
        this.characteristicUUID = characteristicUUID;
    }

    async connect() {
        try {
            this.bleDevice = await navigator.bluetooth.requestDevice({
                filters: [{ services: [this.serviceUUID] }]
            });
            this.bleServer = await this.bleDevice.gatt.connect();
            this.timeService = await this.bleServer.getPrimaryService(this.serviceUUID);
            this.timeCharacteristic = await this.timeService.getCharacteristic(this.characteristicUUID);
        } catch (error) {
            console.error("Error connecting to BLE device:", error);
        }
    }

    async sendCurrentTime() {
        if (!this.timeCharacteristic) {
            await this.connect();
        }
        const now = new Date();
        const currentTime = `${now.getHours()}:${now.getMinutes()}:${now.getSeconds()}`;
        const encoder = new TextEncoder('utf-8');
        const data = encoder.encode(currentTime);
        try {
            await this.timeCharacteristic.writeValue(data);
            console.log(`Sent time: ${currentTime}`);
        } catch (error) {
            console.error("Error sending time:", error);
        }
    }

    async sendData(value) {
        if (!this.timeCharacteristic) {
            await this.connect();
        }
        try {
            await this.timeCharacteristic.writeValue(Uint8Array.of(value));
            console.log(`${value} send ok.`);
        } catch (error) {
            console.error(error);
        }
    }
}
