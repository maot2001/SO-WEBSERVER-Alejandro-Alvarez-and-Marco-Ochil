# SO-WEBSERVER-Alejandro-Alvarez-and-Marco-Ochil

## Integrantes:
Alejandro Alvarez Lamazares
Marco Antonio Ochil Trujillo

## Funcionalidades:
Funcionalidades Básicas (3 puntos)

Funcionalidades Adicionales:
1- Mostrar información adicional de los archivos y directorios, además del nombre (fécha de modificación, tamaño, permisos, etc.) (1pt adicional)
• Esta funcionalidad debe permitir ordenar por nombre, fecha, tamaño, etc.

2- Permitir múltiples peticiones de varios clientes simultáneamente (1 pt adicional)

Total de puntos: 5

## Instrucciones:
1-Compilar el webserver con el comando:
gcc myws.c -o "NOMBRE"

2-Ejecutarlo pasandole el puerto por donde va a escuchar y el directorio raíz desde donde se va a mostrar información con el comando:
./"NOMBRE" "PUERTO" "RAIZ"

3-Abrir un navegador y escribir la direccion:
localhost:"PUERTO"/

Ejemplo:
1- gcc myws.c -o myws
  
2-./myws 1025 /mnt/c/musica

3-localhost:1025

## Detalles de implementación:
- Al ejecutar el webserver este crea un socket y un servidor (donde se utiliza el puerto) y luego estos se vinculan usando bind
- Entonces se mantiene un proceso escuchando peticiones de clientes y cuando acepta una se crea un proceso hijo para atenderla, mientras el proceso padre regresa a escuchar peticiones, lo que permite las multiples peticiones de clientes
- El proceso hijo devuelve en primera instancia una pagina html que tiene todos los subdirectorios y archivos de la "RAIZ" (en caso de existir)
- Es posible acceder a los subdirectorios y descargar los archivos haciendo click sobre ellos
- El tamaño predefindo de las carpetas de 512 es mostrado debido a que quitarlo mostro problemas a la hora de organizar
- Se puede organizar por cualquiera de las 3 columnas mostradas actualmente, haciendo click sobre el nombre de la columna
