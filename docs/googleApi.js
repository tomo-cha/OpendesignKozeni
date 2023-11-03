const CLIENT_ID = '593980784395-mcopk6oi9gc419ognfb8nulkm58j8838.apps.googleusercontent.com';
const API_KEY = 'AIzaSyBulbUZ6M_i2u2R_vuCcDGEDb5Pf48SsFQ';
const DISCOVERY_DOC = 'https://www.googleapis.com/discovery/v1/apis/calendar/v3/rest';
const SCOPES = 'https://www.googleapis.com/auth/calendar.readonly';

let tokenClient;
let gapiInited = false;
let gisInited = false;

document.getElementById('authorize_button').style.visibility = 'hidden';
document.getElementById('signout_button').style.visibility = 'hidden';

function gapiLoaded() {
    gapi.load('client', initializeGapiClient);
}

async function initializeGapiClient() {
    await gapi.client.init({
        apiKey: API_KEY,
        discoveryDocs: [DISCOVERY_DOC],
    });
    gapiInited = true;
    maybeEnableButtons();
}

function gisLoaded() {
    tokenClient = google.accounts.oauth2.initTokenClient({
        client_id: CLIENT_ID,
        scope: SCOPES,
        callback: '',
    });
    gisInited = true;
    maybeEnableButtons();
}

function maybeEnableButtons() {
    if (gapiInited && gisInited) {
        document.getElementById('authorize_button').style.visibility = 'visible';
    }
}

function handleAuthClick() {
    tokenClient.callback = async (resp) => {
        if (resp.error !== undefined) {
            throw (resp);
        }
        document.getElementById('signout_button').style.visibility = 'visible';
        document.getElementById('authorize_button').innerText = 'Refresh';
        await listUpcomingEvents();
    };

    if (gapi.client.getToken() === null) {
        tokenClient.requestAccessToken({ prompt: 'consent' });
    } else {
        tokenClient.requestAccessToken({ prompt: '' });
    }
}

function handleSignoutClick() {
    const token = gapi.client.getToken();
    if (token !== null) {
        google.accounts.oauth2.revoke(token.access_token);
        gapi.client.setToken('');
        document.getElementById('content').innerText = '';
        document.getElementById('authorize_button').innerText = 'Authorize';
        document.getElementById('signout_button').style.visibility = 'hidden';
    }
}

async function listUpcomingEvents() {
    let response;
    try {
        const request = {
            'calendarId': 'primary',
            'timeMin': (new Date()).toISOString(),
            'showDeleted': false,
            'singleEvents': true,
            'maxResults': 10,
            'orderBy': 'startTime',
        };
        response = await gapi.client.calendar.events.list(request);
    } catch (err) {
        document.getElementById('content').innerText = err.message;
        return;
    }

    const events = response.result.items;
    if (!events || events.length == 0) {
        document.getElementById('content').innerText = 'No events found.';
        return;
    }

    const output = events.reduce(
        (str, event) => `${str}${event.summary} (${event.start.dateTime || event.start.date})\n`,
        'Events:\n');
    document.getElementById('content').innerText = output;
}

function containsKeyword(summary, keyword) {
    return summary.toLowerCase().includes(keyword.toLowerCase());
}

function formatDateTime(dateTimeString) {
    const date = new Date(dateTimeString);
    const hours = date.getUTCHours().toString().padStart(2, '0');
    const minutes = date.getUTCMinutes().toString().padStart(2, '0');
    const seconds = date.getUTCSeconds().toString().padStart(2, '0');
    return `${hours}:${minutes}:${seconds}`;
}

function displayMeetingTimes() {
    console.log("displayMeetingTimes was called");
    gapi.client.calendar.events.list({
        'calendarId': 'primary',
        'timeMin': (new Date()).toISOString(),
        'showDeleted': false,
        'singleEvents': true,
        'maxResults': 10,
        'orderBy': 'startTime',
    }).then(response => {
        const events = response.result.items;
        const meetingsOutput = events
            .filter(event => containsKeyword(event.summary, "meeting"))
            .reduce((str, event) => {
                const formattedTime = formatDateTime(event.start.dateTime || event.start.date);
                return `${str}${formattedTime}\n`;
            }, 'Meeting Times:\n');
        document.getElementById('meetingTimes').innerText = meetingsOutput;
    });
}

document.getElementById('displayMeetingTimesButton').addEventListener('click', displayMeetingTimes);
