#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "DHT.h"
#include "secrets.h"

const char* groqModel = "llama-3.3-70b-versatile";

#define DHTPIN 19
#define DHTTYPE DHT11
#define LDRPIN 35
#define TRIGPIN 25
#define ECHOPIN 26

DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);
WiFiClientSecure secureClient;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
  <title>AI Mechanics Assistant</title>
  <style>
    * {
      margin: 0;
      padding: 0;
      box-sizing: border-box;
    }

    body {
      font-family: 'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
      background: #0d1117;
      color: #e6edf3;
      height: 100vh;
      display: flex;
      flex-direction: column;
      justify-content: center;
      align-items: center;
      padding: 1rem;
    }

    .container {
      width: 100%;
      max-width: 650px;
      height: 90vh;
      display: flex;
      flex-direction: column;
      background: #161b22;
      border-radius: 1.5rem;
      box-shadow: 0 25px 50px -12px rgba(0,0,0,0.8);
      overflow: hidden;
      border: 1px solid #30363d;
      animation: fadeInUp 0.6s ease;
    }

    @keyframes fadeInUp {
      from { opacity: 0; transform: translateY(20px); }
      to { opacity: 1; transform: translateY(0); }
    }

    .header {
      padding: 1.25rem 1.5rem;
      border-bottom: 1px solid #21262d;
      display: flex;
      align-items: center;
      gap: 0.75rem;
    }

    .logo {
      width: 36px;
      height: 36px;
      background: linear-gradient(135deg, #58a6ff, #bc8cff);
      border-radius: 50%;
      display: flex;
      align-items: center;
      justify-content: center;
      font-size: 1.2rem;
    }

    .header h1 {
      font-size: 1.25rem;
      font-weight: 600;
      letter-spacing: -0.02em;
      background: linear-gradient(135deg, #58a6ff, #bc8cff);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
      background-clip: text;
    }

    #chat {
      flex: 1;
      overflow-y: auto;
      padding: 1.25rem;
      display: flex;
      flex-direction: column;
      gap: 1rem;
      scroll-behavior: smooth;
    }

    #chat::-webkit-scrollbar {
      width: 5px;
    }
    #chat::-webkit-scrollbar-track {
      background: transparent;
    }
    #chat::-webkit-scrollbar-thumb {
      background: #30363d;
      border-radius: 10px;
    }

    .message {
      max-width: 85%;
      padding: 0.85rem 1rem;
      border-radius: 1.2rem;
      line-height: 1.55;
      font-size: 0.95rem;
      word-wrap: break-word;
      animation: messageIn 0.4s ease;
    }

    @keyframes messageIn {
      from { opacity: 0; transform: scale(0.95); }
      to { opacity: 1; transform: scale(1); }
    }

    .user {
      align-self: flex-end;
      background: #238636;
      border-bottom-right-radius: 0.3rem;
      color: #ffffff;
      box-shadow: 0 4px 6px rgba(0,0,0,0.3);
    }

    .ai {
      align-self: flex-start;
      background: #21262d;
      border-bottom-left-radius: 0.3rem;
      border: 1px solid #30363d;
      color: #e6edf3;
    }

    .typing-indicator {
      display: none;
      align-self: flex-start;
      background: #21262d;
      border: 1px solid #30363d;
      border-radius: 1.2rem;
      padding: 0.8rem 1.2rem;
      margin-top: 0.5rem;
      animation: messageIn 0.4s ease;
    }

    .typing-indicator span {
      display: inline-block;
      width: 8px;
      height: 8px;
      border-radius: 50%;
      background: #8b949e;
      margin: 0 2px;
      animation: typingBounce 1.4s infinite ease-in-out;
    }
    .typing-indicator span:nth-child(2) { animation-delay: 0.2s; }
    .typing-indicator span:nth-child(3) { animation-delay: 0.4s; }

    @keyframes typingBounce {
      0%, 60%, 100% { transform: translateY(0); }
      30% { transform: translateY(-8px); }
    }

    .input-area {
      padding: 1rem 1.25rem;
      border-top: 1px solid #21262d;
      display: flex;
      gap: 0.75rem;
      background: #161b22;
    }

    input[type="text"] {
      flex: 1;
      padding: 0.9rem 1.25rem;
      border-radius: 1.5rem;
      border: 1px solid #30363d;
      background: #0d1117;
      color: #e6edf3;
      font-size: 0.95rem;
      transition: all 0.2s ease;
      outline: none;
    }

    input[type="text"]:focus {
      border-color: #58a6ff;
      box-shadow: 0 0 0 3px rgba(88,166,255,0.15);
    }

    button {
      padding: 0.9rem 1.4rem;
      border-radius: 1.5rem;
      border: none;
      background: linear-gradient(135deg, #58a6ff, #bc8cff);
      color: white;
      font-weight: 600;
      cursor: pointer;
      transition: transform 0.2s ease, box-shadow 0.2s ease;
      box-shadow: 0 4px 12px rgba(88,166,255,0.3);
      white-space: nowrap;
    }

    button:hover {
      transform: translateY(-1px);
      box-shadow: 0 6px 16px rgba(88,166,255,0.4);
    }

    button:active {
      transform: scale(0.96);
    }

    @media (max-width: 500px) {
      .container {
        height: 95vh;
        border-radius: 1rem;
      }
      .message {
        max-width: 90%;
        font-size: 0.9rem;
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <div class="logo">⚙️</div>
      <h1>AI Mechanics Assistant</h1>
    </div>
    <div id="chat"></div>
    <div class="typing-indicator" id="typing">
      <span></span><span></span><span></span>
    </div>
    <div class="input-area">
      <input type="text" id="question" placeholder="Ask a mechanics question..." autocomplete="off">
      <button id="sendBtn">Ask</button>
    </div>
  </div>

  <script>
    const chat = document.getElementById('chat');
    const input = document.getElementById('question');
    const sendBtn = document.getElementById('sendBtn');
    const typingDiv = document.getElementById('typing');

    function appendMsg(role, text) {
      const msg = document.createElement('div');
      msg.className = 'message ' + role;
      msg.textContent = text;
      chat.appendChild(msg);
      chat.scrollTop = chat.scrollHeight;
    }

    function showTyping(show) {
      typingDiv.style.display = show ? 'flex' : 'none';
      chat.scrollTop = chat.scrollHeight;
    }

    async function ask() {
      const q = input.value.trim();
      if (!q) return;
      appendMsg('user', q);
      input.value = '';
      showTyping(true);

      try {
        const res = await fetch('/ask', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ question: q })
        });
        const data = await res.json();
        showTyping(false);
        if (data.answer) {
          appendMsg('ai', data.answer);
        } else {
          appendMsg('ai', 'Error: ' + (data.error || 'Unknown'));
        }
      } catch (err) {
        showTyping(false);
        appendMsg('ai', 'Network error – check connection.');
      }
    }

    sendBtn.addEventListener('click', ask);
    input.addEventListener('keypress', (e) => {
      if (e.key === 'Enter') ask();
    });

  </script>
</body>
</html>
)rawliteral";

float readTemperature() {
  float t = dht.readTemperature();
  if (isnan(t)) return -99.0;
  return t;
}

float readHumidity() {
  float h = dht.readHumidity();
  if (isnan(h)) return -99.0;
  return h;
}

float readLight() {
  int raw = analogRead(LDRPIN);
  float voltage = raw * 3.3 / 4095.0;
  float resistance = 10000 * voltage / (3.3 - voltage);
  float lux = 500 / (resistance / 1000);
  return lux;
}

float readDistance() {
  digitalWrite(TRIGPIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGPIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGPIN, LOW);
  long duration = pulseIn(ECHOPIN, HIGH, 30000);
  float distance = duration * 0.0343 / 2.0;
  if (distance == 0) return -1.0;
  return distance;
}

String askGroqWithTools(String userMessage) {
  if (WiFi.status() != WL_CONNECTED) return "WiFi not connected";
  timeClient.update();

  String url = "https://api.groq.com/openai/v1/chat/completions";
  HTTPClient https;
  https.begin(secureClient, url);
  https.addHeader("Content-Type", "application/json");
  https.addHeader("Authorization", "Bearer " + String(groqApiKey));

  DynamicJsonDocument doc(16384);
  doc["model"] = groqModel;
  JsonArray messages = doc.createNestedArray("messages");

  JsonObject sysMsg = messages.createNestedObject();
  sysMsg["role"] = "system";
  sysMsg["content"] = "You are a helpful applied mechanics assistant. You can request live sensor data using the provided tools. Answer clearly and use formulas where appropriate.";

  JsonObject userMsg = messages.createNestedObject();
  userMsg["role"] = "user";
  userMsg["content"] = userMessage;

  JsonArray tools = doc.createNestedArray("tools");

  JsonObject tool1 = tools.createNestedObject();
  tool1["type"] = "function";
  JsonObject func1 = tool1.createNestedObject("function");
  func1["name"] = "get_temperature";
  func1["description"] = "Get the current ambient temperature in degrees Celsius";
  func1["parameters"]["type"] = "object";
  func1["parameters"].createNestedObject("properties");

  JsonObject tool2 = tools.createNestedObject();
  tool2["type"] = "function";
  JsonObject func2 = tool2.createNestedObject("function");
  func2["name"] = "get_humidity";
  func2["description"] = "Get the current relative humidity in percent";
  func2["parameters"]["type"] = "object";
  func2["parameters"].createNestedObject("properties");

  JsonObject tool3 = tools.createNestedObject();
  tool3["type"] = "function";
  JsonObject func3 = tool3.createNestedObject("function");
  func3["name"] = "get_light";
  func3["description"] = "Get the ambient light level in lux";
  func3["parameters"]["type"] = "object";
  func3["parameters"].createNestedObject("properties");

  JsonObject tool4 = tools.createNestedObject();
  tool4["type"] = "function";
  JsonObject func4 = tool4.createNestedObject("function");
  func4["name"] = "get_distance";
  func4["description"] = "Measure distance to an object in cm using ultrasonic sensor";
  func4["parameters"]["type"] = "object";
  func4["parameters"].createNestedObject("properties");

  String requestBody;
  serializeJson(doc, requestBody);
  int httpCode = https.POST(requestBody);
  if (httpCode != 200) {
    String errorBody = https.getString();
    https.end();
    Serial.println("Groq error: " + errorBody);
    return "Groq API error " + String(httpCode) + ": " + errorBody;
  }
  String response1 = https.getString();
  https.end();

  DynamicJsonDocument respDoc(16384);
  DeserializationError err = deserializeJson(respDoc, response1);
  if (err) return "Parsing error: " + String(err.c_str());

  JsonVariant choices = respDoc["choices"];
  if (choices.isNull() || choices.size() == 0) return "No AI response";
  JsonVariant msg = choices[0]["message"];
  JsonVariant toolCalls = msg["tool_calls"];

  if (toolCalls.isNull() || toolCalls.size() == 0) {
    if (msg["content"].isNull()) return "No content in response";
    return msg["content"].as<String>();
  }

  JsonVariant call = toolCalls[0];
  String functionName = call["function"]["name"].as<String>();
  String funcResult = "";

  if (functionName == "get_temperature") {
    float t = readTemperature();
    DynamicJsonDocument rd(256);
    rd["temperature"] = t;
    serializeJson(rd, funcResult);
  } else if (functionName == "get_humidity") {
    float h = readHumidity();
    DynamicJsonDocument rd(256);
    rd["humidity"] = h;
    serializeJson(rd, funcResult);
  } else if (functionName == "get_light") {
    float lux = readLight();
    DynamicJsonDocument rd(256);
    rd["light_lux"] = lux;
    serializeJson(rd, funcResult);
  } else if (functionName == "get_distance") {
    float dist = readDistance();
    DynamicJsonDocument rd(256);
    rd["distance_cm"] = dist;
    serializeJson(rd, funcResult);
  } else {
    return "Unknown function requested: " + functionName;
  }

  HTTPClient https2;
  https2.begin(secureClient, url);
  https2.addHeader("Content-Type", "application/json");
  https2.addHeader("Authorization", "Bearer " + String(groqApiKey));

  DynamicJsonDocument doc2(16384);
  doc2["model"] = groqModel;
  JsonArray messages2 = doc2.createNestedArray("messages");

  JsonObject sysMsg2 = messages2.createNestedObject();
  sysMsg2["role"] = "system";
  sysMsg2["content"] = "You are a helpful applied mechanics assistant.";

  JsonObject userMsg2 = messages2.createNestedObject();
  userMsg2["role"] = "user";
  userMsg2["content"] = userMessage;

  JsonObject assistantMsg = messages2.createNestedObject();
  assistantMsg["role"] = "assistant";
  assistantMsg["content"] = (char*)0;
  JsonArray asstToolCalls = assistantMsg.createNestedArray("tool_calls");
  asstToolCalls.add(call);

  JsonObject toolRespMsg = messages2.createNestedObject();
  toolRespMsg["role"] = "tool";
  toolRespMsg["tool_call_id"] = call["id"].as<String>();
  toolRespMsg["content"] = funcResult;

  JsonArray tools2 = doc2.createNestedArray("tools");
  tools2.add(tool1); tools2.add(tool2); tools2.add(tool3); tools2.add(tool4);

  String requestBody2;
  serializeJson(doc2, requestBody2);
  int httpCode2 = https2.POST(requestBody2);
  if (httpCode2 != 200) {
    String errorBody2 = https2.getString();
    https2.end();
    Serial.println("Groq follow-up error: " + errorBody2);
    return "Groq API error (follow-up) " + String(httpCode2) + ": " + errorBody2;
  }
  String response2 = https2.getString();
  https2.end();

  DynamicJsonDocument respDoc2(16384);
  err = deserializeJson(respDoc2, response2);
  if (err) return "Parsing error: " + String(err.c_str());

  JsonVariant finalChoices = respDoc2["choices"];
  if (finalChoices.isNull() || finalChoices.size() == 0) return "No final response";
  return finalChoices[0]["message"]["content"].as<String>();
}

void setup() {
  Serial.begin(115200);

  dht.begin();
  pinMode(LDRPIN, INPUT);
  pinMode(TRIGPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());
  delay(1000);

  timeClient.begin();
  while (timeClient.getEpochTime() < 100000) {
    timeClient.update();
    delay(500);
  }
  secureClient.setInsecure();

  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", index_html);
  });

  server.on("/ask", HTTP_POST, []() {
    DynamicJsonDocument inputDoc(1024);
    DeserializationError error = deserializeJson(inputDoc, server.arg("plain"));
    if (error) {
      server.send(400, "text/plain", "Invalid JSON");
      return;
    }
    String question = inputDoc["question"];
    if (question.length() == 0) {
      server.send(400, "text/plain", "Missing question");
      return;
    }

    String answer = askGroqWithTools(question);

    DynamicJsonDocument outputDoc(4096);
    outputDoc["answer"] = answer;
    String responseJson;
    serializeJson(outputDoc, responseJson);
    server.send(200, "application/json", responseJson);
  });

  server.begin();
  Serial.println("Server ready! Open http://" + WiFi.localIP().toString());
}

void loop() {
  server.handleClient();
  timeClient.update();
}
