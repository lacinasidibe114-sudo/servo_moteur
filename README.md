# `servo_moteur`

> **Interface ROS 2 `ros2_control` pour une tête robotique Pan/Tilt**

`servo_moteur` est un package ROS 2 permettant de piloter deux servomoteurs standards pour réaliser les axes **Pan** et **Tilt** d'une tête robotique.

Le système utilise :

- un Arduino comme contrôleur bas niveau ;
- deux servomoteurs pilotés avec la bibliothèque Arduino `Servo` ;
- une liaison USB série à `115200 bauds` ;
- une interface matérielle C++ `hardware_interface::SystemInterface` ;
- `ros2_control` et `forward_command_controller` ;
- une conversion automatique radians ROS 2 ↔ degrés Arduino ;
- un filtrage anti-jitter côté PC et côté Arduino.

> **Statut :** prototype d'intégration matérielle. Testez les servomoteurs sans charge, avec une alimentation adaptée et un arrêt électrique accessible avant toute installation sur un robot.

## Sommaire

- [Vue d'ensemble](#vue-densemble)
- [Architecture](#architecture)
- [Structure du dépôt](#structure-du-dépôt)
- [Prérequis](#prérequis)
- [Installation](#installation)
- [Câblage](#câblage)
- [Préparer et téléverser le firmware](#préparer-et-téléverser-le-firmware)
- [Compiler le package](#compiler-le-package)
- [Lancer le système](#lancer-le-système)
- [Interfaces ROS 2](#interfaces-ros-2)
- [Protocole série](#protocole-série)
- [Configuration](#configuration)
- [Conversion des unités et limites](#conversion-des-unités-et-limites)
- [Dépannage](#dépannage)
- [Validation et développement](#validation-et-développement)
- [Limites connues](#limites-connues)
- [Sécurité](#sécurité)
- [Licence](#licence)

## Vue d'ensemble

```text
┌────────────────────────┐
│ Commande ROS 2         │
│ ForwardCommandControl  │
└───────────┬────────────┘
            │ positions en radians
            ▼
┌─────────────────────────────────────┐
│ ServoInterface                      │
│ C++ / ros2_control                 │
│                                     │
│ - lit les commandes de position     │
│ - radians → degrés                  │
│ - applique l'anti-jitter            │
│ - écrit la trame série              │
│ - lit le retour de position         │
└───────────┬─────────────────────────┘
            │ USB série, 115200 bauds
            ▼
┌─────────────────────────────────────┐
│ Arduino                             │
│                                     │
│ - parse P<pan> T<tilt>              │
│ - borne les angles à 0–180°         │
│ - commande Servo Pan sur D9         │
│ - commande Servo Tilt sur D10       │
│ - renvoie l'état après mouvement    │
└───────────┬─────────────────────────┘
            │ PWM servo
            ▼
┌─────────────────────────────────────┐
│ Tête robotique Pan/Tilt             │
│                                     │
│ neck_pan_joint / neck_tilt_joint   │
└─────────────────────────────────────┘
```

### Flux de commande

1. Une commande de position est envoyée au contrôleur `head_controller`.
2. `ros2_control` transmet les deux positions à `ServoInterface`.
3. Le plugin convertit les radians en degrés autour de la position neutre 90°.
4. Une trame `P<pan> T<tilt>\n` est envoyée à l'Arduino uniquement si la commande change suffisamment.
5. L'Arduino limite chaque angle entre 0° et 180°.
6. Les servomoteurs sont commandés sur les broches D9 et D10.
7. Après un mouvement significatif, l'Arduino renvoie la position appliquée.
8. Le plugin reconvertit les degrés en radians et met à jour les états des joints.

## Architecture

### Composants

| Composant | Technologie | Responsabilité |
| --- | --- | --- |
| `arduino/code.ino` | Arduino C++ | Réception série et commande des servos |
| `ServoInterface` | C++ / `ros2_control` | Adaptation série et conversion d'unités |
| `servo_control.xacro` | URDF/Xacro | Déclaration du plugin et des joints |
| `servo_robot.urdf.xacro` | URDF/Xacro | Modèle minimal de la tête |
| `head_controller` | `forward_command_controller` | Envoi des positions de Pan/Tilt |
| `joint_state_broadcaster` | `ros2_control` | Publication des états de joints |
| `servo_test.launch.py` | ROS 2 Launch | Démarrage du contrôle et des contrôleurs |

### Cycle de vie du plugin

| Méthode | Responsabilité |
| --- | --- |
| `on_init()` | Lit le port série et initialise les deux joints |
| `on_activate()` | Ouvre et configure le port série |
| `export_state_interfaces()` | Exporte la position des deux joints |
| `export_command_interfaces()` | Exporte la commande de position |
| `read()` | Lit une trame de retour et met à jour les positions |
| `write()` | Convertit et transmet une nouvelle commande |
| `on_deactivate()` | Ferme le port série |

## Structure du dépôt

```text
servo_moteur/
├── README.md
├── LICENSE
├── CMakeLists.txt
├── package.xml
├── servo_moteur.xml                  # Manifest pluginlib
├── arduino/
│   └── code.ino                      # Firmware Arduino
├── config/
│   └── controllers.yaml              # Contrôleurs ros2_control
├── include/
│   └── servo_moteur/
│       └── servo_interface.hpp       # Déclaration C++ du plugin
├── launch/
│   └── servo_test.launch.py          # Launch de test
├── src/
│   └── servo_interface.cpp           # Implémentation du plugin
└── urdf/
    ├── servo_control.xacro            # Bloc ros2_control
    └── servo_robot.urdf.xacro         # Modèle Pan/Tilt
```

## Prérequis

### Matériel

- deux servomoteurs compatibles avec la bibliothèque Arduino `Servo` ;
- Arduino compatible ;
- alimentation externe 5 V adaptée au courant des deux servos ;
- câble USB de données ;
- structure mécanique Pan/Tilt ;
- masse commune entre l'Arduino et l'alimentation des servos.

### Logiciels

- Ubuntu compatible avec votre distribution ROS 2 ;
- ROS 2 Humble, Jazzy ou distribution compatible ;
- `colcon`, `rosdep` et `controller_manager` ;
- Arduino IDE ou Arduino CLI ;
- bibliothèque Arduino `Servo` ;
- LibSerial.

### Dépendances ROS 2

Le package utilise :

- `ament_cmake` ;
- `rclcpp` ;
- `rclcpp_lifecycle` ;
- `hardware_interface` ;
- `pluginlib` ;
- `controller_manager` ;
- `forward_command_controller` ;
- `joint_state_broadcaster`.

## Installation

### 1. Créer le workspace

```bash
mkdir -p ~/servo_ws/src
cd ~/servo_ws/src
git clone https://github.com/lacinasidibe114-sudo/servo_moteur.git
cd ~/servo_ws
```

### 2. Installer les dépendances système

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  pkg-config \
  libserial-dev \
  python3-colcon-common-extensions \
  python3-rosdep
```

Installer les paquets ROS 2 :

```bash
sudo apt install -y \
  ros-$ROS_DISTRO-ros2-control \
  ros-$ROS_DISTRO-ros2-controllers \
  ros-$ROS_DISTRO-controller-manager \
  ros-$ROS_DISTRO-forward-command-controller \
  ros-$ROS_DISTRO-joint-state-broadcaster \
  ros-$ROS_DISTRO-xacro
```

### 3. Installer les dépendances ROS

```bash
source /opt/ros/$ROS_DISTRO/setup.bash
rosdep update
rosdep install --from-paths src --ignore-src -r -y
```

## Câblage

### Signaux de commande

Le firmware actuel utilise les broches suivantes :

| Composant | Arduino | Fonction |
| --- | --- | --- |
| Signal servo Pan | D9 | PWM |
| Signal servo Tilt | D10 | PWM |
| Masse signal | GND | Référence commune |

```text
Arduino D9  ───────── Signal servo Pan
Arduino D10 ───────── Signal servo Tilt
Arduino GND ───────── GND alimentation servo
```

### Alimentation

Les deux servomoteurs ne doivent pas être alimentés par le régulateur 5 V de l'Arduino lorsque leur courant total peut dépasser ce que la carte fournit.

Schéma recommandé :

```text
Alimentation 5 V externe
       ├──────── VCC servo Pan
       ├──────── VCC servo Tilt
       └──────── GND commun ───── Arduino GND

Arduino D9  ───────────────────── Signal Pan
Arduino D10 ───────────────────── Signal Tilt
```

### Points critiques

- La masse de l'alimentation servo et celle de l'Arduino doivent être communes.
- Ne branchez pas directement une alimentation moteur élevée sur les broches Arduino.
- Utilisez une alimentation 5 V capable de supporter le courant de démarrage simultané.
- Ajoutez une marge de courant pour éviter les chutes de tension et les resets Arduino.
- Vérifiez le brochage de chaque servo : signal, VCC et GND.
- Ne forcez pas mécaniquement un servo contre une butée.

## Préparer et téléverser le firmware

### Bibliothèque Arduino

La bibliothèque `Servo` est généralement incluse dans l'écosystème Arduino. Si elle n'est pas disponible :

1. ouvrir le gestionnaire de bibliothèques ;
2. rechercher `Servo` ;
3. installer la bibliothèque officielle ;
4. sélectionner la carte et le port USB.

### Configuration actuelle

| Élément | Valeur |
| --- | --- |
| Servo Pan | D9 |
| Servo Tilt | D10 |
| Position initiale Pan | 90° |
| Position initiale Tilt | 90° |
| Angle accepté | 0° à 180° |
| Seuil de mouvement Arduino | `0.1°` |
| Débit série | `115200 bauds` |
| Temporisation de boucle | `20 ms` |

Téléversez `arduino/code.ino`, puis ouvrez le moniteur série à `115200 bauds`.

Au démarrage, les deux servos sont placés à 90° :

```cpp
panServo.write(90);
tiltServo.write(90);
```

## Compiler le package

```bash
cd ~/servo_ws
source /opt/ros/$ROS_DISTRO/setup.bash
colcon build --symlink-install --packages-select servo_moteur
source install/setup.bash
```

Compilation complète :

```bash
colcon build --symlink-install
```

Vérifier LibSerial :

```bash
pkg-config --modversion libserial
pkg-config --cflags --libs libserial
```

Vérifier l'installation :

```bash
ros2 pkg prefix servo_moteur
```

## Lancer le système

### Configurer le port série

Le port est défini dans `urdf/servo_control.xacro` :

```xml
<param name="device">/dev/ttyACM0</param>
```

Identifier le périphérique :

```bash
ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null
```

Pour une installation robuste avec plusieurs périphériques USB, utilisez de préférence :

```text
/dev/serial/by-id/usb-...
```

Ajouter l'utilisateur au groupe série :

```bash
sudo usermod -aG dialout "$USER"
```

Reconnectez-vous à la session après cette modification.

### Démarrer ROS 2

```bash
source /opt/ros/$ROS_DISTRO/setup.bash
source ~/servo_ws/install/setup.bash
ros2 launch servo_moteur servo_test.launch.py
```

Le launch file démarre :

1. `ros2_control_node` avec l'URDF généré par Xacro ;
2. `joint_state_broadcaster` ;
3. `head_controller` après le chargement du premier spawner.

### Vérifier les contrôleurs

```bash
ros2 control list_controllers
ros2 control list_hardware_components
ros2 control list_hardware_interfaces
```

## Interfaces ROS 2

### Commande Pan/Tilt

Le contrôleur `head_controller` accepte deux positions dans l'ordre :

```text
[neck_pan_joint, neck_tilt_joint]
```

Publier une commande :

```bash
ros2 topic pub --once \
  /head_controller/commands \
  std_msgs/msg/Float64MultiArray \
  "{data: [0.5, 0.2]}"
```

Les valeurs sont en radians :

- `0.5` rad pour le Pan ;
- `0.2` rad pour le Tilt.

Revenir à la position neutre :

```bash
ros2 topic pub --once \
  /head_controller/commands \
  std_msgs/msg/Float64MultiArray \
  "{data: [0.0, 0.0]}"
```

### États des joints

```bash
ros2 topic echo /joint_states
```

Le topic est de type :

```text
sensor_msgs/msg/JointState
```

Les joints publiés sont :

```text
neck_pan_joint
neck_tilt_joint
```

Les positions sont exprimées en radians côté ROS 2.

### Interfaces `ros2_control`

Le système exporte :

```text
neck_pan_joint/position   commande
neck_pan_joint/position   état
neck_tilt_joint/position  commande
neck_tilt_joint/position  état
```

## Protocole série

### Commande PC → Arduino

Le plugin envoie une commande uniquement lorsque Pan ou Tilt change de plus de `0.001 rad` :

```text
P<pan_degrees> T<tilt_degrees>\n
```

Exemple :

```text
P118.6 T101.5
```

La conversion appliquée est :

```text
pan_degrees  = pan_radians  × 180 / π + 90
tilt_degrees = tilt_radians × 180 / π + 90
```

Ainsi :

```text
0 rad = 90°
```

### Arduino → PC

Après un mouvement significatif, l'Arduino renvoie :

```text
P<pan_degrees_entiers> T<tilt_degrees_entiers>\n
```

Exemple :

```text
P119 T102
```

Le plugin reconvertit cette réponse en radians :

```text
position_radians = position_degrees × π / 180
```

### Paramètres de communication

| Paramètre | Valeur |
| --- | --- |
| Débit | `115200` |
| Bits de données | `8` |
| Parité | aucune |
| Bits d'arrêt | `1` |
| Contrôle de flux | aucun |
| Fin de ligne | `\n` |
| Timeout de lecture | `10` unités LibSerial |

Tester la liaison sans ROS 2 :

```bash
stty -F /dev/ttyACM0 115200 cs8 -cstopb -parenb
cat /dev/ttyACM0
```

Envoyer une commande manuelle :

```bash
printf 'P90 T90\n' > /dev/ttyACM0
```

Fermez Arduino Serial Monitor avant de tester le port depuis ROS 2.

## Configuration

### Paramètres matériels

Dans `urdf/servo_control.xacro` :

| Paramètre | Valeur actuelle | Signification |
| --- | --- | --- |
| `device` | `/dev/ttyACM0` | port série Arduino |

Le débit est fixé à `115200` dans le plugin C++.

### Contrôleurs

`config/controllers.yaml` définit :

```yaml
controller_manager:
  ros__parameters:
    update_rate: 100

    joint_state_broadcaster:
      type: joint_state_broadcaster/JointStateBroadcaster

    head_controller:
      type: forward_command_controller/ForwardCommandController

head_controller:
  ros__parameters:
    joints:
      - neck_pan_joint
      - neck_tilt_joint
    interface_name: position
```

Le contrôleur attend exactement deux valeurs dans cet ordre :

```text
Pan, Tilt
```

### Limites URDF

Les joints sont déclarés avec les limites suivantes :

```xml
<limit lower="-1.57" upper="1.57" effort="1.0" velocity="1.0"/>
```

Cela correspond approximativement à ±90° autour de la position neutre ROS 2. Ces limites décrivent le modèle et les contrôleurs, tandis que l'Arduino borne toujours la commande physique entre 0° et 180°.

Adaptez ces valeurs à la mécanique réelle afin d'éviter les collisions et les contraintes sur les servos.

## Conversion des unités et limites

ROS 2 utilise les radians pour les joints. Le firmware Arduino utilise des degrés entre 0° et 180°.

```text
angle_servo = angle_ros × 180 / π + 90
angle_ros   = (angle_servo - 90) × π / 180
```

Table de référence :

| Commande ROS 2 | Angle Arduino approximatif |
| ---: | ---: |
| `-1.57 rad` | `0°` |
| `0 rad` | `90°` |
| `+1.57 rad` | `180°` |

Le plugin utilise un seuil anti-jitter de `0.001 rad`, tandis que l'Arduino ignore un changement inférieur à `0.1°`.

### Attention aux servomoteurs réels

Un servo standard n'offre pas nécessairement toute la course 0–180°. Certains modèles acceptent une plage plus petite ou utilisent des impulsions différentes. Commencez par une plage limitée, par exemple ±30°, puis augmentez progressivement après vérification mécanique.

## Dépannage

### Le plugin ne démarre pas

Vérifiez :

```bash
ros2 pkg prefix servo_moteur
grep -R "ServoInterface" install/servo_moteur/share/servo_moteur
```

Recompilez après une modification du code C++, du Xacro ou du manifest pluginlib :

```bash
colcon build --symlink-install --packages-select servo_moteur
source install/setup.bash
```

### Le port série ne s'ouvre pas

```bash
ls -l /dev/ttyACM0
groups
```

Causes fréquentes :

- mauvais nom de périphérique ;
- câble USB sans données ;
- port occupé par Arduino IDE ;
- permissions insuffisantes ;
- utilisateur absent du groupe `dialout` ;
- Arduino non alimenté.

### Les servos ne bougent pas

Vérifiez :

- l'alimentation externe 5 V ;
- la masse commune ;
- les signaux D9 et D10 ;
- la commande publiée ;
- les contrôleurs actifs ;
- le moniteur série à `115200 bauds` ;
- l'absence de butée mécanique.

### Un seul axe bouge

Vérifiez que le tableau de commande contient exactement deux valeurs :

```bash
ros2 topic pub --once /head_controller/commands \
  std_msgs/msg/Float64MultiArray "{data: [0.2, -0.2]}"
```

Confirmez l'ordre :

```text
index 0 = neck_pan_joint
index 1 = neck_tilt_joint
```

### Les servos tremblent

Causes possibles :

- alimentation insuffisante ;
- masse instable ;
- servo proche d'une butée ;
- structure mécanique trop chargée ;
- bruit électrique ;
- consignes trop sensibles ;
- alimentation des servos depuis l'Arduino.

Le logiciel applique déjà deux seuils anti-jitter, mais il ne peut pas corriger une alimentation sous-dimensionnée.

### Les positions publiées ne changent pas

L'Arduino renvoie son état uniquement lorsqu'un mouvement significatif est appliqué. Si la commande ne change pas ou si elle est inférieure aux seuils, aucune nouvelle trame n'est envoyée et la dernière position reste publiée.

### Le mouvement est inversé

Le sens dépend du montage mécanique du servo et de l'axe du joint. Le plugin ne corrige pas séparément le signe de Pan ou Tilt. Inversez le signe dans la commande, ou adaptez la convention dans la couche de commande si nécessaire.

## Validation et développement

### Build et tests

```bash
cd ~/servo_ws
source /opt/ros/$ROS_DISTRO/setup.bash
colcon build --symlink-install --packages-select servo_moteur
colcon test --packages-select servo_moteur --event-handlers console_direct+
colcon test-result --verbose
```

### Vérifications fonctionnelles

1. tester les servos avec l'Arduino seul ;
2. confirmer la position initiale 90° ;
3. vérifier la réception d'une commande manuelle `P90 T90` ;
4. charger le plugin dans `ros2_control` ;
5. vérifier les contrôleurs actifs ;
6. envoyer une petite commande de ±0.2 rad ;
7. contrôler les positions sur `/joint_states` ;
8. vérifier les limites mécaniques ;
9. tester l'arrêt et le retour à zéro.

Commandes utiles :

```bash
ros2 node list
ros2 control list_controllers
ros2 control list_hardware_components
ros2 control list_hardware_interfaces
ros2 topic echo /joint_states
ros2 topic hz /joint_states
```

### Test progressif sans charge

Avant de fixer la tête robotique sur le robot :

1. poser les servos sur une table ;
2. utiliser une alimentation 5 V limitée en courant ;
3. envoyer des angles proches de 90° ;
4. tester séparément chaque axe ;
5. vérifier les butées et le sens ;
6. installer ensuite les pièces mécaniques.

## Limites connues

- Le retour d'état Arduino est envoyé uniquement après un mouvement significatif.
- Il n'y a pas de retour périodique lorsque les servos restent immobiles.
- Aucun watchdog ne coupe automatiquement les servos en cas de perte de communication.
- Le plugin ferme le port à la désactivation, mais le firmware ne reçoit pas de commande d'arrêt dédiée.
- Les valeurs reçues par l'Arduino sont converties en entiers lors de l'appel `Servo.write()`.
- La plage physique est bornée à 0–180°, indépendamment des caractéristiques exactes du servo.
- Le débit série est fixé à `115200` dans le code C++.
- Le port `/dev/ttyACM0` est configuré en dur dans le Xacro.
- Les limites URDF et les butées physiques peuvent être différentes.
- Les covariances, diagnostics et timestamps spécifiques au matériel ne sont pas publiés.
- Le protocole ASCII n'intègre ni checksum ni numéro de séquence.
- Les erreurs de lecture série sont largement ignorées dans `read()`, ce qui peut limiter le diagnostic.

### Améliorations recommandées pour la production

1. ajouter un watchdog de communication côté Arduino ;
2. envoyer périodiquement l'état même sans mouvement ;
3. ajouter un checksum et une validation stricte des trames ;
4. rendre le port, le débit et les plages configurables ;
5. ajouter des diagnostics ROS 2 pour les timeouts et erreurs série ;
6. synchroniser les limites URDF avec les limites physiques ;
7. ajouter un mode arrêt sûr indépendant de ROS 2 ;
8. calibrer les positions neutres et les offsets de chaque servo ;
9. publier un état explicite lorsque la position est obsolète ;
10. ajouter des tests unitaires du parser et de la conversion radians/degrés.

## Sécurité

- Utilisez une alimentation externe 5 V adaptée au courant de blocage des deux servos.
- Ne branchez pas plusieurs servos puissants directement sur le 5 V de l'Arduino.
- Reliez les masses avant d'appliquer la puissance.
- Ne forcez jamais un servo contre une butée mécanique.
- Maintenez la tête dégagée pendant les premiers essais.
- Prévoyez un bouton d'arrêt ou une coupure d'alimentation indépendante.
- Coupez l'alimentation avant de modifier le câblage.
- Vérifiez la température des servos, du câblage et de l'alimentation.
- Ne considérez pas le logiciel ROS 2 comme une protection électrique.

## Licence

Le fichier `LICENSE` à la racine du dépôt constitue la référence légale du projet. Vérifiez également les licences de la bibliothèque Arduino `Servo`, de LibSerial et de ROS 2.

## Auteur et dépôt

- **Mainteneur :** Lacina Sidibe
- **Dépôt :** [lacinasidibe114-sudo/servo_moteur](https://github.com/lacinasidibe114-sudo/servo_moteur)
