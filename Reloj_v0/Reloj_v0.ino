
/*
 * Utilizamos la librería RTC_DS1307 que encontramos en la siguiente ruta:
 * https://github.com/Seeed-Studio/RTC_DS1307/
 * La añadimos al un proyecto y para poner la hora tenemos que utilizar el ejemplo SetTimeAndDisplay que está en
 * Archivo/Ejemplos/Grove RTCDS1307
 * Con esto lo hemos puesto en hora y con la pila se supone que ya no va perder la hora. 
 * Si se necesitara volver a poner en hora, utilizar de nuevo el ejemplo (poner un delay de unos 500ms en el loop 
 * para que no pete el puerto serie).
 * 
 * Para calcular la corriente máxima hay que pensar que cada LED puede consumir unos 20mA por color, por lo que en blanco, cada led consumo 60mA.
 * La peor hora (más LED encendidos) creo que es 23:58, que hay 44 LED encendidos. Si asumimos que como máximo voy a poner los LED al 50%,
 * tendría un consumo máximo de 44 x 60mA x 0,5 = 1,3A
 * 
 */

#include <Adafruit_NeoPixel.h>

//Para el reloj
#include <Wire.h>
#include "DS1307.h"

#define PIN 6
#define BUTTON1 12
#define BUTTON2 11
#define NUMPIXELS 67

//enumerado para saber el estado del programa
enum estadoPrueba {
  hora = 0,
  modificaHora,
  modificaMinuto,
  espera
};

bool estadoPuntos;
bool estadoHoras;
//Para saber si se ha pulsado alguno de los botones o los dos
bool boton1;
bool boton1Larga;
bool boton1Corta;
bool boton1LargaCheck;
bool boton2;
bool boton2Larga;
bool boton2LargaCheck;
bool boton2Corta;
estadoPrueba estadoActual;
//Para definir el color que quieres en los números
int red;
int green;
int blue;

int colorNumber;

int timeHour;
int timeHourDec;
int timeHourUnit;
int timeMinute;
int timeMinuteDec;
int timeMinuteUnit;
int timeSeconds;

//Para conocer el tiempo que ha pasado para varios puntos en el programa
unsigned long timeRefrescoHora;
unsigned long timeCambioColor;
unsigned long periodoCambioColor = 2000;
unsigned long periodoRefrescoHora = 500; //Vamos a refrescar la hora y a conmutar los puntos centrales cada 500ms
unsigned long timePulsacionBoton1;
unsigned long timePulsacionBoton2;

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

DS1307 clock;//define a object of DS1307 class
void setup() {
    Serial.begin(9600);
    clock.begin();
    pixels.begin();

    //Defino los pines de as entradas como entradas y con la resistencia de pull-up interna activada
    pinMode(BUTTON1,INPUT_PULLUP);
    pinMode(BUTTON2,INPUT_PULLUP);
    
    estadoPuntos = false;  
    estadoHoras = false;  
    red = 150;
    green = 0;
    blue = 0;
    timeRefrescoHora = millis();  //Para conocer el tiempo inicial para el refresco de la hora
    timeCambioColor = millis(); 

    colorNumber = 0;
    //Inicialmente no están pulsados ninguno de los botones
    boton1 = false;
    boton1Larga = false;
    boton1LargaCheck = false;
    boton1Corta = false;
    boton2 = false;
    boton2Larga = false;
    boton2LargaCheck = false;
    boton2Corta = false;
    //Inicialmente se quiere mostrar la hora normal
    estadoActual = hora;

    timeHour = 0;
    timeHourDec = 0;
    timeHourUnit = 0;
    timeMinute = 0;
    timeMinuteDec = 0;
    timeMinuteUnit = 0;
    timeSeconds = 0;
}

/*
 * En el bucle principal vamos a hacer las siguientes comprobaciones:
 *    Si se ha pulsado el botón1 o el botón2 y si esta pulsación es corta o larga
 *    Ejecutar la función que corresponda en función de qué botones se hayan pulsado
 *       Si no se ha pulsado nada -> mostrar la hora normal
 *       Pulsación larga en boton1 -> cambiar la hora (luego pulsaciones cortas o lagas harán que suba o baje el dígito o que te salgas de este modo)
 *       Pulsación corta en boton1 o boton2 -> subir o bajar el color de los dígitos.
 */
void loop() {  

  //En este switch definimos qué se muestra en el reloj
  switch(estadoActual){
    case hora:
      muestraHora();
    break;

    case modificaHora:
      cambiaHora();
    break;

    case modificaMinuto:
      cambiaMinuto();
    break;

    default:
    break;
  } 
  /*Para saber si está pulsado el botón 1
   * Si vanimos del estado no pulsado, cogemos el tiempo actual para saber si se pulsa más de 1s
   * Si venimos del estado pulsado, comprobaremos si ha pasado ese segundo.
   * Si la lectura del pin es 1 (el botón no está pulsado) dependerá de donde vengamos:
   *    Si todavía no se ha pulsado el botón, volvemos a poner boton1 a false (por control)
   *    Si ya se había pulsado el botón y no era una pulsación larga, se trata de una pulsación corta
   * Solo decidimos si es corta o larga cuando se deje de pulsar el botón. Si no, se pueden estar detectando pulsaciones
   * largas todo el rato que se tenga pulsado o detectar pulsación larga + corta una vez se suelte el botón
   */
  //Si se pulsa el botón
  if (!digitalRead(BUTTON1)){
    if (boton1 == false){
      boton1 = true; //Así no volvemos a entrar en este if
      timePulsacionBoton1 = millis(); 
    }else{
      if (millis() - timePulsacionBoton1 > 1000){
        boton1LargaCheck = true;
        boton1Larga = true;
        timePulsacionBoton1 = millis(); //Para que una vez que detecte que es pulsación larga y ejecute la acción pertinente y se ponga pulsaciónLarga = false, no vuelva a entrar aquí otra vez hasta que pase otro segundo
      }
    }
    delay(5); //Este retardo para evitar que los rebotes hagan que de detecte pulsación corta mientras se tenga pulsado el botón
  }
  //Si el botón no está pulsado
  else{
    if (boton1 == true && !boton1LargaCheck){ 
      boton1Corta = true;      
    }
    boton1 = false;
    boton1LargaCheck = false;
  }

    /*Para saber si está pulsado el botón 2
   * Si vanimos del estado no pulsado, cogemos el tiempo actual para saber si se pulsa más de 1s
   * Si venimos del estado pulsado, comprobaremos si ha pasado ese segundo.
   * Si la lectura del pin es 1 (el botón no está pulsado) dependerá de donde vengamos:
   *    Si todavía no se ha pulsado el botón, volvemos a poner boton2 a false (por control)
   *    Si ya se había pulsado el botón y no era una pulsación larga, se trata de una pulsación corta
   */
  //Si se pulsa el botón
  if (!digitalRead(BUTTON2)){
    if (boton2 == false){
      boton2 = true; //Así no volvemos a entrar en este if
      timePulsacionBoton2 = millis(); 
    }else{
      if (millis() - timePulsacionBoton2 > 1000){
        boton2LargaCheck = true;
        boton2Larga = true;
        timePulsacionBoton2 = millis(); //Para que una vez que detecte que es pulsación larga y ejecute la acción pertinente y se ponga pulsaciónLarga = false, no vuelva a entrar aquí otra vez hasta que pase otro segundo
      }
    }
    delay(5);  //Este retardo para evitar que los rebotes hagan que de detecte pulsación corta mientras se tenga pulsado el botón
  }
  //Si el botón no está pulsado
  else{
    if (boton2 == true && !boton2LargaCheck){
      boton2Corta = true; 
    }
    boton2 = false;
    boton2LargaCheck = false;
  }

  /*
   * Aquí es donde definimos que hacer tras detectar una pulsación larga o corta
   */

  if (boton1Corta){ 
    //defineColor(1);
    //boton1Corta = false;
  }

  if (boton2Corta){ 
    //defineColor(2);
    //boton2Corta = false;
  }

  /*
   * Si detectamos pulsación larga en el botón1, nos ponemos a cambiar la hora de forma manual
   *    - Si estamos en modo por defecto de mostrar la hora, empezamos a cambiar las horas
   *    - Si estamos en modo de cambio de hora, empezamos a cambiar los minutos
   *    - Si estamos en modo de cambio de minutos, volvemos a mostrar la hora de forma normal
   */
  if (boton1Larga){ 
    if (estadoActual == hora){
      estadoActual = modificaHora;
      //Tendremos que fijar la hora actual para dejarla estable mientras se cambia
      clock.getTime();
      timeHour = clock.hour;
      timeMinute = clock.minute;      
      //Ahora dividimos las horas y los minutos en decenas y unidades para dibujar las cifrast
      timeHourDec = timeHour / 10;
      timeHourUnit = timeHour % 10;
      timeMinuteDec = timeMinute / 10;
      timeMinuteUnit = timeMinute % 10;
      pixels.clear();
      drawDecades(timeMinuteDec, 0);
      drawUnits(timeMinuteUnit, 0);
      drawDecades(timeHourDec, 35);
      drawUnits(timeHourUnit, 35);
      pixels.setPixelColor(32, pixels.Color(red, green, blue));
      pixels.setPixelColor(34, pixels.Color(red, green, blue));
      pixels.show();
    }
    else if (estadoActual == modificaHora) estadoActual = modificaMinuto;
    else if (estadoActual == modificaMinuto) estadoActual = hora;
    boton1Larga = false;
  }

  /*
   * Si detectamos pulsación larga en el botón2, mostramos la temperatura exterior???
   */
  if (boton2Larga){ 
    defineColor(4);
    boton2Larga = false;
  }
  
/*
  if (millis() - timeCambioColor > periodoCambioColor){
    red = random(0,150);   
    green = random(0,150); 
    blue = random(0,150);  

    timeCambioColor = millis();
  } 
*/
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////// FUNCIONES ///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void muestraHora(){

  //Dibujamos las cifras y conmutamos los puntos centrales con un periodo igual al guardado en periodoRefrecoHora (inicialmente 550ms)
  if (millis() - timeRefrescoHora > periodoRefrescoHora){

    clock.getTime();
  
    timeHour = clock.hour;
    timeMinute = clock.minute;
  
    //Ahora dividimos las horas y los minutos en decenas y unidades para dibujar las cifrast
    timeHourDec = timeHour / 10;
    timeHourUnit = timeHour % 10;
  
    timeMinuteDec = timeMinute / 10;
    timeMinuteUnit = timeMinute % 10;
    
    pixels.clear();
    drawDecades(timeMinuteDec, 0);
    drawUnits(timeMinuteUnit, 0);
    drawDecades(timeHourDec, 35);
    drawUnits(timeHourUnit, 35);
  
    if (estadoPuntos == false){
      pixels.setPixelColor(32, pixels.Color(red, green, blue));
      pixels.setPixelColor(34, pixels.Color(red, green, blue));
      estadoPuntos = true;
    }
    else{
      pixels.setPixelColor(32, pixels.Color(0,0,0));
      pixels.setPixelColor(34, pixels.Color(0,0,0));
      estadoPuntos = false;
    }
  //Encendemos todos los LEDs pendientes
  pixels.show();
  //Guardamos el tiempo para volver a entrar cuando se vuelva a cumplir el periodo
  timeRefrescoHora = millis();  
  }  
}


void cambiaHora(){
  //Vamos a hacer parpadear la hora con el periodo de refresco
  if (millis() - timeRefrescoHora > periodoRefrescoHora){
    pixels.clear();
    if (estadoHoras == false){      
      drawDecades(timeMinuteDec, 0);
      drawUnits(timeMinuteUnit, 0);
      drawDecades(timeHourDec, 35);
      drawUnits(timeHourUnit, 35);
      pixels.setPixelColor(32, pixels.Color(red, green, blue));
      pixels.setPixelColor(34, pixels.Color(red, green, blue));
      estadoHoras = true;
    }
    else{
      drawDecades(timeMinuteDec, 0);
      drawUnits(timeMinuteUnit, 0);
      pixels.setPixelColor(32, pixels.Color(red, green, blue));
      pixels.setPixelColor(34, pixels.Color(red, green, blue));
      estadoHoras = false;
    }
    pixels.show();    
    timeRefrescoHora = millis();  
  }
  //Si se recibe pulsación corta sobre botón 1 se suma una unidad y si se recibe sobre botón 2 se resta.
  //Tras ello, se actualiza la hora
  if (boton1Corta || boton2Corta){
    if (boton1Corta == true){
      timeHour++;
      //Si nos pasamos de las 23, volvemos a 0
      if (timeHour > 23) timeHour = 0; 
      boton1Corta = false;     
    }
    if (boton2Corta == true){
      timeHour--;
      //Si nos pasamos de las 23, volvemos a 0
      if (timeHour < 0) timeHour = 23;
      boton2Corta = false;     
    }
    //Actualizamos la hora
    timeHourDec = timeHour / 10;
    timeHourUnit = timeHour % 10;
    timeSeconds = clock.second;
    clock.fillByHMS(timeHour, timeMinute, timeSeconds);
    clock.setTime();
  }
}

void cambiaMinuto(){
  //Vamos a hacer parpadear la hora con el periodo de refresco
  if (millis() - timeRefrescoHora > periodoRefrescoHora){
    pixels.clear();
    if (estadoHoras == false){      
      drawDecades(timeMinuteDec, 0);
      drawUnits(timeMinuteUnit, 0);
      drawDecades(timeHourDec, 35);
      drawUnits(timeHourUnit, 35);
      pixels.setPixelColor(32, pixels.Color(red, green, blue));
      pixels.setPixelColor(34, pixels.Color(red, green, blue));
      estadoHoras = true;
    }
    else{
      drawDecades(timeHourDec, 35);
      drawUnits(timeHourUnit, 35);
      pixels.setPixelColor(32, pixels.Color(red, green, blue));
      pixels.setPixelColor(34, pixels.Color(red, green, blue));
      estadoHoras = false;
    }
    pixels.show();    
    timeRefrescoHora = millis();  
  }
  //Si se recibe pulsación corta sobre botón 1 se suma una unidad y si se recibe sobre botón 2 se resta.
  //Tras ello, se actualiza la hora
  if (boton1Corta || boton2Corta){
    if (boton1Corta == true){
      timeMinute++;
      //Si nos pasamos de las 23, volvemos a 0
      if (timeMinute > 59) timeMinute = 0; 
      boton1Corta = false;     
    }
    if (boton2Corta == true){
      timeMinute--;
      //Si nos pasamos de las 23, volvemos a 0
      if (timeMinute < 0) timeMinute = 59;
      boton2Corta = false;     
    }
    //Actualizamos la hora
    timeMinuteDec = timeMinute / 10;
    timeMinuteUnit = timeMinute % 10;
    timeSeconds = clock.second;
    clock.fillByHMS(timeHour, timeMinute, timeSeconds);
    clock.setTime();
  }
  
}









void drawUnits(int cifra, int HourMinutes){
  int i = 0;
  switch(cifra){
    case 0:
      for (i=2; i < 8; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      }
      for (i=9; i < 15; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      }         
      break;
      
    case 1:
      pixels.setPixelColor(6 + HourMinutes, pixels.Color(red, green, blue));
      pixels.setPixelColor(7 + HourMinutes, pixels.Color(red, green, blue));
      pixels.setPixelColor(9 + HourMinutes, pixels.Color(red, green, blue));
      pixels.setPixelColor(10 + HourMinutes, pixels.Color(red, green, blue));   
      break;
      
    case 2:
      for (i=4; i < 8; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      }
      for (i=11; i < 15; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      }  
      pixels.setPixelColor(0 + HourMinutes, pixels.Color(red, green, blue));
      pixels.setPixelColor(1 + HourMinutes, pixels.Color(red, green, blue));     
      break;
      
    case 3:
      for (i=4; i < 8; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      }
      for (i=9; i < 13; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      }     
      pixels.setPixelColor(0 + HourMinutes, pixels.Color(red, green, blue));
      pixels.setPixelColor(1 + HourMinutes, pixels.Color(red, green, blue));       
      break;
      
    case 4:
      for (i=0; i < 4; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      }   
      pixels.setPixelColor(6 + HourMinutes, pixels.Color(red, green, blue));
      pixels.setPixelColor(7 + HourMinutes, pixels.Color(red, green, blue));
      pixels.setPixelColor(9 + HourMinutes, pixels.Color(red, green, blue));
      pixels.setPixelColor(10 + HourMinutes, pixels.Color(red, green, blue));   
      break;
    case 5:
      for (i=0; i < 6; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      }
      for (i=9; i < 13; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      }    
      break;
    case 6:
      for (i=0; i < 6; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      }
      for (i=9; i < 15; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      }          
      break;
    case 7:
      for (i=4; i < 8; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      } 
      pixels.setPixelColor(9 + HourMinutes, pixels.Color(red, green, blue));
      pixels.setPixelColor(10 + HourMinutes, pixels.Color(red, green, blue));     
      break;
    case 8:
      for (i=0; i < 8; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      }    
      for (i=9; i < 15; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      }    
      break;
    case 9:
      for (i=0; i < 8; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      }  
      pixels.setPixelColor(9 + HourMinutes, pixels.Color(red, green, blue));
      pixels.setPixelColor(10 + HourMinutes, pixels.Color(red, green, blue));  
      pixels.setPixelColor(11 + HourMinutes, pixels.Color(red, green, blue));
      pixels.setPixelColor(12 + HourMinutes, pixels.Color(red, green, blue));   
      break;

    default:
    break;
  }
}

void drawDecades(int cifra, int HourMinutes){
  int i = 0;
  switch(cifra){
    case 0:
      for (i=17; i < 23; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      }
      for (i=24; i < 30; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      }        
      break;
      
    case 1:
      pixels.setPixelColor(17 + HourMinutes, pixels.Color(red, green, blue));
      pixels.setPixelColor(18 + HourMinutes, pixels.Color(red, green, blue));
      pixels.setPixelColor(28 + HourMinutes, pixels.Color(red, green, blue));
      pixels.setPixelColor(29 + HourMinutes, pixels.Color(red, green, blue));   
      break;
      
    case 2:
      for (i=26; i < 32; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      }
      for (i=19; i < 23; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      }  
      break;
      
    case 3:
      for (i=26; i < 32; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      }
      for (i=17; i < 21; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      }       
      break;
      
    case 4:
      pixels.setPixelColor(17 + HourMinutes, pixels.Color(red, green, blue));
      pixels.setPixelColor(18 + HourMinutes, pixels.Color(red, green, blue));
      pixels.setPixelColor(24 + HourMinutes, pixels.Color(red, green, blue));
      pixels.setPixelColor(25 + HourMinutes, pixels.Color(red, green, blue));
      for (i=28; i < 32; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      }     
      break;
    case 5:
      for (i=17; i < 21; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      }
      for (i=24; i < 28; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      } 
      pixels.setPixelColor(30 + HourMinutes, pixels.Color(red, green, blue));
      pixels.setPixelColor(31 + HourMinutes, pixels.Color(red, green, blue));  
      break;
    case 6:
      for (i=17; i < 23; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      }
      for (i=24; i < 28; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      }   
      pixels.setPixelColor(30 + HourMinutes, pixels.Color(red, green, blue));
      pixels.setPixelColor(31 + HourMinutes, pixels.Color(red, green, blue));   
      break;
    case 7:
      pixels.setPixelColor(17 + HourMinutes, pixels.Color(red, green, blue));
      pixels.setPixelColor(18 + HourMinutes, pixels.Color(red, green, blue));
      for (i=26; i < 30; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      }     
      break;
    case 8:
      for (i=17; i < 23; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      } 
      for (i=24; i < 32; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      }  
      break;
    case 9:
      for (i=17; i < 21; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      }  
      for (i=24; i < 32; i++){
        pixels.setPixelColor(i + HourMinutes, pixels.Color(red, green, blue));
      }   
      break;

    default:
    break;
  }
}


/*
 * Colores:
 * 0-rojo, 1-verde, 2-azul, 3-amarillo, 4-morado, 5-naranja
 */
void defineColor(int color){
  switch (color){
    case 0:
      red = 127;
      green = 0;
      blue = 0;
    break;
    case 1:
      red = 0;
      green = 127;
      blue = 0;
    break;
    case 2:
      red = 0;
      green = 0;
      blue = 127;
    break;
    case 3:
      red = 0;
      green = 127;
      blue = 127;
    break;
    case 4:
      red = 127;
      green = 0;
      blue = 127;
    break;
    case 5:
      red = 127;
      green = 127;
      blue = 0;
    break;
  }
}
