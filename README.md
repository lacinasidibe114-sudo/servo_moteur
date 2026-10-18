```markdown
# Servo Moteur (ROS 2 Hardware Interface)

Ce package fournit une interface matérielle `ros2_control` pour contrôler une tête robotique (Pan/Tilt) équipée de deux servomoteurs, pilotés par un Arduino.

## 🏗️ Architecture
- **Microcontrôleur** : Arduino (réception des angles en degrés via le port série).
- **Communication** : USB Série via `libserial`.
- **ROS 2** : Humble / Jazzy.
- **Contrôleur** : `forward_command_controller/ForwardCommandController` (commande en position).

## ⚙️ Fonctionnalités
- **Anti-jitter** : Les commandes ne sont envoyées à l'Arduino que si la position change (évite les tremblements des servos).
- **Watchdog Arduino** : L'Arduino ne renvoie son état que s'il a bougé.

## 🚀 Compilation
```bash
colcon build --packages-select servo_moteur
source install/setup.bash
```

## 🎮 Utilisation
```bash
ros2 launch servo_moteur servo_test.launch.py
```

Envoyer une position (en radians) :
```bash
ros2 topic pub --once /head_controller/commands std_msgs/msg/Float64MultiArray "{data: [0.5, 0.2]}"
```

## 👨‍💻 Auteur
Lacina Sidibe (PFE 2027)
```

### 🚀 Vérification
Allez sur votre page GitHub. Vous devriez voir :
*   Les dossiers `src`, `include`, `urdf`, `config`, `launch`.
*   Le fichier `CMakeLists.txt`, `package.xml`, `servo_moteur.xml`.
*   **Pas de dossiers `build` ou `install`**.

Si vous avez des questions sur une étape, dites-le moi !
