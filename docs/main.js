const SERVICE_UUID = "199a9fa8-94f8-46bc-8228-ce67c9e807e6";
const CHARACTERISTIC_UUID = "208c149e-8266-4686-8918-981e90546c2a";
const DEVICE_NAME = "XIAO_ESP32BLE";  // デバイス名を設定してください

//　BLEクラスをインスタンス化
let bleConnection = new BleConnection(DEVICE_NAME, SERVICE_UUID, CHARACTERISTIC_UUID);

//　接続のためのボタン
document.getElementById('connectBtn').addEventListener('click', async () => {
    await bleConnection.connect();
    document.getElementById('iInfo').innerHTML += "接続成功<br>";
});

// 時間を送信（sendCurrentTimeを使用）
document.getElementById('sendTimeBtn').addEventListener('click', async () => {
    await bleConnection.sendCurrentTime();
    document.getElementById('iInfo').innerHTML += `Sent time: ${new Date().toLocaleTimeString()}<br>`;
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

// document.addEventListener('DOMContentLoaded', function() {
//     document.getElementById('authorize_button').addEventListener('click', handleAuthClick);
//     document.getElementById('signout_button').addEventListener('click', handleSignoutClick);
//     document.getElementById('sendMeetingTimeBtn').addEventListener('click', async () => {
//         const meetingTime = await getNextMeetingTime();
//         if (meetingTime) {
//             await bleConnection.sendData('MEETING', meetingTime);
//             document.getElementById('info').innerText = `Sent meeting time: ${meetingTime}`;
//         } else {
//             document.getElementById('info').innerText = 'No upcoming meetings found.';
//         }
//     });
//     document.getElementById('displayMeetingTimesButton').addEventListener('click', displayMeetingTimes);
// });

// //ここでAPIの処理を記述
// const CLIENT_ID = '593980784395-mcopk6oi9gc419ognfb8nulkm58j8838.apps.googleusercontent.com';
// const API_KEY = 'AIzaSyBulbUZ6M_i2u2R_vuCcDGEDb5Pf48SsFQ';
// const DISCOVERY_DOC = 'https://www.googleapis.com/discovery/v1/apis/calendar/v3/rest';
// const SCOPES = 'https://www.googleapis.com/auth/calendar.readonly';

// let tokenClient;
// let gapiInited = false;
// let gisInited = false;

// function gapiLoaded() {
//     gapi.load('client', initializeGapiClient);
// }

// async function initializeGapiClient() {
//     await gapi.client.init({
//         apiKey: API_KEY,
//         clientId: CLIENT_ID,
//         discoveryDocs: [DISCOVERY_DOC],
//         scope: SCOPES
//     });
//     gapiInited = true;
//     maybeEnableButtons();
// }

// function gisLoaded() {
//     tokenClient = google.accounts.oauth2.initTokenClient({
//         client_id: CLIENT_ID,
//         scope: SCOPES
//     });
//     gisInited = true;
//     maybeEnableButtons();
// }

// function maybeEnableButtons() {
//     if (gapiInited && gisInited) {
//         document.getElementById('authorize_button').style.visibility = 'visible';
//     }
// }

// function handleAuthClick() {
//     tokenClient.callback = async (resp) => {
//         if (resp.error !== undefined) {
//             throw (resp);
//         }
//         document.getElementById('signout_button').style.visibility = 'visible';
//         document.getElementById('authorize_button').innerText = 'Refresh';
//         await listUpcomingEvents();
//     };

//     if (gapi.client.getToken() === null) {
//         tokenClient.requestAccessToken({ prompt: 'consent' });
//     } else {
//         tokenClient.requestAccessToken({ prompt: '' });
//     }
// }

// function handleSignoutClick() {
//     const token = gapi.client.getToken();
//     if (token !== null) {
//         google.accounts.oauth2.revoke(token.access_token);
//         gapi.client.setToken('');
//         document.getElementById('content').innerText = '';
//         document.getElementById('authorize_button').innerText = 'Authorize';
//         document.getElementById('signout_button').style.visibility = 'hidden';
//     }
// }

// async function listUpcomingEvents() {
//     const request = {
//         'calendarId': 'primary',
//         'timeMin': (new Date()).toISOString(),
//         'showDeleted': false,
//         'singleEvents': true,
//         'maxResults': 10,
//         'orderBy': 'startTime',
//     };
//     const response = await gapi.client.calendar.events.list(request);
//     const events = response.result.items;

//     if (!events || events.length == 0) {
//         document.getElementById('content').innerText = 'No events found.';
//     } else {
//         const output = events.reduce(
//             (str, event) => `${str}${event.summary} (${event.start.dateTime || event.start.date})\n`,
//             'Events:\n');
//         document.getElementById('content').innerText = output;
//     }
// }

// function containsKeyword(summary, keyword) {
//     return summary.toLowerCase().includes(keyword.toLowerCase());
// }

// function formatDateTime(dateTimeString) {
//     const date = new Date(dateTimeString);
//     return `${date.getUTCHours().toString().padStart(2, '0')}:${date.getUTCMinutes().toString().padStart(2, '0')}:${date.getUTCSeconds().toString().padStart(2, '0')}`;
// }

// async function getNextMeetingTime() {
//     let meetingTime = null;
//     const response = await gapi.client.calendar.events.list({
//         'calendarId': 'primary',
//         'timeMin': (new Date()).toISOString(),
//         'showDeleted': false,
//         'singleEvents': true,
//         'maxResults': 10,
//         'orderBy': 'startTime',
//     });
//     const nextMeeting = response.result.items.find(event => containsKeyword(event.summary, "meeting"));

//     if (nextMeeting) {
//         meetingTime = formatDateTime(nextMeeting.start.dateTime || nextMeeting.start.date);
//     }
//     return meetingTime;
// }
