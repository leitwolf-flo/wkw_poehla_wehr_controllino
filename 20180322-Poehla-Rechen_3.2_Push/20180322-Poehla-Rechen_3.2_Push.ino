  // Last Update: 2021 02 11
  // Last Test Working: 2018 03 22
  // Wasserkraftanlage Pöhla
  // Steuerung für Rechenreiniger
  // von Florian Kaiser
  // Steuerung: Controllino MAXI
  // Anmerkungen: Blink noch verbessern(ohne Delay)
  // Inhalte: Pushover + mqtt(noch im ausbau)

String versionString = "2018-03-22 13:10 Version: 3.2 mit Pushover";

#include <Controllino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <TimerOne.h>

#define TIMER_US 100000                         // 100mS set timer duration in 

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
 byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 20, 101);
IPAddress gateway( 192, 168, 20, 254 );
IPAddress subnet( 255, 255, 255, 0 );

// Pushover settings
char pushoversite[] = "api.pushover.net";
char apitoken[] = "aebfs5fofuysxqhaz8rwsomw6mrhc2";
char userkey [] = "u65xp5zvixdpjztbs6ob5xbnb7q1k3";
int length;
EthernetClient client;
// Pushover

int schritt = 00001;
int schritt_tmr = 0;
int count = 0;
int error = 0;
int timeout = 0;      //Timer
int timeout_step = 0; //Timer

// Alle 100MS:
void timerIsr(){
  if(schritt_tmr > 0){
    schritt_tmr --;
  }
  if(timeout > 0){
    timeout --;
  }
  if(timeout_step > 0){
    timeout_step --;
  }
}




void setup() {

  pinMode(CONTROLLINO_D4, INPUT);

  Serial.begin(9600);
  Serial.println("WKW Poehla Rechensteuerung by Florian Kaiser");
  Serial.println(versionString);
  Serial.println(" ");
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);

//Pushover
Serial.print(F("Starting ethernet..."));
 if(!Ethernet.begin(mac)) Serial.println("failed");
 else Serial.println(Ethernet.localIP());
 

  Serial.print("Reinigungszyklus Nr.: ");
  Serial.println(count);
  if(error == 0){Serial.println("Kein Fehler");}
  Serial.println(" ");
  Serial.print("Warten auf Start: ");
  
  
  //Contorllino Time
  Controllino_RTC_init(0);
  //Set manual
  /*Controllino_SetTimeDate(8,4,3,18,15,30,23); */ // (day,weekday,month,year,hour,minute,sec) set initial values to the RTC chip


  Timer1.initialize(TIMER_US);                  // Initialise timer 1
  Timer1.attachInterrupt( timerIsr );           // attach the ISR routine here 
}

//START LOOP
void loop() {

// Eingänge auslesen
int in_start      = digitalRead(CONTROLLINO_A0);  // Start taster
int in_end_auf    = digitalRead(CONTROLLINO_A1);  // Endschalter Hauptarm oben
int in_end_ab     = digitalRead(CONTROLLINO_A2);  // Endschalter Hauptarm unten
int in_end_heben  = digitalRead(CONTROLLINO_A3);  // Endschalter Arm Heben
int in_end_press  = digitalRead(CONTROLLINO_A4);  // Endschalter Arm Pressen
int in_park_down  = digitalRead(CONTROLLINO_A5);  // Wählschalter Parken oben/unten
int in_Q1         = digitalRead(CONTROLLINO_A6);  // Q1 Motorschutz Hydraulik ausgelöst
int in_Q2         = digitalRead(CONTROLLINO_A7);  // Q2 Motorschutz Spuelpume ausgelöst
int in_F1         = digitalRead(CONTROLLINO_A8);  // F1 Versorgungsspannung ausgelöst
int reset         = digitalRead(CONTROLLINO_A9);



// Steuervariablen Reset
int out_h_pumpe = LOW;
int out_v_auf = LOW;
int out_v_ab = LOW;
int out_v_heben = LOW;
int out_v_press = LOW;
int out_sp_pumpe = LOW;
int signal_led = LOW;
int out_error = HIGH;  //Fehlerschleife durchgängig = OK

// Q1 Motorschutz Hydraulik ausgelöst
if(in_Q1 == HIGH){
        Serial.println("Abbruch!: Q1 Motorschutz Hydraulik ausgelöst!");
        error = 145;
        schritt = 00000;
        //Pushover send
          pushover("Fehler: Q1 Motorschutz Hydraulik ausgelöst!");
          delay(60000);
          
        }
// Q2 Motorschutz Spuelpume ausgelöst
if(in_Q2 == HIGH){
  Serial.println("Fehler: Q2 Motorschutz Spuelpume ausgelöst!");
        error = 147;
        schritt = 00000;
        //Pushover send
          pushover("Fehler: Q2 Motorschutz Spuelpume ausgelöst!");
          delay(60000);
        }
// F1 Versorgungsspannung ausgelöst
if(in_F1 == HIGH){
  Serial.println("Abbruch!: F1 Versorgungsspannung ausgelöst!");
        error = 148;
        schritt = 00000;
        //Pushover send
          pushover("Fehler: F1 Versorgungsspannung ausgelöst!");
          delay(60000);
        }		
// Logik
switch (schritt) {
// Fehlermodus
	case 00000:
   //Ansteuern
   out_error = LOW;    // ---> LOGO Rechenhaus -----> PC (wenn OK HIGH)
  
	//Blinkleuchte	FEHLERCODE größer 99 anlage stopt!!!
		digitalWrite(CONTROLLINO_D11, HIGH);
		delay(1000);
		digitalWrite(CONTROLLINO_D11, LOW);
		delay(1000);
		
    Serial.print("Fehler: ");
		Serial.println(error);
    Serial.print("Schritt: ");
		Serial.println(schritt);
    Serial.println(reset);

    
  //Fortsetzbedingung
     if(reset == HIGH){
          schritt = 00001;
          error = 0;
          Serial.println("Vor-Ort-Quitierung durchgefuehrt!");
          Serial.print("Gehe zu Schritt ");
          Serial.println(schritt);
          pushover("WKW Poehla Rechen - Vor-Ort-Quitierung durchgefuehrt!");
          delay(2000);
        }
  
		
	break;
  
  // Allgemeine Abfrage
    case 00001:
        //Warten
        if(schritt == 00001){Serial.print(" ");
        }

        //Get TIme from Controllino
        int n;
        Serial.print("20"); n = Controllino_GetYear(); Serial.print(n); Serial.print("-");
        n = Controllino_GetMonth(); Serial.print(n); Serial.print("-");
        n = Controllino_GetDay(); Serial.print(n); Serial.print(" - ");
        n = Controllino_GetHour(); Serial.print(n); Serial.print(":");
        n = Controllino_GetMinute(); Serial.print(n); Serial.print(":");
        n = Controllino_GetSecond(); Serial.println(n);
        delay(1000);


        
        if(error > 1){Serial.println("Fehler!!!!");}
        if(error == 22){Serial.println("Zeit XX:XX - Endlage Hauptzylinder oben nicht erreicht");}
        
        

        if(in_start == LOW && in_park_down == LOW && in_end_auf== LOW){
          schritt = 00002;
          schritt_tmr = 20; //Zeit für Pumpe
          Serial.println(" ");
          Serial.println("Warnung: Druckverlust Zylinder am Hauptarm!!! ");
          Serial.println(" ");
          
          //Pushover send
          pushover("Warnung!: Druckverlust Zylinder am Hauptarm!");
          delay(2000);
          
          Serial.print("Gehe zu Schritt: ");
          Serial.print(schritt);
          Serial.println(" H-Pumpe hochfahren 'Reset Hauptarm' ");
          
        }
       
        
    if(in_start == HIGH || in_park_down == HIGH && in_end_auf== HIGH){
      schritt = 10001;
      Serial.println(" ");
      Serial.print("Gehe zu Schritt: ");
      Serial.print(schritt);
      Serial.println(" !!! Start !!! ");
    }
    delay(1000);
     break;
    
 // Hydraulik Pumpe hochfahren für Reset Hauptarm
     case 00002:    
          // Ansteuern
          out_h_pumpe = HIGH;
  
          // Fortsetzbedingung
          if (schritt_tmr==0){
            schritt = 00003;
            timeout = 400;
		      	timeout_step = 200;
            Serial.print("Gehe zu Schritt: ");
            Serial.print(schritt);
            Serial.println(" 'Reset Hauptarm' ");
          } 
      break;
      
// Hauptarm in obere Endlage fahren
      case 00003:
        // Ansteuern
        out_h_pumpe = HIGH;
        if(in_end_auf == LOW)    { 
          out_v_auf = HIGH; 
          }
        if(in_end_press == LOW)  { out_v_press = HIGH; }
        if(in_end_press == HIGH) { out_v_press = LOW; }
        Serial.print("   warten auf 'end_auf': ");
        Serial.println(timeout);
        delay(100);

        //Timeout Abfrage
        if(timeout_step == 0){Serial.println("Endlage Hauptarm oben nicht erreicht");
        error = 22;
        }
        if(timeout == 0){Serial.println("Abbruch!: Hauptarm in obere endlage fahren");
        error = 122;
        schritt_tmr = 50;
        //Pushover send
          pushover("Fehler: 'end_auf' bei 'Reset' nicht erreicht!!!");
          delay(6000);
		    }
		
        // Fortsetzbedingung
		if(error > 99){schritt = 00000;}
        if(in_end_auf == HIGH && in_end_press== HIGH){
        schritt = 00001;
		  
		    Serial.println("Endlage Hauptarm wieder erreicht! ");
        Serial.print("Gehe zu Schritt: ");
        Serial.print(schritt);
        Serial.print(" Warten auf Start:");
        }
      break;

// Warten auf Start oder weiter wenn Parken unten aktiv
    case 10001:
        // ANzusteuern
                
        // Fortsetzbedingung
        if(in_start == HIGH || in_park_down == HIGH){
          schritt = 10002;
          schritt_tmr = 20;
          Serial.print("Gehe zu Schritt: ");
          Serial.print(schritt);
          Serial.print(" Hydraulik-Pumpe hochfahren: ");   //!!! Nacharbeiten genauere Information
        }
      break;

// Hydraulik Pumpe hochfahren
    case 10002:    
        // Ansteuern
        out_h_pumpe = HIGH;
        Serial.print(schritt_tmr);
        Serial.print(".");

        // Fortsetzbedingung
        if (schritt_tmr==0){
          schritt = 10003;
		  
		    //Timer setzten für nächsten Schritt
        timeout = 600;       // Prozess Timeout
	      // timeout_step = 500;  // Schritt Timeout
        Serial.println(" ");
        Serial.print("Gehe zu Schritt: ");
        Serial.print(schritt);
        Serial.println(" Rechen Strecken");
        }
      delay(100);
      break;

    // Arm Strecken und Absenken
    case 10003:
        // Ansteuern
        out_h_pumpe = HIGH;
        
        if(in_end_ab == LOW) 
        {
        out_v_ab = HIGH;   
        }
        delay(1000);
        if(in_end_heben == LOW) 
        { 
        out_v_heben = HIGH;
        }
        if(in_end_heben == HIGH) 
        { 
        out_v_heben = LOW;
        }
        
        //Timeout Abfrage
        Serial.print("   warten auf 'end_ab' und 'end_heben': ");
        Serial.println(timeout);
        if(timeout == 0){Serial.println("Abbruch!: Rechen Abfahren/Strecken");
        error = 133;
        //Pushover send
          pushover("Fehler: 'end_ab' und/oder 'end_heben' nicht erreicht!!!");
          delay(6000);
		    }
        
        // Fortsetzbedingung
		    if(error > 99){
		      schritt = 00000;
          schritt_tmr = 50;
		      }
        if(in_end_ab == HIGH && in_end_heben == HIGH){
          schritt = 10004;
          schritt_tmr = 20;
          Serial.print("Gehe zu Schritt: ");
          Serial.print(schritt);
          Serial.println(" Anpessen");
        }
      break;
      
      // Anpressen
      case 10004:
        // ANzusteuern
        out_h_pumpe = HIGH;
        out_v_press = HIGH;
 
        // Fortsetzbedingung
        if (schritt_tmr==0){
            schritt = 10005;
            if(in_park_down == HIGH){
            Serial.println(" ");
            Serial.print("Reinigungszyklus Nr.: ");
            Serial.print(count);
            Serial.println(" beendet!");
            }
            Serial.print("Gehe zu Schritt: ");
            Serial.print(schritt);
            if(in_park_down == HIGH){
            Serial.print(" Parken unten 'Aktiv': warten auf Start: ");
            }
        }
      break;

      // Parken unten warten auf Start wenn Parken unten aktiv
      case 10005:
        if(in_park_down == HIGH){
            Serial.print(".");
            delay(1000);
            }
        // Ansteuern
       
        // Fortsetzbedingung
        if (in_start == HIGH || in_park_down == LOW){
          schritt = 10006;
          Serial.println(" ");
          Serial.print("Gehe zu Schritt: ");
          Serial.print(schritt);
          Serial.println(" Anpressen und Auffahren");
          timeout = 2200;   //vorheriger Wert: 8.Mär 21Uhr 1600 -
        }
      break;

      // Anpressen und Hauptarm auf und Spühlen
      case 10006:
        // Ansteuern
        out_h_pumpe = HIGH;
        out_sp_pumpe = HIGH;
        if(in_end_auf == LOW)    { out_v_auf = HIGH; }
        if(in_end_press == LOW)  { out_v_press = HIGH; }
        if(in_end_press == HIGH) { out_v_press = LOW; }

        //Timeout Abfrage
        Serial.print("   warten auf 'end_auf': ");
        Serial.println(timeout);
        if(timeout == 0){Serial.println("Abbruch!: Rechen Pressen/Auffahren");
        error = 144;
        //Pushover send
          pushover("Fehler: 'end_auf' nicht erreicht!!!");
          delay(6000);
        }
                        
        // Fortsetzbedingung
        if(error > 99){
          schritt = 00000;
          schritt_tmr = 50;
          }
        if (in_end_auf == HIGH && in_end_press== HIGH){
            schritt = 10007;
            schritt_tmr = 20;
            Serial.print("Gehe zu Schritt: ");
            Serial.print(schritt);
            Serial.print(" Nachlauf Hydraulik Pumpe: ");
        }
      break;

      // Nachlauf Hydraulik Pumpe
      case 10007:
        delay(1000);
        // Ansteuern
        out_h_pumpe = HIGH;
        out_sp_pumpe = HIGH;
        Serial.print(schritt_tmr);
        Serial.print("."); 

        delay(500);
       
        // Fortsetzbedingung
        if (schritt_tmr==0){
            schritt_tmr = 70;
            schritt = 10008;
            Serial.println(" ");
            Serial.print("Gehe zu Schritt: ");
            Serial.print(schritt);
            Serial.print(" Spuehlpume abschalten in: ");
        }
      break;
      
      // Nachlauf Spühlumpe
      case 10008:
        // Ansteuern
        out_sp_pumpe = HIGH;
        Serial.print(schritt_tmr);
        Serial.print(".");
        delay(500);      
                
        // Fortsetzbedingung
        if (schritt_tmr==0){
            schritt = 1;
            count ++;
            if(in_park_down == LOW){
            Serial.println(" ");
            Serial.print("Reinigungszyklus Nr.: ");
            Serial.print(count);
            Serial.print(" beendet!");
            }
            Serial.println(" ");
            Serial.print("Gehe zu Schritt: ");
            Serial.println(schritt);
                    
            }
      break;   
    }
// Ausgänge
        digitalWrite(CONTROLLINO_R0, out_v_auf);  //grüne LED mitte out_v_press
        digitalWrite(CONTROLLINO_R1, out_v_ab);   
        digitalWrite(CONTROLLINO_R2, out_v_heben);
        digitalWrite(CONTROLLINO_R3, out_v_press);
        digitalWrite(CONTROLLINO_R4, out_h_pumpe);
        digitalWrite(CONTROLLINO_R5, out_sp_pumpe);  //rote LED
        digitalWrite(CONTROLLINO_R9, out_error);     //Störung an LOGO Rechenhaus I4 
} //END LOOP

// PUSHOVER
byte pushover(char *pushovermessage)
{
 String message = pushovermessage;

 length = 81 + message.length();

 if(client.connect(pushoversite,80))
 {
   client.println("POST /1/messages.json HTTP/1.1");
   client.println("Host: api.pushover.net");
   client.println("Connection: close\r\nContent-Type: application/x-www-form-urlencoded");
   client.print("Content-Length: ");
   client.print(length);
   client.println("\r\n");;
   client.print("token=");
   client.print(apitoken);
   client.print("&user=");
   client.print(userkey);
   client.print("&message=");
   client.print(message);
   while(client.connected())  
   {
     while(client.available())
     {
       char ch = client.read();
       Serial.write(ch);
     }
   }
   client.stop();
 }  
}


