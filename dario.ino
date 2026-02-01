

#include <Adafruit_GFX.h>    
#include <Adafruit_ST7789.h> 
#include <SPI.h>
#include <EEPROM.h>

// Configuración de Hardware (ESP32 + ST7789V)
#define TFT_CS     5
#define TFT_RST    4 
#define TFT_DC     2
#define PIN_BL     21  
#define PIN_BOTON  13
#define PIN_BUZZER 12
#define EEPROM_SIZE 10 

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// Paleta de Colores
uint16_t coloresFondo[] = {0x4D39, 0x4810, 0x0208, 0x18C3}; 
int indiceColor = 0;
#define C_BIRD      0xFD20  
#define C_PIPE      0x07E0  
#define C_MUERTE    0x1082  
#define C_TEXTO     0xFFFF  

// Física y Juego
float birdY = 160;
float momentum = 0;
int pilarX = 240;
int huecoY = 100;
int puntuacion = 0;
int highScore = 0;
char highName[4] = "AAA"; 
bool juegoActivo = false;
float velocidadPilar = 6.0;
int tamañoHueco = 100;

// Sistema de Vidas (2 Vidas)
int vidas = 2; 
unsigned long invencibleTimer = 0;
bool parpadeo = false;

// Sistema de Partículas
struct Particula {
  float x, y, vx, vy;
  uint16_t color;
  bool activa = false;
};
#define MAX_PARTICULAS 15
Particula particulas[MAX_PARTICULAS];

// Efectos Visuales (Viento)
int vientoX[5], vientoY[5];

// Melodía Menú
int melodiaMenu[] = {262, 330, 392, 523, 392, 330}; 
int notaMenuActual = 0;
unsigned long tiempoNotaMenu = 0;

// --- FUNCIONES DE SOPORTE ---

void generarExplosion(int x, int y) {
  for (int i = 0; i < MAX_PARTICULAS; i++) {
    particulas[i].x = x;
    particulas[i].y = y;
    particulas[i].vx = (random(-40, 40) / 10.0); 
    particulas[i].vy = (random(-60, 10) / 10.0);
    particulas[i].color = (random(0, 2) == 0) ? C_BIRD : ST77XX_WHITE;
    particulas[i].activa = true;
  }
  
  for (int frames = 0; frames < 20; frames++) {
    for (int i = 0; i < MAX_PARTICULAS; i++) {
      if (particulas[i].activa) {
        tft.drawPixel((int)particulas[i].x, (int)particulas[i].y, coloresFondo[indiceColor]);
        particulas[i].x += particulas[i].vx;
        particulas[i].y += particulas[i].vy;
        particulas[i].vy += 0.25; // Gravedad de partículas
        if (particulas[i].y > 31 && particulas[i].y < 315 && particulas[i].x > 0 && particulas[i].x < 240) {
           tft.drawPixel((int)particulas[i].x, (int)particulas[i].y, particulas[i].color);
        }
      }
    }
    delay(10);
  }
}

void dibujarBanner() {
  tft.fillRect(0, 0, 240, 30, ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 8);
  tft.setTextColor(ST77XX_RED);
  for(int i=0; i<vidas; i++) { tft.print("@ "); } 
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(puntuacion < 10 ? 210 : (puntuacion < 100 ? 200 : 190), 8);
  tft.print(puntuacion); 
}

void dibujarPajaro(int x, int y, uint16_t color) {
  tft.fillRoundRect(x-12, y-8, 24, 18, 8, color); 
  tft.fillTriangle(x-14, y, x-22, y-5, x-22, y+5, ST77XX_WHITE); 
  tft.fillCircle(x+6, y-3, 3, ST77XX_WHITE); 
  tft.fillTriangle(x+10, y, x+18, y+4, x+10, y+8, ST77XX_RED); 
}

// --- FLUJO PRINCIPAL ---

void setup() {
  pinMode(PIN_BOTON, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BL, OUTPUT);
  digitalWrite(PIN_BL, HIGH); 

  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, highScore);
  highName[0] = EEPROM.read(4); highName[1] = EEPROM.read(5);
  highName[2] = EEPROM.read(6); highName[3] = '\0';

  if (highScore < 0 || highScore > 999) highScore = 0; 
  if (highName[0] < 'A' || highName[0] > 'Z') strcpy(highName, "AAA");

  tft.init(240, 320); 
  tft.setRotation(2); 
  tft.invertDisplay(true);
  
  for(int i=0; i<5; i++) { vientoX[i] = random(0, 240); vientoY[i] = random(35, 310); }
  
  pantallaCarga(); 
  menuInicio();
}

void loop() { if (juegoActivo) jugar(); }

void iniciarJuego() {
  birdY = 160; momentum = 0; pilarX = 240;
  puntuacion = 0; velocidadPilar = 6.0; tamañoHueco = 100;
  vidas = 2; invencibleTimer = 0;
  indiceColor = 0; juegoActivo = true;
  tft.fillScreen(coloresFondo[indiceColor]);
}

void jugar() {
  dibujarBanner();

  if (puntuacion > 0 && puntuacion % 5 == 0 && pilarX == 240) {
      indiceColor = (puntuacion / 5) % 4; tft.fillScreen(coloresFondo[indiceColor]);
  }

  if (pilarX == 240 && (puntuacion == 10 || (puntuacion > 10 && puntuacion % 5 == 0))) {
      velocidadPilar += 0.4; if(tamañoHueco > 65) tamañoHueco -= 5;
      tone(PIN_BUZZER, 2200, 40);
  }

  if (puntuacion >= 15) {
    for(int i=0; i<3; i++) {
      tft.drawFastHLine(vientoX[i], vientoY[i], 10, coloresFondo[indiceColor]);
      vientoX[i] -= (velocidadPilar + 4);
      if(vientoX[i] < -10) { vientoX[i] = 240; vientoY[i] = random(35, 310); }
      tft.drawFastHLine(vientoX[i], vientoY[i], 10, ST77XX_WHITE);
    }
  }

  int cleanY = (int)birdY - 18; if (cleanY < 31) cleanY = 31;
  tft.fillRect(60 - 25, cleanY, 50, 36, coloresFondo[indiceColor]);

  if (digitalRead(PIN_BOTON) == LOW) { momentum = -5.8; tone(PIN_BUZZER, 900, 10); }
  momentum += 0.48; birdY += momentum;

  tft.fillRect(pilarX + 40, 31, (int)velocidadPilar + 4, 289, coloresFondo[indiceColor]); 
  pilarX -= (int)velocidadPilar;

  if (pilarX < -45) {
    tft.fillRect(0, 31, 60, 289, coloresFondo[indiceColor]); 
    pilarX = 240; huecoY = random(40, 160);
    puntuacion++; tone(PIN_BUZZER, 1500, 30);
  }

  tft.fillRect(pilarX, 31, 40, huecoY, C_PIPE);
  tft.fillRect(pilarX, huecoY + 31 + tamañoHueco, 40, 320, C_PIPE);

  bool esInvencible = millis() < invencibleTimer;
  if (esInvencible) parpadeo = !parpadeo; else parpadeo = false;

  if (!parpadeo) {
    int drawY = (int)birdY; if (drawY < 45) drawY = 45; 
    dibujarPajaro(60, drawY, C_BIRD);
  }

  // Colisiones
  if (!esInvencible) {
    if (birdY > 310 || birdY < 35 || (pilarX < 75 && pilarX + 40 > 45 && (birdY < huecoY + 31 || birdY > huecoY + 31 + tamañoHueco))) {
      
      generarExplosion(60, (int)birdY); // Efecto visual
      
      vidas--;
      tone(PIN_BUZZER, 150, 200);
      if (vidas <= 0) pantallaChoque();
      else {
        invencibleTimer = millis() + 1500;
        pilarX = 240; birdY = 160; momentum = 0;
        tft.fillRect(0, 31, 240, 289, ST77XX_WHITE); delay(50);
        tft.fillRect(0, 31, 240, 289, coloresFondo[indiceColor]);
      }
    }
  }
  delay(16);
}

// --- MENÚS Y PANTALLAS ---

void menuInicio() {
  tft.fillScreen(coloresFondo[0]);
  tft.setTextSize(6);
  tft.setTextColor(ST77XX_BLACK); tft.setCursor(33, 43); tft.print("DARIO");
  tft.setTextColor(C_BIRD); tft.setCursor(30, 40); tft.print("DARIO");
  dibujarPajaro(120, 120, C_BIRD);
  
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_RED); tft.setCursor(45, 160);
  tft.print("RECORD: "); tft.print(highScore);
  tft.setCursor(95, 185); tft.print(highName);
  tft.setTextColor(C_TEXTO); tft.setCursor(55, 250); tft.print("PULSA BOTON");

  unsigned long lastFlash = 0;
  bool visible = true;

  while(digitalRead(PIN_BOTON) == HIGH) {
    if (millis() - tiempoNotaMenu > 180) {
      tone(PIN_BUZZER, melodiaMenu[notaMenuActual], 80);
      notaMenuActual = (notaMenuActual + 1) % 6;
      tiempoNotaMenu = millis();
    }
    if (millis() - lastFlash > 500) {
      visible = !visible;
      tft.setTextSize(2); tft.setCursor(30, 215);
      tft.setTextColor(visible ? ST77XX_WHITE : coloresFondo[0]);
      tft.print("por LECTRONIC579");
      lastFlash = millis();
    }
    delay(10);
  }
  noTone(PIN_BUZZER);
  delay(200); 
  iniciarJuego();
}

void ingresarNombre() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_YELLOW); tft.setTextSize(3);
  tft.setCursor(15, 50); tft.print("NUEVO RECORD");
  char tempName[4] = "AAA"; int charIndex = 0;
  while(charIndex < 3) {
    tft.fillRect(0, 150, 240, 50, ST77XX_BLACK); tft.setTextSize(5); tft.setCursor(60, 150);
    for(int i=0; i<3; i++) {
      if(i == charIndex) tft.setTextColor(ST77XX_CYAN); else tft.setTextColor(ST77XX_WHITE);
      tft.print(tempName[i]);
    }
    while(digitalRead(PIN_BOTON) == HIGH); unsigned long p = millis();
    while(digitalRead(PIN_BOTON) == LOW); 
    if(millis() - p < 500) { 
       tempName[charIndex]++; if(tempName[charIndex] > 'Z') tempName[charIndex] = 'A'; tone(PIN_BUZZER, 1200, 20); 
    } else { 
       charIndex++; tone(PIN_BUZZER, 2000, 100); delay(200); 
    }
  }
  strcpy(highName, tempName); 
  EEPROM.put(0, highScore);
  EEPROM.write(4, highName[0]); EEPROM.write(5, highName[1]); EEPROM.write(6, highName[2]);
  EEPROM.commit();
}

void pantallaChoque() {
  juegoActivo = false; noTone(PIN_BUZZER); tone(PIN_BUZZER, 100, 600); 
  bool record = (puntuacion > highScore); if (record) highScore = puntuacion;

  tft.fillScreen(C_MUERTE);
  tft.setTextColor(ST77XX_RED); tft.setTextSize(5); tft.setCursor(20, 50); tft.print("HOSTIA!");
  tft.setTextColor(C_TEXTO); tft.setTextSize(3); tft.setCursor(30, 110);
  
  if (puntuacion <= 5) tft.print("HOSTIA!");
  else if (puntuacion <= 10) tft.print("VOLASTE");
  else if (puntuacion <= 15) tft.print("CAGON");
  else if (puntuacion <= 25) tft.print("PAQUETE");
  else if (puntuacion <= 30) tft.print("MAQUINA");
  else if (puntuacion <= 35) tft.print("EPICO");
  else if (puntuacion <= 40) tft.print("LEYENDA");
  else if (puntuacion <= 45) tft.print("PUTO AMO");
  else if (puntuacion <= 50) tft.print("CELESTIAL");
  else tft.print("JOPUTAAA");

  tft.setTextSize(2); tft.setCursor(35, 200); tft.print("PUNTOS: "); tft.print(puntuacion);
  delay(1500); if(record) ingresarNombre();
  
  tft.setTextSize(1); tft.setCursor(45, 280); tft.setTextColor(ST77XX_WHITE);
  tft.print("PULSA BOTON PARA REINTENTAR");
  while(digitalRead(PIN_BOTON) == HIGH); delay(200); menuInicio();
}

void pantallaCarga() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2); tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(45, 120); tft.print("Cargando juego"); 
  tft.drawRect(20, 150, 200, 20, ST77XX_WHITE);
  for (int i = 0; i < 196; i++) {
    tft.fillRect(22 + i, 152, 1, 16, C_BIRD);
    if (i % 25 == 0) tone(PIN_BUZZER, 200 + (i * 4), 20);
    delay(10); 
  }
}
