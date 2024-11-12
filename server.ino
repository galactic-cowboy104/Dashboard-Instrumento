#include <WiFi.h>
#include <ESPAsyncWebSrv.h>
#include <WebSocketsServer.h>

AsyncWebServer server(80);
WebSocketsServer websockets(81);

const uint16_t interval = 500;
uint16_t counter = 1;

// El contenido de index.html como una cadena de texto
const char* indexHTML = R"rawliteral(

  <!DOCTYPE html>
  <html lang="es-MX">
  
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>Dashboard - Instrumento</title>
      <script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.9.4/Chart.js"></script>
    </head>
  
    <body style="background-color: #100C1F;">
  
      <header style="color:#CCC">
        <h1>Dashboard - Instrumento</h1>
      </header>
  
      <div class="container">
        <canvas id="myChart" style="width:100%; max-width:700px"></canvas>
      </div>
          
    </body>
  
    <script>
  
      let ws = new WebSocket('ws://' + window.location.hostname + ':81/');
  
      var xyValues = [{x:0.0, y:0.0}];
  
      ws.onmessage = function(event) {
        var data = JSON.parse(event.data);
        xyValues.push({x: data.x, y: data.y});
        chart.update();
      }
  
        
      const chart = new Chart("myChart", {
      type: "scatter",
      data: {
        datasets: [{
            label: 'Datos',
            pointRadius: 4,
            pointBackgroundColor: "rgb(0,255,0)",
            data: xyValues,
            borderColor: 'rgba(50,50,255,0.5)',
            backgroundColor: 'rgba(0,255,0,1)'
          }]
        },
        options: {
          legend: { display: true },
          scales: {
            xAxes: [{
              ticks: { min: 0, max: 30 },
              gridLines: { color: 'rgba(0,255,255,0.2)' }
            }],
            yAxes: [{
              ticks: { min: 0, max: 30 },
              gridLines: { color: 'rgba(0,255,255,0.2)' }
            }]
          }
        }
      });
  
      ws.onmessage = function(event) {
        
        var data = JSON.parse(event.data);
        xyValues.push({x: data.x, y: data.y});

        let counter = data.x;

        if (counter % 30 === 0) {
          let newMin = counter;
          let newMax = counter + 30;
          chart.options.scales.xAxes[0].ticks.min = newMin;
          chart.options.scales.xAxes[0].ticks.max = newMax;
        }
         
        chart.update();
        
      }
  
    </script>
  
  </html>
  
)rawliteral";

// Manejador de eventos WebSocket
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {

  switch(type) {
    
    case WStype_DISCONNECTED:
      Serial.printf("[%u] ¡Desconectado!\n", num);
      break;
    case WStype_CONNECTED: {
      
      IPAddress ip = websockets.remoteIP(num);
      Serial.printf("[%u] Conectado en %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);  

      websockets.sendTXT(num, "Conectado en servidor:");
    
    }
    break;
  }
  
}

// Respuesta cuando no se encuentra la página solicitada
void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "¡Página no encontrada!");
}

void setup() {
  
  Serial.begin(115200);

  // Conectarse a una red WiFi como cliente
  const char* ssid = "ICN";      // Cambia esto por el nombre de tu red WiFi
  const char* password = "99999999999999999999999999";  // Cambia esto por la contraseña de tu red WiFi

  WiFi.begin(ssid, password);  // Iniciar la conexión WiFi
  
  Serial.print("Conectando a WiFi");
  
  // Esperar a que el ESP32 se conecte a la red
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  
  // Imprimir la IP asignada
  Serial.println("\nConectado a WiFi");
  Serial.print("IP asignada: ");
  Serial.println(WiFi.localIP());

  // Ruta principal que sirve el contenido de index.html
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", indexHTML);
  });

  server.onNotFound(notFound);

  server.begin();  // Iniciar el servidor HTTP
  websockets.begin();  // Iniciar el servidor WebSocket
  websockets.onEvent(webSocketEvent);  // Establecer el manejador de eventos WebSocket
  
}

void loop() {
  
  websockets.loop();  // Mantener el WebSocket en funcionamiento

  static uint32_t prevMillis = 0;
  if(millis() - prevMillis >= interval) {

    prevMillis = millis();

    int y = random(15, 30);
    String data = "{\"x\": " + String(counter) + ",\"y\": " + String(y) + "}";
    websockets.broadcastTXT(data);  // Enviar datos a todos los clientes WebSocket

    Serial.println(data);  // Imprimir los datos en el monitor serie

    counter ++;
    
  }
  
}
