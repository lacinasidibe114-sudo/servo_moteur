```markdown
# 🤖 Servo Moteur : Interface Matérielle ROS 2 (Tête Pan/Tilt)

Ce package fournit une interface matérielle (`ros2_control` SystemInterface) pour contrôler une tête robotique à deux axes (Pan/Tilt) à l'aide de servomoteurs standards et d'un Arduino. 

Ce projet a été développé dans le cadre de l'apprentissage de l'interfaçage matériel réel sous ROS 2 (PFE 2027).

## 📐 Architecture du Système

Le système est divisé en deux parties qui communiquent via un port série (USB).

```text
+-------------------------------------------------------------+
|                       ORDINATEUR (ROS 2)                    |
|                                                             |
|  [Commande Utilisateur]                                     |
|         │ (ex: 0.5 rad)                                     |
|         ▼                                                   |
|  [forward_command_controller]                               |
|         │                                                   |
|         ▼                                                   |
|  [servo_moteur (C++ Hardware Interface)]                     |
|         │                                                   |
|         │ 1. Conversion Rad -> Deg                           |
|         │ 2. Anti-jitter (n'envoie que si ça change)         |
|         ▼                                                   |
|  [libserial] ──────────────┐                                |
+----------------------------|--------------------------------+
                             | USB Série (115200 bauds)
                             | Trame: "P90.0 T45.0\n"
                             ▼
+-------------------------------------------------------------+
|                       ARDUINO (C++)                         |
|                                                             |
|  [Réception Série] ──> [Servo.h]                            |
|                             │                               |
|                             ├──> [Servo Pan]  (Broche 9)    |
|                             └──> [Servo Tilt] (Broche 10)   |
|                                                             |
|  [Envoi Série] <── "P90 T45\n" (Uniquement si changement)   |
+-------------------------------------------------------------+
```

## 📂 Structure du Package

```text
servo_moteur/
├── CMakeLists.txt              # Fichier de compilation
├── package.xml                 # Dépendances ROS 2
├── servo_moteur.xml            # Déclaration du plugin hardware
├── config/
│   └── controllers.yaml        # Configuration des contrôleurs
├── include/servo_moteur/
│   └── servo_interface.hpp     # Header C++ (déclaration)
├── launch/
│   └── servo_test.launch.py    # Lancement du nœud et des contrôleurs
├── src/
│   └── servo_interface.cpp     # Code C++ (logique de communication)
└── urdf/
    ├── servo_robot.urdf.xacro  # Description du robot (URDF)
    └── servo_control.xacro     # Configuration ros2_control
```

## 🔌 Câblage Matériel (Arduino)

| Composant | Broche Arduino | Type |
| :--- | :--- | :--- |
| **Servo Pan** (Signal) | D9 | PWM |
| **Servo Tilt** (Signal) | D10 | PWM |
| **Alimentation Servos** | 5V (Externe recommandé) | VCC |
| **Masse (GND)** | GND | GND |

*(⚠️ Attention : Pour deux servomoteurs, il est fortement recommandé d'utiliser une alimentation externe 5V pour éviter de griller le régulateur de l'Arduino).*

## ⚙️ Fonctionnalités Clés

1. **Conversion d'Unités** : ROS 2 utilise les radians, l'Arduino utilise les degrés. La conversion est faite automatiquement dans le C++.
2. **Anti-Jitter (Anti-Tremblement)** : Le servo standard a tendance à trembler si on lui envoie la même position 100 fois par seconde. Le code C++ filtre les commandes : il n'envoie à l'Arduino que si la position a réellement changé.
3. **Watchdog Arduino** : L'Arduino ne renvoie son état (Position) qu'après avoir bougé, évitant ainsi de saturer le port série.

## 🚀 Installation et Compilation

Assurez-vous d'avoir ROS 2 (Humble/Jazzy) et la librairie `libserial` installés :
```bash
sudo apt install ros-$ROS_DISTRO-controller-manager \
                 ros-$ROS_DISTRO-forward-command-controller \
                 ros-$ROS_DISTRO-joint-state-broadcaster \
                 libserial-dev
```

Compilez le package :
```bash
cd ~/votre_workspace
colcon build --packages-select servo_moteur
source install/setup.bash
```

## 🎮 Utilisation

**1. Lancer le système :**
```bash
ros2 launch servo_moteur servo_test.launch.py
```

**2. Envoyer une commande de position (en radians) :**
```bash
# Format: [Pan, Tilt]
ros2 topic pub --once /head_controller/commands std_msgs/msg/Float64MultiArray "{data: [0.5, 0.2]}"
```

**3. Vérifier l'état des joints :**
```bash
ros2 topic echo /joint_states
```

## 🤖 Utilisation avancée : Nœud de Gestes (Optionnel)
Si vous avez un nœud Python pour faire des gestes (hochement de tête, etc.), il peut publier sur `/head_controller/commands`. 
*(Voir le code du nœud `head_gestures.py` dans le dossier `scripts/` si inclus).*

## 👨‍💻 Auteur
**Lacina Sidibe** - Étudiant en Robotique (PFE 2027)
```

### Pourquoi ce README est meilleur :
1.  **Diagramme ASCII** : Il montre exactement le flux de données, de la commande ROS 2 jusqu'au servo physique. C'est très visuel.
2.  **Câblage** : Un tableau clair indique où brancher les fils. C'est indispensable pour quelqu'un qui veut reproduire votre projet.
3.  **Arborescence** : Les gens voient immédiatement où se trouve chaque fichier.
4.  **Fonctionnalités clés** : Il met en avant vos efforts (l'anti-jitter), ce qui montre que vous avez réfléchi à un vrai problème d'ingénierie.

**Copiez-collez ce texte dans votre fichier `README.md` sur GitHub, et votre projet aura l'air très professionnel !**
