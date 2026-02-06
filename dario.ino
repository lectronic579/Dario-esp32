// Dario ESP32

#include <Adafruit_GFX.h>    
#include <Adafruit_ST7789.h> 
#include <SPI.h>
#include <EEPROM.h>

// --- HARDWARE ---
#define TFT_CS     5
#define TFT_RST    4 
#define TFT_DC     2
#define PIN_BL     21  
#define PIN_BOTON  13
#define PIN_BUZZER 12
#define EEPROM_SIZE 20 

#define SPI_SPEED 27000000 
#define FREC_BASE   2000
#define RES_AUDIO   8

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// --- VARIABLES GLOBALES ---
uint16_t coloresFondo[] = {0x4D39, 0x4810, 0x0208, 0x18C3}; 
#define C_BIRD      0xFD20  
#define C_PIPE      0x07E0  
#define C_MUERTE    0x1082  
#define C_NOCHE     0x0000 
#define C_PIPE_NEON 0x07FF 

float birdY = 160, birdYOld = 160, momentum = 0;
int pilarX = 240, huecoY = 100, puntuacion = 0, highScore = 0;
char highName[4] = "DAR"; 
bool juegoActivo = false, modoNocheActivo = false;
float velocidadPilar = 4.1; 
int tamañoHueco = 100, vidas = 2; 
unsigned long invencibleTimer = 0;

int melodiaMenu[] = {262, 330, 392, 523, 392, 330}; 
int notaMenuActual = 0;
unsigned long tiempoNotaMenu = 0;
int vientoX[3], vientoY[3];
float oscilacionY = 0;

void emitirSonido(int frecuencia, int duracion) {
  if (frecuencia > 0) {
    ledcWriteTone(PIN_BUZZER, frecuencia);
    ledcWrite(PIN_BUZZER, 128); 
  }
  delay(duracion);
  ledcWriteTone(PIN_BUZZER, 0); 
}

void dibujarBanner() {
  tft.fillRect(0, 0, 240, 31, ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(modoNocheActivo ? ST77XX_MAGENTA : ST77XX_RED);
  tft.setCursor(10, 8);
  for(int i=0; i<vidas; i++) { tft.print("@ "); } 
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(190, 8);
  tft.print(puntuacion); 
}

void dibujarPajaro(int x, int y, uint16_t color) {
  if (y < 32 || y > 310) return; // Evitar dibujo fuera de zona segura
  tft.fillRoundRect(x-12, y-8, 24, 18, 8, color); 
  tft.fillTriangle(x-14, y, x-22, y-5, x-22, y+5, ST77XX_WHITE); 
  tft.fillCircle(x+6, y-3, 3, ST77XX_WHITE); 
  tft.fillTriangle(x+10, y, x+18, y+4, x+10, y+8, ST77XX_RED); 
}

void setup() {
  pinMode(PIN_BOTON, INPUT_PULLUP);
  pinMode(PIN_BL, OUTPUT);
  digitalWrite(PIN_BL, HIGH); 
  ledcAttach(PIN_BUZZER, FREC_BASE, RES_AUDIO);
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, highScore);
  tft.init(240, 320); 
  tft.setSPISpeed(SPI_SPEED); 
  tft.setRotation(2); 
  tft.invertDisplay(true); 
  for(int i=0; i<3; i++) { vientoX[i] = random(240, 400); vientoY[i] = random(40, 300); }
  pantallaCarga(); 
  menuInicio();
}

void loop() { if (juegoActivo) jugar(); }

void iniciarJuego() {
  birdY = 160; momentum = 0; pilarX = 240;
  puntuacion = 0; velocidadPilar = 4.1; tamañoHueco = 100;
  vidas = 2; modoNocheActivo = false;
  juegoActivo = true;
  tft.fillScreen(coloresFondo[0]);
}

void jugar() {
  uint16_t colorF = modoNocheActivo ? C_NOCHE : coloresFondo[0];

  if (pilarX >= 235 && pilarX <= 240) {
    if (puntuacion == 22 && !modoNocheActivo) { 
      modoNocheActivo = true;
      tft.fillScreen(C_NOCHE);
    } 
    if (puntuacion >= 5) {
      // CAP DE VELOCIDAD: No superar 7.0 para evitar cuelgues SPI
      if (velocidadPilar < 7.0) velocidadPilar += 0.025; 
      if (puntuacion % 5 == 0 && tamañoHueco > 65) tamañoHueco -= 1;
    }
    dibujarBanner();
  }

  if (puntuacion >= 15) {
    for(int i=0; i<3; i++) {
      tft.drawFastHLine(vientoX[i], vientoY[i], 20, colorF);
      vientoX[i] -= 12;
      if (vientoX[i] < -20) { vientoX[i] = 240; vientoY[i] = random(40, 300); }
      tft.drawFastHLine(vientoX[i], vientoY[i], 20, modoNocheActivo ? ST77XX_CYAN : ST77XX_WHITE);
    }
  }

  // Limpieza con márgenes de seguridad
  tft.fillRect(60-25, (int)birdYOld-11, 52, 26, colorF); 
  int vClean = (int)velocidadPilar + 5;
  tft.fillRect(pilarX + 40, 31, vClean, 289, colorF);

  if (digitalRead(PIN_BOTON) == LOW) { momentum = -5.2; emitirSonido(900, 5); }
  momentum += 0.40; birdY += momentum;
  birdYOld = birdY;
  pilarX -= (int)velocidadPilar;

  if (pilarX < -40) {
    tft.fillRect(0, 31, 60, 289, colorF);
    pilarX = 240; huecoY = random(70, 130); // Rango de hueco más seguro
    puntuacion++; emitirSonido(1500, 20);
  }

  // OSCILACIÓN CONTROLADA PARA NIVEL 38+
  int yH = huecoY;
  if (puntuacion >= 38) {
    yH += (int)(sin(oscilacionY) * 30); // Reducido a 30 para no salir de pantalla
    oscilacionY += 0.07;
  }
  
  uint16_t colorT = modoNocheActivo ? C_PIPE_NEON : C_PIPE;
  // Dibujo de tubos con límites estrictos
  tft.fillRect(pilarX, 31, 40, yH, colorT);
  int tuboBajoY = yH + 31 + tamañoHueco;
  if (tuboBajoY < 320) tft.fillRect(pilarX, tuboBajoY, 40, 320 - tuboBajoY, colorT);

  int dY = (int)birdY; 
  if (dY < 40) dY = 40; if (dY > 305) dY = 305;
  dibujarPajaro(60, dY, C_BIRD);

  if (millis() > invencibleTimer) {
    if (birdY > 310 || birdY < 35 || (pilarX < 75 && pilarX + 40 > 45 && (birdY < yH + 31 || birdY > yH + 31 + tamañoHueco))) {
      vidas--; emitirSonido(150, 150);
      if (vidas <= 0) pantallaChoque();
      else {
        invencibleTimer = millis() + 1500;
        pilarX = 240; birdY = 160; 
        tft.fillScreen(colorF);
      }
    }
  }
  delay(15); 
}

void menuInicio() {
  tft.fillScreen(coloresFondo[0]);
  tft.setTextSize(5);
  tft.setTextColor(ST77XX_BLACK); tft.setCursor(48, 43); tft.print("DARIO");
  tft.setTextColor(C_BIRD); tft.setCursor(45, 40); tft.print("DARIO");
  dibujarPajaro(120, 120, C_BIRD);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_RED); tft.setCursor(40, 175); 
  tft.print("MEJOR: "); tft.print(highName); tft.print(" "); tft.print(highScore);
  tft.setTextColor(ST77XX_WHITE); tft.setCursor(55, 270); tft.print("PULSA BOTON");
  
  unsigned long lastFlash = 0;
  bool visible = true;
  while(digitalRead(PIN_BOTON) == HIGH) {
    if (millis() - tiempoNotaMenu > 120) { 
      emitirSonido(melodiaMenu[notaMenuActual], 60);
      notaMenuActual = (notaMenuActual + 1) % 6; tiempoNotaMenu = millis();
    }
    if (millis() - lastFlash > 500) {
      visible = !visible;
      uint16_t cText = visible ? ST77XX_WHITE : coloresFondo[0];
      tft.setTextSize(1); tft.setCursor(111, 220); tft.setTextColor(cText); tft.print("por"); 
      tft.setTextSize(2); tft.setCursor(48, 235); tft.setTextColor(cText); tft.print("LECTRONIC579");
      lastFlash = millis();
    }
    delay(10);
  }
  iniciarJuego();
}

void pantallaChoque() {
  juegoActivo = false; emitirSonido(100, 600); 
  if (puntuacion > highScore) { highScore = puntuacion; EEPROM.put(0, highScore); EEPROM.commit(); }
  tft.fillScreen(C_MUERTE);
  tft.setTextColor(ST77XX_RED); tft.setTextSize(5); tft.setCursor(25, 30); tft.print("HOSTIA!");
  tft.setTextColor(ST77XX_WHITE); tft.setTextSize(3); tft.setCursor(20, 90);
  
  if (puntuacion <= 10) tft.print("PAQUETE");
  else if (puntuacion <= 28) tft.print("CAGON");
  else if (puntuacion <= 35) tft.print("MAQUINA");
  else if (puntuacion <= 40) tft.print("LEYENDA");
  else if (puntuacion <= 46) { tft.setCursor(15, 90); tft.print("PUTO AMO"); }
  else if (puntuacion <= 52) { tft.setCursor(15, 90); tft.print("PUTO TODO"); }
  else { tft.setTextColor(ST77XX_YELLOW); tft.setCursor(15, 90); tft.print("PUTO DIOS"); }

  tft.setTextSize(2); tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(50, 160); tft.print("PUNTOS: "); tft.print(puntuacion);
  tft.setCursor(50, 190); tft.setTextColor(ST77XX_YELLOW); tft.print("RECORD: "); tft.print(highScore);

  unsigned long timerMsg = millis(); bool showT = true;
  while(digitalRead(PIN_BOTON) == HIGH) {
    if (millis() - timerMsg > 500) {
      showT = !showT; tft.setTextSize(1); tft.setCursor(45, 280); 
      tft.setTextColor(showT ? ST77XX_WHITE : C_MUERTE);
      tft.print("PULSA BOTON PARA REINTENTAR"); timerMsg = millis();
    }
    delay(10);
  }
  delay(200); 
  tft.init(240, 320); 
  tft.setRotation(2);
  tft.invertDisplay(true);
  menuInicio();
}

void pantallaCarga() {
  tft.fillScreen(ST77XX_BLACK); tft.setTextSize(2); tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(45, 120); tft.print("Cargando..."); tft.drawRect(20, 150, 200, 20, ST77XX_WHITE);
  for (int i = 0; i < 196; i++) {
    tft.fillRect(22 + i, 152, 1, 16, C_BIRD);
    if (i % 20 == 0) emitirSonido(300 + (i * 5), 15); 
    else delay(4); 
  }
  emitirSonido(1200, 100);
}
