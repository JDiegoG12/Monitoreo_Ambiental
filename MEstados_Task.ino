#include "StateMachineLib.h"
#include "AsyncTaskLib.h"

// State Alias
enum State {
  INICIO = 0,
  MONITOREO = 1,
  ALARMA = 2,
  EVENTOS = 3,
  BLOQUEADO = 4
};

// Input Alias
enum Input {
  Sign_T = 0,
  Sign_P = 1,
  Sign_B = 2,
  Sign_K = 3,
  Unknown = 4
};

//Buzzer
int buzzer = 6;
#define beat 500  // = 60 s / 120 bpm * 1000 ms
#define NOTE_D4 294
#define NOTE_D5 587
#define NOTE_A4 440
#define NOTE_GS4 415
#define NOTE_G4 392
#define NOTE_F4 349
#define NOTE_B3 247
#define NOTE_D6 1175
#define NOTE_A5 880
#define NOTE_GS5 831
#define NOTE_G5 784
#define NOTE_F5 698
#define NOTE_E5 659
#define NOTE_D7 2349
#define NOTE_C6 1047
#define NOTE_A6 1760
#define NOTE_G6 1568
#define NOTE_D8 4699
#define NOTE_C7 2093

int melodiaBloqueo[] = {
  NOTE_D4, NOTE_D4, NOTE_D5, 0, NOTE_A4, 0, NOTE_GS4, 0, NOTE_G4, 0, NOTE_F4, NOTE_D4, NOTE_F4, NOTE_G4,
  NOTE_B3, NOTE_B3, NOTE_D5, 0, NOTE_A4, 0, NOTE_GS4, 0, NOTE_G4, 0, NOTE_F4, NOTE_D4, NOTE_F4, NOTE_G4,
  NOTE_D5, NOTE_D5, NOTE_D6, 0, NOTE_A5, 0, NOTE_GS5, 0, NOTE_G5, 0, NOTE_F5, NOTE_D5, NOTE_F5, NOTE_G5,
  NOTE_F5, NOTE_F5, NOTE_F5, 0, NOTE_F5, 0, NOTE_E5, NOTE_F5, NOTE_D5, NOTE_D5, NOTE_D5
};

double duracionesBloqueo[] = {
  0.25, 0.25, 0.25, 0.25, 0.25, 0.5, 0.25, 0.25, 0.25, 0.25, 0.5, 0.25, 0.25, 0.25,
  0.25, 0.25, 0.25, 0.25, 0.25, 0.5, 0.25, 0.25, 0.25, 0.25, 0.5, 0.25, 0.25, 0.25,
  0.25, 0.25, 0.25, 0.25, 0.25, 0.5, 0.25, 0.25, 0.25, 0.25, 0.5, 0.25, 0.25, 0.25,
  0.5, 0.25, 0.25, 0.25, 0.15, 0.10, 0.5, 0.5, 1.25, 0.25, 0.25
};


void play(int note, double note_val) {
  tone(buzzer, note);
  delay((beat * note_val) - 20);
  noTone(buzzer);
  delay(20);
}

void rest(double note_val) {
  delay(beat * note_val);
}


//LEDS
#define LED_RED 46
#define LED_BLUE 48
#define LED_GREEN 50

//Sensores INFRAROJO,HALL Y LUZ
#define SENSOR_IR 42
#define SENSOR_SH 44
const int photocellPin = A1;
//DHT
#include "DHT.h"           //DHT Libreria
#define DHTPIN 2           // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11      // DHT 21 (AM2301)
DHT dht(DHTPIN, DHTTYPE);  // Initialize DHT sensor.


//LCD
#include <LiquidCrystal.h>
const int rs = 12, en = 11, d4 = 10, d5 = 9, d6 = 8, d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);


//KeyPad
#include <Keypad.h>
const byte ROWS = 4;  // cuatro filas
const byte COLS = 4;  // cuatro columnas
char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};
byte rowPins[ROWS] = { 24, 26, 28, 30 };  // conectar a los pines de la fila
byte colPins[COLS] = { 32, 34, 36, 38 };  // conectar a los pines de la columna

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

const char* claveCorrecta = "1234";  // Define tu clave correcta aquí
char claveIngresada[4];              // Para almacenar la clave ingresada
unsigned char idx = 0;
int intentos = 0;
char key;
//Procedimientos
void read_Temperatura(void);
void read_Humedad(void);
void read_Luz(void);
void read_Hall(void);
void read_InfraRojo(void);
void timeout(void);
void contrasena(void);
void readKey_K(void);
void Inactivo(void);
void manejarTeclado(void);
//Tareas
AsyncTask TaskTemperatura(1000, true, read_Temperatura);
AsyncTask TaskHumedad(1000, true, read_Humedad);
AsyncTask TaskLuz(1000, true, read_Luz);
AsyncTask TaskHall(50, true, read_Hall);
AsyncTask TaskInfraRojo(50, true, read_InfraRojo);
AsyncTask TaskTiempo(3000, true, timeout);
AsyncTask TaskContrasena(100, true, contrasena);
AsyncTask TaskInactivo(5000, false, Inactivo);
AsyncTask TaskLedRedB_ON(500, []() {
  digitalWrite(LED_RED, HIGH);
});
AsyncTask TaskLedRedB_OFF(500, []() {
  digitalWrite(LED_RED, LOW);
});
AsyncTask TaskLedRedA_ON(150, []() {
  digitalWrite(LED_RED, HIGH);
});
AsyncTask TaskLedRedA_OFF(150, []() {
  digitalWrite(LED_RED, LOW);
});
AsyncTask TaskLedGreen_ON(1000, false, []() {
  digitalWrite(LED_GREEN, HIGH);
});
AsyncTask TaskLedGreen_OFF(1000, false, []() {
  digitalWrite(LED_GREEN, LOW);
});
AsyncTask TaskLedBlue_OFF(1000, false, []() {
  digitalWrite(LED_BLUE, LOW);
});
AsyncTask TaskLeerK(100, true, readKey_K);



//Buzzer


// Create new StateMachine
StateMachine stateMachine(5, 8);  //Primero el numero de estados (5), después el numero de transiciones (8)

// Stores last user input
Input input;

/*----------------------------------SET UP SM----------------------------------*/  //SM -> STATE MACHINE
void setupStateMachine() {
  /*----------------------------------TRANSICIONES SM----------------------------------*/
  stateMachine.AddTransition(INICIO, MONITOREO, []() {
    return input == Sign_P;
  });
  stateMachine.AddTransition(INICIO, BLOQUEADO, []() {
    return input == Sign_B;
  });

  stateMachine.AddTransition(MONITOREO, ALARMA, []() {
    return input == Sign_P;
  });
  stateMachine.AddTransition(MONITOREO, EVENTOS, []() {
    return input == Sign_T;
  });

  stateMachine.AddTransition(ALARMA, INICIO, []() {
    return input == Sign_K;
  });

  stateMachine.AddTransition(EVENTOS, MONITOREO, []() {
    return input == Sign_T;
  });
  stateMachine.AddTransition(EVENTOS, ALARMA, []() {
    return input == Sign_P;
  });

  stateMachine.AddTransition(BLOQUEADO, INICIO, []() {
    return input == Sign_T;
  });


  /*----------------------------------ACCIONES ENTRADA----------------------------------*/
  stateMachine.SetOnEntering(INICIO, funct_Inicio);
  stateMachine.SetOnEntering(MONITOREO, funct_Monitoreo);
  stateMachine.SetOnEntering(ALARMA, funct_Alarma);
  stateMachine.SetOnEntering(BLOQUEADO, funct_Bloqueado);
  stateMachine.SetOnEntering(EVENTOS, funct_Eventos);

  /*----------------------------------ACCIONES SALIDA----------------------------------*/
  stateMachine.SetOnLeaving(INICIO, outputI);
  stateMachine.SetOnLeaving(MONITOREO, outputM);
  stateMachine.SetOnLeaving(ALARMA, outputA);
  stateMachine.SetOnLeaving(BLOQUEADO, outputB);
  stateMachine.SetOnLeaving(EVENTOS, outputE);
}
/*----------------------------------SALIDA INICIO----------------------------------*/
void outputI(void) {
  digitalWrite(LED_BLUE, LOW);
  TaskInactivo.Stop();
  TaskContrasena.Stop();
  TaskTiempo.Stop();
  Serial.println("\n Leaving Inicio");
}
/*----------------------------------SALIDA MONITOREO----------------------------------*/
void outputM(void) {
  TaskTiempo.Stop();
  TaskTemperatura.Stop();
  TaskHumedad.Stop();
  TaskLuz.Stop();
  lcd.clear();
  Serial.println("\n Leaving Monitoreo");
}
/*----------------------------------SALIDA ALARMA----------------------------------*/
void outputA(void) {
  TaskLeerK.Stop();       // Detener la tarea de leer tecla
  TaskLedRedA_ON.Stop();  // Detener parpadeo
  TaskLedRedA_OFF.Stop();
  digitalWrite(LED_RED, LOW);  // Apagar el LED
  lcd.clear();
  Serial.println("\n Leaving Alarma");
}



/*----------------------------------SALIDA BLOQUEADO----------------------------------*/
void outputB(void) {
  TaskLedRedB_ON.Stop();
  TaskLedRedB_OFF.Stop();
  digitalWrite(LED_RED, LOW);
  intentos = 0;
  lcd.clear();
  Serial.println("\n Leaving Bloqueado");
}
/*----------------------------------SALIDA EVENTOS----------------------------------*/
void outputE(void) {
  lcd.clear();
  TaskHall.Stop();
  TaskInfraRojo.Stop();
  TaskTiempo.Stop();
  Serial.println("\nLeaving Eventos");
}

/*----------------------------------LEER INPUT----------------------------------*/
int readInput() {
  Input currentInput = Input::Unknown;
  if (Serial.available()) {
    char incomingChar = Serial.read();

    switch (incomingChar) {
      case 'T': currentInput = Input::Sign_T; break;
      case 'P': currentInput = Input::Sign_P; break;
      case 'B': currentInput = Input::Sign_B; break;
      case 'K': currentInput = Input::Sign_K; break;
      default: break;
    }
  }

  return currentInput;
}

/*----------------------------------ENTRADA INICIO----------------------------------*/
void funct_Inicio() {
  lcd.clear();  // Limpiamos la pantalla al entrar al estado inicial
  idx = 0;      // Aseguramos que el índice esté en 0
  strcpy(claveIngresada, "");
  intentos = 0;
  TaskContrasena.Start();
}
/*----------------------------------ENTRADA MONITOREO----------------------------------*/
void funct_Monitoreo() {
  TaskContrasena.Stop();
  TaskTemperatura.Start();
  TaskHumedad.Start();
  TaskLuz.Start();
  TaskTiempo.SetIntervalMillis(5000);
  TaskTiempo.Start();
}
/*----------------------------------ENTRADA ALARMA----------------------------------*/

void funct_Alarma() {
  lcd.clear();
  lcd.print("Alarma");
  TaskLedRedA_ON.Start();
  TaskLedRedA_OFF.Start();
  TaskLeerK.Start();
}
/*----------------------------------ENTRADA BLOQUEADO----------------------------------*/

void funct_Bloqueado() {
  lcd.clear();
  lcd.print("Bloqueado");
  TaskLedRedB_ON.SetIntervalMillis(500);
  TaskLedRedB_OFF.SetIntervalMillis(500);
  TaskLedRedB_ON.Reset();   // Reiniciar tarea
  TaskLedRedB_OFF.Reset();  // Reiniciar tarea
  TaskLedRedB_ON.Start();

  TaskTiempo.SetIntervalMillis(7000);
  TaskTiempo.Start();
  unsigned long currentTime = millis();
  while (millis() - currentTime < 5000) {
    digitalWrite(LED_RED, LOW);
    for (int i = 0; i < sizeof(melodiaBloqueo) / sizeof(int); i++) {
      digitalWrite(LED_RED, HIGH);
      if (melodiaBloqueo[i] == 0) {
        rest(duracionesBloqueo[i]);
      } else {
        play(melodiaBloqueo[i], duracionesBloqueo[i]);
      }
    }
  }
}

/*----------------------------------ENTRADA EVENTOS----------------------------------*/
void funct_Eventos() {
  TaskInfraRojo.Start();
  TaskHall.Start();
  lcd.clear();
  lcd.print("InfraRojo:");
  lcd.setCursor(0, 1);
  lcd.print("Hall:");


  TaskTiempo.SetIntervalMillis(3000);
  TaskTiempo.Start();
}
/*----------------------------------LEER TEMPERATURA----------------------------------*/
void read_Temperatura(void) {
  float tempC = dht.readTemperature();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp:");
  lcd.setCursor(6, 0);
  lcd.print(tempC);
  if (tempC > 25) {
    input = Input::Sign_P;
  }
}
/*----------------------------------LEER HUMEDAD----------------------------------*/
void read_Humedad(void) {
  float humedad = dht.readHumidity();
  lcd.setCursor(8, 1);
  lcd.print("H:");
  lcd.setCursor(10, 1);
  lcd.print(humedad);
}
/*----------------------------------LEER LUZ----------------------------------*/
int luz = 0;
void read_Luz(void) {
  luz = analogRead(photocellPin);
  lcd.setCursor(0, 1);
  lcd.print("Luz:");
  lcd.setCursor(4, 1);
  lcd.print(luz);
}
/*----------------------------------LEER HALL----------------------------------*/
void read_Hall(void) {
  if (digitalRead(SENSOR_SH) == 1) {  //1 prendido
    digitalWrite(LED_BLUE, HIGH);
    lcd.setCursor(7, 1);
    lcd.print("ON");
    Serial.println("SENSOR_SH");
    //input = Input::Sign_P;
  } else {
    lcd.setCursor(7, 1);
    lcd.print("OFF");
    digitalWrite(LED_BLUE, LOW);
  }
}
/*----------------------------------LEER INFRA ROJO----------------------------------*/
void read_InfraRojo(void) {
  if (digitalRead(SENSOR_IR) == 0) {  //0 prendido
    digitalWrite(LED_RED, HIGH);
    lcd.setCursor(11, 0);
    lcd.print("ON");
    input = Input::Sign_P;
  } else {
    digitalWrite(LED_RED, LOW);
    lcd.setCursor(11, 0);
    lcd.print("OFF");
  }
}

/*----------------------------------TIME OUT----------------------------------*/
void timeout(void) {
  input = Input::Sign_T;
}
/*----------------------------------RESET DE PANTALLA (CONTRASEÑA)----------------------------------*/
void reset(void) {
  idx = 0;
  strcpy(claveIngresada, "");
  lcd.clear();
  lcd.print("Ingrese clave:");
}
/*----------------------------------INGRESAR CONTRASEÑA----------------------------------*/
void contrasena(void) {
  lcd.setCursor(0, 0);
  lcd.print("Ingrese clave:");
  key = keypad.getKey();
  if (key) {
    TaskInactivo.Start();
    if (key == '#' && idx > 0) {
      // Si se presiona '#', se valida la clave
      claveIngresada[idx] = '\0';  // Terminar la cadena
      if (strcmp(claveIngresada, claveCorrecta) == 0) {
        lcd.clear();
        lcd.print("Clave Correcta");
        digitalWrite(LED_GREEN, HIGH);
        TaskLedGreen_OFF.Reset();
        TaskLedGreen_OFF.Start();
        input = Input::Sign_P;
        return;
      } else {
        intentos++;
        digitalWrite(LED_BLUE, HIGH);
        TaskLedBlue_OFF.Reset();
        TaskLedBlue_OFF.Start();
        if (intentos >= 3) {
          input = Input::Sign_B;
          return;
        }
        reset();
      }
    } else if (idx < 8 && key != '*') {  // Solo aceptar caracteres hasta 8
      claveIngresada[idx++] = key;       // Guardar la clave
      lcd.setCursor(idx - 1, 1);
      lcd.print('*');  // Mostrar el caracter enmascarado
    }
  }
}
/*----------------------------------LEER #----------------------------------*/
void readKey_K(void) {
  char key_k = keypad.getKey();
  if (key_k == '#') {       // Si se presiona '#'
    TaskLedRedA_ON.Stop();  // Detener parpadeo
    TaskLedRedA_OFF.Stop();
    digitalWrite(LED_RED, LOW);  // Asegurarse de apagar el LED
    input = Input::Sign_K;       // Cambiar estado
  }
}
/*----------------------------------INACTIVO----------------------------------*/

void Inactivo(void) {
  lcd.setCursor(0, 0);
  intentos++;
  digitalWrite(LED_BLUE, HIGH);
  delay(1000);
  digitalWrite(LED_BLUE, LOW);
  idx = 0;
  reset();
}

void manejarTeclado() {
  char tecla = keypad.getKey();

  if (tecla) {
    TaskInactivo.Start();
    lcd.setCursor(idx, 1);
    lcd.print("*");
    claveIngresada[idx++] = tecla;
  }
}

/*----------------------------------SET UP----------------------------------*/
void setup() {
  pinMode(buzzer, OUTPUT);  // Configura el pin del buzzer
  pinMode(SENSOR_IR, INPUT);
  pinMode(SENSOR_SH, INPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  lcd.begin(16, 2);
  Serial.begin(9600);

  Serial.println("Starting State Machine...");
  setupStateMachine();
  Serial.println("Start Machine Started");
  dht.begin();

  // Initial state
  stateMachine.SetState(INICIO, false, true);
}
/*----------------------------------LOOP----------------------------------*/
void loop() {
  // Leer entrada del usuario
  input = static_cast<Input>(readInput());

  // Actualizar todas las tareas
  TaskContrasena.Update();
  TaskTemperatura.Update();
  TaskHumedad.Update();
  TaskHall.Update();
  TaskLuz.Update();
  TaskInfraRojo.Update();
  TaskTiempo.Update();

  // Actualizar las tareas de parpadeo del LED rojo
  TaskLedRedB_ON.Update(TaskLedRedB_OFF);
  TaskLedRedB_OFF.Update(TaskLedRedB_ON);
  TaskLedRedA_ON.Update(TaskLedRedA_OFF);
  TaskLedRedA_OFF.Update(TaskLedRedA_ON);
  TaskLedGreen_ON.Update();
  TaskLedGreen_OFF.Update();
  TaskLedBlue_OFF.Update();
  TaskLeerK.Update();
  TaskInactivo.Update();;
  // Actualizar la máquina de estados
  stateMachine.Update();
  input = Input::Unknown;
}
