const SERVICE_NAME = "XIAO_ESP32BLE";
const SERVICE_UUID = "199a9fa8-94f8-46bc-8228-ce67c9e807e6";
const CHARACTERISTIC_UUID = "208c149e-8266-4686-8918-981e90546c2a";

var g_device = null;
var g_characteristic = null;
var g_value = 65;

_onload = function(){
    var info=document.getElementById('iInfo');
    ckFnc=async function(){
        info.innerHTML+="start<br>";
        if((g_device!=null)&&(g_device.gatt.connected)){
            info.innerHTML+="disconnect<br>";
            console.log('Disconnecting from Bluetooth Device...');
            g_device.gatt.disconnect();
            document.getElementById('iCheckBtn').value="check";
            document.getElementById('iSendBtn').value="---";
            g_device=null;
            g_characteristic=null;
            return;
        }
        navigator.bluetooth.requestDevice({
            filters: [{ name: SERVICE_NAME }],
            optionalServices: [SERVICE_UUID]
            //acceptAllDevices: false,
        })
        .then(device => {
            info.innerHTML+="navigator.bluetooth.requestDevice.then<br>";
            g_device = device;
            device.gatt.connect()
            .then(server => {
                info.innerHTML+="device.gatt.connect.then<br>";
                console.log(server);
                server.getPrimaryService(SERVICE_UUID)
                .then(service => {
                    info.innerHTML+="server.getPrimaryService(SERVICE_UUID).then<br>";
                    console.log(service);
                    document.getElementById('iCheckBtn').value="connected";
                    document.getElementById('iSendBtn').value="send";
                    service.getCharacteristic(CHARACTERISTIC_UUID)
                    .then(characteristic => {
                        info.innerHTML+="service.getCharacteristic(CHARACTERISTIC_UUID).then<br>";
                        console.log(characteristic);
                        g_characteristic = characteristic;
                        onRead();
                    }).catch(error => { console.error(error); })
                }).catch(error => { console.error(error); })
            }).catch(error => { console.error(error); });
        }).catch(error => { console.error(error); })
    }
}

onSend=function(){
    var info=document.getElementById('iInfo');
    info.innerHTML+="onSend<br>";
    if(g_characteristic!=null){
        info.innerHTML+="g_characteristic!=null<br>";
        console.log('send');
        const asciiA = 'A'.charCodeAt(0);
        g_value = (g_value==asciiA?0:asciiA); // 'A'なら点灯
        g_characteristic.writeValue(Uint8Array.of(g_value))
        .then(_ => {
            info.innerHTML+="g_characteristic.writeValue(Uint8Array.of(g_value)<br>";
            console.log(`${g_value} send ok.`);
        }).catch(error => { console.error(error); });
    }
}

onSendB = function() {
    var info = document.getElementById('iInfo');
    info.innerHTML += "onSendB<br>";
    if (g_characteristic != null) {
        info.innerHTML += "g_characteristic != null for B<br>";
        console.log('send B');
        const asciiB = 'B'.charCodeAt(0);
        g_characteristic.writeValue(Uint8Array.of(asciiB))
        .then(_ => {
            info.innerHTML += "g_characteristic.writeValue(Uint8Array.of(asciiB))<br>";
            console.log('B send ok.');
        }).catch(error => { console.error(error); });
    }
}

onRead=function(){
    var info=document.getElementById('iInfo');
    info.innerHTML+="onRead<br>";
    if(g_characteristic!=null){
        g_characteristic.readValue()
        .then(value => {
            info.innerHTML+="g_characteristic.readValue().then<br>";
            console.log(`value= ${value.getUint8(0)}`);
        }).catch(error => { console.error(error); })
    }
}
onClear=function(){
    var info=document.getElementById('iInfo');
    info.innerHTML="---";
}
