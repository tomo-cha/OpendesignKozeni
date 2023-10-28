
function init() {
    gapi.client.init({
        'apiKey': 'AIzaSyBulbUZ6M_i2u2R_vuCcDGEDb5Pf48SsFQ',
        'clientId': '593980784395-mcopk6oi9gc419ognfb8nulkm58j8838.apps.googleusercontent.com',
        'scope': 'https://www.googleapis.com/auth/calendar',
        'discoveryDocs': ['https://www.googleapis.com/discovery/v1/apis/calendar/v3/rest']
    }).then(function () {
        // APIの初期化が完了した後の処理をここに書きます。
    });
}

// APIクライアントとOAuth2ライブラリのロード
function loadClient() {
    gapi.client.setApiKey("YOUR_API_KEY");
    return gapi.client.load("https://content.googleapis.com/discovery/v1/apis/calendar/v3/rest")
        .then(function() { console.log("GAPI client loaded for API"); },
              function(err) { console.error("Error loading GAPI client for API", err); });
}

function handleAuthClick() {
    gapi.auth2.getAuthInstance().signIn().then(function() {
        console.log("User signed in.");
    });
}

function handleSignoutClick() {
    gapi.auth2.getAuthInstance().signOut().then(function() {
        console.log("User signed out.");
    });
}
