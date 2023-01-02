## Membres de l'equipe

**RODRIGO Anthony, FACQUET Justin et HEURTEBISE Tom** 

## Objectifs remplis : 

 - Le code et les données des tâches seront inclus dans le noyau à la compilation, comme nous l'avons fait dans les TPs précédents pour la fonction `userland()`.
 - Chaque tâche sera représentée par une fonction (ex. `user1()`, `user2()`) exécutant une boucle infinie.
 - La pagination doit être activée:
   + Le noyau est identity mappé :   
     * plage de mémoire physique accordée = 0x100000-0x400000
     * identity mapping des adresses 0x000000-0x800000
   + Les tâches sont identity mappées : 
     * plage de mémoire physique accordée pour la tâche 1  : 0x400000-0x500000, identity mapping des adresses 0x400000-0x500000 + mapping des adresses virtuelles 0x800000-0x801000 aux adresses physiques 0x800000-0x801000 (cela correspond à la mémoire partagée entre les deux tâches)
     * plage de mémoire physique accordée pour la tâche 2  : 0x500000-0x600000, identity mapping des adresses 0x500000-0x600000 + mapping des adresses virtuelles 0x801000-0x802000 aux adresses physiques 0x800000-0x801000 (cela correspond à la mémoire partagée entre les deux tâches)
   
   + Les tâches possèdent leurs propres PGD/PTB : 
     * Pour la tâche 1 : 0x410000 = adresse de base PGD,  0x411000 = adresse de base PTB1 , 0x413000 = adresse de base PTB2 tâche 1(mappe la zone partagée) , **Notez** que l'on garde 0x10000 en début de mémoire de la tâche pour le code de celle-ci
     * Pour la tâche 2 : 0x510000 = adresse de base PGD,  0x511000 = adresse de base PTB1 , 0x=513000 = adresse de base PTB2 tâche 2 (mappe la zone partagée), **Notez** que l'on garde 0x10000 en début de mémoire de la tâche pour le code de celle-ci

   + Les tâches ont une zone de mémoire partagée:
     - De la taille d'une page (4KB)
     - À l'adresse physique de votre choix : 
       * 0x800000-0x801000
     - À des adresses virtuelles différentes
       * 0x800000-0x801000 pour l'espace mémoire de la tâche 1
       * 0x801000-0x802000 pour l'espace mémoire de la tâche 1

   + Les tâches doivent avoir leur propre pile noyau (4KB)
       * tâche 1 : 0x4feffe-0x4fdffe       
       * tâche 2 : 0x5feffe-0x5fdffe

   + Les tâches doivent avoir leur propre pile utilisateur (4KB)
       * tâche 1 : 0x4fffff-0x4fefff
       * tâche 2 : 0x5fffff-0x5fefff


## Où nous nous sommes arrêtés 

Nous avons mis en place toute la mémoire virtuelle comme exigé par le sujet ( segmentation en mode flat et mise en place de tables des pages ) mais ayant eu des difficultés et par manque de temps nous n'avons pas pu mettre en place le basculement du noyau vers les tâches ainsi que la partie jouant sur l'IDT pour avoir des tâches préemptives.

