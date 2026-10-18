#include <Servo.h>

Servo panServo;
Servo tiltServo;

bool has_changed = false;
float last_pan = 90.0;
float last_tilt = 90.0;

void setup() {
  Serial.begin(115200);
  
  panServo.attach(9);
  tiltServo.attach(10);
  
  panServo.write(90);
  tiltServo.write(90);
}

void loop() {
  // 1. Lire la commande ROS 2 : "P90.0 T45.0\n"
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    
    int p_idx = cmd.indexOf('P');
    int t_idx = cmd.indexOf('T');
    
    if (p_idx != -1 && t_idx != -1) {
      float pan_angle = cmd.substring(p_idx + 1, t_idx).toFloat();
      float tilt_angle = cmd.substring(t_idx + 1).toFloat();
      
      pan_angle = constrain(pan_angle, 0.0, 180.0);
      tilt_angle = constrain(tilt_angle, 0.0, 180.0);
      
      // Anti-jitter : on ne bouge que si le changement est significatif
      if (abs(pan_angle - last_pan) > 0.1 || abs(tilt_angle - last_tilt) > 0.1) {
        panServo.write((int)pan_angle);
        tiltServo.write((int)tilt_angle);
        last_pan = pan_angle;
        last_tilt = tilt_angle;
        has_changed = true; 
      }
    }
  }

  // 2. Renvoyer l'état UNIQUEMENT si on a bougé (évite de saturer le port série)
  if (has_changed) {
    Serial.print("P"); Serial.print((int)last_pan);
    Serial.print(" T"); Serial.println((int)last_tilt);
    has_changed = false;
  }
  
  delay(20);
}