// BleConnection.js
class BleConnection {
    constructor(deviceName, serviceUUID, characteristicUUID) {
        this.deviceName = deviceName; // デバイス名を追加
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
                filters: [{ name: this.deviceName }], // ここをデバイス名でのフィルタリングに変更
                optionalServices: [this.serviceUUID] // サービスUUIDはoptionalServicesに追加
            });
            this.bleServer = await this.bleDevice.gatt.connect();
            this.timeService = await this.bleServer.getPrimaryService(this.serviceUUID);
            this.timeCharacteristic = await this.timeService.getCharacteristic(this.characteristicUUID);
        } catch (error) {
            console.error("Error connecting to BLE device:", error);
        }
    }

    //データを識別するためのメソッド
    createCommandData(commandType, payload) {
        return commandType + ":" + payload;
    }

    // 現在時刻を送信する
    async sendCurrentTime() {
        const now = new Date();
        const currentTime = `${now.getHours()}:${now.getMinutes()}:${now.getSeconds()}`;
        await this.sendData('TIME', currentTime);
    }

    //ミーティング時間を送信する
    async sendMeetingTime(meetingTime) {
        await this.sendData('MEETING', meetingTime);
    }

    //データを送信するためのフォーマット
    async sendData(commandType, payload = '') {
        if (!this.timeCharacteristic) {
            await this.connect();
        }
        const data = this.createCommandData(commandType, payload);
        const encoder = new TextEncoder('utf-8');
        const encodedData = encoder.encode(data);
        try {
            await this.timeCharacteristic.writeValue(Uint8Array.from(encodedData));
            console.log(`Sent command: ${commandType}, payload: ${payload}`);
        } catch (error) {
            console.error(error);
        }
    }

    //データを読み込むためのメソッド
    async onRead() {
        if (!this.timeCharacteristic) {
            await this.connect();
        }
        try {
            const value = await this.timeCharacteristic.readValue();
            console.log(`value= ${value.getUint8(0)}`);
        } catch (error) {
            console.error("Error reading value:", error);
        }
    }
}
