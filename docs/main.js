const SERVICE_UUID = "199a9fa8-94f8-46bc-8228-ce67c9e807e6";
const CHARACTERISTIC_UUID = "208c149e-8266-4686-8918-981e90546c2a";
const DEVICE_NAME = "XIAO_ESP32BLE";  // デバイス名を設定してください

//　BLEクラスをインスタンス化
let bleConnection = new BleConnection(DEVICE_NAME, SERVICE_UUID, CHARACTERISTIC_UUID);

// onBPMReceived メソッドをオーバーライドして、ページに BPM 値を表示する
bleConnection.onBPMReceived = function (bpmValue) {
    document.getElementById('bpmDisplay').textContent = `${bpmValue}`;
    if (isRecording) {
        const currentTime = new Date().toLocaleTimeString(); // 現在の時刻
        bpmData.push({ time: currentTime, bpm: bpmValue });
    }
};

bleConnection.onBpmUpdate = function(bpmValue) {
    updateBpm(bpmValue);
};

//接続のためのボタン
document.getElementById('connectBtn').addEventListener('click', async () => {
    await bleConnection.connect();
    document.getElementById('iInfo').textContent = "接続成功";
});

// 時間を送信（sendCurrentTimeを使用）
document.getElementById('sendTimeBtn').addEventListener('click', async () => {
    await bleConnection.sendCurrentTime();
    document.getElementById('iInfo').innerHTML += `Sent time: ${new Date().toLocaleTimeString()}<br>`;
});

//meetings時間を送信
document.getElementById('meetingTimes').addEventListener('DOMSubtreeModified', async () => {
    const meetingTimeElem = document.getElementById('meetingTimes');
    if (meetingTimeElem.innerText.includes('Meeting Times:')) {
        const meetingTime = meetingTimeElem.innerText.split('\n')[1]; // 2行目に時刻があると仮定
        if (meetingTime) {
            await bleConnection.sendMeetingTime(meetingTime);
            document.getElementById('iInfo').innerHTML += `Sent meeting time: ${meetingTime}<br>`;
        }
    }
});

// データ 'A' の送信ボタン
document.getElementById('sendABtn').addEventListener('click', async () => {
    await bleConnection.sendData('CMD', 'A');
    document.getElementById('iInfo').innerHTML += "Sent command: A<br>";
});

// データ 'B' の送信ボタン
document.getElementById('sendBBtn').addEventListener('click', async () => {
    await bleConnection.sendData('CMD', 'B');
    document.getElementById('iInfo').innerHTML += "Sent command: B<br>";
});

// iInfoの内容をクリアするボタン
document.getElementById('clearBtn').addEventListener('click', () => {
    document.getElementById('iInfo').innerHTML = "---";
});