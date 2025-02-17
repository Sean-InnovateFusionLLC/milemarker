#include <AsyncTCP.h>
#include <AsyncEventSource.h>
#include <AsyncJson.h>
#include <SPIFFSEditor.h>
#include <WebHandlerImpl.h>
#include <WebAuthentication.h>
#include <AsyncWebSynchronization.h>
#include <AsyncWebSocket.h>
#include <WebResponseImpl.h>
#include <StringArray.h>
#include <ESPAsyncWebSrv.h>
#include <DNSServer.h>


const byte DNS_PORT = 53;
DNSServer dnsServer;
AsyncWebServer webServer(80);

void setupCaptivePortal() {
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  const char* htmlContent = R"rawliteral(
    <!DOCTYPE html>
    <html>
        <head>
            <meta charset="UTF-8">
            <title>Captive Portal</title>
            <style>
                body {
                    font-family: Arial, sans-serif;
                    margin: 0;
                    padding: 0;
                    background-color: #f0f0f0;
                }
                .container {
                    max-width: 600px;
                    margin: 0 auto;
                    padding: 20px;
                }
                h1 {
                    color: #333;
                    text-align: center;
                }
                p {
                    text-align: center;
                }
                .form-group {
                    margin-bottom: 20px;
                }
                .form-group label {
                    display: block;
                    margin-bottom: 10px;
                }
                .form-group input[type="text"],
                .form-group input[type="password"] {
                    width: 100%;
                    padding: 10px;
                    font-size: 16px;
                }
                .form-group button {
                    background-color: #007BFF;
                    color: #fff;
                    padding: 10px 20px;
                    border: none;
                    border-radius: 5px;
                    cursor: pointer;
                }
                .form-group button:hover {
                    background-color: #0056b3;
                }
            </style>
        </head>
        <body>
            <div class="container">
                <h1>Restricted Captive Portal!</h1>
                <p>Employee Only Captive Portal </p>
                <form>
                    <div class="form-group">
                        <label for="username">Username:</label>
                        <input type="text" id="username" name="username">
                    </div>
                    <div class="form-group">
                        <label for="password">Password:</label>
                        <input type="password" id="password" name="password">
                    </div>
                    <div class="form-group">
                        <button type="submit">Submit</button>
                    </div>
                </form>
            </div>
        </body>
    </html>
  )rawliteral";

  webServer.onNotFound([htmlContent](AsyncWebServerRequest *request){
    request->send(200, "text/html", htmlContent);
  });
  
  webServer.begin();
}

void handleCaptivePortal() {
  dnsServer.processNextRequest();
}
