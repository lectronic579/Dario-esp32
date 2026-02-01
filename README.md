Proyecto modificado del Flappy bird ESP32 para mi hijo Dario.

Sistema de puntuación y ranking
Rangos "curiosos" según puntuación obtenida
2 vidas únicamente
Dificultad progresiva
Pantalla grande de 2,4" con driver ST7789
Único botón para TODO
Sonido por Buzzer ligeramente aumentado con transistor BC547, resistencia de 330 ohms y condensador de 100 uF
Gracias a la IA de Gemini por el trabajito realizado.

Esquema Detallado de Conexiones
________________________________________

1. Sistema de Audio (Amplificado)
Esta es la parte crítica para que el volumen sea máximo.
•	Buzzer (+): Conectado a pin VIN (5V)
•	Buzzer (-): Conectado a Colector del BC547
•	Transistor BC547:
o	Base: Resistencia de 330 ohms y a Pin 12
o	Emisor: Directo a GND
•	Condensador (100 uF): Entre VIN y GND

2. Pantalla TFT (ST7789)
Sigue este orden de pines para que coincida con tu código:
•	GND: GND
•	VCC: VIN (5 V)
•	SCL (Reloj): Pin 18
•	SDA (Datos): Pin 23
•	RES (Reset): Pin 4
•	DC (Data/Command): Pin 2
•	CS (Chip Select): Pin 5
•	BL (Backlight): Pin 21

3. Entrada de Control (Botón)
•	Pin 1: Al Pin 13
•	Pin 2: A GND
