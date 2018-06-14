Sockit 

Mise en place du linux sur la carte SD (https://rocketboards.org/foswiki/Documentation/ArrowSoCKITEvaluationBoardLinuxGettingStarted)
Configurer les jumpers de la carte Sockit pour que le boot se fasse sur la carte SD.

Accéder au linux via le lien UART : 
putty -serial /dev/ttyUSB1 -sercfg 115200,8,n,1,N    (USBX a choisir selon le port connecté)
Login : "root" , pas de mdp

Configuration de l'interface réseau de la carte 
Le fichier config_ip.sh configure l'interface réseau, il est lancé automatiquement au démarrage de la carte Sockit.
Adresse IP de la carte : 192.168.2.100

Software pour la Sockit 
Il contient un serveur TCP ainsi qu'une socket UDP qui transmet les données à l'adresse 192.168.2.1.

La compilation des sources se fait avec le fichier Sockit/SoC_software/scripts/build_script.sh
Le compilateur est téléchargeable chez altera (Intel SoC FPGA Embedded Development Suite) => SoC EDS command shell puis source de build_script.sh


Utilisation :
A l'allumage de la Sockit, le processeur se lance automatiquement et lance également la configuration du FPGA. 
Ensuite, le software tcp_serv_v2 est lancé (serv TCP et UDP).
