#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <DHT.h>

#define echo 9
#define trig 10
#define right_ir 11
#define left_ir 12
#define ENA 5
#define IN1 7
#define IN2 2
#define IN3 4
#define IN4 8
#define ENB 6
#define button A1
#define DHTPIN A0
#define DHTTYPE DHT11 

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo my_servo;
bool isAutoMode = true;
int lastButtonState = HIGH;
DHT dht(DHTPIN, DHTTYPE);
unsigned long lastTempUpdate = 0;
float currentTemp = 0.0;
int speed = 80;
int rotation_speed = 180;

void setup() {
  my_servo.attach(3);
  my_servo.write(90); 
  lcd.init();
  lcd.backlight();
  updateLCD();
  
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(right_ir, INPUT);
  pinMode(left_ir, INPUT);
  pinMode(button, INPUT_PULLUP);
  
  dht.begin();
  Serial.begin(9600);
}

void loop() {
  // 1. Read Temperature every 2 seconds
  if (millis() - lastTempUpdate >= 2000) {
    lastTempUpdate = millis();
    float t = dht.readTemperature();
    if (!isnan(t)) {
      currentTemp = t;

        lcd.setCursor(9, 1);
        lcd.print("T:");
        lcd.print((int)currentTemp);
        lcd.print("C");
    }
  }

  // 2. Check Button State to switch modes
  int buttonState = digitalRead(button);
  if (buttonState == LOW && lastButtonState == HIGH) {
    isAutoMode = !isAutoMode;
    stopCar();
    lcd.clear();
    updateLCD();
    delay(200); 
  }
  lastButtonState = buttonState;

  // 3. Execute Current Mode
  if (isAutoMode) {
    int distance = getDistance();
    
    // Display Distance if too close
    if (distance < 30) {
      lcd.setCursor(0, 1);
      lcd.print("D=");
      lcd.print(distance);
      lcd.print("cm ");
    } else {
      lcd.setCursor(0, 1);
      lcd.print("       ");
    }

    // Obstacle Avoidance vs Line Following Logic
    if (distance > 0 && distance < 15) {
      moveBackward();
      delay(400);
      avoidObstacle();
    } else {
      int right_ir_state = digitalRead(right_ir);
      int left_ir_state = digitalRead(left_ir);
      
      // Assuming Black line is centered (both sensors on White = LOW)
      if (right_ir_state == LOW && left_ir_state == LOW) {
          moveForward(); 
          lcd.setCursor(9, 0); lcd.print("FWD    ");
      } else if (right_ir_state == LOW && left_ir_state == HIGH) { 
        turnLeft();
        lcd.setCursor(9, 0);
        lcd.print("LEFT   ");
      } else if (right_ir_state == HIGH && left_ir_state == LOW) { 
        turnRight();
        lcd.setCursor(9, 0);
        lcd.print("RIGHT  ");
      } else if (right_ir_state == HIGH && left_ir_state == HIGH) {
        stopCar();
        lcd.setCursor(9, 0);
        lcd.print("STOP   ");
      }
    }
  } else {
    runBluetoothMode();
  }
}

// ================== Core Functions ==================

void updateLCD() {
  if (isAutoMode) {
    lcd.setCursor(0, 0);
    lcd.print("AUTO-M");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("B-MODE");
  }
}

int getDistance() {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long duration = pulseIn(echo, HIGH, 30000); 
  int distance = duration * 0.034 / 2;
  if (distance == 0) distance = 999;
  return distance;
}

void runBluetoothMode() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    switch (cmd) {
      case 'F': BmoveForward(); break;
      case 'B': BmoveBackward(); break;
      case 'L': BturnLeft(); break;
      case 'R': BturnRight(); break;
      case 'S': stopCar(); break;
    }
  }
}

// ================== Movement Functions ==================

void moveForward() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, speed); analogWrite(ENB, speed);
}

void moveBackward() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  analogWrite(ENA, speed); analogWrite(ENB, speed);
}

void turnRight() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, rotation_speed); analogWrite(ENB, rotation_speed);
}


void turnLeft() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  analogWrite(ENA, rotation_speed); analogWrite(ENB, rotation_speed);
}


void stopCar() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  analogWrite(ENA, 0); analogWrite(ENB, 0);
}
void BmoveForward (){
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, 210); analogWrite(ENB, 210);
}
void BmoveBackward (){
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  analogWrite(ENA, 210); analogWrite(ENB, 210);
}
void BturnRight (){
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, 230); analogWrite(ENB, 230);
}
void BturnLeft (){
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  analogWrite(ENA, 230); analogWrite(ENB, 230);
}

// ================== Obstacle Avoidance ==================

void avoidObstacle() {
  stopCar();
  lcd.setCursor(8, 0); 
  lcd.print("AVOIDING");

  // 1. Scan Left and Right
  my_servo.write(10); 
  delay(600);
  int rightDist = getDistance();

  my_servo.write(170); 
  delay(600);
  int leftDist = getDistance();

  my_servo.write(90); 
  delay(350);

  if (rightDist > leftDist) {
    
    turnRight(); delay(400); 
    stopCar(); delay(200);
    
    moveForward(); delay(1200); 
    stopCar(); delay(200);
    
    turnLeft(); delay(400); 
    stopCar(); delay(200);
    
    moveForward(); delay(900); 
    stopCar(); delay(200);
    
    turnLeft(); delay(300); 
    stopCar(); delay(200);
    
    moveForward();
    while(digitalRead(right_ir) == LOW && digitalRead(left_ir) == LOW) {
    }
    stopCar(); delay(100);

  } else {
    
    turnLeft(); delay(400); 
    stopCar(); delay(200);
    
    moveForward(); delay(1200); 
    stopCar(); delay(200);
    
    turnRight(); delay(400); 
    stopCar(); delay(200);
    
    moveForward(); delay(900); 
    stopCar(); delay(200);
    
    turnRight(); delay(300); 
    stopCar(); delay(200);
    
    moveForward();
    while(digitalRead(right_ir) == LOW && digitalRead(left_ir) == LOW) {
    }
    stopCar(); delay(100);
  }
  
  lcd.clear();
  updateLCD();

}