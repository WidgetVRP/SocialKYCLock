const char* html_content = R"RAW_STRING(
<html>
<head>
  <title>RFID Reader</title>
  <script>
    var socket = new WebSocket('ws://192.168.1.60:81');  // WebSocket bağlantısını açın

    socket.onmessage = function(event) {
      var data = JSON.parse(event.data);
      document.getElementById('rfidText').value = data.uid;  // UID'yi metin kutusuna ayarlayın
    };
  </script>
</head>
<body>
<input type="text" id="rfidText" readonly>
</body>
</html>
)RAW_STRING";
