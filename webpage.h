String webpage = "\
<html>\n\
<head>\n\
  <script>\n\
  var ws;\n\
  var connected = false;\n\
  var rgb_array;\n\
  var fa;\n\
  function hex02(c) {\n\
    s = c.toString(16); \n\
    if (s.length == 1) { s = '0' + s; }\n\
    return s; \n\
  }\n\
  function send() {\n\
    let s = '';\n\
    if (document.getElementById(\"ledon\").checked) {\n\
      s += '1';\n\
    } else {\n\
      s += '0';\n\
    }\n\
    s += '#';\n\
    s += hex02(parseInt(document.getElementById(\"r\").value).toString(16));\n\
    s += hex02(parseInt(document.getElementById(\"g\").value).toString(16));\n\
    s += hex02(parseInt(document.getElementById(\"b\").value).toString(16));\n\
    ws.send(s);\n\
  }\n\
  function doConnect(addr) {\n\
    var msg;\n\
    ws = new WebSocket(addr);\n\
    ws.binaryType = 'arraybuffer';\n\
    ws.onopen = function()\n\
    {\n\
      connected = true;\n\
      document.getElementById(\"status\").innerHTML = \"Connected\";\n\
    };\n\
    ws.onmessage = function (evt)\n\
    {\n\
      rgb_array = new Uint8Array(evt.data.slice(0,4));\n\
      let s = '#' + hex02(rgb_array[0]) + hex02(rgb_array[1]) + hex02(rgb_array[2]);\n\
      document.getElementById(\"RGB\").innerHTML = s;\n\
      document.getElementById(\"LED\").innerHTML = rgb_array[3] == 1 ? 'ON' : 'OFF';\n\
      fa = new Float32Array(evt.data.slice(4));\n\
      document.getElementById(\"accel\").innerHTML = fa[0].toFixed(3) + ',' + fa[1].toFixed(3) + ',' + fa[2].toFixed(3);\n\
      document.getElementById(\"gyro\").innerHTML = fa[3].toFixed(3) + ',' + fa[4].toFixed(3) + ',' + fa[5].toFixed(3);\n\
      document.getElementById(\"temp\").innerHTML = fa[6].toFixed(3);\n\
    };\n\
    ws.onclose = function(event)\n\
    {\n\
      document.getElementById(\"btConn\").value = \"Connect!\";\n\
      document.getElementById(\"taLog\").value +=\n\
        (\"Connection closed: wasClean: \" + event.wasClean + \", evCode: \"\n\
          + event.code + \"\\n\");\n\
      connected = false;\n\
    };\n\
  }\n\
  doConnect('ws://'+location.hostname+':8080/');\n\
  </script>\n\
</head>\n\
<body>\n\
\n\
  <div id=\"header\">\n\
    <h1 align=\"left\">Arduino Uno Wifi rev2</h1>\n\
    <div id=\"status\">Disconnected</div>\n\
    RGB: <br />\n\
    <input id=\"r\" type=\"range\" min=\"0\" max=\"255\" step=\"1\" value=\"0\" oninput=\"send();\" /><br />\n\
    <input id=\"g\" type=\"range\" min=\"0\" max=\"255\" step=\"1\" value=\"0\" oninput=\"send();\" /><br />\n\
    <input id=\"b\" type=\"range\" min=\"0\" max=\"255\" step=\"1\" value=\"0\" oninput=\"send();\" /><br />\n\
    LED: <br />\n\
    <input id=\"ledoff\" type=\"radio\" name=\"led\" value=\"0\" oninput=\"send();\" checked=\"checked\"/>\n\
    <label for=\"ledoff\">OFF</label>\n\
    <input id=\"ledon\" type=\"radio\" name=\"led\" value=\"1\" oninput=\"send();\" />\n\
    <label for=\"ledon\">ON</label>\n\
    <br /> <br />\n\
    RGB: <div id=\"RGB\"></div><br />\n\
    LED: <div id=\"LED\"></div><br />\n\
    Accel: <div id=\"accel\"></div><br />\n\
    Gyro: <div id=\"gyro\"></div><br />\n\
    Temperature: <div id=\"temp\"></div><br />\n\
  </div>\n\
</body>\n\
</html>\
";
